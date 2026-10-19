#!/usr/bin/env bash
# PreToolUse hook (global, self-scoped). Registered once in ~/.claude/settings.json
# for matchers "Read" and "Bash". No-ops outside PS Vita porting projects, so it
# is silent in every other repo/session.
set -euo pipefail

LINE_LIMIT=350

if ! command -v jq >/dev/null 2>&1; then
  exit 0   # fail-open: sin jq no podemos parsear stdin, no bloqueamos nada
fi

INPUT=$(cat)

CWD=$(echo "$INPUT" | jq -r '.cwd // "."')
TOOL_NAME=$(echo "$INPUT" | jq -r '.tool_name // ""')

# Solo actuar si el directorio (o alguno de sus padres cercanos) es un port
# administrado por psvita-toolkit. En cualquier otro proyecto, no-op total.
is_vita_port_dir() {
  local dir="$1"
  local depth=0
  while [[ "$dir" != "/" && $depth -lt 6 ]]; do
    if [[ -f "$dir/.psvita-toolkit.json" || -f "$dir/PORTING_PLAN.md" ]]; then
      return 0
    fi
    dir=$(dirname "$dir")
    depth=$((depth + 1))
  done
  return 1
}

if ! is_vita_port_dir "$CWD"; then
  exit 0
fi

deny() {
  local reason="$1"
  jq -n --arg reason "$reason" '{
    hookSpecificOutput: {
      hookEventName: "PreToolUse",
      permissionDecision: "deny",
      permissionDecisionReason: $reason
    }
  }'
  exit 2
}

count_lines() {
  wc -l < "$1" 2>/dev/null | tr -d ' '
}

SKILL_HINT="skill psvita-token-economy: usá scripts/bulk-read.sh <archivo> \"<pregunta puntual>\" para delegar la lectura a un modelo económico y traer solo bullets."

if [[ "$TOOL_NAME" == "Read" ]]; then
  FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // ""')
  HAS_OFFSET=$(echo "$INPUT" | jq -r 'if (.tool_input.offset != null) or (.tool_input.limit != null) then "1" else "0" end')

  if [[ "$HAS_OFFSET" == "1" || -z "$FILE_PATH" || ! -f "$FILE_PATH" ]]; then
    exit 0
  fi

  LINES=$(count_lines "$FILE_PATH")
  if [[ "$LINES" -gt "$LINE_LIMIT" ]]; then
    deny "'$FILE_PATH' tiene $LINES líneas (límite $LINE_LIMIT para un Read sin offset/limit). Usá Read con offset/limit para un rango puntual, grep -n para buscar algo específico, o delega la lectura completa: $SKILL_HINT"
  fi
  exit 0
fi

if [[ "$TOOL_NAME" == "Bash" ]]; then
  COMMAND=$(echo "$INPUT" | jq -r '.tool_input.command // ""')

  # Los comandos con pipe ya filtran/acotan (cat x | grep ...) -- no bloquear.
  if [[ "$COMMAND" == *"|"* ]]; then
    exit 0
  fi

  if [[ "$COMMAND" =~ ^[[:space:]]*(cat|head|tail|less|more)[[:space:]]+ ]]; then
    CMD_NAME=$(echo "$COMMAND" | awk '{print $1}')
    for TOKEN in $COMMAND; do
      if [[ -f "$TOKEN" ]]; then
        LINES=$(count_lines "$TOKEN")
        if [[ "$LINES" -gt "$LINE_LIMIT" ]]; then
          deny "'$TOKEN' tiene $LINES líneas -- '$CMD_NAME' sin pipe volcaría todo al contexto. Usá '$CMD_NAME ... | grep ...' para acotar, o delega: $SKILL_HINT"
        fi
      fi
    done
  fi
  exit 0
fi

exit 0
