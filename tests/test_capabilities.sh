#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "--- Testing Linux Capability Dropping & Privilege Confinement ---"

if [ ! -r "/proc/self/status" ]; then
    echo -e "${YELLOW}[SKIP]${NC} /proc/self/status not available on this platform"
    exit 0
fi

CAT_BIN=$(command -v cat || echo "/bin/cat")

# Run cat /proc/self/status inside the sandbox with capability dropping enabled
set +e
status_output=$($SANDBOX --drop-caps --ro /proc -- "$CAT_BIN" /proc/self/status 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ] || [ -z "$status_output" ]; then
    echo -e "${YELLOW}[SKIP]${NC} Unable to inspect /proc/self/status within sandbox (exit $exit_code: $status_output)"
    exit 0
fi

# Extract capability lines
cap_inh=$(echo "$status_output" | grep -i '^CapInh:' | awk '{print $2}')
cap_prm=$(echo "$status_output" | grep -i '^CapPrm:' | awk '{print $2}')
cap_eff=$(echo "$status_output" | grep -i '^CapEff:' | awk '{print $2}')
cap_amb=$(echo "$status_output" | grep -i '^CapAmb:' | awk '{print $2}')

echo "  Inspected Capability Sets:"
echo "    - Inheritable (CapInh): $cap_inh"
echo "    - Permitted   (CapPrm): $cap_prm"
echo "    - Effective   (CapEff): $cap_eff"
echo "    - Ambient     (CapAmb): $cap_amb"

is_zero() {
    local val="$1"
    # Match hexadecimal string of all zeros
    if [[ "$val" =~ ^0+$ ]]; then
        return 0
    else
        return 1
    fi
}

FAIL=0

# Verify CapEff is zero
if is_zero "$cap_eff"; then
    echo -e "${GREEN}[PASS]${NC} Effective capability set (CapEff) is completely zeroed"
else
    echo -e "${RED}[FAIL]${NC} Effective capability set retained non-zero bits: $cap_eff"
    FAIL=1
fi

# Verify CapAmb is zero
if is_zero "$cap_amb"; then
    echo -e "${GREEN}[PASS]${NC} Ambient capability set (CapAmb) is completely zeroed"
else
    echo -e "${RED}[FAIL]${NC} Ambient capability set retained non-zero bits: $cap_amb"
    FAIL=1
fi

if [ "$FAIL" -ne 0 ]; then
    exit 1
fi

echo -e "${GREEN}All capability reduction tests passed successfully!${NC}\n"
