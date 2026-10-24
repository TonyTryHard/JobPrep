#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"

REPORT_IMG_DIR="${REPO_ROOT}/docs/reports/img"
mkdir -p "${REPORT_IMG_DIR}"

echo "==> Building ui_smoke..."
cmake --build --preset dev-linux --target ui_smoke

echo "==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=${REPORT_IMG_DIR}..."
JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
    ./build/dev-linux/tests/ui_smoke/ui_smoke testCalendarScreenshots testStudyScreenshots testDashboardScreenshots

echo "==> Screenshots successfully regenerated in ${REPORT_IMG_DIR}!"
