# ZERO-TRUST EPHEMERAL SANDBOX (LANDLOCK LSM + SECCOMP-BPF)

**Course:** BCSE303P — Operating Systems Lab  
**Department:** School of Computer Science Engineering and Information Systems (SCSEIS)  
**Institution:** Vellore Institute of Technology (VIT), Vellore  
**Academic Year:** Fall Semester 2026–2027  
**Review Stage:** Project Review - 1  

#### Project Team
- **Mohammad Ammar Abdullah** (`24BYB0017`)  
- **Siddhesh Srinivasan** (`24BYB0035`)  
- **Aditi Soni** (`24BDE0144`)  

**Faculty Guide:**  
**Dr. PORKODI S** (Assistant Professor Senior Grade – 1, SCSEIS, VIT Vellore)  

---

## ABSTRACT

Modern software development regularly requires executing untrusted or semi-trusted third-party code, including package manager lifecycle hooks, dynamic compilation scripts, and autonomous artificial intelligence code artifacts. Traditional isolation mechanisms, such as hardware virtual machines and containerization engines like Docker, impose prohibitive initialization latency (500 ms to 2000 ms), substantial memory overhead, and frequently demand administrative superuser privileges (`sudo`). This project designs and implements the Zero-Trust Ephemeral Sandbox, a lightweight, sub-millisecond process confinement engine engineered in pure C using native Linux kernel security primitives. The architecture enforces zero-trust confinement across four sequential barriers: unprivileged privilege locking (`PR_SET_NO_NEW_PRIVS`), Linux capability dropping via `capset()`, in-kernel path-based access control via Landlock Linux Security Module (LSM), and system call interception using classic Berkeley Packet Filter bytecode (Seccomp-BPF). Payload dispatch is executed via `execvp()`, yielding zero wrapper memory retention during execution. Evaluated empirically on an ARM64 Linux 6.3 environment across 1,000 measured iterations using hardware monotonic timers, the sandbox achieved a median initialization overhead of just 360.10 microseconds (0.360 ms) with 100% test suite completion across nine test suites and zero execution failures.

---

## Keywords

`Zero-Trust Architecture`, `Process Sandboxing`, `Landlock LSM`, `Seccomp-BPF`, `Unprivileged Execution`, `Ephemeral Computing`

---

## INTRODUCTION

Process containment is a foundational requirement in contemporary operating systems security. As modern software engineering transitions toward decentralized package dependencies (`npm`, `PyPI`, `crates.io`) and automated continuous integration pipelines, developers routinely execute foreign code directly on host workstations. Without strict confinement, malicious scripts can read sensitive SSH keys, exfiltrate environment variables, compromise root namespaces, or spawn background reverse shells.

Historically, isolation has relied on full hardware virtualization (QEMU/KVM) or container daemons (Docker, Podman, LXC). While effective for long-running services, these platforms are ill-suited for short-lived, high-frequency, ephemeral commands due to their second-scale initialization latency and dependency on root-privileged background daemons. 

This project explores an in-kernel, unprivileged paradigm. By harnessing Linux 5.13+ security features—specifically Landlock LSM and Seccomp-BPF—the Zero-Trust Ephemeral Sandbox establishes strict, programmable security boundaries around untrusted processes in fractions of a millisecond without administrative intervention.

---

## PROBLEM STATEMENT

To engineer a lightweight, sub-millisecond, dependency-free process isolation sandbox in C that guarantees zero-trust containment for arbitrary untrusted payloads on Linux without requiring superuser (`root`) privileges, background daemons, or virtualized guest operating systems, while strictly enforcing filesystem path allowlisting, network socket denial, capability stripping, and system call filtering.

---

## OBJECTIVES

1. **Sub-Millisecond Initialization:** Achieve an initialization overhead strictly below 1.0 ms (< 1000 µs) to make ephemeral process isolation practically imperceptible.
2. **100% Unprivileged Confinement:** Enforce comprehensive security barriers without requiring `sudo`, setuid binaries, or elevated privileges.
3. **In-Kernel Filesystem Jailing:** Utilize Landlock LSM (ABI v1–v3) to enforce strict read-only (`--ro`) and read-write (`--rw`) path hierarchies that cannot be bypassed via symlink manipulation or directory traversals.
4. **Syscall-Level Attack Surface Reduction:** Assemble and inject raw Seccomp-BPF filters to eliminate attack surfaces including `socket()`, `ptrace()`, `process_vm_readv/writev`, and asynchronous `io_uring` subsystems.
5. **Zero Runtime Overhead:** Ensure the sandbox supervisor memory image is entirely replaced upon payload handoff via `execvp()`, eliminating ongoing CPU and RAM overhead.

