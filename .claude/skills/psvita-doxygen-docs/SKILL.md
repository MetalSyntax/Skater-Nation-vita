---
name: psvita-doxygen-docs
description: Estándar y automatización de documentación en código C/C++ en formato Doxygen para ports de PS Vita (soloader/FalsoJNI), junto con la generación obligatoria de un archivo Markdown crudo (.md) por cada archivo fuente para documentar la arquitectura profunda y la ingeniería inversa. Incluye plantillas de prompts blindadas y scripts de delegación para que IAs de menor capacidad (Haiku, Flash-Lite, GPT-4o-mini, LLMs locales) conviertan bloques de comentarios informales `//` o `/*` en bloques Doxygen válidos sin romper el código ni alterar la lógica. Usar al documentar código de loader, wrappers JNI, parches de memoria o al refactorizar comentarios dispersos en un port.
---

# psvita-doxygen-docs — Documentación Doxygen y Companion Markdown para Ports PS Vita

Esta skill establece la metodología estándar para documentar código nativo C/C++ en ports de Android a PS Vita (soloader, FalsoJNI, parches de memoria, wrappers GXM/audio/input).

Resuelve tres necesidades críticas del ecosistema de ports:
1. **Código limpio y estandarizado con Doxygen**: Reemplaza comentarios informales `//` dispersos por bloques Doxygen (`/** ... */`) rigurosos, parseables por IDEs, Doxygen y generadores de documentación.
2. **Documentación pura y cruda en Markdown independiente**: La arquitectura pesada, las notas de ingeniería inversa (Ghidra / JADX) y la justificación de stubs **NO** deben sobrecargar el archivo C con 80 líneas de prosa. Se genera obligatoriamente un archivo Markdown (`docs/<ruta_relativa>.md`) por cada archivo fuente como referencia profunda.
3. **Protocolo para delegar a IAs menos inteligentes**: Reglas estrictas y prompts blindados para que modelos más baratos (Claude Haiku, Gemini Flash-Lite, GPT-4o-mini, o LLMs locales) conviertan comentarios `//` a Doxygen sin tocar una sola línea de código ejecutable ni alucinar parámetros.

---

## 1. Cuándo usar esta skill

- Al escribir nuevo código de loader (`main.c`, `patch.c`, wrappers JNI, audio o video).
- Al encontrar archivos con explicaciones largas en comentarios informales `//` o `/* ... */` que saturan la lectura del código.
- Al necesitar generar la documentación arquitectónica profunda de un módulo del port sin ensuciar la fuente C.
- Al orquestar una sesión donde se delega a un modelo económico (o un subagente ligero) la tarea mecánica de convertir comentarios `//` a Doxygen.

---

## 2. Estándar Doxygen para C/C++ en Ports PS Vita

En el entorno VitaSDK (GCC ARM, C99 / C++11), las convenciones deben ser estrictas y legibles:

### A. Bloques de Función y Símbolos
Usar siempre la sintaxis de bloque `/** ... */`:

```c
/**
 * @brief Implementa el callback JNI para medir ancho de texto en pantalla.
 * @details Consulta las métricas de glifo precalculadas en el motor del juego.
 *          Si la cadena contiene caracteres no imprimibles o nulos, retorna 0.
 *
 * @param[in] env   Puntero al entorno JNI simulado (FalsoJNI).
 * @param[in] obj   Instancia del objeto Java invocador (no usado en stub estático).
 * @param[in] text  Cadena de texto UTF-8 o identificador de string en assets.
 *
 * @return Ancho en píxeles virtuales del texto (>= 0), o 0 si text es NULL.
 *
 * @note Ver `docs/source/falso_jni/text_renderer.md` para detalles de fuentes e ingeniería inversa.
 * @warning Debe llamarse dentro del hilo principal de renderizado (afectado por contexto GXM).
 * @see FalsoJNI_RegisterNatives, sceGxmMapVertexUsable
 */
int Java_com_gameloft_Natives_GFA_MeasureText(JNIEnv *env, jobject obj, jstring text);
```

### B. Miembros de Estructuras y Enumeraciones
Usar la sintaxis de documentación inline tras el miembro (`/**< ... */`):

```c
typedef struct VitaAudioVoice {
    SceUID thread_id;        /**< ID del hilo reproductor de audio asignado por el kernel de Vita. */
    int channel_index;       /**< Índice del canal estéreo (0 a VITA_MAX_VOICES - 1). */
    volatile int is_playing; /**< Bandera atómica de reproducción activa (1 = sonando, 0 = detenido). */
    void *pcm_buffer;        /**< Búfer PCM en memoria LPDDR de la consola. */
} VitaAudioVoice;
```

### C. Encabezado de Archivo Obligatorio (`@file`)
Todo archivo `.c`, `.h` o `.cpp` debe comenzar con un bloque Doxygen de archivo:

