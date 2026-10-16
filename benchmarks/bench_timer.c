#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <math.h>

#define DEFAULT_ITERATIONS 1000
#define WARMUP_ITERATIONS 50

/* Structure to store benchmark summary statistics */
struct bench_stats {
    double min_us;
    double max_us;
    double mean_us;
    double median_us;
    double p95_us;
    double p99_us;
    double stddev_us;
    size_t failures;
};

static int compare_double(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int run_command_timed(char *const argv[], double *duration_us) {
    uint64_t start = get_time_ns();

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        /* Child process: silence output during benchmark */
        fclose(stdout);
        fclose(stderr);
        execvp(argv[0], argv);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }

    uint64_t end = get_time_ns();
    *duration_us = (double)(end - start) / 1000.0;

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }
    return -1;
}

static int compute_stats(double *samples, size_t count, size_t failures, struct bench_stats *stats) {
    if (count == 0) return -1;

    qsort(samples, count, sizeof(double), compare_double);

    stats->min_us = samples[0];
    stats->max_us = samples[count - 1];
    stats->median_us = samples[count / 2];
    stats->p95_us = samples[(size_t)(count * 0.95)];
    stats->p99_us = samples[(size_t)(count * 0.99)];
    stats->failures = failures;

    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += samples[i];
    }
    stats->mean_us = sum / count;

    double variance_sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = samples[i] - stats->mean_us;
        variance_sum += diff * diff;
    }
    stats->stddev_us = sqrt(variance_sum / count);

    return 0;
}

int main(int argc, char *argv[]) {
    size_t iterations = DEFAULT_ITERATIONS;
    if (argc > 1) {
        int parsed = atoi(argv[1]);
        if (parsed > 0) {
            iterations = (size_t)parsed;
        }
    }

    printf("\033[1;36m====================================================\033[0m\n");
    printf("\033[1;36m  Zero-Trust Ephemeral Sandbox - Precision Benchmark\033[0m\n");
    printf("\033[1;36m====================================================\033[0m\n\n");

    /* Environment information */
    struct utsname un;
    if (uname(&un) == 0) {
        printf("Environment:\n");
        printf("  - OS:       %s %s\n", un.sysname, un.release);
        printf("  - Machine:  %s\n", un.machine);
    }
    printf("  - Samples:  %zu iterations (with %d warm-up runs)\n", iterations, WARMUP_ITERATIONS);
    printf("  - Monotonic clock: CLOCK_MONOTONIC (nanosecond resolution)\n\n");

    char *const native_cmd[] = {"/bin/true", NULL};
    char *const sandbox_cmd[] = {"./bin/sandbox", "--", "/bin/true", NULL};

    /* Warm-up phase */
    printf("Warming up page cache and execution paths (%d iterations)...\n", WARMUP_ITERATIONS);
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        double d;
        run_command_timed(native_cmd, &d);
        run_command_timed(sandbox_cmd, &d);
    }

    double *native_samples = malloc(iterations * sizeof(double));
    double *sandbox_samples = malloc(iterations * sizeof(double));
    if (!native_samples || !sandbox_samples) {
        fprintf(stderr, "Failed to allocate sample buffers\n");
        free(native_samples);
        free(sandbox_samples);
        return 1;
    }

    /* Benchmark Native Execution */
    printf("Measuring baseline native execution (/bin/true)...\n");
    size_t native_failures = 0;
    for (size_t i = 0; i < iterations; i++) {
        if (run_command_timed(native_cmd, &native_samples[i]) != 0) {
            native_failures++;
        }
    }

    /* Benchmark Sandboxed Execution */
    printf("Measuring sandboxed execution (./bin/sandbox -- /bin/true)...\n");
    size_t sandbox_failures = 0;
    for (size_t i = 0; i < iterations; i++) {
        if (run_command_timed(sandbox_cmd, &sandbox_samples[i]) != 0) {
            sandbox_failures++;
        }
    }

    struct bench_stats native_stats, sandbox_stats;
    compute_stats(native_samples, iterations, native_failures, &native_stats);
    compute_stats(sandbox_samples, iterations, sandbox_failures, &sandbox_stats);

    printf("\n\033[1mBenchmark Results (Latency in Microseconds / µs):\033[0m\n");
    printf("+--------------------------------+-----------------+-----------------+\n");
    printf("| Metric                         | Native Baseline | Sandbox Jail    |\n");
    printf("+--------------------------------+-----------------+-----------------+\n");
    printf("| Samples / Total Iterations     | %15zu | %15zu |\n", iterations, iterations);
    printf("| Failed Invocations             | %15zu | %15zu |\n", native_stats.failures, sandbox_stats.failures);
    printf("| Minimum Latency (Min)          | %12.2f µs | %12.2f µs |\n", native_stats.min_us, sandbox_stats.min_us);
    printf("| Median Latency (P50)           | %12.2f µs | %12.2f µs |\n", native_stats.median_us, sandbox_stats.median_us);
    printf("| Arithmetic Mean                | %12.2f µs | %12.2f µs |\n", native_stats.mean_us, sandbox_stats.mean_us);
    printf("| 95th Percentile (P95)          | %12.2f µs | %12.2f µs |\n", native_stats.p95_us, sandbox_stats.p95_us);
    printf("| 99th Percentile (P99)          | %12.2f µs | %12.2f µs |\n", native_stats.p99_us, sandbox_stats.p99_us);
    printf("| Maximum Latency (Max)          | %12.2f µs | %12.2f µs |\n", native_stats.max_us, sandbox_stats.max_us);
    printf("| Standard Deviation             | %12.2f µs | %12.2f µs |\n", native_stats.stddev_us, sandbox_stats.stddev_us);
    printf("+--------------------------------+-----------------+-----------------+\n\n");

    double diff_mean = sandbox_stats.mean_us - native_stats.mean_us;
    if (diff_mean < 0) diff_mean = 0;
    double diff_median = sandbox_stats.median_us - native_stats.median_us;
    if (diff_median < 0) diff_median = 0;

    printf("\033[1mEstimated Sandbox Initialization Overhead:\033[0m\n");
    printf("  - Overhead (Mean):   \033[1;33m%.2f µs\033[0m (%.3f ms)\n", diff_mean, diff_mean / 1000.0);
    printf("  - Overhead (Median): \033[1;33m%.2f µs\033[0m (%.3f ms)\n\n", diff_median, diff_median / 1000.0);

    if (diff_median < 1000.0) {
        printf("\033[1;32m[VERIFIED] Sub-millisecond objective achieved: median initialization overhead < 1.0 ms (%.2f µs)\033[0m\n\n", diff_median);
    } else {
        printf("\033[1;33m[TARGET MISSED] Overhead exceeded 1.0 ms: median is %.3f ms\033[0m\n\n", diff_median / 1000.0);
    }

    printf("Methodological Note:\n");
    printf("  End-to-end launch latency includes fork() + execve(sandbox) + sandbox barrier setup\n");
    printf("  (Landlock + Seccomp + capset) + execve(payload) + payload exit + waitpid().\n");
    printf("  The differential (Sandbox - Native) isolates the true security setup overhead.\n\n");

    free(native_samples);
    free(sandbox_samples);
    return 0;
}
