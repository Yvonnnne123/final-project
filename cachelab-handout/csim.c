/*
 * csim.c - Cache Simulator
 * 
 * Group Members:
 * Qihan Zhang - qz192
 * 
 * 
 * This cache simulator takes a valgrind memory trace as input,
 * simulates the hit/miss behavior of a cache memory on this trace,
 * and outputs the total number of hits, misses, and evictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

int s = 0;
int E = 0;
int b = 0;
int S = 0;
int B = 0;
int verbosity = 0;
char *trace_file = NULL;
unsigned long set_index_mask = 0;
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
unsigned long lru_counter = 0;

typedef struct {
    unsigned char valid;
    unsigned long tag;
    unsigned long lru_counter;
} cache_line_t;

cache_line_t **cache = NULL;

void initCache(void);
void freeCache(void);
void accessData(unsigned long address);
void replayTrace(char *filename);
void printSummary(int hits, int misses, int evictions);
void printUsage(char *argv[]);

int main(int argc, char *argv[]) {
    int opt;
    
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printUsage(argv);
                exit(0);
                break;
            case 'v':
                verbosity = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                printUsage(argv);
                exit(1);
        }
    }
    
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
        printUsage(argv);
        exit(1);
    }
    
    S = 1 << s;
    B = 1 << b;
    
    initCache();
    
    replayTrace(trace_file);
    
    freeCache();
    
    printSummary(hit_count, miss_count, eviction_count);
    
    return 0;
}

void initCache(void) {
    int i, j;
    
    cache = malloc(S * sizeof(cache_line_t*));
    if (cache == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    
    for (i = 0; i < S; i++) {
        cache[i] = malloc(E * sizeof(cache_line_t));
        if (cache[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            exit(1);
        }
        
        for (j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru_counter = 0;
        }
    }
    
    set_index_mask = (1 << s) - 1;
}

void freeCache(void) {
    int i;
    
    for (i = 0; i < S; i++) {
        free(cache[i]);
    }
    
    free(cache);
}

void accessData(unsigned long address) {
    int i;
    unsigned long set_index, tag;
    cache_line_t *set;
    int lru_line = 0;
    unsigned long oldest_lru = (unsigned long)-1;
    
    set_index = (address >> b) & set_index_mask;
    
    tag = address >> (s + b);
    
    set = cache[set_index];
    
    for (i = 0; i < E; i++) {
        if (set[i].tag == tag && set[i].valid) {
            hit_count++;
            
            if (verbosity) {
                printf(" hit");
            }
            
            unsigned long old_lru = lru_counter;
            lru_counter++;
            set[i].lru_counter = old_lru;
            
            return;
        }
    }
    
    miss_count++;
    
    if (verbosity) {
        printf(" miss");
    }
    
    for (i = 0; i < E; i++) {
        if (!set[i].valid) {
            lru_line = i;
            break;
        }
        if (oldest_lru > set[i].lru_counter) {
            oldest_lru = set[i].lru_counter;
            lru_line = i;
        }
    }
    
    if (set[lru_line].valid) {
        eviction_count++;
        
        if (verbosity) {
            printf(" eviction");
        }
    }
    
    set[lru_line].valid = 1;
    set[lru_line].tag = tag;
    
    unsigned long old_lru = lru_counter;
    lru_counter++;
    set[lru_line].lru_counter = old_lru;
}

void replayTrace(char *filename) {
    FILE *fp;
    char line[1000];
    char operation;
    unsigned long address;
    int size;
    
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open trace file %s\n", filename);
        exit(1);
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) <= 1) continue;
        
        if (line[0] == 'I') continue;
        
        if (sscanf(line, " %c %lx,%d", &operation, &address, &size) == 3) {
            if (verbosity) {
                printf("%c %lx,%d", operation, address, size);
            }
            
            accessData(address);
            
            if (operation == 'M') {
                accessData(address);
            }
            
            if (verbosity) {
                printf("\n");
            }
        }
    }
    
    fclose(fp);
}

void printSummary(int hits, int misses, int evictions) {
    FILE *fp;
    
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    
    fp = fopen(".csim_results", "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open results file\n");
        exit(1);
    }
    
    fprintf(fp, "%d %d %d\n", hits, misses, evictions);
    
    fclose(fp);
}

void printUsage(char *argv[]) {
    printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message\n");
    printf("  -v         Optional verbose flag that shows trace info\n");
    printf("  -s <s>     Number of set index bits (S = 2^s is the number of sets)\n");
    printf("  -E <E>     Associativity (number of lines per set)\n");
    printf("  -b <b>     Number of block bits (B = 2^b is the block size)\n");
    printf("  -t <tracefile>  Name of the valgrind trace to replay\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
} 