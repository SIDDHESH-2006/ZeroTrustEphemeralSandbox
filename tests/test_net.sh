#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "--- Testing Seccomp-BPF Network Containment ---"

# 1. Deny socket creation in default zero-trust mode
set +e
err_out=$($SANDBOX -- /usr/bin/python3 -c "import socket; s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)" 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ] && ([[ "$err_out" =~ "Operation not permitted" ]] || [[ "$err_out" =~ "PermissionError" ]] || [[ "$err_out" =~ "Errno 1" ]]); then
    echo -e "${GREEN}[PASS]${NC} Network socket() blocked by Seccomp filter (EPERM / Operation not permitted)"
else
    echo -e "${RED}[FAIL]${NC} Expected Seccomp to deny socket creation, got: $err_out"
    exit 1
fi

# 2. Allow socket creation when --allow-net is explicitly provided
set +e
out=$($SANDBOX --allow-net -- /usr/bin/python3 -c "import socket; s = socket.socket(socket.AF_INET, socket.SOCK_STREAM); print('socket_ok')" 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -eq 0 ] && [ "$out" = "socket_ok" ]; then
    echo -e "${GREEN}[PASS]${NC} Network operations permitted when --allow-net is specified"
else
    echo -e "${RED}[FAIL]${NC} --allow-net failed to permit socket creation: $out"
    exit 1
fi

echo -e "${GREEN}All Seccomp network tests passed successfully!${NC}\n"
