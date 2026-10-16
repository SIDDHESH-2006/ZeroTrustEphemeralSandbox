#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "--- Testing Sandbox Bypass Regressions & Confinement ---"

TEST_ROOT="/tmp/sb_bypass_$$"
ALLOWED_DIR="$TEST_ROOT/allowed"
FORBIDDEN_DIR="$TEST_ROOT/forbidden"
RO_DIR="$TEST_ROOT/readonly"
RW_DIR="$TEST_ROOT/readwrite"

mkdir -p "$ALLOWED_DIR" "$FORBIDDEN_DIR" "$RO_DIR" "$RW_DIR"

echo "dummy confidential token" > "$FORBIDDEN_DIR/secret.txt"
echo "allowed data" > "$ALLOWED_DIR/data.txt"
echo "read only data" > "$RO_DIR/ro.txt"

# Symlink pointing outside the sandbox jail
ln -s "$FORBIDDEN_DIR/secret.txt" "$ALLOWED_DIR/escape_symlink"
# Symlink pointing inside the sandbox jail
ln -s "$ALLOWED_DIR/data.txt" "$ALLOWED_DIR/internal_symlink"

cleanup() {
    rm -rf "$TEST_ROOT"
}
trap cleanup EXIT

PASS_COUNT=0
FAIL_COUNT=0

# 1. Symlink escape test: Landlock must deny following symlinks to non-allowlisted destinations
set +e
err_out=$($SANDBOX --ro "$ALLOWED_DIR" -- /bin/cat "$ALLOWED_DIR/escape_symlink" 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}[PASS]${NC} Symlink path traversal to outside file blocked by Landlock (Permission denied)"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} Symlink jail escape not blocked! Output: $err_out"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# 2. Relative path traversal (..) escape test
set +e
err_out=$($SANDBOX --ro "$ALLOWED_DIR" -- /bin/cat "$ALLOWED_DIR/../forbidden/secret.txt" 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}[PASS]${NC} Relative path traversal (..) blocked by Landlock (Permission denied)"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} Path traversal escape not blocked! Output: $err_out"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# 3. Reading internal symlink inside allowed tree should succeed
out=$($SANDBOX --ro "$ALLOWED_DIR" -- /bin/cat "$ALLOWED_DIR/internal_symlink")
if [ "$out" = "allowed data" ]; then
    echo -e "${GREEN}[PASS]${NC} Symlink within allowed hierarchy resolves successfully"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} Internal symlink resolution failed: $out"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# 4. Write tampering rejection in read-only directories
set +e
err_out=$($SANDBOX --ro "$RO_DIR" -- /bin/touch "$RO_DIR/tamper.txt" 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}[PASS]${NC} Write in --ro directory strictly blocked by Landlock (Permission denied)"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} Write in --ro directory was not blocked: $err_out"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# 5. Legitimate write in read-write directory
$SANDBOX --rw "$RW_DIR" -- /bin/touch "$RW_DIR/created.txt"
if [ -f "$RW_DIR/created.txt" ]; then
    echo -e "${GREEN}[PASS]${NC} Write in --rw directory succeeds as configured"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} Write in --rw directory failed"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo -e "\n${RED}Bypass regression tests had $FAIL_COUNT failure(s).${NC}"
    exit 1
fi

echo -e "${GREEN}All bypass regression tests passed successfully! ($PASS_COUNT checks)${NC}\n"
