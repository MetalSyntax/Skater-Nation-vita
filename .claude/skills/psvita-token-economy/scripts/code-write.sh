#!/usr/bin/env bash
# Delegate boilerplate generation to a cheap model. It writes straight to
# disk; the caller gets a one-line confirmation, never the generated code.
set -euo pipefail

MODEL="${PSVITA_ECON_MODEL:-claude-haiku-4-5-20251001}"
MAX_CHARS=300000

if [[ $# -lt 3 ]]; then
  echo 'Uso: code-write.sh <spec_archivo_o_texto> <archivo_referencia> <ruta_salida>' >&2
  echo 'Ej: code-write.sh "stub JNI para GFA_MeasureText, firma (F)I, ver tabla de riesgo del wrapper" \' >&2
  echo '    source/falso_jni/FalsoJNI_Impl.c source/falso_jni/stubs/gfa_measuretext.c' >&2
  exit 1
fi

SPEC_ARG="$1"
REFERENCE_FILE="$2"
OUTPUT_PATH="$3"

if [[ -f "$SPEC_ARG" ]]; then
  SPEC=$(cat "$SPEC_ARG")
else
  SPEC="$SPEC_ARG"
fi

if [[ ! -f "$REFERENCE_FILE" ]]; then
  echo "Error: no existe el archivo de referencia '$REFERENCE_FILE'" >&2
  exit 1
fi

PROMPT_FILE=$(mktemp)
RAW_OUT=$(mktemp)
trap 'rm -f "$PROMPT_FILE" "$RAW_OUT"' EXIT

{
  echo "Generá código boilerplate copiando EXACTAMENTE las convenciones (nombres, estilo,"
  echo "manejo de errores, includes) del archivo de referencia. Output: SOLO el contenido"
  echo "crudo que va a ir en el archivo de salida -- nada de \`\`\`bloques de código\`\`\`,"
  echo "nada de explicación antes o después, nada de comentarios tipo \"aquí está tu código\"."
  echo
  echo "ESPECIFICACIÓN:"
  echo "$SPEC"
  echo
  echo "<reference path=\"${REFERENCE_FILE}\">"
  cat "$REFERENCE_FILE"
  echo "</reference>"
  echo
  echo "RUTA DE SALIDA (para que el estilo/extensión/lenguaje coincida): ${OUTPUT_PATH}"
} > "$PROMPT_FILE"

PROMPT_SIZE=$(wc -c < "$PROMPT_FILE")
if (( PROMPT_SIZE > MAX_CHARS )); then
  echo "Error: la especificación + referencia suman $PROMPT_SIZE bytes (límite $MAX_CHARS)." >&2
  echo "Usá un archivo de referencia más chico o más puntual." >&2
  exit 1
fi

claude -p "$(cat "$PROMPT_FILE")" \
  --model "$MODEL" \
  --tools "" \
  --no-session-persistence \
  --output-format text \
  --permission-prompts none \
  < /dev/null \
  > "$RAW_OUT"

# Si el modelo igual metió un bloque ```lang ... ```, pelarlo.
if head -n1 "$RAW_OUT" | grep -q '^```'; then
  sed -e '1d' -e '$d' "$RAW_OUT" > "${RAW_OUT}.stripped"
  mv "${RAW_OUT}.stripped" "$RAW_OUT"
fi

mkdir -p "$(dirname "$OUTPUT_PATH")"
cp "$RAW_OUT" "$OUTPUT_PATH"

LINES=$(wc -l < "$OUTPUT_PATH")
echo "Escrito: ${OUTPUT_PATH} (${LINES} líneas). Revisar con un diff puntual, no releyendo el archivo entero."
