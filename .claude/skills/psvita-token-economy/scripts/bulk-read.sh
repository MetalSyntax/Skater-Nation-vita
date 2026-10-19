#!/usr/bin/env bash
# Delegate a large-file read to a cheap model; only terse bullets come back
# into the caller's context, never the raw file content.
set -euo pipefail

MODEL="${PSVITA_ECON_MODEL:-claude-haiku-4-5-20251001}"
MAX_CHARS=300000   # ~75k tokens; keeps the delegate call cheap and safe under arg limits

if [[ $# -lt 2 ]]; then
  echo "Uso: bulk-read.sh <archivo> \"<pregunta puntual>\"" >&2
  echo 'Ej: bulk-read.sh decompiled_so/out_ghidra.c "listar todas las funciones que llaman a pthread_mutex_lock, con su nombre y offset"' >&2
  exit 1
fi

FILE="$1"
QUESTION="$2"

if [[ ! -f "$FILE" ]]; then
  echo "Error: no existe el archivo '$FILE'" >&2
  exit 1
fi

SIZE=$(wc -c < "$FILE")
if (( SIZE > MAX_CHARS )); then
  echo "Error: '$FILE' tiene $SIZE bytes (límite $MAX_CHARS para una sola delegación)." >&2
  echo "Acotá primero con grep/awk (ej. grep -n -A30 'NombreDeFuncion' '$FILE' > /tmp/recorte.c) y delegá ese recorte, no el archivo entero." >&2
  exit 1
fi

PROMPT_FILE=$(mktemp)
trap 'rm -f "$PROMPT_FILE"' EXIT

{
  echo "Respondé ÚNICAMENTE en bullets cortos y densos. Nada de prosa, nada de resúmenes"
  echo "introductorios, nada de bloques de código salvo que la pregunta pida una línea exacta."
  echo "Si citás algo del archivo, incluí el número de línea entre paréntesis. Si la pregunta no"
  echo "aplica a nada del archivo, respondé solo: \"(sin coincidencias)\"."
  echo
  echo "PREGUNTA: ${QUESTION}"
  echo
  echo "<file path=\"${FILE}\">"
  cat "$FILE"
  echo "</file>"
} > "$PROMPT_FILE"

claude -p "$(cat "$PROMPT_FILE")" \
  --model "$MODEL" \
  --tools "" \
  --no-session-persistence \
  --output-format text \
  --permission-prompts none \
  < /dev/null
