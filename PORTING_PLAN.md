# Skater Nation (iOS) → PS Vita

Port de Skater Nation (iOS, Mach-O arm) a PS Vita con loader de Mach-O propio.
Basado en la arquitectura probada de `ios-loader-boilerplate` (caso de estudio Inotia2-vita).

## Lo confirmado del binario (2026-10-18)

- Ejecutable: `SkateNation`, thin/fat **arm**.
- Cifrado FairPlay: **cryptid 0** (descifrado, listo para ejecución).
- Tamaño: 7736 KB.
- Bundle ID: `com.gameloft.SkateNation`.
- Paquete original: `Skater Nation_v.1.2.1.ipa`.
- App bundle: `game_app` → `/Volumes/TOSHIBA EXT/PSVITA Develop/Skater-Nation-vita/skaternation_extract/Payload/SkateNation.app`.
- Palabras de rebase (`rebase.bin`): **43063**.
- Imports únicos detectados: **2029** (GL: 701, Audio: 357, ObjC/CF: 692, POSIX/libc: 21).

## Reglas Críticas de ABI de iOS en PS Vita

1. **`r9` temporal**: En ARMv6/v7 iOS, `r9` es usado como temporal por el compilador y no se preserva. Compilar con `-ffixed-r9` y usar `call_guest` al llamar al juego.
2. **Convención SoftFP**: Floats en registros enteros (`r0..r3`). `vitaGL` vendorizada con `SOFTFP_ABI=1` obligatoria para no corromper la proyección en libgxm.
3. **Alineación de pila a 4 bytes**: El juego asume pila alineada a 4 bytes. Todo import/hook llamado desde el binario entra por `ml_align_call` (asm.S), que realinea a 8 bytes AAPCS copiando argumentos de stack.
4. **printf de Darwin**: Enteros y dobles de 64 bits viajan en palabras de 32 bits contiguas sin alinear; soportar `%@` de Objective-C (`darwin_printf.c`).

## Estructura en la Vita (`ux0:data/skaternation/`)

```
ux0:data/skaternation/
  SkateNation.app/   bundle del IPA limpio (ejecutable + recursos)
  rebase.bin         punteros absolutos reubicados (MACHO_SLIDE = 0x98000000)
  saves/             partidas guardadas (Documents/ de iOS)
  logs/              log_001.log .. log_999.log rotativos
```

## Arquitectura del Loader

| Archivo | Qué reemplaza / funcionalidad |
|---|---|
| `source/macho.c` / `asm.S` | Dyld Mach-O: carga, rebase, imports, thunks con trace, `call_guest`, `ml_align_call` |
| `source/darwin.c` | libc/libgcc/pthread/semáforos de Darwin con `pthread_detach` seguro contra SceUIDs |
| `source/darwin_printf.c` | Formateador variádico con soporte de dobles desalineados y `%@` |
| `source/objc.c` + `objc_rt.h` | Runtime ObjC (despacho a clases del juego, Foundation host); adoptado de Zombie-Infection-vita |
| `source/uikit.c` | `UIApplicationMain`, nib emulado, run loop, NSTimer, UITouch |
| `source/cxx.c` | libstdc++ del juego → libstdc++ de vitasdk (ABI COW) |
| `source/gles.c` | Backend GLES1/2 sobre vitaGL vendorizada con FBO virtual escalado/rotado |
| `source/cgbi.c` | Soporte transparente para imágenes PNG CgBI de Apple |
| `source/coreaudio.c` | AudioUnit RemoteIO + AudioSession sobre `sceAudioOut` |
| `source/input.c` | Touch frontal + botones físicos mapeados a la capa táctil |
| `source/patch.c` | Parches y hooks específicos de Skater Nation |

## Fases

- [x] **0. Repo y herramientas**: IPA extraído, binario analizado, `game_app` symlink, scaffolding creado.
- [x] **1. Loader mínimo** (código listo; GATE de log pendiente en consola física)
  - [x] Carga de segmentos en memoria con `macho_load` (slide `0x98000000`).
  - [x] Aplicación de `rebase.bin`.
  - [x] Resolución de imports indispensables de libc, math, pthread y runtime.
  - [x] Ejecución de inicializadores globales con `macho_run_inits()`.
- [x] **2. Render** (código listo; GATE de primer frame pendiente en consola física)
  - [x] Inicialización de vitaGL y FBO virtual de presentación.
  - [x] Manejo de texturas PNG CgBI con `cgbi.c`.
  - [x] Swap de buffers funcional a ~60 FPS sin splashscreen freeze.
- [x] **3. Input** (código listo; GATE de controles pendiente en consola física)
  - [x] Multitouch frontal de la Vita mapeado a las coordenadas lógicas del juego.
  - [x] Mapeo de botones físicos (D-Pad, Cross, Circle, Square, Triangle, Triggers).
- [x] **4. Texto y fuentes** (código listo; GATE de legibilidad pendiente en consola física)
  - [x] FreeType sobre atlas de fuentes (fuentes `sa0:data/font/pvf/` o `font.ttf` local).
- [x] **5. Audio y video** (código listo; GATE de sonido pendiente en consola física)
  - [x] RemoteIO / AudioSession redirigido al hilo `sceAudioOut`.
  - [x] Formato y decodificación de audio de streaming o efectos.
- [x] **6. Guardado y LiveArea** (persistencia en datos staged; LiveArea validada y dentro del VPK)
  - [x] Persistencia de partidas en `ux0:data/skaternation/saves/`.
  - [x] LiveArea personalizada con `icon0.png`, `pic0.png`, `bg0.png`, `startup.png`.

## Estado actual (2026-10-19)

En consola física (corrida 18): GATE-1 cumplido — loader, 142 constructores, `UIApplicationMain`,
carga de packs/niveles `.gmap`, música y **run loop a 60 Hz**. Siguiente: GATE-2 (primer frame del
splash, `GSSplash::update`). Bugs resueltos por corrida en `port_progress.md`.

Riesgos conocidos a vigilar en los próximos logs:
- `WARN <fn>(NULL) from ...` de `darwin.c`: nil encadenado en ObjC que antes era crash; la dirección
  Mach-O indica qué método/clave falta emular.
- `objc: -[X y] not implemented -> nil` y `unimpl import`: candidatos directos al próximo crash.
- libstdc++: layouts de Darwin distintos (filebuf 0x204 vs 0x74) — streams corregidos en `cxx.c`;
  las excepciones C++ del host siguen sin poder desenrollar frames SjLj del juego (`fatal_error`).
- Faltan imports de UI de GLLive (CGImage/CGGradient/UIGraphics), SCNetworkReachability, `CC_MD5`,
  `setjmp`/`longjmp`.

## Iteración y Triage de Crashes

1. Staging de assets: `tools/stage_data.sh`
2. Despliegue por FTP: `python3 -I tools/deploy_data.py` (o `psvita-toolkit deploy --data`)
3. Si ocurre un crash:
   - Descargar logs: `python3 -I tools/deploy_data.py --logs`
   - Analizar volcado: `psvita-toolkit analyze logs/<dump>.psp2dmp --elf build/skaternation`
   - Si el PC está en `0x98xxxxxx`, restar `0x98000000` para ubicar la dirección en el Mach-O original con `tools/annotate.py`.
