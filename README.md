# Zero-Trust Ephemeral Sandbox (Landlock + Seccomp)

## 1. Abstract

Modern software development frequently involves executing untrusted code, such as third-party package manager hooks (`npm install`, `pip setup.py`), AI-generated scripts, or dynamic binaries. Traditional isolation methods like virtual machines or containerization engines (e.g., Docker, Podman) incur significant startup latency, excessive resource overhead, and often require elevated root privileges, rendering them unsuitable for ephemeral, high-frequency command execution.

This project implements a **sub-millisecond, zero-dependency process sandbox engineered entirely in C**. By leveraging modern unprivileged Linux security primitives—specifically, the **Landlock Linux Security Module (LSM)** and **Seccomp-BPF (Secure Computing with Berkeley Packet Filter)**—the system enforces strict runtime containment without requiring `sudo`. The architecture intercepts and filters system calls to deny unauthorized operations while simultaneously restricting filesystem access to explicitly allowlisted paths. This zero-trust environment drops all inherited Linux capabilities before invoking the target payload, ensuring that even compromised or malicious scripts cannot escalate privileges, read sensitive SSH keys, or establish unauthorized network connections. The resulting ephemeral sandbox provides near-native execution performance, presenting a lightweight, highly secure alternative to traditional heavy virtualization.

---

## 2. Keywords
`Zero-Trust`, `Process Sandboxing`, `Landlock LSM`, `Seccomp-BPF`, `Unprivileged Execution`, `Linux Security Primitives`

---

## 3. Architecture Overview

```
+-------------------------------------------------------------------------+
| Sandbox Wrapper (User Space - C Supervisor)                             |
|                                                                         |
|  1. CLI Supervisor                                                      |
|     - Parses flags: --ro, --rw, --no-net, --drop-caps, -- <payload>     |
|                                                                         |
|  2. Isolation Setup Engine                                              |
|     - Sets prctl(PR_SET_NO_NEW_PRIVS, 1)                                |
|     - Clears ambient & inheritable capabilities via capset()            |
|     - Optionally decouples namespaces via unshare()                     |
|                                                                         |
|  3. Landlock LSM Enforcement Barrier                                    |
|     - Detects Landlock ABI version (v1-v3)                              |
|     - landlock_create_ruleset()                                         |
|     - Populates rules for RO / RW paths via landlock_add_rule()         |
|     - Enforces filesystem jail via landlock_restrict_self()             |
|                                                                         |
|  4. Seccomp-BPF Syscall Filter Injection                                |
|     - Classic BPF bytecode assembled directly (zero external libs)      |
|     - Denies socket(), socketpair(), connect(), bind(), listen()        |
|     - Denies ptrace(), chroot(), pivot_root(), kexec_load()             |
|     - Injects filter via seccomp(SECCOMP_SET_MODE_FILTER, ...)          |
|                                                                         |
|  5. Execution Handoff                                                   |
|     - Dispatches payload via execvp(cmd, args)                          |
|     - Replaces memory image: zero parent overhead, inescapable jail     |
+-------------------------------------------------------------------------+
                                    |
                                    v (execvp)
+-------------------------------------------------------------------------+
| Target Payload (Untrusted Binary / Script)                             |
| Executing within Kernel-Enforced Boundaries                             |
+-------------------------------------------------------------------------+
```

---

## 4. Key Differentiators

| Feature | Docker / Podman | chroot | **Zero-Trust Sandbox (This Project)** |
|---|---|---|---|
| **Root Privileges Required** | Yes (Daemon or rootless setup) | Yes (`CAP_SYS_CHROOT`) | **No (100% Unprivileged)** |
| **Startup Overhead** | ~500ms – 2000ms | ~5ms – 20ms | **< 1ms (Sub-millisecond)** |
| **External Dependencies** | Container runtime, storage driver | Host rootfs preparation | **Zero Dependencies (Pure C)** |
| **Filesystem Isolation** | Mount namespaces & overlays | Inode jail (Trivially escapable) | **Kernel Landlock LSM (Inescapable)** |
| **Syscall Filtering** | Default Seccomp profiles | None | **Custom Seccomp-BPF (EPERM)** |
| **Use Case** | Long-running microservices | System rescue / bootstrapping | **Ephemeral commands, scripts, builds** |

