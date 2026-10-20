# Skater Nation — Progreso del Port (iOS Mach-O)

Actualizado: 2026-10-18. Estado: **todo el trabajo en host está hecho**; solo queda
desplegar y probar en la consola física.

## Fases

- [x] **0. Repo y herramientas**: IPA extraído, binario analizado, `game_app` symlink, scaffolding creado.
- [x] **1. Loader mínimo** (código listo; GATE de log pendiente en consola)
  - [x] Carga de segmentos en memoria con `macho_load` (slide `0x98000000`).
  - [x] Aplicación de `rebase.bin` (43063 rebase words, generado por `tools/macho_prep.py`).
  - [x] Resolución de imports (2029 únicos; indirect symbols + thunks `ml_align_call`).
  - [x] Ejecución de inicializadores globales con `macho_run_inits()`.
- [x] **2. Render** (código listo; GATE de primer frame pendiente en consola)
  - [x] Inicialización de vitaGL y FBO virtual de presentación (`gles.c`, FBO escalado/rotado).
  - [x] vitaGL vendorizada compilada con `SOFTFP_ABI=1` (paso de pre-requisito crítico ABI).
  - [x] Manejo de texturas PNG CgBI con `cgbi.c`.
  - [x] Swap de buffers ~60 FPS (`main.c`, bucle a 16 ms).
- [x] **3. Input** (código listo; GATE de control pendiente en consola)
  - [x] Multitouch frontal mapeado a coordenadas lógicas (`input.c` + `objc.c` IME).
  - [x] Mapeo de botones físicos (D-Pad, Cross, Circle, Square, Triangle, Triggers).
- [x] **4. Texto y fuentes** (código listo; GATE de legibilidad pendiente en consola)
  - [x] FreeType sobre atlas (`objc.c`), fuentes `sa0:data/font/pvf/` o local.
- [x] **5. Audio** (código listo; GATE de sonido pendiente en consola)
  - [x] RemoteIO / AudioSession sobre `sceAudioOut` (`coreaudio.c`).
  - [x] AudioSession y streaming/efectos soportados en la capa HLE.
- [x] **6. Guardado y LiveArea**
  - [x] Persistencia de partidas en `ux0:data/skaternation/saves/` (`DOCS_PATH`, `main.c`).
  - [x] Logs rotativos `logs/log_001..999` (`next_log_number()`).
  - [x] LiveArea validada (icon0 2.7KB, pic0 8.1KB, bg0 41KB, startup 4.6KB, template.xml; 8-bit indexado).

## Trabajo de host realizado hoy (2026-10-18)

- [x] `doctor` del toolkit: entorno OK (vitasdk, cmake, make, vita-*); `psp2cgc` no instalado (solo afecta validación local de shaders, no bloquea).
- [x] Fix `.gitignore`: patrón global `Makefile` estaba excluyendo el Makefile de `vendor/vitaGL` al copiar código → anclado a `/Makefile`.
- [x] Fix `source/macho.c`: usar `s->flags & 0xFF` (SECTION_TYPE) y `s->reserved1` (indirect symbol base) en la resolución de imports; el struct no tenía `type`/`r1`.
- [x] Fix `source/log.{c,h}`: añadido `l_debug()` que faltaba (undefined references en darwin.c/coreaudio.c).
- [x] Build release OK → `build/eboot.bin`, `skaternation.elf`, `skaternation.velf`, `skaternation.vpk` (1.0 MB).
- [x] Staging de datos `tools/stage_data.sh` → `ux0_data/skaternation/` (337 MB): bundle `SkateNation.app/`, `rebase.bin`, `saves/`, `logs/`.
- [x] `align-check`: sin hallazgos (orientado a .so; el Mach-O se maneja por thunks alineados AAPCS).
- [x] `livearea --auto extras/livearea --validate`: 5/5 assets OK.
- [x] VPK reempaquetado con los LiveArea validados (verificado con `unzip -l`: sce_sys completo + eboot.bin).
- [x] `check-port`: port COMPLETO (config, CMake, git, gitignore anti-DMCA, planes, skills, análisis Mach-O).

## Registro de corridas en consola — 2026-10-18/19

- [x] Corrida 1 (`logs/log_001.log`): FATAL "Failed to open ux0:data/skaternation/game_app/SkateNation".
  - Causa: el loader usaba `BUNDLE_PATH = game_app/` (symlink del host), pero el staging en consola crea `SkateNation.app/`.
  - Fix: `source/hle.h` → `BUNDLE_PATH = DATA_PATH "SkateNation.app/"`. Build regenerado (eboot 23:50).
  - Desplegado y superado: corrida 2 cargó el Mach-O completo.
- [x] Corridas 2-4 (`log_004.log` + dump `*0x000059292b*`): prefetch abort en `__mod_init_func` #1.
  - PC=0x300033EC == call_guest (`mov r12,r0; blx r12`) con fn=0x980033EC+0x98000000 truncado → **doble rebase** (0x33EC era un vmaddr real de __TEXT).
  - Causa: `tools/macho_prep.py` incluía en rebase.bin (a) `__mod_init_func` (142 entradas) y (b) `__nl_symbol_ptr local` (413 slots), pero el loader ya les añade MACHO_SLIDE en memoria (`macho.c` :374 y :414). 0x98+0x98 → wrap a 0x3000... → saltó a memoria no mapeada.
  - Fix: excluir `__mod_init_func` del escaneo y no añadir `local_ptr_slots` al rebase (tools/macho_prep.py). Rebase regenerado: 43063 → 42508 palabras (−142 −413 = −555, cuenta exacta).
  - Pendiente: subir SOLO `ux0_data/skaternation/rebase.bin` (eboot no cambia) y relanzar.

- [x] Corrida 2 (`logs/log_002.log`): FATAL "Mach-O does not contain __TEXT or __DATA segments".
  - Causa: `segment_command` en `source/macho.c` no tenía `cmd`/`cmdsize`; `segname` leía 8 bytes corridos y nunca matcheaba `__TEXT`.
  - Fix: añadidos `uint32_t cmd, cmdsize` al struct. Build release regenerado (2026-10-19 00:05).
  - Sospecha para la próxima corrida: `sceKernelAllocMemBlockForVM` no mapea en `MACHO_SLIDE + vmaddr`, pero los slots/rebase/memcpy de __TEXT escriben ahí → probable data abort tras el log "__DATA vm".

