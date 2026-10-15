#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "--- Testing Seccomp-BPF Ptrace Blocking ---"

# Attempt ptrace(PTRACE_TRACEME, 0, NULL, NULL) via python ctypes
set +e
err_out=$($SANDBOX -- /usr/bin/python3 -c "import ctypes, os; libc = ctypes.CDLL(None); res = libc.ptrace(0, 0, 0, 0); assert res == 0, 'ptrace failed: ' + str(ctypes.get_errno())" 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ]; then
    echo -e "${GREEN}[PASS]${NC} ptrace() denied by Seccomp filter"
else
    echo -e "${RED}[FAIL]${NC} ptrace() was unexpectedly permitted"
    exit 1
fi

echo -e "${GREEN}All ptrace tests passed successfully!${NC}\n"
