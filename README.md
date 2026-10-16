# Zero-Trust Ephemeral Sandbox (Landlock + Seccomp)

**BCSE303P — Operating Systems Lab | Project Review - 1**  
**School of Computer Science Engineering and Information Systems (SCSEIS)**  
**Vellore Institute of Technology (VIT)**  
*Fall Semester 2026–2027*

---

### Project Team
- **Mohammad Ammar Abdullah** — `24BYB0017`
- **Siddhesh Srinivasan** — `24BYB0035`
- **Aditi Soni** — `24BDE0144`

**Faculty Guide:**  
**Dr. PORKODI S**, Assistant Professor Senior Grade – 1, School of Computer Science Engineering and Information Systems

---

## 1. Abstract & Overview

Modern software development workflows regularly involve executing untrusted or semi-trusted third-party code — such as package manager build hooks (`npm install`, `pip setup.py`, `cargo build`), dynamically compiled scripts, and AI-generated code. Traditional process isolation solutions (such as virtual machines or container runtimes like Docker and Podman) incur heavy startup overhead (500ms to 2000ms), consume substantial memory, and often demand administrative privileges or background daemons.

The **Zero-Trust Ephemeral Sandbox** provides a **sub-millisecond, pure C process jail** utilizing native unprivileged Linux kernel security primitives:
- **Landlock LSM (Linux Security Module)**: In-kernel path-based filesystem access control (read-only vs. read-write containment).
- **Seccomp-BPF (Secure Computing Berkeley Packet Filter)**: Fine-grained system call interception denying unauthorized operations (`io_uring`, `process_vm_readv/writev`, network sockets, `ptrace`, container escape vectors).
- **Privilege Confinement**: Strict enforcement of `PR_SET_NO_NEW_PRIVS` and dropping of effective, permitted, and ambient Linux capabilities via raw `capset()`.
- **Namespace Decoupling**: Optional network namespace isolation via `unshare(CLONE_NEWUSER | CLONE_NEWNET)`.
- **Zero-Overhead Payload Dispatch**: Immediate memory image replacement via `execvp()`.

---

## 2. Platform Requirements

| Component | Minimum Requirement | Notes |
|---|---|---|
| **Operating System** | Linux (Kernel ≥ 5.13) | Linux-only project. macOS/Windows not supported. |
| **Landlock LSM** | Enabled in kernel (`CONFIG_SECURITY_LANDLOCK=y`) | Enabled by default in Ubuntu 22.04+, Debian 12+, Fedora 36+, Kali Linux 2022+, Arch Linux. |
| **Architectures** | `x86_64`, `aarch64`, `arm`, `i386` | Explicit architecture verification in BPF filter. |
| **Compiler & Tools** | GCC or Clang (C11 support), GNU Make | Strict flags: `-Wall -Wextra -Werror -pedantic -std=c11 -O2`. |
| **External Libraries** | **None (Zero Dependencies)** | Does not require `libseccomp`, `libcap`, or container runtimes. |

---

## 3. Architecture & Execution Pipeline

```
+-----------------------------------------------------------------------------------+
|  CLI SUPERVISOR (src/cli.c)                                                       |
|  - Parses flags (--ro, --rw, --no-net / --deny-net, --drop-caps, -- <cmd>)        |
|  - Validates user path existence (stat/realpath) — fails safe on invalid inputs   |
|  - Injects auto-allowlisted system library paths (/usr, /lib, /bin, /dev)         |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  PRIVILEGE LOCK (src/isolate.c)                                                   |
|  - Enforces prctl(PR_SET_NO_NEW_PRIVS, 1) [Mandatory for unprivileged security]   |
|  - Decouples network/user namespace via unshare() [Optional: --unshare-net]       |
|  - Clears ambient & permitted/effective Linux capabilities via capset()           |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  FILESYSTEM BARRIER (src/landlock_mgr.c)                                          |
|  - Detects Landlock ABI version (v1-v3); aborts if Landlock is unavailable        |
|  - Handles directories (tree-beneath) vs. regular files (execute/read/truncate)   |
|  - Locks filesystem boundary via landlock_restrict_self()                         |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|  SYSCALL FILTER BARRIER (src/seccomp_bpf.c)                                       |
|  - Verifies host architecture (AUDIT_ARCH_*) to prevent ABI mismatch confusion    |
|  - Denies io_uring (setup, enter, register)                                       |
|  - Denies cross-process memory inspection (process_vm_readv, process_vm_writev)   |
|  - Denies debugging & tracing (ptrace)                                            |
|  - Denies container/mount manipulation (chroot, pivot_root, mount, reboot, etc.)  |
|  - Denies network operations unless --allow-net (socket, connect, sendto, etc.)  |
|  - Injects raw classic BPF program via seccomp(SECCOMP_SET_MODE_FILTER)           |
+-----------------------------------------------------------------------------------+
                                          |
                                          v (execvp)
+-----------------------------------------------------------------------------------+
|  CONFINED PAYLOAD EXECUTION                                                       |
|  - Executes target program within hardware and kernel-enforced security boundaries|
+-----------------------------------------------------------------------------------+
```

