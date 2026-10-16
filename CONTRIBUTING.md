# Developer & Contributing Guide

Welcome to the **Zero-Trust Ephemeral Sandbox** repository. This document provides technical instructions for extending the codebase, adding security restrictions, writing regression tests, and executing benchmarks.

---

## 1. Repository Layout

```
├── Makefile            # POSIX build system with strict flags (-Wall -Wextra -Werror -pedantic)
├── include/            # Internal header files (API contracts, structures, logging macros)
├── src/                # Implementation sources (C11, native Linux syscalls)
├── tests/              # Automated bash and python test suites (9 suites)
├── benchmarks/         # Performance benchmarking tools (C timer and driver)
└── examples/           # End-to-end demonstrations (demo.sh)
```

---

## 2. Supported Environment & Prerequisites

- **Host OS:** Linux with Kernel ≥ 5.13 (Landlock LSM enabled).
- **Toolchain:** GCC or Clang supporting C11, GNU Make, Bash 4+.
- **Zero Third-Party Dependencies:** Do not add dependencies on external libraries (such as `libseccomp`, `libcap`, or GLib). The sandbox is designed to link solely against standard `libc` and use raw kernel interfaces (`syscall()`, `prctl()`).
- **macOS / Windows:** macOS is not supported because Linux-specific kernel interfaces (`linux/landlock.h`, `linux/seccomp.h`, `linux/filter.h`, `unshare()`) do not exist on Darwin.

---

## 3. Build & Test Workflow

Compile with strict compiler flags:
```bash
make clean && make
```

Execute all test suites:
```bash
make test
```

Execute the latency benchmark:
```bash
make bench
```

Run the interactive feature demonstration:
```bash
./examples/demo.sh
```

---

## 4. How to Add a Syscall Restriction in Seccomp-BPF

System call filtering is located in `src/seccomp_bpf.c`.

1. **Verify the Syscall Across Architectures:**
   Ensure the system call number is defined across `x86_64`, `aarch64`, `arm`, and `i386`. Guard it using `#ifdef SYS_<name>` or provide a fallback `#define`:
   ```c
   #ifndef SYS_example_syscall
   #ifdef __NR_example_syscall
   #define SYS_example_syscall __NR_example_syscall
   #endif
   #endif
   ```

2. **Add Denial to `seccomp_apply_filter()`:**
   Use the `DENY_SYSCALL` macro within the appropriate category block:
   ```c
   #ifdef SYS_example_syscall
   DENY_SYSCALL(SYS_example_syscall);
   #endif
   ```

3. **Update Documentation:**
   Update the comment in `include/seccomp_bpf.h` and the list in `README.md`.

4. **Write a Verification Test:**
   Add a test case in `tests/test_seccomp_hardened.sh` that attempts to issue the prohibited syscall and asserts that `EPERM` (Operation not permitted, `errno=1`) is returned.

---

## 5. How to Add a Regression Test

Regression tests live in `tests/`.

1. **Follow Repository Test Conventions:**
   - Use `set -euo pipefail` (or trap errors gracefully).
   - Use safe temporary directories in `/tmp/` and always register a cleanup handler via `trap cleanup EXIT`.
   - Never use real sensitive host files (like `~/.ssh` or `/etc/shadow`); create mock files in temporary directories.
   - Use standardized color indicators: `[PASS]`, `[FAIL]`, `[SKIP]`.
2. **Handle Optional Features Honestly:**
   - If a test tests an optional feature that is not available in the current environment (e.g. unprivileged user namespaces disabled), print `[SKIP]` and exit `0`.
   - Never forge terminal output or falsify results to make an unverified feature look passed.
3. **Integrate into `tests/run_all.sh`:**
   Add the new test script to the `TEST_SUITES` array in `tests/run_all.sh`.

---

## 6. How to Run and Interpret Benchmarks

The benchmark measures startup overhead using `benchmarks/bench_timer.c` via `make bench`:
- **Native Baseline:** Measures the time taken to `fork()`, `execve("/bin/true")`, and `waitpid()`.
- **Sandbox Jail:** Measures the time taken to `fork()`, `execve("./bin/sandbox", ["./bin/sandbox", "--", "/bin/true"])`, apply security barriers, and `waitpid()`.
- **Initialization Overhead:** The difference (`Sandbox Median - Native Median`).
- **Sub-Millisecond Target:** Target is `< 1,000 µs` (1 ms). If the overhead is lower than 1 ms, the target is achieved.

---

## 7. Coding Standards

- **Language:** ISO C11 with `-D_GNU_SOURCE`.
- **Formatting:** 4 spaces indentation, no tabs.
- **Fail-Safe Principle:** All security checks must fail closed. If Landlock cannot be applied or an explicit path rule cannot be resolved, the application must abort immediately and never execute the untrusted payload.
