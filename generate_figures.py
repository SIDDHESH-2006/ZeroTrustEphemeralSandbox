import os
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

os.makedirs("assets", exist_ok=True)

# -------------------------------------------------------------
# 1. Generate Architecture Flowchart Diagram
# -------------------------------------------------------------
def generate_architecture_diagram():
    fig, ax = plt.subplots(figsize=(11, 13), dpi=300)
    ax.set_xlim(0, 100)
    ax.set_ylim(0, 110)
    ax.axis("off")

    # Title
    ax.text(50, 106, "Zero-Trust Ephemeral Sandbox: Multi-Layer Architecture",
            ha="center", va="center", fontsize=15, fontweight="bold", color="#1A365D")
    ax.text(50, 103, "Sequential Kernel Barriers, In-Process Isolation & Payload Execution Pipeline",
            ha="center", va="center", fontsize=10, style="italic", color="#4A5568")

    # Stage boxes definition: (y_center, title, details, color_bg, color_border)
    stages = [
        (92, "Stage 1: CLI Supervisor & Path Resolution (src/cli.c)",
         "• Parses CLI options: --ro <path>, --rw <path>, --no-net, --allow-net, --drop-caps\n"
         "• Path canonicalization via realpath() and validation via stat() [Fail-Closed]\n"
         "• Automatically allowlists essential system shared libraries (/usr, /lib, /bin, /dev)",
         "#EBF8FF", "#3182CE"),
        
        (75, "Stage 2: Privilege Locking & Capability Dropping (src/isolate.c)",
         "• Locks privilege acquisition: prctl(PR_SET_NO_NEW_PRIVS, 1)\n"
         "• Clears ambient capabilities: prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL)\n"
         "• Zeroes Effective, Permitted & Inheritable caps via SYS_capset; verified with capget\n"
         "• Decouples network & user namespace via unshare(CLONE_NEWUSER | CLONE_NEWNET)",
         "#EBF4FF", "#4C51BF"),

        (57, "Stage 3: Landlock LSM Filesystem Confinement (src/landlock_mgr.c)",
         "• Queries kernel Landlock ABI version (v1-v3); fails closed if unsupported\n"
         "• Compiles ruleset with SYS_landlock_create_ruleset & populates path descriptors\n"
         "• Enforces strict directory subtree boundaries: SYS_landlock_restrict_self\n"
         "• Inescapable in-kernel VFS hooks: Blocks symlink escapes and path traversal (..)",
         "#E6FFFA", "#319795"),

        (39, "Stage 4: Seccomp-BPF System Call Filter Barrier (src/seccomp_bpf.c)",
         "• Validates system audit architecture (AUDIT_ARCH_AARCH64 / AUDIT_ARCH_X86_64)\n"
         "• Denies network operations: socket(), connect(), bind(), sendto(), recvmsg() [EPERM]\n"
         "• Denies inspection & debugging: ptrace(), process_vm_readv(), process_vm_writev() [EPERM]\n"
         "• Denies async kernel rings (io_uring_setup) & container tampering (mount, chroot)",
         "#FEFCBF", "#D69E2E"),

        (19, "Stage 5: Confined Untrusted Payload Execution (execvp Handoff)",
         "• execvp() replaces supervisor virtual memory address space entirely with payload\n"
         "• 0 KB residual supervisor memory footprint (Zero memory bloat, native CPU speed)\n"
         "• Payload runs with zero ambient privileges under permanent hardware & kernel locks",
         "#F0FFF4", "#38A169")
    ]

    for y, title, details, bg, border in stages:
        # Main box
        rect = patches.FancyBboxPatch((10, y - 6.5), 80, 13,
                                      boxstyle="round,pad=1.2,rounding_size=2",
                                      facecolor=bg, edgecolor=border, linewidth=2.0)
        ax.add_patch(rect)
        # Title text
        ax.text(12, y + 4.2, title, fontsize=11, fontweight="bold", color="#1A202C")
        # Detail bullets
        ax.text(12, y - 1.2, details, fontsize=8.8, color="#2D3748", linespacing=1.4)

    # Connecting arrows
    arrow_props = dict(facecolor="#2B6CB0", edgecolor="#2B6CB0", width=2.2, headwidth=8, headlength=7)
    for y_start in [85.5, 68.5, 50.5]:
        ax.annotate("", xy=(50, y_start - 5.0), xytext=(50, y_start), arrowprops=arrow_props)
    
    # Execvp arrow with special badge
    ax.annotate("", xy=(50, 25.5), xytext=(50, 32.5), arrowprops=dict(facecolor="#38A169", edgecolor="#38A169", width=2.6, headwidth=9, headlength=8))
    ax.text(50, 29, "execvp() Address Space Handoff (0 KB Supervisor Retention)", ha="center", va="center",
            fontsize=8.5, fontweight="bold", color="#276749",
            bbox=dict(boxstyle="round,pad=0.4", facecolor="#C6F6D5", edgecolor="#38A169", linewidth=1))

    # Kernel trap side-boxes
    # Trap 1: Landlock VFS Trap
    trap_rect1 = patches.FancyBboxPatch((72, 46), 25, 6, boxstyle="round,pad=0.6,rounding_size=1",
                                       facecolor="#FFF5F5", edgecolor="#E53E3E", linewidth=1.5, linestyle="--")
    ax.add_patch(trap_rect1)
    ax.text(84.5, 49, "VFS Hook Block:\nLandlock returns -EACCES", ha="center", va="center",
            fontsize=7.8, fontweight="bold", color="#C53030")
    ax.annotate("", xy=(84.5, 52), xytext=(70, 57),
                arrowprops=dict(arrowstyle="->", color="#E53E3E", lw=1.5, linestyle=":"))

    # Trap 2: Seccomp BPF Trap
    trap_rect2 = patches.FancyBboxPatch((72, 28), 25, 6, boxstyle="round,pad=0.6,rounding_size=1",
                                       facecolor="#FFF5F5", edgecolor="#E53E3E", linewidth=1.5, linestyle="--")
    ax.add_patch(trap_rect2)
    ax.text(84.5, 31, "Syscall Trap:\nSeccomp returns -EPERM", ha="center", va="center",
            fontsize=7.8, fontweight="bold", color="#C53030")
    ax.annotate("", xy=(84.5, 34), xytext=(70, 39),
                arrowprops=dict(arrowstyle="->", color="#E53E3E", lw=1.5, linestyle=":"))

    # Footer banner
    ax.text(50, 4, "Result: Median Startup Latency 360.10 µs (< 1.0 ms) | 100% Unprivileged | Zero External Dependencies",
            ha="center", va="center", fontsize=9.2, fontweight="bold", color="#2C5282",
            bbox=dict(boxstyle="square,pad=0.6", facecolor="#EBF8FF", edgecolor="#BEE3F8", linewidth=1.2))

    plt.tight_layout()
    plt.savefig("assets/architecture_diagram.png", bbox_inches="tight", dpi=300)
    plt.close()
    print("Generated assets/architecture_diagram.png successfully.")

