---
name: psvita-port-init
description: Checklist reproducible para inicializar un port nuevo de un juego Android (.apk / soloader) o iOS (.ipa / Mach-O loader) a PS Vita, desde el APK/IPA original hasta un repo git listo para portar. Cubre decompilación (jadx, Ghidra headless o macho_prep.py), análisis de binarios por símbolos, git init con .gitignore anti-DMCA, configuración de boilerplates (soloader-boilerplate para Android o ios-loader-boilerplate para iOS), y plantillas de PORTING_PLAN.md y CLAUDE.md.
---

# Inicialización de un port Android (soloader) o iOS (Mach-O) → PS Vita

Esta skill estandariza la **fase 0** de cualquier port nuevo: pasar de "tengo un `.apk` o `.ipa` y quiero portarlo" a
"tengo un repo git limpio, con el binario y motor analizados, el scaffolding adecuado (`soloader-boilerplate` para Android o `ios-loader-boilerplate` para iOS), un
`PORTING_PLAN.md` honesto, y el entorno configurado" — listo para implementar y depurar en hardware real. Destilada de los ports reales de Zenonia 2/3/4, Inotia 3 (Android) y del caso de estudio Inotia 2 (iOS).

## Cuándo usar esta skill

- Se va a inicializar un port desde un paquete original (`.apk` o `.ipa`).
- Se usa `psvita-toolkit init <paquete>` o el wizard interactivo de `psvita-toolkit`.
- Todavía no hay scaffolding inicializado o el repositorio está incompleto.
- Se necesita preparar la estructura de datos sandbox (`ux0:data/<slug>/`) y tablas de rebase.

No uses esta skill para debugging de crashes ya en marcha (eso es `so-crash-triage`) ni para dudas de
arquitectura del loader ya en desarrollo (eso es `psvita-porting`).

## Fase 0 — Prerrequisitos

Verificar antes de tocar nada (`which jadx`, `which docker`, `docker images | grep so-decompiler`):

- **`jadx`** (nativo, `brew install jadx` — más rápido que levantarlo por Docker).
- **Docker con la imagen `devrvk/so-decompiler`** (Ghidra headless empaquetado) — `docker pull
  devrvk/so-decompiler` si no está. En Apple Silicon hace falta `--platform linux/amd64` al correrla.
- `objdump`/`nm`/`strings` del sistema alcanzan para inspeccionar un `.so` ELF32 ARM — no hace falta el
  toolchain ARM de VitaSDK para esto, solo para compilar el loader.
- VitaSDK instalado si esta fase va a terminar en un primer `cmake .` (no estrictamente necesario solo para
  decompilar/documentar).
- **`vita-upscale`** (o `realesrgan` en `~/.local/bin/`) — escalador con IA (Real-ESRGAN GPU) para generar assets nítidos de LiveArea (`bg0.png`, `pic0.png`, `startup.png`, `icon0.png`) a partir de imágenes de baja resolución del APK original sin borrosidad ni desfasaje de tiles en Apple Silicon.

## Fase 1 — Ubicar los artefactos de entrada

1. El `.apk` (es un zip) del juego.
2. El/los `.so` reales bajo `lib/<abi>/` dentro del APK extraído (`armeabi`, `armeabi-v7a`, etc.) — puede
   haber más de uno (motor + wrapper del publisher).
3. Si hay `.obb`, extraerlo también — puede contener assets que el `.apk` no trae.

## Fase 2 — Decompilar

```bash
# Java del APK
jadx -d decompiled_apk juego.apk

# Pseudo-C del/los .so vía Ghidra headless en Docker (por cada .so relevante).
# El 2do argumento de "decompile" es un DIRECTORIO de salida -- la herramienta
# escribe out_ghidra.c/out_ghidra.h/out_angr.c adentro. Pasarle un nombre
# terminado en ".c" crea un directorio con ese nombre (no un archivo) --
# pasar siempre el directorio a secas.
mkdir -p decompiled_so/<nombre_so>_<abi>
docker run --rm --platform linux/amd64 \
  -v "$(pwd)/ruta/al/lib/<abi>:/input" \
  -v "$(pwd)/decompiled_so/<nombre_so>_<abi>:/output" \
  devrvk/so-decompiler decompile /input/<nombre>.so /output
```