```c
/**
 * @file patch.c
 * @brief Parches binarios en memoria y ganchos de ensamblador para Zenonia 3.
 * @details Neutraliza llamadas a Google Play Licensing, parchea alineación ARMv7
 *          en rutinas de animación y desvía la apertura de rutas hacia `app0:/`.
 *
 * @author Porting Team
 * @note Ver documentación detallada en `docs/source/patch.md`.
 */
```

### D. Tags Doxygen Permitidos y Esperados

| Tag | Uso | Ejemplo |
|---|---|---|
| `@file` | Nombre del archivo y propósito general | `@file falso_jni.c` |
| `@brief` | Resumen de una única línea clara y concisa | `@brief Inicializa el subsistema de audio FMOD.` |
| `@details` | Explicación técnica extendida del comportamiento | `@details Asigna memoria en CDRAM y configura...` |
| `@param[in]` | Parámetro de entrada (lectura) | `@param[in] filepath Ruta del archivo a cargar.` |
| `@param[out]` | Parámetro de salida (escritura/puntero) | `@param[out] out_size Tamaño real leído en bytes.` |
| `@param[in,out]` | Parámetro que se lee y se modifica | `@param[in,out] ctx Contexto del decompresor.` |
| `@return` | Qué valor retorna y qué significan los errores | `@return 0 en éxito, < 0 (código SCE) en fallo.` |
| `@note` | Notas de diseño o enlace al archivo Markdown | `@note Ver docs/source/falso_jni.md para offsets Ghidra.` |
| `@warning` | Trampas de memoria, condiciones de carrera o threads | `@warning No invocar antes de sceKernelAllocMemBlock.` |
| `@see` | Referencia cruzada a otra función, struct o macro | `@see audio_init, SceUID` |

---

## 3. Regla del Archivo Markdown Crudo por Archivo Fuente

### ¿Por qué esta regla?
En ports de Android a Vita, los desarrolladores suelen dejar comentarios gigantes explicando:
- Por qué un stub de Java devuelve `0` o `NULL`.
- Cómo se comporta la función original decompilada en Ghidra en la dirección `0x81234a`.
- Diferencias de formato de texturas (PVRTC vs GXM).
- Problemas con alineación ARMv7 `ldrd`/`strd`.

Tener 50 líneas de comentarios por función destruye la legibilidad del archivo C.

### Ubicación del Markdown Complementario:
Para un archivo fuente en:
- `source/falso_jni/game_wrapper.c`

El archivo Markdown complementario **DEBE** crearse en:
- `docs/source/falso_jni/game_wrapper.md`
*(o `docs/<relpath_sin_extension>.md` en la raíz del proyecto).*

### Estructura Mandatoria del Archivo Markdown (`docs/.../<archivo>.md`):

```markdown
# Documentación Arquitectónica: `source/falso_jni/game_wrapper.c`

- **Archivo Fuente:** `source/falso_jni/game_wrapper.c`
- **Módulo:** FalsoJNI Subsystem / Wrapper
- **Última actualización:** [FECHA]

---

## 1. Propósito y Arquitectura en el Port

Explicación de alto nivel de qué hace este archivo, por qué existe en el loader,
y cómo se conecta con el binario `.so` original del juego Android.

---

## 2. Mapa de Ingeniería Inversa y Símbolos

Tabla cruzada de símbolos originales de Android vs implementación en el loader:

| Símbolo Android / JNI | Dirección Ghidra / Firma | Implementación Vita | Comportamiento / Parche |
|---|---|---|---|
| `Java_com_ea_Natives_init` | `0x0024f180 (Thumb)` | `Java_com_ea_Natives_init` | Inicializa render GL y memoria |
| `checkLicenseNative` | `0x00312014 (ARM)` | Stub en línea | Retorna `1` (licencia válida) |

---

## 3. Decisiones de Diseño y Memoria

- **Tipo de Memoria Vita:** Especificar si usa LPDDR (`SCE_KERNEL_MEMBLOCK_TYPE_USER_RW`) o CDRAM (`SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW`).
- **Seguridad entre hilos:** Qué llamadas se ejecutan en el hilo principal de renderizado y cuáles en threads secundarios.
- **Manejo de diferencias POSIX/Android:** Manejo de rutas `sdcard/` -> `ux0:data/juego/`.

---

## 4. Registro Crudo de Comentarios y Rationale por Función

### `init_audio_driver` (línea ~45)
> Comentario original extraído:
> El motor original intenta abrir OpenSL ES con buffer de 4096 frames a 44100Hz.
> En PS Vita, OpenSL no existe nativamente. Redirigimos a libsound/sceAudioOut
> usando doble búfer de 1024 frames para evitar chasquidos por underrun.

**Rationale:**
Se sustituyó por `sceAudioOutOpenPort` con formato PCM estéreo de 16 bits.

---
```

Y en el código C, la función solo tiene un bloque Doxygen conciso:

```c
/**
 * @brief Inicializa el backend de audio redirigiendo OpenSL a sceAudioOut.
 * @param[in] sample_rate Frecuencia de muestreo en Hz (ej. 44100).
 * @return 0 en éxito, código de error negativo si falla la apertura del puerto.
 * @note Ver `docs/source/audio.md` para detalles de buffer y latencia.
 */
int init_audio_driver(int sample_rate);
```

