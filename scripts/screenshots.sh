#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"

REPORT_IMG_DIR="${REPO_ROOT}/docs/reports/img"
mkdir -p "${REPORT_IMG_DIR}"

echo "==> Building screenshot test binaries..."
cmake --build --preset dev-linux --target tst_PageCalendar tst_PageStudy tst_PageHome tst_PageSettings -j2

echo "==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=${REPORT_IMG_DIR}..."
JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
    ./build/dev-linux/tests/ui/tst_PageCalendar testCalendarScreenshots
JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
    ./build/dev-linux/tests/ui/tst_PageStudy testStudyScreenshots
JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
    ./build/dev-linux/tests/ui/tst_PageHome testDashboardScreenshots testToastScreenshots
JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
    ./build/dev-linux/tests/ui/tst_PageSettings testSettingsScreenshots testDataCardScreenshots

echo "==> Screenshots successfully regenerated in ${REPORT_IMG_DIR}!"