---

## SCOPE OF THE PROJECT

- **Target Workloads:** Ephemeral commands, untrusted utility scripts (Python, Bash, Node.js), build-time package manager hooks, dynamic code evaluation, and automated grading pipelines.
- **Operating System Environment:** Modern Linux systems (Kernel ≥ 5.13) with Landlock LSM enabled across `x86_64`, `aarch64`, `arm`, and `i386` architectures.
- **Out of Scope:** Emulation of foreign instruction sets, hardware memory encryption (Intel SGX / AMD SEV), and cgroup-based CPU/RAM quota enforcement (which requires root-level cgroups v2 control).

---

## Literature Survey / Existing Systems

| Existing System | Core Mechanism | Startup Overhead | Privilege Requirement | Limitations |
|---|---|:---:|:---:|---|
| **Virtual Machines (KVM / QEMU)** | Full hardware virtualization, guest kernel, virtual disk image | 5,000 ms – 15,000 ms | High (`/dev/kvm`, root) | Massive RAM overhead, disk bloat, prohibitive latency for one-off scripts. |
| **Container Engines (Docker / Podman)** | Namespaces, cgroups, OverlayFS, containerd / runc | 500 ms – 2,000 ms | High (Daemon or complex rootless subuid mapping) | Multi-second launch latency; daemon vulnerabilities; container escape vectors. |
| **Traditional `chroot(2)`** | Virtual root directory inode redirection | ~10 ms – 20 ms | Requires `CAP_SYS_CHROOT` (`root`) | Trivially escapable by unconfined root processes; no syscall filtering; no network restrictions. |
| **gVisor (runsc)** | User-space kernel interception in Go (Sentry) | 150 ms – 300 ms | Low to Medium | High system call translation penalty (2×–10× syscall overhead); heavy memory footprint. |
| **Bubblewrap (`bwrap`)** | User namespaces, mount namespaces, pivot_root | 10 ms – 30 ms | Unprivileged (if enabled by sysctl) | Relies heavily on mount namespace manipulation; noticeable millisecond overhead. |
| **Proposed System (Zero-Trust Sandbox)** | **Landlock LSM + Raw Seccomp-BPF + `execvp`** | **`0.360 ms`** | **100% Unprivileged** | **Near-zero latency; inescapable kernel VFS isolation; raw BPF syscall blocking.** |

---

## PROPOSED SYSTEM

The proposed system is an unprivileged single-binary CLI utility written in ISO C11. It accepts an arbitrary target command with granular path permissions (`--ro <path>`, `--rw <path>`) and network flags (`--no-net`, `--allow-net`, `--unshare-net`).

The system implements a strictly ordered, fail-safe defense pipeline:
1. **Argument Parsing & Fail-Safe Verification:** Validates user-specified directories with `stat()`; aborts immediately if any requested path does not exist.
2. **Privilege Locking:** Calls `prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)` to guarantee that child processes can never obtain privileges via setuid or file capabilities.
3. **Capability Reduction:** Invokes `prctl(PR_CAP_AMBIENT_CLEAR_ALL)` and raw `syscall(SYS_capset)` to zero out Effective, Permitted, and Inheritable capabilities, verifying the zeroed state with `capget()`.
4. **Namespace Decoupling:** Optionally unshares network and user namespaces via `unshare(CLONE_NEWUSER | CLONE_NEWNET)`.
5. **Landlock LSM Enforcement:** Creates a kernel ruleset via `SYS_landlock_create_ruleset`, populates path rules using `SYS_landlock_add_rule`, and locks the calling process tree with `SYS_landlock_restrict_self`.
6. **Seccomp-BPF Syscall Filtering:** Assembles a classic BPF instruction array denying prohibited syscall numbers with `SECCOMP_RET_ERRNO(EPERM)` and injects the filter into the kernel via `SYS_seccomp`.
7. **Execution Handoff:** Calls `execvp()` to execute the untrusted payload.

---

## SYSTEM ARCHITECTURE