---

## 4. Command-Line Reference

```
Usage: ./bin/sandbox [options] -- <command> [args...]

Options:
  -r, --ro <path>          Allow read-only access to <path> (repeatable)
  -w, --rw <path>          Allow read-write access to <path> (repeatable)
  -n, --no-net, --deny-net Deny all network socket and packet operations (default)
      --allow-net          Permit network socket syscalls
      --unshare-net        Unshare network and user namespace (CLONE_NEWNET)
      --drop-caps          Drop all ambient, permitted and inheritable capabilities (default)
      --no-default-paths   Do not auto-allowlist system shared library paths (/usr, /lib, etc.)
  -v, --verbose            Enable verbose diagnostic output
  -h, --help               Display usage syntax and exit
      --version            Display project details and version
```

### Practical Usage Examples

1. **Run a shell script with read-only `/usr` and isolated temporary read-write workspace:**
   ```bash
   ./bin/sandbox --ro /usr --rw /tmp -- /bin/bash script.sh
   ```

2. **Execute Python code under default zero-trust network block:**
   ```bash
   ./bin/sandbox --rw . -- /usr/bin/python3 -c "import socket; socket.socket()"
   # Result: PermissionError: [Errno 1] Operation not permitted
   ```

3. **Explicitly allow network operations:**
   ```bash
   ./bin/sandbox --allow-net -- /usr/bin/curl -I https://example.com
   ```

4. **Run with complete network namespace isolation:**
   ```bash
   ./bin/sandbox --unshare-net -- /bin/ip link
   ```

---

## 5. Security Model & Explicit Limitations

The Zero-Trust Ephemeral Sandbox is engineered as an **in-process kernel jail** for unprivileged command containment. To maintain architectural honesty, its security model and inherent boundaries are defined as follows:

1. **Lightweight Process Sandbox vs. Virtual Machine**:
   The sandbox shares the host Linux kernel. It is designed to block filesystem tampering, network exfiltration, process debugging, and unauthorized syscalls. It does not provide hardware-isolated memory encryption (e.g. AMD SEV / Intel SGX) or a guest kernel.
2. **Inherited File Descriptors**:
   Standard streams (stdin, stdout, stderr) are preserved so pipe workflows work seamlessly. Sockets opened prior to sandbox execution cannot issue `sendto()`, `recvfrom()`, `sendmsg()`, `recvmsg()`, or `setsockopt()` due to Seccomp-BPF filters; however, inherited streaming sockets may respond to generic `write()` if already connected before sandbox launch. Payloads should be launched directly or with closed pre-existing descriptors.
3. **Resource Controls (cgroups)**:
   The sandbox focuses on unprivileged security boundaries. Imposing strict memory limits (OOM killing) or CPU cycle quotas without root privileges requires cgroups v2 (`cgroup.subtree_control`), which is beyond the scope of a raw C unprivileged utility.
4. **Fail-Closed Guarantee**:
   If mandatory security restrictions cannot be applied (e.g., Landlock unsupported by kernel, user-specified path missing, capability dropping blocked), the sandbox **aborts immediately with an error and never executes the untrusted payload**.

---

## 6. Building and Installation

### Build on Linux
```bash
make clean && make
```
The compiled executable is placed at `bin/sandbox`.