- [x] Corrida 3 (`logs/log_003.log`): FATAL "sceKernelAllocMemBlockForVM(text) = 0x80024B05".
  - Causa: bloques ForVM sin dirección fija; el Mach-O (no PIE) necesita __TEXT/__DATA en `MACHO_SLIDE + vmaddr`.
  - Fix: `alloc_block()` en `source/macho.c` con `kuKernelAllocMemBlock` + HAS_VBASE (`field_C`): text USER_RX en 0x98000000 (0x51B000), data USER_RW en 0x9851B000, thunks USER_RX sin dirección fija.

- [x] Corrida 5 (`logs/log_005.log` + psp2dmp 1792384717): crash en el primer constructor de `macho_run_inits`, PC=0x300033EC.
  - Causa: doble slide (0x33EC + 2*0x98000000). El `rebase.bin` de la consola (43063 palabras) era anterior al fix de `tools/macho_prep.py` que excluye `__mod_init_func`.
  - Fix: subido el `rebase.bin` regenerado (42508 palabras, 0 en `__mod_init_func`). Sin cambios de código. El log debe decir "Applying 42508 rebase words".

- [x] Corrida 6 (`logs/log_006.log` + psp2dmp 1792384915): data abort en Mach-O 0x2FF8 (`str r2,[r3]`, r3=0x561ABC sin slide), primer constructor.
  - Causa: `tools/macho_prep.py` no rebasaba los literal pools de `__text`; el binario no es PIE (nlocrel=0) y mezcla literales absolutos con offsets PIC (`ldr rX,[pc,rX]` / `add rX,pc,rX`).
  - Fix: seguir cada `ldr rt,[pc,#imm]` (ARM y Thumb-1, por símbolo con N_ARM_THUMB_DEF) hasta su literal; se excluye si rt se usa luego como offset PIC (`arm_pc_reg_use`, `add rt,pc` en Thumb); se rebasea si `valid_target()` lo acepta.
  - rebase.bin: 42508 -> 57072 palabras (+14564 literales, 10303 PIC excluidos, 1680 constantes rechazadas). Subido a la consola; el log debe decir "Applying 57072 rebase words".

- [x] Corrida 7 (`logs/log_007.log` + psp2dmp 1792385181): data abort en Mach-O 0x900C (`ldr r2,[pc,r2]`, r2=0x9851227C) en el constructor de gameswf.
  - Causa: el literal 0x902C es offset PIC, pero la carga de r2 estaba a >8 instrucciones del uso; la ventana fija de detección PIC no la vio y se rebaseó.
  - Fix: `tools/macho_prep.py` atribuye cada uso PIC (Rn=pc, Rm=rt / Thumb `add rt,pc`) al último literal cargado en rt dentro de la función. rebase.bin 57072 -> 57016 palabras. Subido.
  - Nota: `__ZNSt8ios_base4InitC1Ev` cae en unimpl (stub que devuelve 0); inofensivo mientras no se use iostream.

- [x] Corrida 8 (`logs/log_008.log`): pantalla negra sin crash. Los 142 constructores pasan y `main` corre, pero `objc_msgSend(0x0, alloc)` (classref de NSAutoreleasePool en 0) y `_UIApplicationMain` sin implementar: `main` vuelve y queda el bucle vacío.
  - Fix `source/macho.c`: `resolve_import()` común + pasada de relocaciones externas (7723 extrel: classrefs/superclases ObjC, vtables, símbolos weak). Las clases `_OBJC_CLASS_$_*` externas resuelven a clases host (`objc_host_class`).
  - Fix `source/asm.S` + `source/objc.c`: dispatch ObjC real. `objc_msgSend`/`objc_msgSendSuper2`/`objc_msgSend_stret` pasan por trampolines: método de clase del juego (lista de métodos en `__objc_const`) → tail-call al IMP; si no, emulación host (`objc_msgSend_host`).
  - `UIApplicationMain` recrea `MainWindow.nib`: `Gangstar2AppDelegate` + `EAGLView initWithCoder:` → `setWindow:`/`setGlView:` → `applicationDidFinishLaunching:` → `layoutSubviews` → bucle que dispara los `NSTimer` (drawView) una vez por frame; `presentRenderbuffer:` → `gles_present()`.
  - Pendiente: subir `eboot.bin` (`psvita-toolkit deploy --eboot`; FTP estaba apagado). El log debe decir "External relocations bound: 7723/7723" y "UIApplicationMain: Gangstar2AppDelegate + EAGLView".

- [x] Corrida 9 (`logs/log_009.log` + psp2dmp 1792429700): el runtime ObjC ya funciona (7723/7723 extrel, `UIApplicationMain` entra a `EAGLView initWithCoder:`). Data abort en Mach-O 0x349220 justo tras `unimpl import: __Znwm`.
  - Causa: los 79 imports de libstdc++ (`operator new`, std::string COW, iostream, `_Rb_tree`) caían en stubs que devuelven 0; `new` devolvía NULL.
  - Fix: `source/cxx.c` enlaza los 79 a la libstdc++ de vitasdk con ABI vieja (`_ZNSs*`, mismo layout COW que GCC 4.2). Solo cambia el mangling de size_t (`m` iOS → `j` Vita). vtables/VTT/`_S_empty_rep_storage` como datos; `__throw_*` → fatal con log (no pueden desenrollar frames SjLj del juego).
  - Riesgo conocido: `basic_filebuf` incluye `mbstate_t` (128 B en Darwin, 8 en newlib) → offsets de ifstream/ofstream inline no coinciden.
  - Además: `CFBundleGetMainBundle`/`CopyBundleURL`/`CFURLCopyFileSystemPath`/`CFStringGetCString`/`CFRelease` para `getApplicationPath`; setters UIKit sin efecto ya no ensucian el log; `gles.c` incluye `objc.h`.
  - eboot.bin subido.

- [x] Corrida 10 (`logs/log_010.log` + psp2dmp 1792431057): prefetch abort PC=0x34948c (sin slide) en `irr::CIrrDeviceIPhoneOS::CIrrDeviceIPhoneOS`.
  - Causa: tabla de saltos de un switch en `__text` (`cmp r3,#24; ldrls pc,[pc,r3,lsl #2]` + 25 direcciones absolutas). `tools/macho_prep.py` solo seguía literales `ldr rt,[pc,#imm]`.
  - Fix: `macho_prep.py` detecta `ldr pc,[pc,rm,lsl #2]` (ARM) y rebasea n+1 entradas (n del `cmp rm,#n` previo; si no hay, mientras apunten dentro de la función). 170 tablas, 2675 entradas. rebase.bin 57016 -> 59691 palabras (solo altas). Subido; el log debe decir "Applying 59691 rebase words".
  - `_uname` sin implementar (devuelve 0, buffer sin tocar): inofensivo por ahora.