```
+-----------------------------------------------------------------------------------+
|  1. CLI SUPERVISOR & CONFIGURATION ENGINE (src/cli.c)                             |
|     - Parses: --ro <dir>, --rw <dir>, --no-net, --allow-net, --unshare-net        |
|     - Enforces Fail-Safe Rule: Aborts if explicit user path cannot be accessed     |
|     - Auto-allowlists host standard library paths (/usr, /lib, /bin, /dev)        |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  2. PRIVILEGE CONFINEMENT BARRIER (src/isolate.c)                                 |
|     - prctl(PR_SET_NO_NEW_PRIVS, 1) [Enables unprivileged security locks]        |
|     - prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL)                             |
|     - Raw capset() zeroing: Effective, Permitted, Inheritable capabilities        |
|     - Capget() verification: Confirms CapEff == 0, CapAmb == 0                    |
|     - Optional: unshare(CLONE_NEWUSER | CLONE_NEWNET) [Namespace Isolation]       |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  3. LANDLOCK LSM FILESYSTEM CONTAINMENT BARRIER (src/landlock_mgr.c)              |
|     - Queries kernel Landlock ABI version (v1-v3); fails closed on unsupported OS |
|     - Creates in-kernel ruleset: landlock_create_ruleset()                        |
|     - Adds read-only & read-write tree rules: landlock_add_rule()                 |
|     - Locks filesystem sandbox: landlock_restrict_self()                          |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  4. SECCOMP-BPF SYSTEM CALL FILTER BARRIER (src/seccomp_bpf.c)                    |
|     - Validates host architecture (AUDIT_ARCH_AARCH64 / AUDIT_ARCH_X86_64)       |
|     - Denies io_uring: io_uring_setup, io_uring_enter, io_uring_register (EPERM) |
|     - Denies cross-process memory: process_vm_readv, process_vm_writev (EPERM)   |
|     - Denies tracing & debugging: ptrace (EPERM)                                  |
|     - Denies container manipulation: chroot, pivot_root, mount, reboot (EPERM)    |
|     - Denies network syscalls: socket, connect, bind, sendto, recvmsg (EPERM)     |
|     - Injects raw BPF program into kernel: seccomp(SECCOMP_SET_MODE_FILTER)       |
+-----------------------------------------------------------------------------------+
                                          |
                                          v (execvp)
+-----------------------------------------------------------------------------------+
|  5. ZERO-OVERHEAD TARGET PAYLOAD EXECUTION                                        |
|     - Memory image replaced: Supervisor terminates, zero wrapper overhead         |
|     - Payload executes within inescapable hardware & kernel security boundaries   |
+-----------------------------------------------------------------------------------+
```

---

## OS Concepts Used

1. **Process Lifecycle & Image Replacement (`fork`, `execvp`, `waitpid`):** Decoupling configuration from payload execution; replacing virtual memory space via `execvp()` to achieve zero ongoing CPU/memory overhead.
2. **Access Control & Reference Monitors (Landlock LSM):** Linux Security Module hook architecture operating at the Virtual Filesystem (VFS) layer, enforcing mandatory access control (MAC) beneath directory subtrees.
3. **Berkeley Packet Filter (Seccomp-BPF):** In-kernel instruction set architecture (ISA) evaluating accumulator registers against system call numbers (`nr`) and system architectures (`arch`).
4. **POSIX Linux Capabilities (`capset`, `capget`):** Fine-grained privileges partitioning the traditional monolithic `root` privilege into distinct capabilities (`CAP_NET_ADMIN`, `CAP_SYS_ADMIN`), completely stripped before payload dispatch.
5. **Namespaces & Isolation (`unshare`, `CLONE_NEWNET`, `CLONE_NEWUSER`):** Kernel virtualization partitioning global system resources (network routing tables, loopback interfaces) into independent process-specific instances.
6. **Process Credentials & Privilege Restrictions (`PR_SET_NO_NEW_PRIVS`):** Bit-flag preventing processes and their descendants from acquiring new privileges through setuid binaries or filesystem capabilities.
7. **Monotonic Clocks & Benchmarking (`CLOCK_MONOTONIC`):** High-precision hardware timers immune to Network Time Protocol (NTP) adjustments and wall-clock shifts.

---

## Experimental Setup

- **Host Machine:** Apple Silicon Architecture running Kali Linux Rolling.
- **Kernel Version:** Linux Kernel `6.3.0-kali1-arm64`.
- **Hardware Architecture:** `aarch64` (64-bit ARM).
- **Compilation Toolchain:** GNU Compiler Collection (`gcc` 12.2.0) with strict ANSI flags:
  ```bash
  gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2
  ```
