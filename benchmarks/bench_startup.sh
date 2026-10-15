#!/usr/bin/env bash
set -euo pipefail

BOLD='\033[1m'
CYAN='\033[1;36m'
GREEN='\033[1;32m'
NC='\033[0m'

SANDBOX="./bin/sandbox"
ITERATIONS=100

echo -e "${BOLD}${CYAN}====================================================${NC}"
echo -e "${BOLD}${CYAN}  Ephemeral Sandbox - Startup Latency Benchmark     ${NC}"
echo -e "${BOLD}${CYAN}====================================================${NC}\n"

echo "Executing $ITERATIONS iterations of /bin/true..."

# Benchmark Native
start_native=$(date +%s%N)
for ((i=1; i<=ITERATIONS; i++)); do
    /bin/true
done
end_native=$(date +%s%N)
native_total_ns=$((end_native - start_native))
native_avg_us=$((native_total_ns / ITERATIONS / 1000))

# Benchmark Sandbox
start_sandbox=$(date +%s%N)
for ((i=1; i<=ITERATIONS; i++)); do
    $SANDBOX -- /bin/true
done
end_sandbox=$(date +%s%N)
sandbox_total_ns=$((end_sandbox - start_sandbox))
sandbox_avg_us=$((sandbox_total_ns / ITERATIONS / 1000))

overhead_us=$((sandbox_avg_us - native_avg_us))
if [ "$overhead_us" -lt 0 ]; then
    overhead_us=0
fi

echo -e "\nResults:"
echo -e "  - Native /bin/true average latency:    ${BOLD}${native_avg_us} µs${NC}"
echo -e "  - Sandbox /bin/true average latency:   ${BOLD}${sandbox_avg_us} µs${NC}"
echo -e "  - Sandbox initialization overhead:     ${BOLD}${overhead_us} µs${NC}"

if [ "$overhead_us" -lt 1000 ]; then
    echo -e "\n${BOLD}${GREEN}[VERIFIED] Sub-millisecond latency achieved! Overhead < 1.0 ms (${overhead_us} µs)${NC}\n"
else
    overhead_ms=$(awk "BEGIN {print $overhead_us/1000}")
    echo -e "\nOverhead: ${overhead_ms} ms\n"
fi
