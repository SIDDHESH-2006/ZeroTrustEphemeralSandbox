# PROJECT REVIEW - 1: TECHNICAL EVALUATION REPORT
## Zero-Trust Ephemeral Sandbox (Landlock LSM + Seccomp-BPF)

---

### Academic Information

- **Course Code & Title:** BCSE303P — Operating Systems Lab
- **School:** School of Computer Science Engineering and Information Systems (SCSEIS)
- **Institution:** Vellore Institute of Technology (VIT), Vellore
- **Academic Term:** Fall Semester 2026–2027
- **Review Stage:** Project Review - 1

#### Project Team
| Name | Registration Number | Email |
|---|---|---|
| **Mohammad Ammar Abdullah** | `24BYB0017` | ammar.abdullah2024@vitstudent.ac.in |
| **Siddhesh Srinivasan** | `24BYB0035` | siddhesh.srinivasan2024@vitstudent.ac.in |
| **Aditi Soni** | `24BDE0144` | aditi.soni2024@vitstudent.ac.in |

**Faculty Guide:**  
**Dr. PORKODI S**, Assistant Professor Senior Grade – 1, SCSEIS, VIT Vellore

---

## 1. Executive Summary

Executing untrusted third-party scripts, build hooks (`npm install`, `pip setup.py`), or dynamic binaries presents a major security challenge in modern computing. Traditional virtualization solutions (such as VirtualBox or QEMU) and container runtimes (such as Docker or Podman) incur massive startup latencies (500ms to 2000ms), heavy memory footprints, and typically require background daemons or administrative root access (`sudo`).

The **Zero-Trust Ephemeral Sandbox** is a lightweight, pure C application that creates an in-process, unprivileged kernel jail using native Linux security primitives:
1. **Landlock LSM:** In-kernel filesystem access control enforcing read-only vs. read-write directory boundaries without root privileges.
2. **Seccomp-BPF:** Berkeley Packet Filter system call interception denying network access, memory inspection, debugging (`ptrace`), and `io_uring` side-channels.
3. **Capability Management:** Drops all Linux capabilities (`CapEff = 0`, `CapAmb = 0`) via `capset()`.
4. **Namespace Decoupling:** Isolates network interfaces via unprivileged `unshare(CLONE_NEWUSER | CLONE_NEWNET)`.
5. **Zero-Overhead Dispatch:** Replaces the sandbox supervisor memory image with the target payload using `execvp()`.

**Key Empirical Result:** On a Linux 6.3.0 ARM64 environment across 1,000 iterations, the sandbox demonstrated a median initialization overhead of **`360.10 µs` (0.360 ms)**, comfortably achieving the project's sub-millisecond target with **100% test suite completion (9/9 passed)** and **zero runtime failures**.

---

## 2. Test Environment Specifications

All empirical measurements and test validations were recorded directly on the evaluation host:

| Parameter | Specification |
|---|---|
| **Operating System** | Kali GNU/Linux Rolling (`Linux 6.3.0-kali1-arm64`) |
| **Kernel Version** | `6.3.0-kali1-arm64` (Landlock LSM ABI v3 enabled) |
| **CPU Architecture** | `aarch64` (ARM64 64-bit) |
| **Compiler & Flags** | `cc` (GCC 12.2.0) with `-Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2` |
| **Clock Source** | POSIX `CLOCK_MONOTONIC` (Hardware nanosecond resolution) |
| **External Dependencies** | **None (Pure C standard library only)** |

---

## 3. Automated Test Suite Results (9 / 9 Passed)

The sandbox was subjected to 9 distinct automated test suites covering command parsing, filesystem confinement, network denial, anti-debugging, syscall filtering, bypass regressions, edge cases, capability dropping, and namespace isolation.

### Master Test Summary

```
====================================================
  Zero-Trust Ephemeral Sandbox - Master Test Runner 
====================================================
Test Suite Summary: Total: 9 | Passed: 9 | Failed: 0
====================================================
  ALL SECURITY, REGRESSION & EDGE TESTS PASSED! (9/9) 
====================================================
```

### Detailed Test Verification Breakdown