- [x] Corrida 11 (`logs/log_011.log` + psp2dmp 1792431391): data abort en `strlen` (r0=0xfffffff8) tras `unimpl import: _glGetString` / `_ceilf`. Irrlicht hacía `strlen(glGetString(GL_EXTENSIONS))` sobre NULL.
  - Fix: las tablas de imports venían de otro juego; de 229 imports faltantes quedan 76. Agregados:
    - `gles.c`: `glGetString` (cadenas de iPhone 3G/MBX, extensiones limitadas a lo emulado), wrappers SOFTFP de las ~20 funciones GL con float (`glClearColor`, `glTranslatef`, `glOrthof`, `glFrustumf`…), `glDrawElements` con conversión GL_FIXED, buffers/VBO, stencil, lights/materials, etc.
    - `darwin.c`: math double/float SOFTFP (`ceilf`, `asinf`, `atan`, `exp`, `log`…), `strtod`/`atof` SOFTFP, strings, `printf`/`fprintf`/`vfprintf`/`snprintf` con formateador Darwin, `stat` (struct stat Darwin de 96 B), `opendir`/`readdir` (dirent Darwin), `uname`, `remove`/`rename`/`realpath` con traducción de ruta.
    - **Bug latente corregido**: `time_t` es 64 bits en vitasdk y 32 en iOS. `gettimeofday` (antes `D()` directo) escribía 16 bytes en un `timeval` de 8 → corrupción de pila. Ahora `gettimeofday`/`time`/`localtime(_r)`/`mktime`/`nanosleep` convierten.
    - `objc.c`: `CFArray*`, `CFAbsoluteTimeGetCurrent`, `NSSearchPathForDirectoriesInDomains` (→ `saves/`), `objc_setProperty`, constantes NSString (UIKeyboard*/MP*/NS*), `CGAffineTransformIdentity`/`CGRectZero`/`CGPointZero`.
    - `cxx.c`: `__dynamic_cast` → libsupc++ del host.
  - Faltan: OpenAL (audio), CGImage/CGGradient/UIGraphics (UI de GLLive), excepciones C++/ObjC, `setjmp`/`longjmp` (jmp_buf Darwin vs newlib), `CC_MD5`, SCNetworkReachability.
  - eboot.bin subido.

- [x] Corrida 12 (`logs/log_012.log` + psp2dmp 1792432691): data abort en Mach-O 0x2adb9c (`ldr pc,[r3,#0x24]`, r3=0x533a58) en `irr::video::Blit`: vptr de `CImage` sin slide.
  - Causa: en `CImage::CImage` la vtable se carga en r3 y después r3 se reutiliza como índice de un switch (`ldrls pc,[pc,r3,lsl #2]`). `arm_pc_reg_use` lo tomaba como uso PIC (no miraba el desplazamiento) y atribuía el uso al último literal de r3 aunque r3 ya se había redefinido.
  - Fix `tools/macho_prep.py`: detección PIC portada de Zombie-Infection-vita (solo formas registro sin desplazamiento; se sigue Rt hasta que se redefine, return, o 64 instrucciones; se saltean las palabras de los literal pools). rebase.bin 59691 -> 59754 (+63, 0 quitadas). Casos de las corridas 6/7/12 verificados. Subido.

- [x] Runtime adoptado de Zombie-Infection-vita (mismo linaje Gameloft: Irrlicht + EAGLView + GLLive, ya jugable en consola). Respaldo del código anterior: scratchpad de la sesión (`backup_sn/`).
  - Copiados: `macho.c`, `asm.S`, `objc.c` + `objc_rt.h`, `uikit.c` (nuevo), `darwin.c`, `gles.c`, `coreaudio.c` (OpenAL → openal-soft), `cxx.c`, `imports.*`, `input.*`, `main.c`, `log.*`, `tools/macho_prep.py`, `tools/test_darwin_path.sh`. Se mantienen `hle.h` y `patch.c` de SN (`patch_input`/`patch_overlay` vacíos: los botones físicos todavía no generan toques).
  - Adaptado: ejecutable/bundle `SkateNation`, `com.gameloft.SkateNation` 1.2.1, delegate por defecto `Gangstar2AppDelegate`. SN no tiene `CTouchPad::s_currentDeviceOrientation` → orientación 3 fija (landscape).
  - Convención de rebase de ZI: `__mod_init_func` (142) y slots locales de `__nl_symbol_ptr` (413) van en rebase.bin y el loader no los vuelve a deslizar. rebase.bin = 60309 palabras (= SN 59754 + esas 555).
  - Agregado sobre ZI lo que SN usa y ZI no: 84 símbolos de libstdc++ (`cxx.c` regenerado como unión, `__throw_*` → fatal), `CFBundleCopyBundleURL`/`CFURLCopyFileSystemPath`/`CFStringGetCString`, `CFArray*`, `CFAbsoluteTimeGetCurrent`, `CGAffineTransformIdentity`/`CGRectZero`/`CGPointZero`, constantes `UIKeyboardDidShowNotification`/`MPMediaItemPropertyPersistentID`, libc (`fprintf`/`vfprintf`/`write` al log, `remove`/`rename` con ruta, `sleep`, `abort`, math `ceil`/`exp`/`log2`/`fmax`/`fmin`, mach host), GL `glFlush`/`glFrustumf`/`glOrthof`, OpenAL `alBufferData`/`alcGetContextsDevice`/`alcGetCurrentContext`.
  - Bug de ZI corregido aquí: `time_t` (64 bits en vitasdk, 32 en iOS) — `gettimeofday`/`time`/`mktime` iban directos y `localtime_r`/`nanosleep` leían 8 bytes. Convendría portar el fix a ZI.
  - Cobertura: 479/518 imports. Faltan 39: CGImage/CGGradient/UIGraphics/UIImage*Representation (UI de GLLive), SCNetworkReachability, `CC_MD5`, excepciones (`__gxx_personality_sj0`, `OBJC_EHTYPE`), `setjmp`/`longjmp`, `kCFAllocatorDefault`/`kCFRunLoopDefaultMode` (bloques de datos en cero).
  - Tests: `python3 -I tools/test_macho_prep.py` (7 casos de las corridas 6/7/10/12 + mod_init) y `sh tools/test_darwin_path.sh`: OK.
  - **Pendiente de subir** (FTP apagado): `build/eboot.bin` y `ux0_data/skaternation/rebase.bin`. El log debe decir "rebase: 60309 words" y "objc: runtime ready".

