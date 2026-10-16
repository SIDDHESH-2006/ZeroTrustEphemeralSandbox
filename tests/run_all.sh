#!/usr/bin/env bash
set -uo pipefail

BOLD='\033[1m'
GREEN='\033[1;32m'
BLUE='\033[1;34m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BOLD}${BLUE}====================================================${NC}"
echo -e "${BOLD}${BLUE}  Zero-Trust Ephemeral Sandbox - Master Test Runner ${NC}"
echo -e "${BOLD}${BLUE}====================================================${NC}\n"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

chmod +x tests/*.sh

TEST_SUITES=(
    "tests/test_cli.sh"
    "tests/test_fs.sh"
    "tests/test_net.sh"
    "tests/test_ptrace.sh"
    "tests/test_seccomp_hardened.sh"
    "tests/test_bypass_regression.sh"
    "tests/test_edge_cases.sh"
    "tests/test_capabilities.sh"
    "tests/test_namespaces.sh"
)

TOTAL_SUITES=${#TEST_SUITES[@]}
PASSED_SUITES=0
FAILED_SUITES=0

for suite in "${TEST_SUITES[@]}"; do
    if [ ! -f "$suite" ]; then
        echo -e "${RED}[ERROR] Test script missing: $suite${NC}"
        FAILED_SUITES=$((FAILED_SUITES + 1))
        continue
    fi

    echo -e "${BOLD}Running $suite...${NC}"
    "$suite"
    status=$?

    if [ $status -eq 0 ]; then
        PASSED_SUITES=$((PASSED_SUITES + 1))
    else
        echo -e "${RED}[FAILED SUITE] $suite exited with status $status${NC}\n"
        FAILED_SUITES=$((FAILED_SUITES + 1))
    fi
done

echo -e "${BOLD}----------------------------------------------------${NC}"
echo -e "Test Suite Summary: Total: $TOTAL_SUITES | Passed: ${GREEN}$PASSED_SUITES${NC} | Failed: ${RED}$FAILED_SUITES${NC}"
echo -e "${BOLD}----------------------------------------------------${NC}\n"

if [ $FAILED_SUITES -ne 0 ]; then
    echo -e "${BOLD}${RED}====================================================${NC}"
    echo -e "${BOLD}${RED}  ONE OR MORE TEST SUITES FAILED! ($FAILED_SUITES/$TOTAL_SUITES failed)  ${NC}"
    echo -e "${BOLD}${RED}====================================================${NC}"
    exit 1
else
    echo -e "${BOLD}${GREEN}====================================================${NC}"
    echo -e "${BOLD}${GREEN}  ALL SECURITY, REGRESSION & EDGE TESTS PASSED! (${PASSED_SUITES}/${TOTAL_SUITES}) ${NC}"
    echo -e "${BOLD}${GREEN}====================================================${NC}"
    exit 0
fi
