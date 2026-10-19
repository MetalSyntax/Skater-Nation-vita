---
name: psvita-token-economy
description: Optimiza y reduce en más de un 90% el consumo de tokens en triage de crashes y ports de PS Vita. Delega lecturas masivas de archivos gigantes (pseudo-C de Ghidra, fuentes jadx, dumps objdump/nm, triage_summary.md) y generación de boilerplate repetitivo (stubs JNI, glue de loader) a subagentes ligeros (Flash-Lite / Flash / Research) o scripts de delegación rápida (`bulk-read.sh`, `code-write.sh`), reservando el contexto principal para el razonamiento profundo. Activar proactivamente antes de leer archivos de desensamblado/decompilación extensos o generar stubs masivos en proyectos de PS Vita.
---

# Economía de Tokens para Triage de Crashes y Ports PS Vita en Antigravity

Los archivos más grandes en el desarrollo de ports para PS Vita (`decompiled_so/out_ghidra.c`, fuentes `jadx`, dumps de `objdump -d`, logs de consola completos) contienen decenas de miles de líneas. Rara vez se requiere leerlos por completo: suele bastar una pregunta puntual (*"¿qué funciones llaman a X?"*, *"¿cuál es la firma JNI de Y?"*). 

Cargarlos enteros en el contexto del agente principal satura la ventana de contexto y encarece la sesión. Esta skill establece la metodología para resolver consultas sobre archivos gigantes y generar código repetitivo sin desperdiciar tokens.

---

## 1. Los Dos Mecanismos de Ahorro en Antigravity

### Opción A: Subagentes Ligeros Nativos (Recomendado dentro de Antigravity)
Antigravity permite invocar subagentes especializados (`invoke_subagent`):
- **Lectura/Extracción Masiva**: Invocar un subagente con `Model: "flash_lite"` o `Role: "research"` con la tarea de leer el archivo y extraer únicamente una lista de bullets con la información requerida (números de línea y firmas). El archivo gigante reside solo en la memoria del subagente; el agente principal recibe únicamente el resumen compacto.
- **Generación de Boilerplate**: Invocar un subagente con `Model: "flash"` para generar stubs repetitivos o glue code basados en un archivo de referencia y escribirlo directamente a disco.

### Opción B: Scripts de Delegación CLI (Terminal)
Ubicados en `~/.gemini/config/skills/psvita-token-economy/scripts/`:
- **`bulk-read.sh <archivo> "<pregunta puntual>"`**:
  Envía el archivo a un modelo económico (`claude-haiku` vía CLI o script) y devuelve exclusivamente bullets densos al stdout:
  ```bash
  ~/.gemini/config/skills/psvita-token-economy/scripts/bulk-read.sh decompiled_so/out_ghidra.c \
    "buscar la función CGxFACharCache::findChar: qué hace con el puntero que recibe, y si lo chequea contra NULL antes de desreferenciarlo (citar línea)"
  ```
- **`code-write.sh <spec> <archivo_referencia> <ruta_salida>`**:
  Genera el código directamente en disco respetando las convenciones del archivo de referencia, evitando que el código generado contamine el contexto:
  ```bash
  ~/.gemini/config/skills/psvita-token-economy/scripts/code-write.sh \
    "stub JNI para GFA_MeasureText, firma (F)I" \
    source/falso_jni/stubs/gfa_drawtext.c \
    source/falso_jni/stubs/gfa_measuretext.c
  ```

---

## 2. Cuándo Delegar vs. Cuándo Leer Directamente

### Delegar a Subagente / `bulk-read.sh`:
- Búsqueda en `out_ghidra.c` (>10.000 líneas).
- Escaneo de clases grandes en `apk_decompilado/sources/**/*.java`.
- Búsqueda de strings o firmas en volcados de `objdump` o `nm`.
- La pregunta es puntual: *"¿dónde se define X?"*, *"¿qué métodos llaman a Y?"*.

### NO Delegar (Leer directamente con `view_file` acotado):
- **Paso 8 de `so-crash-triage`**: El razonamiento fino sobre el wrapper nativo antes de programar un fix. Un modelo ultraligero puede pasar por alto sutilezas de punteros o flags de retorno.
- **Diagnóstico de causa raíz**: Decisión de parchear memoria o desensamblado exacto.
- Archivos ya pequeños (<300 líneas): Es más rápido y seguro leerlos directamente.

---

## 3. Cuándo Usar Generación Desacoplada (`code-write.sh` o Subagente Escritor)

- Generar un nuevo stub JNI que imita la estructura de 5 stubs hermanos en `source/falso_jni/`.
- Crear wrappers repetitivos de funciones de OpenGL / VitaGL / audio.
- Tras la generación, verificar únicamente mediante `git diff --stat` o inspección puntual de líneas, **nunca releer todo el archivo generado**.

**Cuándo NO delegar escritura**:
- Edición quirúrgica de lógica compleja en un wrapper existente.
- Parcheo binario en el `.so` (`psvita-toolkit so-patch --apply-patch`).
- Código donde una sutil alucinación pueda provocar un kernel panic en la consola física.

---

## 4. Parámetros y Límites

| Parámetro | Valor | Detalle |
|---|---|---|
| Umbral de líneas para delegar | >350 líneas | Archivos menores se leen directamente con `view_file`. |
| Subagente preferido | `flash_lite` / `flash` | Para lectura rápida y generación sin saturar el contexto. |
| Modelo económico CLI | `claude-haiku-4-5-20251001` | Override con variable de entorno `PSVITA_ECON_MODEL`. |
| Límite por delegación CLI | ~300.000 caracteres | Archivos más grandes deben acotarse primero con `grep` antes de delegar. |
