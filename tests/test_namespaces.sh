#!/usr/bin/env bash
set -euo pipefail

SANDBOX="./bin/sandbox"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "--- Testing Linux Namespace Decoupling (--unshare-net) ---"

# Check if /proc/self/ns/net exists on this host
if [ ! -e "/proc/self/ns/net" ]; then
    echo -e "${YELLOW}[SKIP]${NC} Linux namespace virtual filesystem (/proc/self/ns/net) unavailable on this host"
    exit 0
fi

PARENT_NET_NS=$(readlink /proc/self/ns/net 2>/dev/null || echo "")

# Test unshare invocation
set +e
child_net_ns=$($SANDBOX --unshare-net --ro /proc --ro /bin --ro /usr --ro /lib --ro /lib64 -- /bin/readlink /proc/self/ns/net 2>&1)
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ]; then
    # Check if unshare failed due to environment permission restrictions
    if [[ "$child_net_ns" =~ "Failed to unshare" ]] || [[ "$child_net_ns" =~ "Operation not permitted" ]]; then
        echo -e "${YELLOW}[SKIP]${NC} Kernel disallowed unprivileged namespace decoupling (kernel.unprivileged_userns_clone=0 or container restriction)"
        exit 0
    else
        echo -e "${RED}[FAIL]${NC} Namespace isolation failed with unexpected error: $child_net_ns"
        exit 1
    fi
fi

if [ -n "$PARENT_NET_NS" ] && [ -n "$child_net_ns" ]; then
    if [ "$PARENT_NET_NS" != "$child_net_ns" ]; then
        echo -e "${GREEN}[PASS]${NC} Network namespace isolated successfully (Parent: $PARENT_NET_NS, Child: $child_net_ns)"
    else
        echo -e "${RED}[FAIL]${NC} Child remained in parent network namespace: $child_net_ns"
        exit 1
    fi
else
    echo -e "${GREEN}[PASS]${NC} --unshare-net executed and returned clean exit status"
fi

echo -e "${GREEN}Namespace isolation tests completed successfully!${NC}\n"