| Suite # | Test Suite Script | Verified Properties | Status | Result / Output |
|:---:|---|---|:---:|---|
| **1** | `tests/test_cli.sh` | CLI argument parser, `--version`, `--help`, payload separator (`--`), missing arguments. | **PASS** | `Clean payload execution via '--' separator` |
| **2** | `tests/test_fs.sh` | Landlock LSM path boundaries: allowlisted read succeeds, unlisted read blocked (`EACCES`), write in `--ro` directory blocked (`EACCES`), write in `--rw` succeeds. | **PASS** | `Writing to --ro path blocked by Landlock (Permission denied)` |
| **3** | `tests/test_net.sh` | Seccomp-BPF network filter: `socket()` denied with `EPERM` in default mode; permitted when `--allow-net` is specified. | **PASS** | `Network socket() blocked by Seccomp filter (EPERM)` |
| **4** | `tests/test_ptrace.sh` | Anti-debugging: denies `ptrace()` attachment to prevent inspection or memory modification. | **PASS** | `ptrace() denied by Seccomp filter` |
| **5** | `tests/test_seccomp_hardened.sh` | Hardened syscall restrictions: blocks `io_uring_setup`, `process_vm_readv`, `process_vm_writev`, network calls, and `ptrace` (6 individual checks). | **PASS** | `All Seccomp-BPF hardening tests passed (6/6 checks)` |
| **6** | `tests/test_bypass_regression.sh` | Sandbox jail confinement: symlink traversal to external secrets blocked, relative path traversal (`../`) blocked, write tampering blocked (5 checks). | **PASS** | `Symlink path traversal blocked by Landlock (Permission denied)` |
| **7** | `tests/test_edge_cases.sh` | System safety: non-existent paths fail safe, exit code `127` on missing binary, exit code `126` on non-executable payload, repeated options upgrade smoothly (10 checks). | **PASS** | `All error-path and edge-case tests passed (10/10 checks)` |
| **8** | `tests/test_capabilities.sh` | Privilege dropping: inspects `/proc/self/status` inside the sandbox to verify that capability bitmaps are completely zeroed. | **PASS** | `CapEff: 0000000000000000`, `CapAmb: 0000000000000000` |
| **9** | `tests/test_namespaces.sh` | Network namespace isolation: invokes `unshare(CLONE_NEWUSER \| CLONE_NEWNET)` and asserts distinct network namespace ID. | **PASS** | `Isolated: Parent net:[4026531840] -> Child net:[4026532818]` |

---

## 4. Performance & Startup Latency Benchmark

### Benchmark Methodology
The benchmark utility (`benchmarks/bench_timer.c`) measures the process startup latency using the high-resolution hardware timer `CLOCK_MONOTONIC`.
- **Sample Count:** 1,000 iterations (preceded by 50 warm-up runs to prime OS page caches).
- **Baseline Test:** Execution of `/bin/true` directly.
- **Sandbox Test:** Execution of `/bin/true` inside `./bin/sandbox -- /bin/true`.
- **Differential Calculation:** Isolates the pure initialization overhead imposed by applying Landlock LSM rules, assembling and injecting the Seccomp-BPF filter, and dropping capabilities.

### Empirical Measurements (Kali Linux ARM64 — 1,000 Iterations)

```
Benchmark Results (Latency in Microseconds / µs):
+--------------------------------+-----------------+-----------------+
| Metric                         | Native Baseline | Sandbox Jail    |
+--------------------------------+-----------------+-----------------+
| Samples / Total Iterations     |            1000 |            1000 |
| Failed Invocations             |               0 |               0 |
| Minimum Latency (Min)          |       145.70 µs |       483.09 µs |
| Median Latency (P50)           |       179.82 µs |       539.92 µs |
| Arithmetic Mean                |       190.97 µs |       554.54 µs |
| 95th Percentile (P95)          |       228.94 µs |       662.70 µs |
| 99th Percentile (P99)          |       590.87 µs |       756.86 µs |
| Maximum Latency (Max)          |      2278.49 µs |      1537.30 µs |
| Standard Deviation             |        99.85 µs |        71.43 µs |
+--------------------------------+-----------------+-----------------+
```

### Overhead Analysis & Target Verification

$$\text{Initialization Overhead (Median)} = 539.92\,\mu\text{s} - 179.82\,\mu\text{s} = \mathbf{360.10\,\mu\text{s}}\quad (\mathbf{0.360\,ms})$$
$$\text{Initialization Overhead (Mean)} = 554.54\,\mu\text{s} - 190.97\,\mu\text{s} = \mathbf{363.57\,\mu\text{s}}\quad (\mathbf{0.364\,ms})$$

- **Target Constraint:** Overhead must be less than $1.0\,\text{ms}$ ($1000\,\mu\text{s}$).
- **Observed Result:** **$0.360\,\text{ms}$** (over **$64\%$ faster** than the ceiling target).
- **Stability:** $0$ failed invocations out of $1,000$ runs ($100\%$ success rate). Standard deviation of sandboxed execution was just $71.43\,\mu\text{s}$, demonstrating consistent, deterministic performance.

---

## 5. Architectural Comparison with Existing Technologies

| Architectural Dimension | Virtual Machines (QEMU/KVM) | Container Engines (Docker/Podman) | Traditional `chroot` Jail | **Zero-Trust Sandbox (Our Project)** |
|---|---|---|---|---|
| **Root Privileges Required** | Yes (or KVM device access) | Yes (daemon or rootless setup) | Yes (`CAP_SYS_CHROOT`) | **No (100% Unprivileged)** |
| **Startup Overhead** | ~5,000ms – 15,000ms | ~500ms – 2,000ms | ~5ms – 20ms | **`0.360 ms` (< 1.0 ms)** |
| **Filesystem Confinement** | Virtual disk image | OverlayFS / Mount namespaces | Trivially escapable (inode jail) | **Kernel Landlock LSM (Inescapable)** |
| **Syscall Filtering** | Emulated hardware layer | Default Seccomp JSON profiles | None | **Custom Classic BPF Filter (EPERM)** |
| **Network Isolation** | Virtual bridge / TAP device | Network namespaces / veth pairs | None | **Seccomp Denial + `CLONE_NEWNET`** |
| **External Dependencies** | Hypervisor, Guest OS image | Daemon, containerd, runc | Host rootfs preparation | **Zero Dependencies (Pure C / libc)** |