- [x] Corrida 14 (`logs/log_014.log` + psp2dmp 1792433961): primera con el runtime de ZI. rebase 60309, 142 constructores, `UIApplicationMain` y `EAGLContext initWithAPI:1` OK. Data abort en `is_meta` (objc.c) con isa=NULL, desde `unknown_method`.
  - Causa (bug del runtime de ZI): `-[UIActivityIndicatorView sizeThatFits:]` va por `objc_msgSend_stret`, que no estaba implementado. `lookup()` devolvía `unknown_method` (firma `self, sel`), pero en stret r0 es el puntero de retorno → `self` = buffer de la pila. ZI nunca llamó un stret desconocido.
  - Fix `objc.c`: `objc_lookup_stret` cambia `unknown_method` por `unknown_method_stret` (log + pone el struct a cero: 16 B si el selector parece un rect, 8 si no). `uikit.c`: `-[UIView sizeThatFits:]` devuelve el tamaño pedido. Conviene portar el fix a ZI.
  - eboot.bin subido.

- [x] Corrida 15 (`logs/log_015.log` + psp2dmp 1792434406): **avance grande**: Irrlicht arranca y carga los packs (`_packMac/*.gmap`, `*_pycst.bin`, `oconf.bar`; `saves/gameprofile.sav` no existe aún, normal). Data abort en `gameswf::sprite_instance::execute_frame_tags` (0x15552c) justo después de `gameswf::movie_def_impl::read() -- file does not start with a SWF header!` al abrir `_packMac/dummy.swf`.
  - Descartado offline: el archivo es válido en la consola (`FWS\x08`, 36 B, bajado por FTP); `tu_file(char*, "rb")` usa `fs_read_func` → `fread(dst, 1, n, FILE*)`; los punteros a callbacks salen de slots locales de `__nl_symbol_ptr` que están en rebase.bin; el chequeo es `(header & 0xFFFFFF) == 'FWS'`.
  - Diagnóstico (no fix): `fread` envuelto en `darwin.c` loguea las primeras 24 lecturas de ≤8 bytes (posición, bytes, eof/err). Eboot subido. Con esa línea del próximo log se ve si `fread` devuelve 0/otros bytes o si el problema está después.

- [x] Corrida 16 (`logs/log_016.log` + psp2dmp 1792434772): el diagnóstico mostró `fread` correcto (`46 57 53 08` = `FWS`) y aun así "file does not start with a SWF header".
  - Causa: los literales de la firma, `'FWS'` = 0x535746 y `'CWS'` = 0x535743 (en `movie_def_impl::read`, Mach-O 0x1264f4/0x1264f8), caen dentro del rango de la imagen (`__DATA`); `macho_prep.py` los rebaseaba como punteros → el código comparaba la cabecera contra 0x98535746.
  - Fix `tools/macho_prep.py`: literal "numérico" si su registro solo se lee en compare/lógico (and/eor/tst/teq/cmp/cmn/orr/bic) sin ningún uso como puntero antes de redefinirse (`bl` = argumento → puntero; `b` corta el bloque), **y** el valor no está alineado a 4, cae en `__DATA` y fuera de cadenas. Quita exactamente esos 2 (60309 -> 60307). Criterio estricto a propósito: comparaciones con `typeinfo`, `irrEmptyString` o stubs son punteros reales y se mantienen. Test con 11 casos.
  - Revisados los otros ports (Asphalt 6 es Android y su caso FWS era ofuscación de assets): ninguno tenía este problema, el fix es nuevo; conviene llevarlo a ZI.
  - Diagnóstico de `fread` quitado. rebase.bin y eboot.bin subidos.

- [x] Corrida 17 (`logs/log_017.log` + psp2dmp 1792435197): pasa `dummy.swf` y los `.gmap`; abort por `ios_failure` sin capturar (`__cxa_throw` ← `basic_ios::clear` del host) en `MusicPlayer::Load` (Mach-O 0xd0d2c), que lee la lista de música con un `std::ifstream` inlineado. Los `.gmap` no eran el problema.
  - Causa (el `ponytail:` de `cxx.c`): el `basic_filebuf` de Darwin mide 0x204 B (mutex de 44 B + `mbstate_t` de 128 B) contra 0x74 en vitasdk. El ctor inlineado sacaba el offset de la base virtual `basic_ios` de las vtables/VTT del host (0x7C) en vez de 0x20C → `clear`/`widen`/`getline` operaban sobre basura dentro del filebuf. Además `__basic_file` está en +0x4C en el guest y en +0x24 en el host.
  - Fix `cxx.c`: copias de vtable/VTT/construction vtable de `ifstream`/`ofstream` con el vbase cambiado (0x7C→0x20C, 0x78→0x208), armadas en un constructor; `__basic_file::is_open` y `~__basic_file` re-basados −0x28. El filebuf del host (0x74 B) entra en el hueco de 0x204 del guest. `tools/annotate.py` actualizado a la firma nueva de `parse()`.
  - eboot.bin subido.

- [x] Corrida 18 (`logs/log_018.log` + psp2dmp 1792444073): **avance grande**: `MusicPlayer::Load` OK (fix de fstream confirmado), se cargan todos los niveles `.gmap`, EAGL framebuffer 320x480, `glogo.m4v` saltado, **run loop arrancado** con NSTimer a 60 Hz. Data abort en `strcpy(dst, NULL)` desde `GetDeviceLanguage()` (Mach-O 0x2a6ec), llamado por `GSSplash::update`.
  - Causa: `[[[NSUserDefaults standardUserDefaults] objectForKey:@"AppleLanguages"] objectAtIndex:0] UTF8String` sin chequeo de nil; nuestro NSUserDefaults arrancaba vacío.
  - Fix `objc.c`: `standardUserDefaults` se crea con `AppleLanguages = ("en")`.
  - Prevención `darwin.c`: `strlen/strcpy/strncpy/strcat/strcmp/strncmp/strcasecmp/strstr/atoi/atof/__strcpy_chk` toman NULL como `""` y loguean `WARN <fn>(NULL) from <lr> (Mach-O 0x...)` (máx. 16), en vez de un data abort dentro de newlib. Si un WARN aparece, el bug real está en esa dirección del juego.
  - Prevención `objc.c`: implementados `CGAffineTransformConcat/MakeRotation/MakeScale/Rotate/Translate` y `CGRectOffset` (antes eran "unimpl import" y dejaban basura en el struct de retorno). ABI verificado en el desensamblado (r0 = retorno, struct por valor en r1-r3 + pila).
  - eboot.bin subido.

