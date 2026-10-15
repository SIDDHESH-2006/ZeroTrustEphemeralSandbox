#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "--- Testing Landlock LSM Filesystem Containment ---"

TEST_ROOT="/tmp/sb_test_fs_$$"
ALLOWED_DIR="$TEST_ROOT/allowed"
SECRET_DIR="$TEST_ROOT/secret"
RO_DIR="$TEST_ROOT/readonly"
RW_DIR="$TEST_ROOT/readwrite"

mkdir -p "$ALLOWED_DIR" "$SECRET_DIR" "$RO_DIR" "$RW_DIR"
echo "allowed content" > "$ALLOWED_DIR/file.txt"
echo "secret SSH key or password" > "$SECRET_DIR/secret.txt"
echo "ro content" > "$RO_DIR/data.txt"

cleanup() {
    rm -rf "$TEST_ROOT"
}
trap cleanup EXIT

# 1. Read allowed directory (should succeed)
out=$($SANDBOX --ro "$ALLOWED_DIR" -- /bin/cat "$ALLOWED_DIR/file.txt")
if [ "$out" = "allowed content" ]; then
    echo -e "${GREEN}[PASS]${NC} Reading explicitly allowlisted path succeeds"
else
    echo -e "${RED}[FAIL]${NC} Failed to read allowlisted path"
    exit 1
fi

# 2. Read non-allowlisted secret directory (should fail with Permission denied)
set +e
err_out=$($SANDBOX --ro "$ALLOWED_DIR" -- /bin/cat "$SECRET_DIR/secret.txt" 2>&1)
exit_code=$?
set -e
if [ "$exit_code" -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}[PASS]${NC} Reading unauthorized path blocked by Landlock (Permission denied)"
else
    echo -e "${RED}[FAIL]${NC} Expected Landlock to deny access to $SECRET_DIR, but got: $err_out"
    exit 1
fi

# 3. Write to read-only path (should fail)
set +e
err_out=$($SANDBOX --ro "$RO_DIR" -- /bin/touch "$RO_DIR/new_file.txt" 2>&1)
exit_code=$?
set -e
if [ "$exit_code" -ne 0 ] && [[ "$err_out" =~ "Permission denied" ]]; then
    echo -e "${GREEN}[PASS]${NC} Writing to --ro path blocked by Landlock (Permission denied)"
else
    echo -e "${RED}[FAIL]${NC} Expected Landlock to deny write to read-only directory, but got: $err_out"
    exit 1
fi

# 4. Write to read-write path (should succeed)
$SANDBOX --rw "$RW_DIR" -- /bin/touch "$RW_DIR/created.txt"
if [ -f "$RW_DIR/created.txt" ]; then
    echo -e "${GREEN}[PASS]${NC} Writing to --rw path succeeds"
else
    echo -e "${RED}[FAIL]${NC} Failed to create file in read-write path"
    exit 1
fi

echo -e "${GREEN}All Landlock filesystem tests passed successfully!${NC}\n"