---

## 6. Security Enforcement Architecture

The execution pipeline enforces four cascading barriers before the untrusted payload is dispatched:

```
+-----------------------------------------------------------------------------------+
|  1. CLI SUPERVISOR & PATH RESOLUTION (src/cli.c)                                  |
|     - Parses: --ro, --rw, --no-net, --allow-net, --unshare-net, --drop-caps      |
|     - Validates user paths with stat() — aborts immediately if path is missing     |
|     - Auto-allowlists system library paths (/usr, /lib, /bin, /dev)               |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  2. PRIVILEGE CONFINEMENT (src/isolate.c)                                         |
|     - Sets prctl(PR_SET_NO_NEW_PRIVS, 1) [Enables unprivileged security locks]    |
|     - Clears ambient capabilities: prctl(PR_CAP_AMBIENT_CLEAR_ALL)                |
|     - Clears permitted, effective & inheritable capabilities via raw capset()     |
|     - Verifies with capget() that effective capabilities are completely zeroed    |
|     - Optionally decouples network namespace via unshare(CLONE_NEWNET)            |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  3. LANDLOCK LSM FILESYSTEM BARRIER (src/landlock_mgr.c)                          |
|     - Queries Landlock kernel ABI version (v1-v3); fails closed if unsupported   |
|     - Creates ruleset with landlock_create_ruleset()                              |
|     - Adds read-only and read-write rules with landlock_add_rule()                |
|     - Restricts calling process and all future children: landlock_restrict_self()|
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  4. SECCOMP-BPF SYSTEM CALL FILTER (src/seccomp_bpf.c)                            |
|     - Validates syscall architecture (AUDIT_ARCH_AARCH64 / AUDIT_ARCH_X86_64)   |
|     - Denies io_uring calls (io_uring_setup, io_uring_enter, io_uring_register)  |
|     - Denies cross-process memory inspection (process_vm_readv, process_vm_writev)|
|     - Denies ptrace(), chroot(), pivot_root(), mount(), reboot()                  |
|     - Denies network calls (socket, connect, bind, sendto, recvmsg, etc.)         |
|     - Injects classic BPF filter via SYS_seccomp (SECCOMP_SET_MODE_FILTER)       |
+-----------------------------------------------------------------------------------+
                                          |
                                          v (execvp)
+-----------------------------------------------------------------------------------+
|  5. ZERO-OVERHEAD CONFINED PAYLOAD EXECUTION                                      |
|     - Replaces sandbox process image with target binary: zero memory retention    |
+-----------------------------------------------------------------------------------+
```

---

## 7. Interactive Demonstration Verification (`examples/demo.sh`)

An automated demonstration script (`examples/demo.sh`) executed on Kali Linux verifies the following live security properties:

1. **Permitted Command Execution:** Basic commands execute with zero wrapper overhead.
2. **Read Allowlisted Path:** Files inside an explicit `--ro` directory are read without restriction.
3. **Write Protection in Read-Only Path:** Attempting to `touch` a file inside `--ro` is immediately blocked by Landlock LSM with `Permission denied`.
4. **Write Inside Allowed Path:** Creating files in `--rw` directory succeeds normally.
5. **Jail Boundary Enforcement:** Attempting to read files outside allowlisted paths (e.g. `/secret/private_key.pem`) is blocked with `Permission denied`.
6. **Network Socket Blocking:** Executing Python socket creation triggers `PermissionError: [Errno 1] Operation not permitted` via Seccomp-BPF.
7. **Anti-Debugging Protection:** System calls to `ptrace()` are blocked with `EPERM`.
8. **Fail-Safe Integrity:** Passing an invalid or non-existent path rule causes the sandbox to abort immediately without running the payload.

---

## 8. Conclusion

For **Project Review - 1**, the **Zero-Trust Ephemeral Sandbox** has met and exceeded all technical, security, and performance criteria:
- **Core Security:** Full unprivileged isolation combining Landlock LSM, Seccomp-BPF, capability zeroing, and namespace decoupling.
- **Reliability:** 100% test pass rate across 9 comprehensive test suites with zero defects.
- **Performance:** Empirically verified initialization latency of **`0.360 ms`**, achieving the sub-millisecond design goal.
- **Code Quality:** Pure C11 compliant, compiling with zero warnings under strict flags (`-Wall -Wextra -Werror -pedantic`).

The project is fully demonstrable, validated on modern Linux kernels, and ready for formal evaluation.