El decompile de Ghidra tarda minutos — lanzarlo en background (`run_in_background`/`&`) y seguir con el
análisis de símbolos mientras corre. Si hay más de un `.so` con el mismo nombre en ABIs distintas, el
directorio de salida **debe** incluir el ABI (`libnativeinterface_armeabi/`, no solo `libnativeinterface/`)
para no pisar resultados.

`jadx` puede terminar con "finished with errors" — normal si son solo SDKs de ads/analytics
(Tapjoy/MoPub/Flurry) fallando en clases irrelevantes; no es indicio de que la decompilación del juego en sí
falló.

## Fase 3 — Analizar el motor real (no asumir, confirmar)

**No dar por sentado que "el mismo publisher" o "un juego parecido de la misma época" usa el mismo motor.**
Confirmar con evidencia antes de heredar arquitectura de otro port:

```bash
objdump -T lib/<abi>/<nombre>.so | grep "Java_"   # exports JNI reales — de qué paquete Java son
objdump -T lib/<abi>/<nombre>.so | grep "UND"     # imports — libc/GL/sistema de audio propio, etc.
strings lib/<abi>/<nombre>.so | grep -iE "<publisher>|<motor-candidato>|clet|nexus|engine"
```

Buscar el paquete Java real de los símbolos `Java_com_<publisher>_...` y abrir esa clase en
`decompiled_apk/sources/` para ver el ciclo de vida nativo real (orden de `nativeInit`/`nativePreInit`,
firma de `nativeEvent`, si hay un `nativeRender`/`nativeReDraw` separado, etc.). Si un port previo en el
mismo directorio de trabajo (ej. otro juego del mismo publisher) ya documentó su propio análisis de símbolos
en su `plan_*.md`, compararlo explícitamente en vez de copiarlo:

- **Idéntico de verdad** (mismo namespace JNI, mismos imports, misma firma de funciones) → se puede forkear
  la implementación del loader casi sin cambios, como Zenonia 3 hizo con Zenonia 2 (confirmado: mismo
  `Java_com_gamevil_nexus2_Natives_*`).
- **Solo "se parece" superficialmente** (mismo tipo de juego, misma época, o incluso vocabulario compartido
  como "Clet" — que puede venir de una herencia común de plataformas Java ME coreanas y no implica mismo
  motor) → tratarlo como un port nuevo. Reusar la **arquitectura** de SoLoader/FalsoJNI de un port anterior
  (eso sí es genérico), pero no el ABI concreto (nombres de método, orden de argumentos, tipos). Documentar
  la diferencia explícitamente en el plan — ver Fase 6.

## Fase 4 — git init + `.gitignore` anti-DMCA

```bash
git init
```

El `.gitignore` debe evitar commitear **cualquier activo derivado o extraído del juego original** — el
código del loader (`source/`, `loader/`, `CMakeLists.txt`, scripts) sí se commitea, los assets/binarios del
juego no. Plantilla base (adaptar rutas al layout real del proyecto):

```gitignore
# macOS metadata
.DS_Store
._*
.Spotlight-V100
.Trashes

# APK/ZIP originales y extracción — nunca commitear el juego
*.apk
*.zip
/<carpeta_apk_extraida>/

# Java decompilado con jadx (derivado de material propietario, regenerable con `jadx`)
/decompiled_apk/
/apk_decompiled/

# Pseudo-C decompilado del/los .so (derivado, regenerable con devrvk/so-decompiler)
/decompiled_so/

# Librerías .so propietarias del juego original
lib/*.so
lib/**/*.so

# Build artifacts
/build/
CMakeCache.txt
CMakeFiles/
Makefile
cmake_install.cmake
*.elf
*.vpk
*.suprx

# Debugging en consola real
/logs/
log_*.txt
*.psp2dmp

# Python
__pycache__/
*.pyc

# IDE
.vscode/
.idea/
*.swp
```