- **Measurement Engine:** Standalone C timing utility (`benchmarks/bench_timer.c`) evaluating 50 warm-up runs followed by 1,000 recorded iterations of native `/bin/true` versus `./bin/sandbox -- /bin/true`.

---

## Implementation

The implementation is structured modularly across five source files and five header definitions:

- **`src/main.c`:** Program entry point orchestrating the security pipeline and final `execvp()` dispatch.
- **`src/cli.c`:** CLI argument parser (`getopt_long`), path validation (`stat`, `realpath`), default system library auto-allowlisting (`/usr`, `/lib`, `/bin`, `/dev`), and alias handling (`--no-net`, `--deny-net`).
- **`src/isolate.c`:** Enforces `no_new_privs`, clears ambient capabilities, drops effective/permitted capability sets via raw `capset`, verifies capability state with `capget`, and decouples namespaces with `unshare()`.
- **`src/landlock_mgr.c`:** Implements pure C system call wrappers for `landlock_create_ruleset`, `landlock_add_rule`, and `landlock_restrict_self` (handling ABI versions 1, 2, and 3).
- **`src/seccomp_bpf.c`:** Assembles classic BPF bytecode into a 256-instruction array validating system architecture (`AUDIT_ARCH_AARCH64`) and emitting conditional jump instructions to block unauthorized syscalls with `EPERM`.

---

## Testing - Test cases, expected output, actual output, pass/fail

| Test ID | Test Category & Script | Description / Invoked Command | Expected Output | Actual Output | Status |
|:---:|---|---|---|---|:---:|
| **TC-01** | **CLI Verification**<br>`test_cli.sh` | Execution of `--version`, `--help`, and valid command via `--` | Clean version banner and command execution | Contributor details displayed; `/bin/echo` executed cleanly | **PASS** |
| **TC-02** | **Filesystem Read**<br>`test_fs.sh` | Reading allowlisted file in `--ro` directory (`/cat allowed/file.txt`) | File contents printed to stdout | `allowed content` printed cleanly | **PASS** |
| **TC-03** | **Filesystem Unauthorized Read**<br>`test_fs.sh` | Reading unlisted directory outside jail (`/cat secret/secret.txt`) | Kernel aborts read with `Permission denied` | `cat: secret.txt: Permission denied` | **PASS** |
| **TC-04** | **Filesystem Write Protection**<br>`test_fs.sh` | Attempting to create file in `--ro` path (`touch readonly/tamper.txt`) | Kernel aborts write with `Permission denied` | `touch: cannot touch ...: Permission denied` | **PASS** |
| **TC-05** | **Filesystem Allowed Write**<br>`test_fs.sh` | Creating file inside explicitly permitted `--rw` directory | File created successfully on disk | File exists on disk; exit code `0` | **PASS** |
| **TC-06** | **Network Socket Blocking**<br>`test_net.sh` | Attempting `socket(AF_INET, SOCK_STREAM)` in default mode | System call returns `EPERM` (errno 1) | `PermissionError: [Errno 1] Operation not permitted` | **PASS** |
| **TC-07** | **Network Explicit Permission**<br>`test_net.sh` | Attempting `socket()` when `--allow-net` is provided | Socket created successfully | Socket returns fd successfully; exit code `0` | **PASS** |
| **TC-08** | **Anti-Debugging / Tracing**<br>`test_ptrace.sh` | Calling `ptrace(PTRACE_TRACEME, 0, 0, 0)` inside sandbox | Kernel denies ptrace with `EPERM` | `ptrace denied with errno 1` | **PASS** |
| **TC-09** | **Hardened Syscalls (`io_uring`)**<br>`test_seccomp_hardened.sh` | Issuing `syscall(SYS_io_uring_setup, ...)` | System call returns `EPERM` | `io_uring_setup() blocked by Seccomp (EPERM)` | **PASS** |
| **TC-10** | **Cross-Process Memory**<br>`test_seccomp_hardened.sh` | Issuing `process_vm_readv` and `process_vm_writev` | System call returns `EPERM` | `process_vm_readv/writev blocked by Seccomp (EPERM)` | **PASS** |
| **TC-11** | **Symlink Escape Bypass**<br>`test_bypass_regression.sh` | Accessing symlink inside jail pointing to file outside jail | Landlock blocks symlink resolution | `Permission denied` | **PASS** |
| **TC-12** | **Path Traversal Bypass**<br>`test_bypass_regression.sh` | Traversing jail boundary via relative dots (`allowed/../secret`) | Landlock blocks directory traversal | `Permission denied` | **PASS** |
| **TC-13** | **Command Not Found**<br>`test_edge_cases.sh` | Invoking non-existent target binary | Sandbox returns POSIX exit code `127` | Shell exit code is `127` | **PASS** |
| **TC-14** | **Permission Denied Binary**<br>`test_edge_cases.sh` | Invoking non-executable file as payload | Sandbox returns POSIX exit code `126` | Shell exit code is `126` | **PASS** |
| **TC-15** | **Invalid Path Fail-Safe**<br>`test_edge_cases.sh` | Providing non-existent `--ro /nonexistent/path` | Sandbox aborts execution before payload runs | `[sandbox:error] Specified path does not exist` | **PASS** |
| **TC-16** | **Capability Verification**<br>`test_capabilities.sh` | Inspecting `/proc/self/status` capability bitmaps | Effective and ambient capabilities are `0` | `CapEff: 0000000000000000`, `CapAmb: 0000000000000000` | **PASS** |
| **TC-17** | **Namespace Decoupling**<br>`test_namespaces.sh` | Comparing `/proc/self/ns/net` inode between parent and child | Child network namespace ID differs from parent | `Parent: net:[4026531840] -> Child: net:[4026532818]` | **PASS** |

