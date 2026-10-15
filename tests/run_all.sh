#!/usr/bin/env bash
set -euo pipefail

BOLD='\033[1m'
GREEN='\033[1;32m'
BLUE='\033[1;34m'
NC='\033[0m'

echo -e "${BOLD}${BLUE}====================================================${NC}"
echo -e "${BOLD}${BLUE}  Zero-Trust Ephemeral Sandbox - Test Suite Runner  ${NC}"
echo -e "${BOLD}${BLUE}====================================================${NC}\n"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

chmod +x tests/*.sh

./tests/test_cli.sh
./tests/test_fs.sh
./tests/test_net.sh
./tests/test_ptrace.sh

echo -e "${BOLD}${GREEN}====================================================${NC}"
echo -e "${BOLD}${GREEN}  ALL SECURITY AND CONFINEMENT TESTS PASSED! (4/4)   ${NC}"
echo -e "${BOLD}${GREEN}====================================================${NC}"