- [x] Corrida 19 (`logs/log_019.log` + psp2dmp 1792447755): **nuevo avance grande**: el juego superó el splash, cargó `saves/gameprofile.sav`, `_packMac/text.EN.bin`, `Loader.swf`, `loading_bg01.tga`, y el timer a 10 Hz (`drawView`). Data abort en `gameswf::string_to_number(double*, const char*)` (Mach-O 0xe52e8) al parsear el SWF.
  - Causa: llamada a `_strtod` (0xe52d0) no estaba registrada en `darwin_imports` (`[WARN] unimpl import: _strtod`). El stub dummy `ml_unimpl_entry` no escribía `*endptr`, dejando `r3 = endptr = NULL`. El código del guest luego desreferenciaba `r3` con `ldrsb r0, [r3]` (0xe52e8).
  - Fix `source/darwin.c`: implementado `d_strtod(const char *s, char **endptr)` con convención `SOFTFP` y protección contra NULL (`*endptr = NULL; return 0.0;` para que `endptr == s` resulte falso en vez de desreferenciar NULL). Registrado `F(strtod, d_strtod)`. También registrado `F(log, m_log)` preventiva (hermana faltante en math softfp).
  - eboot.bin y skaternation.vpk regenerados en `build/`.

- [x] Corrida 20 (`logs/log_020.log`): **¡HITO ALCANZADO: IN-GAME ACTIVO!** El juego cargó menús, selección de personaje y entró completamente al gameplay 3D interactivo. El usuario solicitó resolver caídas graves de rendimiento y mapear controles físicos.
  - Causa de caídas de FPS: `AudioSessionGetProperty('othr')` en `coreaudio.c` llamaba `l_debug()` múltiples veces por frame en bucle continuo; `l_debug()` realizaba `fflush(log_file)` síncrono a la memoria flash `ux0:`, costando 50–150 ms por frame. Además, el timer `drawView` apuntaba a 60 FPS sin pacing estable.
  - Fix Rendimiento:
    1. `source/log.h` y `source/log.c`: `l_debug()` compilado condicionalmente con `#ifdef DEBUG_LOADER`; en modo Release se optimiza a un no-op inline sin I/O a disco.
    2. `source/uikit.c`: `timer_scheduled` fija el intervalo de `drawView` a `0.03333333` (30 FPS estables) cuando el juego solicita un framerate superior a 30 Hz.
  - Fix Controles Físicos (`source/patch.c` y `source/input.h`):
    1. Ampliado `MAX_TOUCHES` a 16 para permitir concurrencia de multitouch y botones sintéticos.
    2. Virtual D-Pad (Left Stick y D-Pad físico): Mapeado al centro `(90.0f, 235.0f)` con radio `45.0f`. En el primer frame de deflexión inicia el toque en el centro para registrar `touchesBegan` y en frames siguientes arrastra la deflexión para girar de forma suave e instantánea.
    3. Botón A (Cross / R-Trigger): Mapeado a `(434.0f, 276.0f)` (Ollie / Salto / Confirmación).
    4. Botón B (Cuadrado / Círculo / L-Trigger): Mapeado a `(350.0f, 241.0f)` (Grind / Trick).
    5. Triángulo: Mapeado a `(350.0f, 241.0f)` (Modificador de truco).
    6. Start: Mapeado a `(450.0f, 25.0f)` (Pausa / Menú).
    7. Select: Mapeado a `(355.0f, 23.0f)` (Mapa / Retos).
    8. Stick Analógico Derecho: Rotación de cámara orbital mediante arrastre táctil continuo anclado en `(240.0f, 120.0f)` con ID `3000`, zona muerta cuadrática, y control de sensibilidad 1..10 persistente en `camera_sens.txt` (ajustable con `SELECT + UP/DOWN` y visualizable vía `patch_overlay()`).
  - Compilación Release completada: `build/eboot.bin` (1.87 MB) y `build/skaternation.vpk` (1.91 MB) listos para desplegar.

- [x] Corrida 21 (`logs/log_021.log`): Sesión de juego fluida de más de 6 minutos continuos confirmada por el usuario. Solicitadas mejoras: (1) pantalla completa 960x544 sin barras negras y sin estiramiento de aspecto, y (2) opacidad al 1% en los controles táctiles virtuales (solo joystick y botones A/B).
  - Causa de las barras negras: `wide_span()` en `source/gles.c` recortaba el scissor a 36..444 en Y (dejando bandas negras fijas en los extremos) y aplicaba `WIDE_K` de forma indiscriminada.
  - Fix Pantalla Completa Panorámica (`source/gles.c` e `input.c`):
    1. Eliminado el recorte artificial de scissor en `apply_scissor()`, permitiendo que el renderizado 3D y el clear ocupen los 480 px completos del FBO sin pillarbox.
    2. Adaptación 3D en `wide_begin()`: solo escala por `WIDE_K` (= 816/960) las proyecciones de perspectiva 3D (`p[15] == 0, p[11] != 0`). Esto amplía el campo de visión horizontal (FOV) de la cámara a 16:9 real. Al proyectarse en la pantalla 960x544, el aspecto 3D no sufre estiramiento ni deformación anamórfica.
    3. Mapeo táctil en `source/input.c` ajustado a pantalla completa (0..960 -> 0..480) sin el offset de 72 px anterior.
  - Fix Opacidad 1% en Controles (`source/patch.c`):
    1. Implementado `set_character_alpha()` modificando el multiplicador de canal alfa en el `gameswf::cxform` de cada objeto de interfaz.
    2. Aplicado 1% de opacidad (`alpha = 0.01f`) exclusivamente a `btn_dpad`, `mc_pad`, `mc_thumb` (joystick virtual) y `mc_btn_a`, `mc_btn_b` (botones de acción).
    3. El menú de pausa (`btn_menu`), el mapa (`btn_map`), marcador y retos permanecen 100% visibles.
    4. Los controles virtuales a 1% conservan sus pruebas de impacto (hit-testing) de Flash activas, permitiendo control híbrido y botones táctiles funcionales.
    5. Combinación `L + R` añadida para alternar la opacidad entre 1% y 100% en tiempo real.
  - Optimización de E/S (`source/darwin.c`): Redirigidos logs de `printf` y `[guest]` a `l_debug` para evitar micro-pausas por `fflush` durante combos y trucos.
  - Compilación Release completada: `build/eboot.bin` y `build/skaternation.vpk` actualizados.

