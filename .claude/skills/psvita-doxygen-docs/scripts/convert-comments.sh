#!/usr/bin/env bash
# ==============================================================================
# convert-comments.sh — Conversión de comentarios '//' a Doxygen + Companion MD
# Parte de la skill psvita-doxygen-docs
# ==============================================================================
set -euo pipefail

MODEL="${PSVITA_ECON_MODEL:-claude-haiku-4-5-20251001}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE_FILE="${SCRIPT_DIR}/prompt-template-cheap-ai.txt"

usage() {
    cat << 'EOF'
Uso: convert-comments.sh [--local] <archivo_fuente_o_directorio>

Opciones:
  --local    Usa el extractor local basado en AST/regex (gen_docs.py) sin invocar IA.
             Por defecto, si 'claude' está disponible, delega la conversión a un
             modelo económico (Haiku o $PSVITA_ECON_MODEL) para mayor fidelidad semántica.

Ejemplos:
  convert-comments.sh source/falso_jni/game.c
  convert-comments.sh --local source/patch.c
  convert-comments.sh source/
EOF
    exit 1
}

if [[ $# -lt 1 ]]; then
    usage
fi

USE_LOCAL=false
if [[ "$1" == "--local" ]]; then
    USE_LOCAL=true
    shift
fi

TARGET="$1"
if [[ ! -e "$TARGET" ]]; then
    echo "[-] Error: No existe el objetivo '$TARGET'" >&2
    exit 1
fi

# Buscar raíz del repositorio (donde está .git, CMakeLists.txt o .psvita-toolkit.json)
find_repo_root() {
    local dir
    dir="$(cd "$(dirname "$1")" && pwd)"
    while [[ "$dir" != "/" ]]; do
        if [[ -d "$dir/.git" || -f "$dir/CMakeLists.txt" || -f "$dir/.psvita-toolkit.json" ]]; then
            echo "$dir"
            return 0
        fi
        dir="$(dirname "$dir")"
    done
    pwd
}

process_single_file() {
    local file="$1"
    local repo_root
    repo_root="$(find_repo_root "$file")"

    # Obtener ruta relativa al repo
    local rel_path
    if [[ "$file" = /* ]]; then
        rel_path="${file#"$repo_root"/}"
    else
        rel_path="$file"
    fi

    local extension="${file##*.}"
    if [[ ! "$extension" =~ ^(c|h|cpp|hpp|cc)$ ]]; then
        echo "[*] Ignorando '$rel_path' (no es archivo C/C++)"
        return 0
    fi

    local stem="${rel_path%.*}"
    local doc_md_rel="docs/${stem}.md"
    local doc_md_abs="${repo_root}/${doc_md_rel}"

    echo "[*] Procesando: $rel_path"
    echo "    -> Documentación pura en: $doc_md_rel"

    # Si se pide modo local o no está instalado claude CLI
    if [[ "$USE_LOCAL" == "true" ]] || ! command -v claude &>/dev/null; then
        # Buscar gen_docs.py en toolkit o repo
        local gen_docs_path=""
        if [[ -f "${repo_root}/dev-tools/gen_docs.py" ]]; then
            gen_docs_path="${repo_root}/dev-tools/gen_docs.py"
        elif [[ -f "${repo_root}/psvita_toolkit/gen_docs.py" ]]; then
            gen_docs_path="${repo_root}/psvita_toolkit/gen_docs.py"
        elif command -v psvita-toolkit &>/dev/null; then
            gen_docs_path="$(python3 -c "import psvita_toolkit.gen_docs, os; print(os.path.abspath(psvita_toolkit.gen_docs.__file__))" 2>/dev/null || true)"
        fi

        if [[ -n "$gen_docs_path" && -f "$gen_docs_path" ]]; then
            echo "    [i] Ejecutando extractor local: $gen_docs_path"
            python3 "$gen_docs_path" --extract-comments "$file"
            echo "    [+] Procesado con éxito vía extractor local."
            return 0
        else
            echo "[-] Error: 'claude' no está disponible y no se encontró 'gen_docs.py' para procesado local." >&2
            return 1
        fi
    fi

    # Delegar a IA económica con claude -p
    echo "    [i] Delegando a modelo económico ($MODEL)..."

    local temp_prompt
    temp_prompt="$(mktemp)"
    local raw_output
    raw_output="$(mktemp)"
    trap 'rm -f "$temp_prompt" "$raw_output"' EXIT

    # Generar prompt desde plantilla
    sed -e "s|<SOURCE_PATH>|$rel_path|g" \
        -e "s|<DOC_MD_PATH>|$doc_md_rel|g" \
        "$TEMPLATE_FILE" > "$temp_prompt"

    # Adjuntar código fuente
    cat "$file" >> "$temp_prompt"
    echo "" >> "$temp_prompt"
    echo "------------------------------ FIN DEL PROMPT ------------------------------" >> "$temp_prompt"

    # Invocación headless sin herramientas
    claude -p "$(cat "$temp_prompt")" \
        --model "$MODEL" \
        --tools "" \
        --no-session-persistence > "$raw_output" 2>/dev/null || {
            echo "[-] Falló la llamada a $MODEL con claude -p. Intentando fallback local..."
            USE_LOCAL=true process_single_file "$file"
            return $?
        }

    # Extraer secciones
    if ! grep -q "=== ARCHIVO_MD ===" "$raw_output" || ! grep -q "=== CODIGO_FUENTE ===" "$raw_output"; then
        echo "[-] Advertencia: La respuesta del modelo no contiene los delimitadores esperados. Reintentando con extractor local..."
        USE_LOCAL=true process_single_file "$file"
        return $?
    fi

    # Crear carpeta para el MD
    mkdir -p "$(dirname "$doc_md_abs")"

    # Escribir ARCHIVO_MD
    awk '/=== ARCHIVO_MD ===/{flag=1;next}/=== FIN_ARCHIVO_MD ===/{flag=0}flag' "$raw_output" > "$doc_md_abs"

    # Hacer backup y escribir CODIGO_FUENTE
    cp "$file" "${file}.bak"
    awk '/=== CODIGO_FUENTE ===/{flag=1;next}/=== FIN_CODIGO_FUENTE ===/{flag=0}flag' "$raw_output" > "$file"

    # Chequeo de seguridad: si el archivo C quedó vacío, restaurar backup
    if [[ ! -s "$file" ]]; then
        echo "[-] Error: El archivo C generado quedó vacío. Restaurando backup."
        mv "${file}.bak" "$file"
        return 1
    fi

    rm -f "${file}.bak"
    echo "    [+] C generado y documentado correctamente."
    echo "    [+] Markdown companion generado en: $doc_md_abs"
}

# Procesamiento recursivo si es directorio o archivo único
if [[ -d "$TARGET" ]]; then
    while IFS= read -r -d '' f; do
        process_single_file "$f"
    done < <(find "$TARGET" -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) -print0)
else
    process_single_file "$TARGET"
fi

echo "[✓] Proceso completado exitosamente."
