/* ctuner.c - Cache Simulator Optimizer
 *
 * Group Members:
 * Luke Spivak - ljs302
 * Qihan Zhang - qz192
 * Stephen Eggly - sre71
 * Rajesh Sannidhi - drag00n
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

typedef struct {
    int s, E, b;
    int size; // total cache size in bytes
} config_t;

float compute_metric(char *metric, int hits, int misses, int evictions) {
    int total = hits + misses;
    if (total == 0) return 0.0;

    if (strcmp(metric, "h") == 0) return (float)hits / total * 100.0;
    if (strcmp(metric, "m") == 0) return (float)misses / total * 100.0;
    if (strcmp(metric, "e") == 0) return (float)evictions / total * 100.0;
    return 0.0;
}

static int compare_by_size(const void *a, const void *b) {
    const config_t *ca = (const config_t *)a;
    const config_t *cb = (const config_t *)b;
    return ca->size - cb->size;
}

void run_simulator(char *csim_binary, int s, int E, int b, char *tracefile, int *hits, int *misses, int *evictions) {
    char command[256];
    snprintf(command, sizeof(command), "%s -s %d -E %d -b %d -t %s", 
            csim_binary, s, E, b, tracefile);
    FILE *fp = popen(command, "r");
    int h, m, e;
    if (fscanf(fp, "hits:%d misses:%d evictions:%d", &h, &m, &e) != 3) {
        fprintf(stderr, "Failed to parse output of %s\n", csim_binary);
        pclose(fp); exit(EXIT_FAILURE);
    }
    pclose(fp);
    *hits = h; *misses = m; *evictions = e;
}

void optimize_cache_config(char *csim_binary, char *tracefile, char *metric, float target_rate, int *best_s, int *best_E, int *best_b, float *best_metric) {
    // Build and sort config size list
    config_t configs[5 * 4 * 5];
    int n_cfg = 0;
    for (int s = 1; s <= 5; s++) 
        for (int E = 1; E <= 4; E++)
            for (int b = 1; b <= 5; b++) {
                configs[n_cfg].s = s;
                configs[n_cfg].E = E;
                configs[n_cfg].b = b;
                configs[n_cfg].size = (1 << s) * E * (1 << b);
                n_cfg++;
            }
    qsort(configs, n_cfg, sizeof(config_t), compare_by_size);

    // Check configs in ascending order of size (Brute force to find the smallest optimal one)
    int config_found = 0;
    for (int i = 0; i < n_cfg; i++) {
        int s = configs[i].s;
        int E = configs[i].E;
        int b = configs[i].b;

        int h, m, e;
        run_simulator(csim_binary, s, E, b, tracefile, &h, &m, &e);
        float cur = compute_metric(metric, h, m, e);

        if (strcmp(metric, "h") == 0) config_found = cur >= target_rate;
        else config_found = cur <= target_rate;

        if (config_found) {
            *best_s = s;
            *best_E = E;
            *best_b = b;
            *best_metric = cur;
            config_found = 1;
            break;
        }
    }

    if (!config_found) {
        printf("No valid configuration found\n");
        *best_s = *best_E = *best_b = -1;
    } else {
        printf("%s -s %d -E %d -b %d -t %s\n",
               csim_binary, *best_s, *best_E, *best_b, tracefile);
    }
}

int main(int argc, char **argv) {
    char *metric = NULL;
    float target_rate = 0.0;
    char *csim_binary = NULL;
    char *tracefile = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "p:r:b:t:")) != -1) {
        switch (opt) {
            case 'p':
                metric = optarg;
                break;
            case 'r':
                target_rate = atof(optarg);
                break;
            case 'b':
                csim_binary = optarg;
                break;
            case 't':
                tracefile = optarg;
                break;
        }
    }

    // Check for missing arguments 
    if (!metric || !csim_binary || !tracefile || target_rate < 0.0 || target_rate > 100.0) {
        fprintf(stderr, "Invalid arguments\n");
    }

    int best_s; // Number of set index bits (log2 of number of sets)
    int best_E; // Associativity (lines per set)
    int best_b; // Number of block offset bits (log2 of block size)
    float best_metric;
    optimize_cache_config(csim_binary, tracefile, metric, target_rate, &best_s, &best_E, &best_b, &best_metric);

    return 0;
}