---

## 5. Directory Structure

```
.
├── Makefile                # POSIX build system with test & bench targets
├── README.md               # Project documentation and review details
├── .gitignore              # Ignores build artifacts and test logs
├── include/
│   ├── sandbox.h           # Core data structures and ANSI logging macros
│   ├── cli.h               # Command-line parser interfaces
│   ├── isolate.h           # Capabilities dropping and namespace decoupling
│   ├── landlock_mgr.h      # Landlock LSM ruleset creation and ABI handlers
│   └── seccomp_bpf.h       # Raw classic BPF bytecode generator
├── src/
│   ├── main.c              # Entry point and supervisor execution lifecycle
│   ├── cli.c               # Argument parser and default system library paths
│   ├── isolate.c           # Capset and unshare implementations
│   ├── landlock_mgr.c      # Pure C Landlock syscall wrappers (ABI v1-v3)
│   └── seccomp_bpf.c       # Pure C BPF assembly for syscall denial
├── tests/
│   ├── run_all.sh          # Master test suite runner with colorized output
│   ├── test_cli.sh         # CLI argument validation tests
│   ├── test_fs.sh          # Landlock filesystem containment tests
│   ├── test_net.sh         # Seccomp network socket denial tests
│   └── test_ptrace.sh      # Seccomp ptrace denial tests
└── benchmarks/
    └── bench_startup.sh    # Microsecond-precision startup latency benchmark
```

---

## 6. Building and Installation

### Prerequisites
- Linux Kernel 5.13+ with Landlock LSM enabled (standard on Ubuntu 22.04+, Debian 12+, WSL2 with Kernel 5.15+).
- GCC or Clang with C11 support.
- GNU Make.
- No external development libraries required (`libseccomp` or `libcap` are **not** needed).

### Compile
```bash
make clean && make
```
This outputs the compiled binary to `bin/sandbox`.

---

## 7. How the Build System (Makefile) Works

The project uses a standard POSIX `Makefile` as its central build and test automation orchestrator.

### Compiler Configuration & Flags
- `CC ?= gcc`: Defaults to GNU C Compiler, while allowing overrides (`make CC=clang`).
- `CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude`:
  - `-Wall -Wextra -Werror`: Zero tolerance for warnings—every compiler warning is treated as a fatal error.
  - `-pedantic -std=c11`: Strict adherence to the standard ISO C11 specification.
  - `-D_GNU_SOURCE`: Exposes modern Linux system calls and glibc extensions.
  - `-O2`: Enables level 2 compiler optimization to ensure **sub-millisecond** execution speed.
  - `-Iinclude`: Points the preprocessor to the header directory (`include/`).

### Incremental Compilation & Pattern Rules
- `build/%.o: src/%.c | build`: Pattern rule that compiles each `.c` source file into an object file in `build/`.
- Make automatically tracks file modification timestamps. If only `src/cli.c` is edited, only `build/cli.o` is recompiled and re-linked, avoiding unnecessary recompilation of the whole codebase.
- Order-only prerequisites (`| bin` and `| build`) ensure output directories are automatically created (`mkdir -p`) before placing target files into them.

### Role of the Makefile in `make test`
```makefile
test: $(TARGET)
	@chmod +x tests/*.sh
	@./tests/run_all.sh
```
1. **Automatic Prerequisite Compilation (`$(TARGET)`)**: `test` explicitly depends on `bin/sandbox`. If the binary has not yet been built, or if any source file was modified, running `make test` automatically recompiles the binary before running any tests.
2. **Permission Assurance (`@chmod +x tests/*.sh`)**: Automatically grants executable permissions to all test scripts, preventing permission denied errors.
3. **Test Orchestration**: Executes `tests/run_all.sh`, which sequentially runs all 4 automated security test suites (CLI, Landlock filesystem, Seccomp network, and Seccomp ptrace). If any assertion fails, `make` immediately halts with a non-zero exit code.