- [x] Corrida 22 (`logs/log_022.log` + Core Dump `skaternation-psp2core-1792460612-0x001dfb2621-eboot.bin.psp2dmp`): Crash fatal al iniciar el juego tras los cambios de pantalla panorámica y opacidad.
  - Causa raíz 1 (Fatal en SceGxm): `source/gles.c` implementaba un wrapper `apply_scissor()` que se invocaba incondicionalmente durante `g_glClear` y `wide_begin` con `scissor_box[4] = {0, 0, 0, 0}` sin inicializar. Al pasar dimensiones 0x0 a `glScissor`, vitaGL calculaba `region.w - 1 = -1` (0xFFFFFFFF) y lo enviaba a `sceGxmSetRegionClip()`, provocando un Data Abort inmediato en el driver gráfico de Sony (`SceGxm seg1 + 0xa548`).
  - Causa raíz 2 (Corrupción de memoria en gameswf): `set_character_alpha()` intentaba asignar memoria en `ch + 0x98` asumiendo un buffer `cxform`. En gameswf, `ch + 0x98` es una tabla hash interna con constructores propios de C++, por lo que `calloc(1, 0x54)` corrompía la tabla de símbolos del objeto Flash en el arranque.
  - Fix implementado:
    1. `source/gles.c`: Eliminados todos los wrappers manuales de scissor (`apply_scissor`, `g_glScissor`, `g_glClear`, `g_glCopyTexImage2D`). Mapeados directamente a las implementaciones nativas y seguras de vitaGL (`D(glClear)`, `D(glScissor)`, `D(glCopyTexImage2D)`). En `wide_begin()`, solo se escala la matriz de proyección si `p[15] == 0.0f && p[11] != 0.0f` (3D puro), sin tocar el estado de scissor.
    2. `source/patch.c`: Reemplazada la manipulación directa de memoria por el uso de la API oficial de C++ de gameswf exportada en el Mach-O: `gameswf::character::set_member(tu_stringi("_alpha"), as_value(alpha_pct))` usando los símbolos `__ZN7gameswf9tu_stringC1EPKc`, `__ZN7gameswf8as_valueC1Ed` y `__ZN7gameswf9character10set_memberERKNS_10tu_stringiERKNS_8as_valueE`.
    3. Añadida validación de rangos de punteros válidos (`0x81000000 <= ptr < 0x99000000`) y verificación de que `btn_a`, `btn_b` y `dpad` hayan sido instanciados por `HudMenu::Init` antes de aplicar opacidad.
    4. Caché de opacidad (`last_applied_alpha`) para evitar invocar el despacho ActionScript en cada frame, haciéndolo una sola vez al cambiar de estado.
  - Compilación Release completada: `build/eboot.bin` (1.87 MB) y `build/skaternation.vpk` (1.91 MB) listos y verificados.

- [x] Corrida 23 (`logs/log_023.log` + Core Dump `skaternation-psp2core-1792468119-0x00075020f9-eboot.bin.psp2dmp.analysis.txt`): Crash en SceGxm y diagnóstico de audio sin sonido.
  - Causa del crash: Data Abort en `SceGxm seg1 + 0xa548` (`sceGxmSetRegionClip`) desde `scene_reset` en vitaGL invocado por `glClear`. vitaGL se compilaba sin `-DDISABLE_TILE_CLIPPER`, activando el recortador por hardware de SceGxm que aborta cuando se reciben rectángulos degenerate o fuera de límites.
  - Causa de la falta de audio:
    1. Conflicto de puertos Vita: `audio_thread` en `source/coreaudio.c` abría el puerto exclusivo `SCE_AUDIO_OUT_PORT_TYPE_BGM`, bloqueando el backend OpenAL de VitaSDK (`openal-soft`) que requiere dicho puerto para la salida de audio.
    2. Faltaban stubs de UIKit para `MPMusicPlayerController` (`play`, `pause`, `stop`, `setQueueWithItemCollection:`, `setNowPlayingItem:`, `indexOfNowPlayingItem`).
    3. Ausencia de instrumentación y ajuste de ganancia en OpenAL.
  - Fixes implementados:
    1. `CMakeLists.txt`: Añadido `NO_TILE_CLIPPER=1` (`-DDISABLE_TILE_CLIPPER`) a `VITAGL_MAKE_FLAGS_STR`.
    2. `source/gles.c`: Sanitizado `g_glScissor` con clamp `w >= 1, h >= 1`.
    3. `source/patch.c`: Desactivadas llamadas directas a `glScissor` / `glClear` en `patch_overlay()`.
    4. `source/coreaudio.c`: Cambiado el puerto a `SCE_AUDIO_OUT_PORT_TYPE_MAIN` para permitir que OpenAL tome `BGM`, y añadidos wrappers con logging detallado y ganancia global (`alListenerf(AL_GAIN, 1.0f)`).
    5. `source/uikit.c`: Registrados métodos faltantes de `MPMusicPlayerController`.
  - Compilación Release completada: `build/eboot.bin` (1.8 MB) y `build/skaternation.vpk` (2.2 MB).

- [x] Corrida 24 (`logs/log_024.log` + Core Dump `skaternation-psp2core-1792470237-0x00076d26bb-eboot.bin.psp2dmp`): El subsistema OpenAL inicializó y reprodujo correctamente, pero se produjo un Data Abort en `SceGxm` (`0xe008e958`) durante la pantalla de carga del nivel 3D (`Application::_Draw` -> `CCommonGLDriver::endScene` -> `glFlush`).
  - Causas raíz:
    1. `NO_TILE_CLIPPER=1` (`DISABLE_TILE_CLIPPER`) forzaba a vitaGL a usar el búfer de stencil para emular `glScissor`. Como el FBO de la pantalla es solo RGB565/RGBA con profundidad de 16 bits sin stencil (`DEPTHBUFFER_MISSING`), `update_scissor_test()` invocaba operaciones de renderizado en un búfer inexistente.
    2. `glFlush()` en vitaGL forzaba `scene_reset()`, provocando múltiples llamadas a `sceGxmBeginScene()` en el mismo frame sobre el FBO. Al estar `gxm_fbo_rt_size = 1` por defecto, la segunda escena fallaba y `sceGxmSetViewport` intentaba operar sobre una escena no iniciada en `SceGxm`. En iOS/PowerVR SGX, `glFlush()` es un no-op y no debe reiniciar escenas a mitad de presentación.
    3. `sceGxmSetRegionClip` en `vendor/vitaGL` no verificaba si `region.w <= 0 || region.h <= 0`, causando que `region.w - 1` hiciera underflow a `0xFFFFFFFF`.
    4. Convención de llamada ABI softfp en `gameswf::as_value::as_value(double)`: `as_ctor` requería que el valor `double` se pasara en `(r1, r2)` y no en el registro de hardware VFP `d0`.
  - Soluciones implementadas:
    1. `CMakeLists.txt`: Eliminado `NO_TILE_CLIPPER=1` para restaurar el clipping nativo por hardware de la GPU de PS Vita.
    2. `vendor/vitaGL/source/gxm.c` y `vendor/vitaGL/source/tests.c`: Añadidas guardas `region.w > 0 && region.h > 0` para evitar underflows en `sceGxmSetRegionClip`.
    3. `source/gles.c`: Añadido `vglSetupRenderTargetScenesNum(4, 16)` para permitir múltiples escenas por frame sin agotar descriptores.
    4. `source/gles.c`: Mapeado `glFlush` a `g_glNoop` (no-op estándar en TBDR / iOS).
    5. `source/patch.c`: Implementado empaquetado de `double` a `(uint32_t val_lo, uint32_t val_hi)` para `as_ctor` en `set_character_alpha`.
  - Compilación Release completada: `build/eboot.bin` (1.8 MB) y `build/skaternation.vpk` (2.2 MB).

