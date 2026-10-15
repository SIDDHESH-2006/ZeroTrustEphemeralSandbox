#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "--- Testing CLI supervisor interface ---"

# 1. Version output check
output=$($SANDBOX --version)
if [[ "$output" =~ "Zero-Trust Ephemeral Sandbox" ]] && [[ "$output" =~ "Mohammad Ammar Abdullah" ]]; then
    echo -e "${GREEN}[PASS]${NC} --version displays metadata and contributors"
else
    echo -e "${RED}[FAIL]${NC} --version output mismatch"
    exit 1
fi

# 2. Help output check
output=$($SANDBOX --help)
if [[ "$output" =~ "Usage:" ]] && [[ "$output" =~ "--ro" ]] && [[ "$output" =~ "--rw" ]]; then
    echo -e "${GREEN}[PASS]${NC} --help displays command syntax and options"
else
    echo -e "${RED}[FAIL]${NC} --help output mismatch"
    exit 1
fi

# 3. Missing payload check
set +e
output=$($SANDBOX 2>&1)
exit_code=$?
set -e
if [ "$exit_code" -ne 0 ]; then
    echo -e "${GREEN}[PASS]${NC} Missing payload returns non-zero error"
else
    echo -e "${RED}[FAIL]${NC} Expected failure on missing payload"
    exit 1
fi

# 4. Basic execution check
output=$($SANDBOX -- /bin/echo "CLI test passed")
if [ "$output" = "CLI test passed" ]; then
    echo -e "${GREEN}[PASS]${NC} Clean payload execution via '--' separator"
else
    echo -e "${RED}[FAIL]${NC} Payload execution failed"
    exit 1
fi

echo -e "${GREEN}All CLI tests passed successfully!${NC}\n"