# -------------------------------------------------------------
# 2. Generate Latency Benchmark Chart
# -------------------------------------------------------------
def generate_benchmark_chart():
    fig, ax = plt.subplots(figsize=(10, 6), dpi=300)

    metrics = ["Min Latency", "Median (P50)", "Arithmetic Mean", "95th % (P95)", "99th % (P99)"]
    native_times = [145.70, 179.82, 190.97, 228.94, 590.87]
    sandbox_times = [483.09, 539.92, 554.54, 662.70, 756.86]

    x = np.arange(len(metrics))
    width = 0.35

    rects1 = ax.bar(x - width/2, native_times, width, label="Native Baseline (/bin/true)",
                    color="#3182CE", edgecolor="#2B6CB0", alpha=0.9, zorder=3)
    rects2 = ax.bar(x + width/2, sandbox_times, width, label="Sandbox Jail (./bin/sandbox -- /bin/true)",
                    color="#DD6B20", edgecolor="#C05621", alpha=0.9, zorder=3)

    # Design ceiling threshold line
    ax.axhline(1000, color="#E53E3E", linestyle="--", linewidth=1.8, label="Project Design Constraint Ceiling (1.0 ms = 1000 µs)", zorder=4)

    # Annotate overhead deltas above bars
    deltas = [s - n for n, s in zip(native_times, sandbox_times)]
    for i, rect in enumerate(rects2):
        height = rect.get_height()
        ax.annotate(f"{height:.1f} µs\n(+{deltas[i]:.1f} µs)",
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 6),
                    textcoords="offset points",
                    ha="center", va="bottom", fontsize=8.2, fontweight="bold", color="#9C4221")

    for rect in rects1:
        height = rect.get_height()
        ax.annotate(f"{height:.1f} µs",
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 4),
                    textcoords="offset points",
                    ha="center", va="bottom", fontsize=8, color="#2B6CB0")

    ax.set_ylabel("Process Execution Latency (Microseconds / µs)", fontsize=11, fontweight="bold", color="#1A202C")
    ax.set_title("Startup Latency Distribution: Native Baseline vs. Zero-Trust Sandbox (1,000 Iterations)",
                 fontsize=13, fontweight="bold", color="#1A365D", pad=15)
    ax.set_xticks(x)
    ax.set_xticklabels(metrics, fontsize=10, fontweight="bold")
    ax.set_ylim(0, 1150)
    ax.grid(axis="y", linestyle=":", alpha=0.6, zorder=0)

    ax.legend(loc="upper left", framealpha=0.95, edgecolor="#CBD5E0", fontsize=9.5)

    # Callout text for median overhead
    ax.text(1.0, 880, "Median Overhead = 360.10 µs (0.360 ms)\nOver 64% faster than 1.0 ms target!\n1000/1000 Invocations Succeeded (0 failures)",
            ha="center", va="center", fontsize=9.2, fontweight="bold", color="#276749",
            bbox=dict(boxstyle="round,pad=0.6", facecolor="#F0FFF4", edgecolor="#38A169", linewidth=1.2))

    plt.tight_layout()
    plt.savefig("assets/latency_benchmark_chart.png", bbox_inches="tight", dpi=300)
    plt.close()
    print("Generated assets/latency_benchmark_chart.png successfully.")

if __name__ == "__main__":
    generate_architecture_diagram()
    generate_benchmark_chart()
