---
name: psvita-toolkit
description: Usar el CLI `psvita-toolkit` (instalado en `/Users/metalsyntax/.local/bin/psvita-toolkit`) para cualquier tarea operativa sobre un port de Android a PS Vita administrado por él -- compilar, desplegar por FTP, analizar crash dumps, LiveArea, shaders, telemetría de rendimiento, profiler de memoria, GDB, parcheo binario, y más -- en vez de reinventar scripts bash/FTP/CMake ad hoc. Activar cuando el directorio actual (o el que el usuario menciona) tenga `.psvita-toolkit.json` o `PORTING_PLAN.md`, o cuando el usuario pida compilar/desplegar/subir/analizar un dump/parchear un shader/etc. para un port de PS Vita.
---

# psvita-toolkit — Referencia operativa para Antigravity

`psvita-toolkit` es el CLI/TUI central que administra ports de Android (soloader + FalsoJNI) a PS Vita:
compilación, despliegue FTP, triage de crashes, LiveArea, shaders, y una capa de herramientas de
consola real (telemetría, profiler de memoria, GDB, parcheo binario, soak testing).

Está instalado globalmente en el host:
- Binario: `/Users/metalsyntax/.local/bin/psvita-toolkit`
- Módulo Python alternativo: `python3 -m psvita_toolkit`

Funciona desde **cualquier directorio**, sin necesidad de hacer `cd` al repo del toolkit.

---

## Regla de Oro en Antigravity

Si el directorio activo (o el que el usuario señala) tiene un archivo `.psvita-toolkit.json` en
la raíz (o un `PORTING_PLAN.md`), es un port administrado por este toolkit. Para compilar,
desplegar, analizar un crash, tocar LiveArea/shaders, o cualquier otra tarea:

> [!IMPORTANT]
> **Usar siempre el subcomando correspondiente de `psvita-toolkit`** mediante `run_command` en vez de escribir scripts bash ad hoc, invocar `cmake`/`ftp` a mano o adivinar rutas. El CLI ya lee la configuración del proyecto (`vita_ip`, `build_dir`, `vita_cg_dir`, etc.) y gestiona reintentos y reconexiones de FTP.

### Modo Headless para el Agente
Sin ningún subcomando (`psvita-toolkit` solo) se abre el TUI interactivo. Al ejecutar desde Antigravity, **siempre usar el modo headless**:
```bash
psvita-toolkit <subcomando> --project <ruta> ...
```
No requiere TTY y devuelve códigos de salida estándar (`0` / `1`) limpios.

Si algo falla y no está claro por qué (falta una herramienta, VITASDK mal configurado, etc.):
```bash
psvita-toolkit doctor
```

---

## Tabla de Subcomandos

*Nota: `--project <ruta>` toma por defecto el directorio de trabajo actual.*