### Run Automated Tests
```bash
make test
```
Executes the full suite of 9 automated test runners:
- `tests/test_cli.sh`: CLI parser, flag handling, help/version screens.
- `tests/test_fs.sh`: Landlock filesystem read-only and read-write confinement.
- `tests/test_net.sh`: Seccomp network socket blocking and `--allow-net` bypass.
- `tests/test_ptrace.sh`: Anti-debugging and ptrace restriction.
- `tests/test_seccomp_hardened.sh`: Restrictions on `io_uring`, `process_vm_readv/writev`, and socket calls.
- `tests/test_bypass_regression.sh`: Symlink escapes, path traversals (`..`), and write tampering.
- `tests/test_edge_cases.sh`: Command-not-found (127), permission-denied (126), missing payload, invalid paths.
- `tests/test_capabilities.sh`: Validates zeroed capability sets (`CapEff`, `CapAmb`, `CapInh`) in `/proc/self/status`.
- `tests/test_namespaces.sh`: Validates namespace decoupling (`--unshare-net`).

### Run Performance Benchmark
```bash
make bench
```
Executes the high-precision C benchmark (`benchmarks/bench_timer.c`) comparing native execution vs. sandbox startup over 1,000 iterations using `CLOCK_MONOTONIC`.

### Interactive Demonstration
```bash
chmod +x examples/demo.sh
./examples/demo.sh
```

---

## 7. Directory Structure

```
.
├── Makefile                     # POSIX build system with strict flags (-Wall -Wextra -Werror)
├── README.md                    # Project documentation, security model, and guide
├── CONTRIBUTING.md              # Contributor guidelines and architecture standards
├── include/
│   ├── sandbox.h                # Core configuration, path rules, and ANSI logging macros
│   ├── cli.h                    # CLI parser declarations and system library defaults
│   ├── isolate.h                # Capability dropping and namespace isolation prototypes
│   ├── landlock_mgr.h           # Landlock LSM ABI definitions and ruleset functions
│   └── seccomp_bpf.h            # Classic BPF filter generator declarations
├── src/
│   ├── main.c                   # Entry point and security barrier lifecycle
│   ├── cli.c                    # Argument parser, option validation, and default paths
│   ├── isolate.c                # PR_SET_NO_NEW_PRIVS, capset, and unshare implementations
│   ├── landlock_mgr.c           # Landlock ABI detection, ruleset creation, and enforcement
│   └── seccomp_bpf.c            # Pure C BPF assembly for syscall denials
├── tests/
│   ├── run_all.sh               # Master test runner summarizing PASS/FAIL/SKIP
│   ├── test_cli.sh              # CLI supervisor interface tests
│   ├── test_fs.sh               # Landlock LSM filesystem boundary tests
│   ├── test_net.sh              # Seccomp-BPF network socket denial tests
│   ├── test_ptrace.sh           # Anti-debugging / ptrace denial tests
│   ├── test_seccomp_hardened.sh # io_uring, process_vm, and network-adjacent tests
│   ├── test_bypass_regression.sh# Symlink jail escapes and path traversal regression tests
│   ├── test_edge_cases.sh       # Exit codes (126, 127), invalid paths, option conflicts
│   ├── test_capabilities.sh     # /proc/self/status capability verification
│   └── test_namespaces.sh      # CLONE_NEWNET network namespace decoupling tests
├── benchmarks/
│   ├── bench_timer.c            # Precision C benchmark timer (nanosecond resolution, P95/P99)
│   └── bench_startup.sh         # Benchmark script orchestrating timer with warm-up
└── examples/
    └── demo.sh                  # Interactive end-to-end feature demonstration
```

---

## 8. Academic Review Details

- **Course:** BCSE303P — Operating Systems Lab
- **Institution:** Vellore Institute of Technology (VIT), Vellore
- **Semester:** Fall Semester 2026–2027
- **Project Title:** Zero-Trust Ephemeral Sandbox
- **Team:** Mohammad Ammar Abdullah (24BYB0017), Siddhesh Srinivasan (24BYB0035), Aditi Soni (24BDE0144)
- **Faculty Guide:** Dr. PORKODI S (Assistant Professor Senior Grade – 1, SCSEIS)