- [x] Corrida 25 (`logs/log_025.log` + Core Dump `skaternation-psp2core-1792471263-0x001fe52db1-eboot.bin.psp2dmp`): Data Abort en `SceGxm seg1 + 0x1e638` en el primer `glClear` (vía `update_scissor_test`), antes de la pantalla de carga.
  - Causa raíz: `vglSetupRenderTargetScenesNum(4, 16)` de la corrida 24. sceGxm admite máximo 8 escenas por render target (`MAX_SCENES_PER_FRAME`); vitaGL marcaba el error pero asignaba 16 igual, `sceGxmCreateRenderTarget` fallaba, el FBO quedaba sin render target, `sceGxmBeginScene` fallaba y todos los comandos GXM siguientes se emitían fuera de escena.
  - Revisión de corridas 22-24: los dumps de 23 y 24 son idénticos (mismo PC/registros, ~18 s tras arrancar, al crearse el HUD): los fixes de tile clipper/glFlush/escenas no tocaron la causa. Causa real: `set_character_alpha()` en `source/patch.c` llamaba a funciones gameswf del guest por puntero C directo, sin `call_guest` -> el código iOS pisa r9 (callee-saved en Vita) y corrompe el estado del host.
  - Fix:
    1. `source/patch.c`: las 5 llamadas a gameswf (tu_string, as_value, set_member) pasan por `call_guest`. ABI de `as_value(double)` verificada con otool: double en r1:r2. La opacidad se re-aplica si el HUD se recrea (nuevo nivel).
    2. `source/gles.c`: quitado `vglSetupRenderTargetScenesNum` y restaurado `D(glFlush)` como en el build jugable 7e6ebbe (eran fixes de un diagnóstico erróneo).
    3. `vendor/vitaGL/source/gxm.c`: `vglSetupRenderTargetScenesNum` acota a `[1, 8]`.

- [x] Corrida 26 (`logs/log_026.log` + Core Dump `skaternation-psp2core-1792471843-0x001bad2f0d-eboot.bin.psp2dmp`): crash en la pantalla de carga, mismos registros que 22/23/24 (el fix de r9 de la corrida 25 no era la causa: el loader ya compila con `-ffixed-r9`).
  - Pila resuelta con atos (guest base 0x98000000): `Application::_Draw` -> `irr::video::CCommonGLDriver::endScene()+28` -> stub 0x4d7f14 = `_glFlush` (otool -Iv) -> vitaGL `glFlush` -> `scene_reset` (termina y reabre la escena del FBO 320x480) -> `glViewport` -> Data Abort en `SceGxm seg1 + 0xa548`.
  - Fix: `source/gles.c` mapea `glFlush` a `g_glNoop` (solo, sin cambiar escenas por frame; en la corrida 24 se aplicó junto a `(4, 16)`, que rompió el arranque y nunca se llegó a probar).

- [ ] Corrida 27 (`logs/log_027.log` + Core Dump `skaternation-psp2core-1792472151-0x001d2625ff-eboot.bin.psp2dmp`): con `glFlush` en no-op el crash **se movió**: misma instrucción en SceGxm (`seg1 + 0xa548`, `sceGxmSetViewport` tras un `sceGxmBeginScene` fallido) pero ahora desde `gles_present` -> `glClear` sobre la pantalla (gles.c:503) -> `scene_reset`. Conclusión: a partir de cierto punto de la carga *cualquier* BeginScene falla (FBO o display); `glFlush` era solo el primero en tropezar. El no-op de `glFlush` se mantiene.
  - Experimento: **audio pausado** para descartarlo. `source/coreaudio.c`: `#define AUDIO_ENABLED 0` selecciona una tabla de imports con stubs mudos (OpenAL con dispositivo/contexto falsos no nulos, IDs secuenciales, `AL_SOURCE_STATE` = `AL_STOPPED`; `AudioOutputUnitStart` no crea el hilo `coreaudio`). La implementación real (OpenAL de openal-soft + RemoteIO sobre sceAudioOut) queda intacta en la rama `#if AUDIO_ENABLED`; para reactivar, poner `1`.
  - Puntos de acceso del audio (todos en `source/coreaudio.c`): tabla `coreaudio_imports` (OpenAL `al*`/`alc*`, `AudioComponent*`, `AudioUnit*`, `AudioOutputUnitStart/Stop`, `AudioSession*`), registrada en `source/imports.c:16`. Ningún otro archivo llama a OpenAL ni a sceAudioOut.
  - Nota: el crash ya ocurría en la corrida 22, antes de los cambios de audio (puerto MAIN, wrappers OpenAL, commit 56d62fc); el audio original de 7e6ebbe sí estaba activo. Si el crash persiste sin audio, siguiente sospechoso: el resto del delta 7e6ebbe -> 297e1cc (`wide_begin` solo en perspectiva, opacidad del HUD, scissor).

- [ ] Corrida 28 (`logs/log_028.log` + Core Dump `skaternation-psp2core-1792472379-0x000d3a2b75-eboot.bin.psp2dmp`): con el audio pausado el crash cambió a código del juego, antes (cargando `*.gmap`).
  - Pila (atos): `Application::LoadOrientationStyle()` -> `SetStatusBarOrientation(bool)+264`: `ldr r3, [WorldManager::GetInstance() + 0x4528]` = `0x1a000003` (basura) -> `ldr [r3, #0x60]` -> Data Abort.
  - Causa raíz: `WorldManager` se crea con `operator new(0x46d0)` y su constructor pone a cero `+0x4520/+0x4524/+0x4534` pero nunca `+0x4528`. En iOS la memoria nueva llega en cero; newlib devuelve basura. El juego depende de memoria inicializada en cero, así que el lugar del crash depende de cómo esté el heap: explica por qué los crashes "se mueven" al cambiar cosas no relacionadas (audio, widescreen, HUD). Probablemente también sea la causa de fondo del crash de SceGxm (22-27).
  - Fix: `source/cxx.c`: `__Znwm`/`__Znam`/`__ZnwmRKSt9nothrow_t` -> `cxx_new_zeroed` (calloc). `source/darwin.c`: `malloc` -> `d_malloc` (calloc). El audio sigue pausado (un cambio por corrida).