| Subcomando | Qué hace |
|---|---|
| `doctor` | Chequea el entorno del host (VITASDK, Docker, jadx, CMake/Ninja, paquetes de Python, gdb-multiarch, glslangValidator/spirv-cross, PVRTexToolCLI). Primer paso ante fallos del entorno. |
| `build --preset debug\|release\|relwithdebinfo\|minsizerel [--clean]` | Compila el proyecto (`build.sh` si existe, o CMake directo con el toolchain de Vita). |
| `deploy --eboot [--yes]` / `deploy --vpk [--vpk-path ...]` | Sube por FTP solo el `eboot.bin` (rápido) o el `.vpk` completo a la consola física. |
| `analyze <dump.psp2dmp> [--elf ...] [--so ...]` | Analiza un crash dump: resuelve símbolos, cruza con Ghidra + JADX, escribe `<dump>.triage_summary.md`. |
| `init --apk <juego.apk> \| --ipa <juego.ipa> --name "Nombre" [--titleid ...] [--skip-ghidra]` | Crea un port nuevo desde cero (Android o iOS Mach-O). Si ya existe o se pasa `--resume`/`--project`, audita y completa lo faltante. |
| `check-port [--project ...]` | Audita el estado de completitud de un port (Android soloader o iOS Mach-O): config, CMakeLists, git, gitignore anti-DMCA, planes, progreso, CLAUDE.md, skills, y análisis binario. |
| `repair-port [--project ...] [--skip-ghidra]` | Completa y repara un port incompleto o interrumpido (agrega planes, git, skills, config, JNI stubs en Android o estructura Mach-O en iOS). |
| `livearea --auto <carpeta> [--template] [--bgm ...] [--validate]` | Convierte assets de LiveArea (PNG 8-bit indexado, límites de tamaño) sin el picker interactivo. |
| `clean-junk` | Borra basura `._*` de macOS de todo el proyecto (frecuente en discos externos no-HFS+). |
| `jni-analyze [--out-dir ...]` | Detecta middleware conocido (FMOD, OpenAL, Unity, etc.) y genera candidatos de stubs JNI. |
| `logs-live [--port ...] [--filter <regex>]` | Escucha logs UDP en vivo de la consola real. |
| `export-context <dump> [--format markdown\|json]` | Empaqueta el contexto de un crash (Ghidra + JADX + stub actual) para triage. |
| `so-patch [--gen-stubs]` | Escanea SDKs de telemetría/IAP (Google Play Services, AdMob, etc.) y rutas hardcodeadas de Android; genera stubs de neutralización. |
| `so-patch --apply-patch 0x<dirección> --so <archivo.so> [--mode thumb\|arm]` | Aplica un parche binario REAL (retorno seguro) en una dirección ya confirmada con Ghidra/`analyze` -- edita el `.so` en disco con backup automático (`.orig`) y log JSON revertible. |
| `so-patch --revert-patches --so <archivo.so>` | Revierte todos los parches binarios aplicados a ese `.so`. |
| `sync-shared --engine "..." --module "source/falso_jni" [--yes]` | Propaga un componente compartido (falso_jni, wrappers de audio, etc.) a otros ports de la misma familia de motor. |
| `ecosystem-status` | Vista global de todos los ports adoptados bajo `base_dir` (progreso, commits pendientes, LiveArea, shaders). |
| `align-check` | Escanea el `.so` por riesgos de alineación de memoria ARMv7 (`ldrd`/`vld1`/...) y struct packing; documenta en `PORTING_PLAN.md`. |
| `mem-profile [--gen-hooks]` | Escucha métricas de heap en vivo por UDP desde la consola real (detección de fugas), o genera los hooks C (`mem_profiler_hooks.c/.h`). |
| `web [--port 8080]` | Levanta el dashboard web local: logs en vivo, estado de la consola, crash dumps, assets de LiveArea, mapeador Touch-to-Pad, y telemetría de rendimiento. |
| `gdb-map [--so-base 0x...] [--watch-base]` | Genera un script `.gdb` para `gdb-multiarch` contra un gdbstub real. `--watch-base` captura la dirección base del `.so` sola, por UDP. |
| `transcode-assets [--textures-dir ...] [--audio-dir ...] [--gen-loader]` | Convierte texturas a `.rawtex` (+ mipmaps, compresión GPU con `PVRTexToolCLI`) y audio a `.at9` en lote. `--gen-loader` genera el C que carga `.rawtex` (`sceGxmTextureInitLinear`). |
| `perf-telemetry [--gen-hooks]` | Escucha frame-pacing (FPS/p95/stutters) + tiempo de `sceGxmFinish()` desde la consola real, o genera los hooks C. |
| `soak-test [--with-mem-profile] [--hang-timeout ...] [--gen-hooks]` | Escucha un heartbeat de soak-test para detectar cuelgues en partidas largas sin supervisión. |
| `auto-bootstrap [--max-iterations ...] [--run-seconds ...]` | Loop asistido: compila, despliega, espera, chequea un crash nuevo en la consola física, regenera stubs candidatos, reintenta. |
| `shader-transpile --dump-dir <glsl_dump> --out-dir <assets/cg>` | Transpila shaders GLSL -> Cg basado en AST real (`glslangValidator` + `spirv-cross`), no regex. |
| `shader-live-reload [--poll-interval ...]` | Vigila `assets/cg/*.cg` y auto-sube por FTP cada shader al guardarlo (valida con `psp2cgc` antes). |

---

## Buenas Prácticas con Antigravity

- **Captura de artefactos**: Cuando se analice un dump con `analyze`, el archivo `<dump>.triage_summary.md` generado puede convertirse en un Artifact o sintetizarse directamente para el usuario.
- **Procesos en segundo plano**: Comandos como `logs-live` o `shader-live-reload` pueden iniciarse como demonios o procesos async mediante `run_command` con `IsDaemon: true` cuando se requiera monitoreo continuo.
- **Hooks y stubs generados**: Los comandos que generan código C (`mem-profile --gen-hooks`, `so-patch --gen-stubs`, etc.) generan stubs base listos para ser revisados antes de compilar.
