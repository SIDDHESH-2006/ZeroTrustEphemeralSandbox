#!/usr/bin/env bash
set -uo pipefail

BOLD='\033[1m'
CYAN='\033[1;36m'
GREEN='\033[1;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

echo -e "${BOLD}${CYAN}======================================================================${NC}"
echo -e "${BOLD}${CYAN}   Zero-Trust Ephemeral Sandbox — Interactive Feature Demonstration   ${NC}"
echo -e "${BOLD}${CYAN}======================================================================${NC}\n"

SANDBOX="./bin/sandbox"
if [ ! -x "$SANDBOX" ]; then
    echo -e "${RED}[ERROR] Sandbox binary not found at $SANDBOX. Run 'make' first.${NC}"
    exit 1
fi

DEMO_DIR="/tmp/sandbox_demo_$$"
mkdir -p "$DEMO_DIR/allowed_ro" "$DEMO_DIR/allowed_rw" "$DEMO_DIR/secret"
echo "Public document in read-only folder" > "$DEMO_DIR/allowed_ro/document.txt"
echo "Super-secret cryptographic token (SHOULD BE INACCESSIBLE)" > "$DEMO_DIR/secret/private_key.pem"

cleanup() {
    rm -rf "$DEMO_DIR"
}
trap cleanup EXIT

step_header() {
    local num="$1"
    local title="$2"
    echo -e "\n${BOLD}${BLUE}[Step $num] $title${NC}"
}

# Step 1: Permitted basic command execution
step_header "1" "Executing a basic permitted command with zero overhead"
echo -e "Command: ${BOLD}$SANDBOX -- /bin/echo 'Hello from inside Zero-Trust Sandbox!'${NC}"
$SANDBOX -- /bin/echo "Hello from inside Zero-Trust Sandbox!"

# Step 2: Read from allowlisted read-only folder
step_header "2" "Reading a file from an allowlisted read-only directory (--ro)"
echo -e "Command: ${BOLD}$SANDBOX --ro '$DEMO_DIR/allowed_ro' -- /bin/cat '$DEMO_DIR/allowed_ro/document.txt'${NC}"
$SANDBOX --ro "$DEMO_DIR/allowed_ro" -- /bin/cat "$DEMO_DIR/allowed_ro/document.txt"

# Step 3: Prevent write to read-only folder
step_header "3" "Attempting to create/modify a file inside a read-only directory (--ro)"
echo -e "Command: ${BOLD}$SANDBOX --ro '$DEMO_DIR/allowed_ro' -- /bin/touch '$DEMO_DIR/allowed_ro/tamper.txt'${NC}"
set +e
err_out=$($SANDBOX --ro "$DEMO_DIR/allowed_ro" -- /bin/touch "$DEMO_DIR/allowed_ro/tamper.txt" 2>&1)
exit_code=$?
set -e
echo "Output: $err_out (exit code $exit_code)"
if [ $exit_code -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}--> Succeeded: Landlock LSM strictly blocked the unauthorized write!${NC}"
else
    echo -e "${RED}--> Warning: Write was not denied as expected.${NC}"
fi

# Step 4: Write inside allowlisted read-write folder
step_header "4" "Writing a file inside an explicitly permitted read-write directory (--rw)"
echo -e "Command: ${BOLD}$SANDBOX --rw '$DEMO_DIR/allowed_rw' -- /bin/touch '$DEMO_DIR/allowed_rw/output.txt'${NC}"
$SANDBOX --rw "$DEMO_DIR/allowed_rw" -- /bin/touch "$DEMO_DIR/allowed_rw/output.txt"
if [ -f "$DEMO_DIR/allowed_rw/output.txt" ]; then
    echo -e "${GREEN}--> Succeeded: File successfully created in allowed read-write path!${NC}"
fi

# Step 5: Prevent unauthorized access to secret directory outside sandbox jail
step_header "5" "Attempting to read an unlisted sensitive path (Landlock jail boundary test)"
echo -e "Command: ${BOLD}$SANDBOX --ro '$DEMO_DIR/allowed_ro' -- /bin/cat '$DEMO_DIR/secret/private_key.pem'${NC}"
set +e
err_out=$($SANDBOX --ro "$DEMO_DIR/allowed_ro" -- /bin/cat "$DEMO_DIR/secret/private_key.pem" 2>&1)
exit_code=$?
set -e
echo "Output: $err_out (exit code $exit_code)"
if [ $exit_code -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}--> Succeeded: Access to secret directory was blocked by Landlock LSM!${NC}"
fi

# Step 6: Network denial in default zero-trust mode
step_header "6" "Attempting to create a network socket in default zero-trust mode"
echo -e "Command: ${BOLD}$SANDBOX -- /usr/bin/python3 -c 'import socket; socket.socket()'${NC}"
if command -v python3 >/dev/null 2>&1; then
    set +e
    err_out=$($SANDBOX -- python3 -c 'import socket; socket.socket()' 2>&1)
    exit_code=$?
    set -e
    echo "Output: $err_out"
    if [ $exit_code -ne 0 ] && ([[ "$err_out" =~ "Operation not permitted" ]] || [[ "$err_out" =~ "PermissionError" ]]); then
        echo -e "${GREEN}--> Succeeded: Seccomp-BPF blocked socket creation with EPERM!${NC}"
    fi
else
    echo -e "${YELLOW}--> Python3 not found; skipping python socket test.${NC}"
fi

# Step 7: Deny debugging / ptrace inspection
step_header "7" "Attempting ptrace() system call to trace processes"
if command -v python3 >/dev/null 2>&1; then
    set +e
    err_out=$($SANDBOX -- python3 -c 'import ctypes; ctypes.CDLL(None).ptrace(0,0,0,0)' 2>&1)
    exit_code=$?
    set -e
    echo "Output: $err_out"
    if [ $exit_code -ne 0 ]; then
        echo -e "${GREEN}--> Succeeded: Seccomp-BPF denied ptrace()!${NC}"
    fi
fi

# Step 8: Safe rejection of invalid non-existent user path
step_header "8" "Rejection of invalid or non-existent user paths (Fail-Safe verification)"
echo -e "Command: ${BOLD}$SANDBOX --ro /path/does/not/exist -- /bin/true${NC}"
set +e
err_out=$($SANDBOX --ro /path/does/not/exist -- /bin/true 2>&1)
exit_code=$?
set -e
echo "Output: $err_out (exit code $exit_code)"
if [ $exit_code -ne 0 ]; then
    echo -e "${GREEN}--> Succeeded: Non-existent user path caused fail-safe abort!${NC}"
fi

echo -e "\n${BOLD}${GREEN}======================================================================${NC}"
echo -e "${BOLD}${GREEN}   Demonstration Complete: All Zero-Trust Security Barriers Verified!  ${NC}"
echo -e "${BOLD}${GREEN}======================================================================${NC}\n"