### Role of the Makefile in `make bench`
```makefile
bench: $(TARGET)
	@chmod +x benchmarks/*.sh
	@./benchmarks/bench_startup.sh
```
1. Guarantees that `bin/sandbox` is up to date and compiled with `-O2` optimizations before profiling.
2. Ensures execute permissions on `benchmarks/bench_startup.sh`.
3. Runs the microsecond-precision startup latency benchmark (100 iterations of `/bin/true` native vs sandboxed).

### Role of `.PHONY`
```makefile
.PHONY: all clean test bench install uninstall
```
Declares non-file targets. Without `.PHONY`, if a file or directory named `test` or `bench` were created in the project root, Make would mistake it for the target and print `"test is up to date"`. `.PHONY` ensures that `make test`, `make bench`, and `make clean` always execute their recipes unconditionally.

---

## 8. Command-Line Reference

```
Usage: ./bin/sandbox [options] -- <command> [args...]

Options:
  -r, --ro <path>          Allow read-only access to <path> (repeatable)
  -w, --rw <path>          Allow read-write access to <path> (repeatable)
  -n, --no-net             Deny all network socket operations (default)
      --allow-net          Permit network socket syscalls
      --unshare-net        Unshare network and user namespace (CLONE_NEWNET)
      --drop-caps          Drop all ambient, permitted and inheritable capabilities
      --no-default-paths   Do not auto-allowlist system shared library paths (/usr, /lib, etc.)
  -v, --verbose            Enable verbose diagnostic output
  -h, --help               Display this help message and exit
      --version            Display project details and version
```

### Examples

1. **Execute an untrusted script with read-only `/usr` and read-write `./tmp`:**
   ```bash
   ./bin/sandbox --ro /usr --rw ./tmp -- /bin/bash untrusted.sh
   ```

2. **Execute Python code with network blocking:**
   ```bash
   ./bin/sandbox --rw . -- /usr/bin/python3 -c "import socket; socket.socket()"
   # Result: PermissionError: [Errno 1] Operation not permitted
   ```

3. **Allow network operations when required:**
   ```bash
   ./bin/sandbox --allow-net -- /usr/bin/curl https://example.com
   ```

---

## 9. Automated Verification & Testing

Run the full automated test suite:
```bash
make test
```
The test suite validates:
- **CLI Supervisor**: Syntax verification, `--help`, `--version`, argument error handling.
- **Landlock Filesystem**: Access allowed to designated directories, denied to sensitive unauthorized paths (`EACCES`), and write rejection on `--ro` paths.
- **Seccomp Network**: Socket creation (`socket()`) denied with `EPERM`, and `--allow-net` bypass.
- **Seccomp Ptrace**: Denies `ptrace()` execution to prevent debugger attachment and inspection.

---

## 10. Performance Benchmark

Measure startup latency overhead:
```bash
make bench
```
Executes 100 iterations of `/bin/true` natively vs inside `sandbox` to compute the kernel-level initialization cost.
- **Overhead**: `< 1.0 ms` (sub-millisecond latency).

---

## 11. References
1. Corbet, J. (2021). *Landlock comes to 5.13*. LWN.net.
2. Linux Kernel Documentation. *Landlock: unprivileged access control*.
3. Edge, J. (2015). *Seccomp BPF (SECure COMPuting with filters)*. LWN.net.
4. Kerrisk, M. (2010). *The Linux Programming Interface: A Linux and UNIX System Programming Handbook*. No Starch Press.
5. Linux man-pages: `landlock(7)`, `seccomp(2)`, `prctl(2)`, `capabilities(7)`.
