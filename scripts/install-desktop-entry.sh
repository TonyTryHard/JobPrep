#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

EXE_PATH="${REPO_ROOT}/build/dev-linux/jobprep"
REMOVE=0
DRY_RUN=0

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Installs a desktop launcher entry for the local JobPrep build into the user application menu.

Options:
  --exe PATH     Path to jobprep executable (default: ${REPO_ROOT}/build/dev-linux/jobprep)
  --remove       Remove the installed jobprep.desktop entry
  --dry-run      Print actions without modifying files
  -h, --help     Show this help message and exit
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --exe)
            if [ $# -lt 2 ]; then
                echo "Error: --exe requires a path argument" >&2
                exit 1
            fi
            EXE_PATH="$2"
            shift 2
            ;;
        --remove)
            REMOVE=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

TARGET_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}/applications"
TARGET_FILE="${TARGET_DIR}/jobprep.desktop"

if [ "${REMOVE}" -eq 1 ]; then
    if [ -f "${TARGET_FILE}" ]; then
        if [ "${DRY_RUN}" -eq 1 ]; then
            echo "[dry-run] Would remove: ${TARGET_FILE}"
        else
            rm -f "${TARGET_FILE}"
            echo "Removed ${TARGET_FILE}"
        fi
    else
        if [ "${DRY_RUN}" -eq 1 ]; then
            echo "[dry-run] File does not exist, nothing to remove: ${TARGET_FILE}"
        else
            echo "File does not exist, nothing to remove: ${TARGET_FILE}"
        fi
    fi
    exit 0
fi

# Resolve absolute path for exe
if [ -e "${EXE_PATH}" ]; then
    RESOLVED_EXE="$(cd "$(dirname "${EXE_PATH}")" && pwd)/$(basename "${EXE_PATH}")"
else
    RESOLVED_EXE="${EXE_PATH}"
fi

if [ ! -f "${RESOLVED_EXE}" ]; then
    echo "Error: Executable not found at ${RESOLVED_EXE}" >&2
    exit 1
fi

if [ ! -x "${RESOLVED_EXE}" ]; then
    echo "Error: File is not executable: ${RESOLVED_EXE}" >&2
    exit 1
fi

ICON_PATH="${REPO_ROOT}/resources/icons/app-256.png"
if [ ! -f "${ICON_PATH}" ]; then
    echo "Error: Icon file not found at ${ICON_PATH}" >&2
    exit 1
fi

quote_exec() {
    local val="$1"
    val="${val//\\/\\\\}"
    val="${val//\"/\\\"}"
    if [[ "${val}" =~ [[:space:]] ]]; then
        printf '"%s"' "${val}"
    else
        printf '%s' "${val}"
    fi
}

EXEC_LINE="$(quote_exec "${RESOLVED_EXE}")"

DESKTOP_CONTENT="[Desktop Entry]
Version=1.0
Type=Application
Name=JobPrep
Comment=Job application and interview preparation tracker
Exec=${EXEC_LINE}
Icon=${ICON_PATH}
Terminal=false
Categories=Office;
StartupWMClass=jobprep
"

if [ "${DRY_RUN}" -eq 1 ]; then
    echo "[dry-run] Would create directory: ${TARGET_DIR}"
    echo "[dry-run] Would write ${TARGET_FILE}:"
    printf "%s" "${DESKTOP_CONTENT}"
    exit 0
fi

mkdir -p "${TARGET_DIR}"

TMP_FILE="$(mktemp "${TARGET_DIR}/jobprep.desktop.tmp.XXXXXX")"
printf "%s" "${DESKTOP_CONTENT}" > "${TMP_FILE}"
mv -f "${TMP_FILE}" "${TARGET_FILE}"

echo "Installed desktop entry at ${TARGET_FILE}"