Si el proyecto va a distribuir un `.vpk` de instalación (no solo el código fuente), el `build.sh`/CI debe
generar assets/rutas en tiempo de build a partir de lo que el usuario final provee (su propio `.apk` legal),
no empaquetar nada extraído en el repo — mismo patrón que "safe VPK distribution with ux0 data fallback" que
ya usan Zenonia2/3.

## Fase 5 — Adaptar `porting_tools/` heredado de un port anterior

Si `porting_tools/` (o `psvita-port-toolkit/`) se copió de un port previo, **va a tener rutas/nombres
hardcodeados del juego anterior** — es el síntoma más común de un init a medio hacer. Grep y reemplazar
antes de usarlos:

- `porting_tools/build/decompile_all.sh` → `BASE_DIR`, `APK_FILE`, `LIB_DIR`, `DECOMPILED_DIR`.
- `porting_tools/build/build_and_install.sh` → `BUILD_DIR`/`SRC_DIR` en `/tmp` (evita el bug de
  `vita-pack-vpk` con rutas con espacio, ver `toolchain_gotchas.md` de `psvita-porting`), `VPK_NAME`, nombre
  del `.elf` exportado, excludes del `rsync` (deben reflejar el `.gitignore` de este proyecto).
- `manage_vita.py` / `porting_tools/manage_vita.py` → `VITA_IP`, `LOCAL_VPK_PATH`, `VITA_DATA_DIR`,
  `VITA_LOGS_DIR`, banner del nombre del juego.
- `CMakeLists.txt` → `VITA_APP_NAME`, `VITA_TITLEID` (**nunca reusar el de otro port**, colisiona en
  LiveArea), `DATA_PATH` (`ux0:data/<juego>/`).

```bash
grep -rl "<nombre-juego-anterior>" porting_tools/ manage_vita.py CMakeLists.txt 2>/dev/null
```

## Fase 6 — `PORTING_PLAN.md`

Un plan modificable, no un documento final. Secciones mínimas (ver los `plan_zenonia*.md` reales como
ejemplo extenso):

1. **Qué se confirmó del motor real** (Fase 3) — símbolos, ciclo de vida nativo, y si se está forkeando la
   implementación de otro port o arrancando de cero. Si el usuario asumió "mismo motor que X" y el análisis
   lo contradice, decirlo acá explícitamente en vez de forzar la comparación.
2. **Guía de decompilación reproducible** (comandos exactos de la Fase 2, con las rutas reales del proyecto).
3. **Fases del port** (bootstrap del loader → primer build → primer arranque en Vita3K → gráficos → input →
   audio → assets → LiveArea/VPK → hardware real) con checkboxes, igual formato que Zenonia2/3.
4. **Riesgos heredados** de ports anteriores que aplican igual (rutas con espacio, `pthread`
   `--whole-archive`, `libshacccg.suprx` en `ur0:data/`) y **riesgos nuevos** específicos de este motor.

## Fase 7 — Skills locales y `CLAUDE.md`

- Copiar `skills/psvita-porting/` a `.claude/skills/psvita-porting/` dentro del repo nuevo (aunque ya esté
  disponible como skill global, mantiene el repo autocontenido/compartible).
- Escribir `CLAUDE.md` con: qué es el proyecto, estructura de carpetas, hallazgos de motor de la Fase 3, y
  el flujo de trabajo esperado (build → probar en Vita3K/consola → un bug a la vez guiado por log, remitiendo
  a `so-crash-triage` para ese paso).

## Checklist final de esta fase

- [ ] `jadx`/Docker `devrvk/so-decompiler` disponibles, decompilación de APK y `.so` corrida y guardada.
- [ ] Símbolos JNI/imports analizados; afirmación de "mismo motor que X" confirmada o refutada con evidencia.
- [ ] `git init` + `.gitignore` anti-DMCA (nada de `.apk`/`.so`/decompilados propietarios trackeado).
- [ ] `porting_tools/`/`manage_vita.py`/`CMakeLists.txt` sin referencias al juego/port anterior.
- [ ] `PORTING_PLAN.md` escrito con hallazgos reales, no copiado de otro port.
- [ ] `.claude/skills/psvita-porting/` presente y `CLAUDE.md` escrito.