### Benchmark Timing Verification (1,000 Iterations on Kali Linux ARM64)

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

Estimated Sandbox Initialization Overhead:
  - Overhead (Mean):   363.57 µs (0.364 ms)
  - Overhead (Median): 360.10 µs (0.360 ms)
```

---

## Conclusion

The **Zero-Trust Ephemeral Sandbox** demonstrates that secure, zero-trust process containment can be achieved with near-native execution latency and zero external dependencies. By synthesizing **Landlock LSM** for in-kernel filesystem enforcement, **Seccomp-BPF** for system call interception, **Linux capabilities dropping**, and **namespace isolation**, the project establishes an unprivileged sandbox that operates in **`0.360 milliseconds`**—over **1,400 times faster** than conventional container engines like Docker. With a 100% pass rate across nine automated test suites and complete validation on modern Linux kernels, the project successfully fulfills all technical, security, and performance criteria for **Project Review - 1**.

---

## REFERENCES

1. Corbet, J. (2021). *Landlock comes to 5.13*. LWN.net Kernel Security Architecture. Available at: https://lwn.net/Articles/850406/
2. Edge, J. (2015). *Seccomp BPF (SECure COMPuting with filters)*. LWN.net Systems Security. Available at: https://lwn.net/Articles/656307/
3. Kerrisk, M. (2010). *The Linux Programming Interface: A Linux and UNIX System Programming Handbook*. No Starch Press, San Francisco, CA. ISBN: 978-1593272203.
4. Linux Kernel Organization. (2023). *Landlock: Unprivileged Access Control Documentation (v6.3)*. Linux Kernel Archives. Available at: https://docs.kernel.org/userspace-api/landlock.html
5. Linux Kernel Organization. (2023). *Seccomp BPF: System Call Filtering Documentation*. Linux Kernel Archives. Available at: https://docs.kernel.org/userspace-api/seccomp_filter.html
6. McCanne, S., & Jacobson, V. (1993). *The BSD Packet Filter: A New Architecture for User-Level Packet Capture*. In Proceedings of the USENIX Winter 1993 Conference, San Diego, CA, pp. 259–270.
7. Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). *Operating System Concepts* (10th ed.). John Wiley & Sons, Hoboken, NJ. ISBN: 978-1119456339.
8. Tanenbaum, A. S., & Bos, H. (2014). *Modern Operating Systems* (4th ed.). Pearson Education, Boston, MA. ISBN: 978-0133591620.
9. Linux Manual Pages Consortium. (2023). *capabilities(7) — Overview of Linux capabilities*. Michael Kerrisk (ed.), Linux man-pages project. Available at: https://man7.org/linux/man-pages/man7/capabilities.7.html
10. Linux Manual Pages Consortium. (2023). *unshare(2) — Disassociate parts of the process execution context*. Linux man-pages project. Available at: https://man7.org/linux/man-pages/man2/unshare.2.html
