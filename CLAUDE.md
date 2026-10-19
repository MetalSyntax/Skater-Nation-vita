# Skater Nation (iOS Mach-O) → PS Vita

Port de Skater Nation (iOS Mach-O) a PS Vita con loader de Mach-O propio.
Leer `PORTING_PLAN.md` antes de modificar el loader.

## Estructura

- `source/macho.c`: Carga, parseo Mach-O, mapeo de segmentos, rebase y resolución de imports.
- `source/darwin.c`: Capa de compatibilidad libc/libgcc/pthread/semáforos de Darwin.
- `source/objc.c` + `objc_rt.h`: runtime ObjC (despacho a clases del juego + Foundation host).
- `source/uikit.c`: `UIApplicationMain` (nib emulado), run loop, NSTimer, toques → UITouch.
- `source/cxx.c`: imports de libstdc++ enlazados a la libstdc++ de vitasdk (ABI COW).
- Runtime adoptado de `../Zombie-Infection-vita` (mismo linaje Gameloft/Irrlicht): revisar ahí antes de reinventar un fix.
- `game_app` → symlink a la carpeta `.app` original del IPA.
- `ux0_data/skaternation/`: Datos extraídos y assets del juego en la consola.
- `PORTING_PLAN.md`: Plan de port y estado actual de fases.
- `port_progress.md`: Registro de progreso.

## Reglas

- ABI iOS ARM: softfp en ARMv6/v7 (floats en registros enteros), r9 temporal (preservar en callbacks).
- Nada del IPA original en el repositorio git (.gitignore anti-DMCA).
- Un bug por corrida de consola física, guiado por logs.
