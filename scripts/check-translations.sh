#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}" 2>/dev/null || echo "${BASH_SOURCE[0]%/*}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"

# Locate lupdate
LUPDATE=""
if [ -n "${QT_PREFIX_PATH:-}" ] && [ -x "${QT_PREFIX_PATH}/bin/lupdate" ]; then
    LUPDATE="${QT_PREFIX_PATH}/bin/lupdate"
elif command -v lupdate >/dev/null 2>&1; then
    LUPDATE="$(command -v lupdate)"
fi

if [ -z "${LUPDATE}" ]; then
    echo "==> lupdate not found, skipping translation sync check."
    exit 0
fi

echo "==> Checking translation sync with ${LUPDATE}..."

JOBPREP_TS_QT_MINOR="${JOBPREP_TS_QT_MINOR:-10}"
LUPDATE_VERSION="$("${LUPDATE}" -version 2>&1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n 1 || echo "unknown")"
LUPDATE_MAJOR_MINOR="$(echo "${LUPDATE_VERSION}" | grep -oE '^[0-9]+\.[0-9]+' || echo "")"
LUPDATE_MINOR="${LUPDATE_MAJOR_MINOR#*.}"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

cp resources/translations/jobprep_uk.ts "${TMP_DIR}/jobprep_uk.ts"
cp resources/translations/jobprep_ru.ts "${TMP_DIR}/jobprep_ru.ts"

"${LUPDATE}" src -no-obsolete -locations none -source-language en -target-language uk -ts "${TMP_DIR}/jobprep_uk.ts" >/dev/null 2>&1
"${LUPDATE}" src -no-obsolete -locations none -source-language en -target-language ru -ts "${TMP_DIR}/jobprep_ru.ts" >/dev/null 2>&1

DIFF_UK="$(diff -u resources/translations/jobprep_uk.ts "${TMP_DIR}/jobprep_uk.ts" || true)"
DIFF_RU="$(diff -u resources/translations/jobprep_ru.ts "${TMP_DIR}/jobprep_ru.ts" || true)"

if [ -n "${DIFF_UK}" ] || [ -n "${DIFF_RU}" ]; then
    if [ -n "${LUPDATE_MINOR}" ] && [ "${LUPDATE_MINOR}" != "${JOBPREP_TS_QT_MINOR}" ]; then
        echo "==> lupdate ${LUPDATE_VERSION} differs from generating minor 6.${JOBPREP_TS_QT_MINOR}: report only."
        if [ -n "${DIFF_UK}" ]; then
            echo "=== Diff for jobprep_uk.ts ==="
            echo "${DIFF_UK}"
        fi
        if [ -n "${DIFF_RU}" ]; then
            echo "=== Diff for jobprep_ru.ts ==="
            echo "${DIFF_RU}"
        fi
        exit 0
    fi

    echo "ERROR: Translation files are out of sync with source code!"
    if [ -n "${DIFF_UK}" ]; then
        echo "=== Diff for jobprep_uk.ts ==="
        echo "${DIFF_UK}"
    fi
    if [ -n "${DIFF_RU}" ]; then
        echo "=== Diff for jobprep_ru.ts ==="
        echo "${DIFF_RU}"
    fi
    echo "Run lupdate to update translation files."
    exit 1
fi

echo "==> Translation files are in sync."
