#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "--- Testing CLI Error-Path and Edge-Case Handling ---"

TEST_TMP="/tmp/sb_edge_$$"
mkdir -p "$TEST_TMP"
cleanup() {
    rm -rf "$TEST_TMP"
}
trap cleanup EXIT

PASS_COUNT=0
FAIL_COUNT=0

assert_fails() {
    local test_name="$1"
    local expected_code="${2:-}"
    shift 2
    set +e
    err_out=$("$@" 2>&1)
    exit_code=$?
    set -e

    if [ "$exit_code" -ne 0 ]; then
        if [ -n "$expected_code" ] && [ "$exit_code" -ne "$expected_code" ]; then
            echo -e "${RED}[FAIL]${NC} $test_name expected exit code $expected_code, got $exit_code: $err_out"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        else
            echo -e "${GREEN}[PASS]${NC} $test_name rejected safely (exit $exit_code)"
            PASS_COUNT=$((PASS_COUNT + 1))
        fi
    else
        echo -e "${RED}[FAIL]${NC} $test_name unexpectedly succeeded!"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# 1. Missing payload entirely
assert_fails "Missing payload" "" $SANDBOX

# 2. Empty payload after '--'
assert_fails "Empty payload after '--'" "" $SANDBOX --ro /usr --

# 3. Unknown CLI option
assert_fails "Unknown CLI option" "" $SANDBOX --unsupported-bogus-option -- /bin/true

# 4. Missing required argument to --ro
assert_fails "Missing argument to --ro" "" $SANDBOX --ro

# 5. Missing required argument to --rw
assert_fails "Missing argument to --rw" "" $SANDBOX --rw

# 6. Non-existent user-specified --ro path
assert_fails "Non-existent path in --ro" "" $SANDBOX --ro "$TEST_TMP/does_not_exist_xyz" -- /bin/true

# 7. Non-existent user-specified --rw path
assert_fails "Non-existent path in --rw" "" $SANDBOX --rw "$TEST_TMP/does_not_exist_xyz" -- /bin/true

# 8. Command not found should return exit code 127
assert_fails "Command not found (exit 127)" 127 $SANDBOX -- "$TEST_TMP/non_existent_binary_123"

# 9. Non-executable file payload should return exit code 126
echo "just plain text" > "$TEST_TMP/plain_text_file"
chmod 644 "$TEST_TMP/plain_text_file"
assert_fails "Permission denied for non-executable file (exit 126)" 126 $SANDBOX -- "$TEST_TMP/plain_text_file"

# 10. Valid execution with upgraded repeated path rule (RO upgraded to RW)
set +e
out=$($SANDBOX --ro "$TEST_TMP" --rw "$TEST_TMP" -- /bin/echo "upgrade_ok")
exit_code=$?
set -e
if [ "$exit_code" -eq 0 ] && [ "$out" = "upgrade_ok" ]; then
    echo -e "${GREEN}[PASS]${NC} Repeated path rule safely upgraded from RO to RW"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}[FAIL]${NC} Repeated path rule handling failed: $out"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo -e "\n${RED}Edge case tests had $FAIL_COUNT failure(s).${NC}"
    exit 1
fi

echo -e "${GREEN}All error-path and edge-case tests passed successfully! ($PASS_COUNT checks)${NC}\n"