---

## 4. Protocolo para Delegar a IAs Menos Inteligentes (Low-Tier LLMs)

Modelos económicos como Claude Haiku, Gemini Flash-Lite, GPT-4o-mini o LLMs locales son ideales para tareas repetitivas de formateo de texto, pero cometen errores graves si las instrucciones no son impenetrables.

### Los 5 Riesgos Típicos de las IAs Menos Inteligentes:
1. **Alterar el código C**: Cambiar tipos, reescribir bloques o agregar casts innecesarios.
2. **"Lazy coding"**: Omitir código con comentarios tipo `// ... rest of code unchanged ...`.
3. **Alucinar parámetros**: Inventar parámetros `@param` que no existen en la firma de la función.
4. **Romper macros y directivas preprocesador**: Malinterpretar `#ifdef` o macros JNI.
5. **No extraer el Markdown crudo**: Dejar la explicación larga dentro del C en vez de separarla.

---

### Prompt Maestro Blindado para la IA Pequeña

Cuando le pidas a una IA menos inteligente transformar comentarios `//` a Doxygen, pásale **EXACTAMENTE** este prompt (disponible también en `scripts/prompt-template-cheap-ai.txt`):

```text
Eres un formateador estricto de código C y documentación Doxygen para un port de videojuego a PS Vita.
Tu trabajo es PURAMENTE MECÁNICO de formato. Tienes PROHIBIDO cambiar una sola línea de código ejecutable.

TAREA:
1. Lee el archivo o fragmento de código C adjunto.
2. Identifica todos los bloques de comentarios informales que empiecen con "//" o "/*" que expliquen funciones, structs, variables o el archivo.
3. Convierte cada comentario explicativo en un bloque Doxygen estándar inmediatamente sobre la función, struct o variable correspondiente.
4. Si el comentario contiene una explicación larga de diseño, arquitectura o ingeniería inversa:
   - Sintetiza una versión concisa en el bloque Doxygen (@brief, @param, @return).
   - Añade la etiqueta: @note Ver <DOC_MD_PATH> para detalles de arquitectura.
   - Extrae el texto explicativo original al archivo Markdown complementario.

REGLAS INQUEBRANTABLES:
1. EL CÓDIGO C ES SAGRADO: No cambies nombres de variables, no cambies lógica, no agregues llaves, no reformatees cuerpos de funciones. Todo el código ejecutable debe quedar IDÉNTICO carácter por carácter.
2. NO USES RESÚMENES: Está TERMINANTEMENTE PROHIBIDO poner comentarios tipo "// ... resto del código igual ...". Debes entregar el archivo completo o el bloque solicitado íntegro.
3. PRECISIÓN DE FIRMA: Si la función recibe (int a, void *b), los @param deben ser EXACTAMENTE 'a' y 'b'. NO inventes parámetros. Si la función es 'void', NO pongas @return.
4. SINTAXIS DOXYGEN: Usa /** ... */ con asteriscos alineados. Para miembros de structs usa /**< @brief ... */.

FORMATO DE TU RESPUESTA:
Debes devolver DOS secciones claramente delimitadas:

=== ARCHIVO_MD ===
(Contenido del archivo Markdown con la documentación cruda extraída y justificaciones de diseño)
=== FIN_ARCHIVO_MD ===

=== CODIGO_FUENTE ===
(El código C completo con los comentarios // reemplazados por bloques Doxygen limpios)
=== FIN_CODIGO_FUENTE ===

ARCHIVO FUENTE A PROCESAR: <SOURCE_PATH>
RUTA DEL ARCHIVO MD: <DOC_MD_PATH>

CÓDIGO:
```

---

## 5. Automatización y Scripts

Esta skill incluye herramientas ejecutables en la subcarpeta `scripts/`:

### A. `scripts/convert-comments.sh`
Permite procesar archivos individuales o directorios enteros. Puede usar el toolkit local (`psvita-toolkit` / `gen_docs.py`) o invocar a un modelo económico headless (`claude -p` con Haiku):

```bash
# Procesar un archivo individual C extrayendo comentarios a docs/
~/.claude/skills/psvita-doxygen-docs/scripts/convert-comments.sh source/falso_jni/game.c

# O usando el script en el entorno de Gemini:
~/.gemini/config/skills/psvita-doxygen-docs/scripts/convert-comments.sh source/audio.c
```

### B. Verificación de Integridad Post-Conversión
Siempre correr después de una transformación con IA:
1. `git diff` para revisar que SOLO cambiaron los comentarios y que el código C es idéntico.
2. Compilar con el toolchain de Vita (`psvita-toolkit build` o `cmake --build build`) para garantizar que no se introdujo ningún error de sintaxis ni comentario no cerrado.
3. Confirmar que el archivo `docs/<archivo>.md` existe y contiene la información arquitectónica cruda.
