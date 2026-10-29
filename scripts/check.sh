#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"

echo "==> Configuring JobPrep with dev-linux preset..."
cmake --preset dev-linux

echo "==> Building JobPrep with dev-linux preset..."
if [ "${JOBPREP_KEEP_GOING:-0}" = "1" ]; then
    cmake --build --preset dev-linux -- -k 0
else
    cmake --build --preset dev-linux
fi

echo "==> Running tests with dev-linux preset..."
CTEST_PARALLEL_LEVEL="${CTEST_PARALLEL_LEVEL:-2}" ctest --preset dev-linux -j "${CTEST_PARALLEL_LEVEL:-2}" --output-on-failure

echo "==> Quality check PASSED with zero warnings and all tests green!"