- [ ] Corrida 29 (`logs/log_029.log` + Core Dump `skaternation-psp2core-1792472728-0x000ac52a97-eboot.bin.psp2dmp`): el crash de `WorldManager` desapareció (fix de memoria en cero OK) y volvió el de SceGxm en la carga: `gles_present` -> `glClear` (pantalla) -> `scene_reset` -> `sceGxmSetViewport` abort. Mismos registros que en 22-27.
  - Descartados con evidencia: audio (stubs mudos, sin puerto ni hilo) y opacidad del HUD (`HUD controls opacity set` nunca aparece en el log, así que `set_member` no corrió).
  - Instrumentación (sin fix especulativo): `vendor/vitaGL/source/gxm.c` reporta vía `GXM_REPORT` cualquier error de `sceGxmEndScene`, `sceGxmBeginScene` (display/fbo) y `sceGxmCreateRenderTarget` a `vgl_gxm_error_hook()` (weak, definido en `source/gles.c`), que lo escribe en el log con la memoria libre de VRAM/RAM/PHYCONT de vitaGL (máx. 16 líneas). `vgl_log` no sirve: va a `sceClibPrintf`, no al archivo.
  - Próxima corrida: leer las líneas `vitaGL: ... = 0x8xxxxxxx` del log para saber POR QUÉ falla la escena (errcode con `psvita-toolkit errcode`).

- [ ] Corrida 30 (`logs/log_030.log` + Core Dump `skaternation-psp2core-1792472976-0x0008452829-eboot.bin.psp2dmp`): mismo abort en `SceGxm seg1 + 0xa548` desde `gles_present -> glClear -> scene_reset -> glViewport`, pero **ninguna** línea `vitaGL: ...` en el log: `sceGxmBeginScene`/`sceGxmEndScene`/`sceGxmCreateRenderTarget` NO fallan. La hipótesis de "escena fallida" (corridas 24-29) era incorrecta.
  - Contexto GXM en el dump (`0x81c05c18`) íntegro, sin datos del guest; viewport de pantalla en `+0x1f0` coherente (480/480/272/-272). Los registros son idénticos en todas las corridas: fallo determinista dentro de SceGxm, no basura aleatoria. El código de SceGxm no está en el dump (no se usa Vita3K para obtenerlo).
  - Bisección: lo único del delta 7e6ebbe -> 297e1cc que corre en la carga (opacidad nunca se ejecuta, audio mudo) es la parte gráfica de `source/gles.c`. Se restauró **exactamente** el bloque de 7e6ebbe (scissor con `wide_span`/`apply_scissor` cacheado, `g_glClear`, `g_glCopyTex*`, `wide_begin` sobre todas las proyecciones, `D(glFlush)` real) y `source/input.c` (táctil `(x-72)*480/816`). Se mantienen: memoria en cero, hook GXM, audio mudo.
  - Si pasa la carga: el bug está en el widescreen de 297e1cc; reintroducirlo en partes (primero `wide_begin` solo perspectiva, luego quitar `wide_span`). Si no: el bug está en lo que queda (vendor vitaGL, `patch.c`, `darwin.c`).

- [x] Corrida 31 (`logs/log_031.log`): **sin crash**, juego jugable. Confirmado: el crash de SceGxm (22-30) venía del widescreen de 297e1cc. Pero ~7 FPS.
  - Causa de los FPS: build Debug (`DEBUG_LOADER`) + `AudioSessionGetProperty('othr')` logueado con `l_debug` (fflush síncrono a ux0, ~10 ms por línea) varias veces por frame (878 líneas en el log). Mismo problema ya visto en la corrida 20; el build de 30 FPS de 7e6ebbe era Release (sin `l_debug`).
  - Fix: `source/coreaudio.c` responde `'othr'` (OtherAudioIsPlaying = 0) sin loguear.

## ÚNICO PASO PENDIENTE: prueba en consola física

Con la Vita conectada a 192.168.3.15 (VitaShell / FTP activo):

1. [ ] Instalar `libshacccg.suprx` si falta (ShaRKBR33D) — `main.c` morirá con FATAL si falta.
2. [ ] Subir VPK e instalar: `psvita-toolkit deploy --vpk --vpk-path build/skaternation.vpk` (o instalar por VitaShell).
3. [ ] Desplegar datos (337 MB): `psvita-toolkit deploy --data`.
4. [ ] Lanzar el juego y ver gate del loader:
   - GATE-1: log llega al bucle principal / init del motor (`logs-live` o `deploy_data.py --logs`).
   - GATE-2: primer frame / pantalla de inicio visible.
   - GATE-3: controles responden.
   - GATE-4: diálogos/menús legibles.
   - GATE-5: música y efectos suenan.
5. [ ] Si hay crash: `deploy_data.py --logs` → `psvita-toolkit analyze <dump>.psp2dmp --elf build/skaternation.elf`; PC en `0x98xxxxxx` − `0x98000000` = offset en Mach-O (`tools/annotate.py`).

- [ ] Corrida 32 (`logs/log_032.log`, Release): sin crash, sigue a 7-8 FPS sin logs por frame -> el cuello de botella NO es el log. Barras negras visibles con el método de ZI (WIDE_K 816/960).
  - Cambios para corrida 33: pantalla completa como Blades of Fury (`gles.c`: `WIDE_K 1.0`, `wide_target()` -> 0, estiramiento simple 480->960; `input.c`: toque `x*480/960`). Audio reactivado (`AUDIO_ENABLED 1`).
  - Instrumentación: `gles_present` escribe cada 5 s `perf: N fps, frame X ms, swap Y ms` (swap = dentro de `vglSwapBuffers`; frame - swap = juego + GL). Decide la próxima optimización.

- [x] Corrida 33 (`logs/log_033.log`): pantalla completa sin barras, audio OK, **25-28 FPS en juego** (`perf: ... swap 0.1-0.3 ms`: el tiempo es del juego/CPU, no de la GPU). Quedaba visible el aro del D-pad.
  - Causa: offsets de `update_hud_opacity` mal etiquetados. Nombres reales (`HudMenu::Init` -> `RenderFX::Find`, base hud+0x1000): `+0x254 btn_map`, `+0x258 mc_map`, `+0x25c mc_thumb`, `+0x260 btn_dpad`, `+0x264 mc_pad`, `+0x28c mc_btn_a`, `+0x294 mc_btn_b`. Se ocultaba `mc_map` por error y nunca `mc_pad` (el aro).
  - Fix: `source/patch.c` oculta `+0x1264` (mc_pad) en lugar de `+0x1258` (mc_map).
