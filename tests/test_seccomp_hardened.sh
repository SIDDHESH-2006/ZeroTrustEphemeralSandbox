#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "--- Testing Seccomp-BPF Hardened Syscall Restrictions ---"

# Detect Python 3
PYTHON=""
if command -v python3 >/dev/null 2>&1; then
    PYTHON="python3"
elif command -v /usr/bin/python3 >/dev/null 2>&1; then
    PYTHON="/usr/bin/python3"
fi

if [ -z "$PYTHON" ]; then
    echo -e "${YELLOW}[SKIP]${NC} python3 not found; skipping seccomp syscall test suite"
    exit 0
fi

PASS_COUNT=0
FAIL_COUNT=0

# Helper function to check if syscall returned EPERM (errno 1)
run_syscall_check() {
    local test_name="$1"
    local py_script="$2"
    local allow_net="${3:-false}"

    set +e
    if [ "$allow_net" = "true" ]; then
        err_out=$($SANDBOX --allow-net -- "$PYTHON" -c "$py_script" 2>&1)
    else
        err_out=$($SANDBOX -- "$PYTHON" -c "$py_script" 2>&1)
    fi
    exit_code=$?
    set -e

    if [ "$exit_code" -ne 0 ] && ([[ "$err_out" =~ "Operation not permitted" ]] || [[ "$err_out" =~ "PermissionError" ]] || [[ "$err_out" =~ "EPERM" ]] || [[ "$err_out" =~ "errno=1" ]] || [[ "$err_out" =~ "Errno 1" ]]); then
        echo -e "${GREEN}[PASS]${NC} $test_name blocked by Seccomp (EPERM / Operation not permitted)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}[FAIL]${NC} $test_name expected EPERM, but got (exit $exit_code): $err_out"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# 1. Test io_uring_setup denial (syscall 425 on x86_64, aarch64, arm, i386)
run_syscall_check "io_uring_setup()" '
import ctypes, os
libc = ctypes.CDLL(None, use_errno=True)
# SYS_io_uring_setup = 425
ret = libc.syscall(425, 32, None)
err = ctypes.get_errno()
if ret == -1 and err == 1:
    raise PermissionError("EPERM (errno=1)")
assert False, f"io_uring_setup returned {ret}, errno={err}"
'

# 2. Test cross-process memory inspection denial: process_vm_readv
run_syscall_check "process_vm_readv()" '
import ctypes, os, platform
libc = ctypes.CDLL(None, use_errno=True)
arch = platform.machine()
# Determine syscall number for process_vm_readv
nr = 310 if "x86_64" in arch else (270 if "aarch64" in arch else 376)
ret = libc.syscall(nr, os.getpid(), None, 0, None, 0, 0)
err = ctypes.get_errno()
if ret == -1 and err == 1:
    raise PermissionError("EPERM (errno=1)")
assert False, f"process_vm_readv returned {ret}, errno={err}"
'

# 3. Test cross-process memory tampering denial: process_vm_writev
run_syscall_check "process_vm_writev()" '
import ctypes, os, platform
libc = ctypes.CDLL(None, use_errno=True)
arch = platform.machine()
nr = 311 if "x86_64" in arch else (271 if "aarch64" in arch else 377)
ret = libc.syscall(nr, os.getpid(), None, 0, None, 0, 0)
err = ctypes.get_errno()
if ret == -1 and err == 1:
    raise PermissionError("EPERM (errno=1)")
assert False, f"process_vm_writev returned {ret}, errno={err}"
'

# 4. Test ptrace denial
run_syscall_check "ptrace()" '
import ctypes
libc = ctypes.CDLL(None, use_errno=True)
ret = libc.ptrace(0, 0, 0, 0)
err = ctypes.get_errno()
if ret == -1 and err == 1:
    raise PermissionError("EPERM (errno=1)")
assert False, f"ptrace returned {ret}, errno={err}"
'

# 5. Test network socket() denial
run_syscall_check "socket() in zero-trust mode" '
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
'

# 6. Test network-allowed mode
set +e
out=$($SANDBOX --allow-net -- "$PYTHON" -c "import socket; s = socket.socket(socket.AF_INET, socket.SOCK_STREAM); print('NET_OK')" 2>&1)
exit_code=$?
set -e
if [ "$exit_code" -eq 0 ] && [ "$out" = "NET_OK" ]; then
    echo -e "${GREEN}[PASS]${NC} Socket operations allowed when --allow-net is specified"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} --allow-net failed to allow socket: $out"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo -e "\n${RED}Seccomp hardening tests had $FAIL_COUNT failure(s).${NC}"
    exit 1
fi

echo -e "${GREEN}All Seccomp-BPF hardening tests passed successfully! ($PASS_COUNT checks)${NC}\n"
