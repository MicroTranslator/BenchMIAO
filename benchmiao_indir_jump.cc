#include <stdio.h>
#include <stdlib.h> /* atoll */
#include <string.h> /* strcmp */
#include <unistd.h> /* getopt */
#include <stdint.h>
#include <algorithm> /* random_shuffle */
#include <time.h> /* time */

asm (
    ".section .text\n\t"
    ".globl rets\n\t"
    "rets:\n\t"
    ".rept 1<<14\n\t"
    "inc %rax\n\t"
    "jmp ret_addr\n\t"
    ".endr\n\t"
);
extern void *rets;
void *prets = &rets;

void usage(void) {
    printf("Usage: indirect_jump_no_dup [-h] [-l <NLOOPS>] [-r <NRANDS>]\n");
    printf("  -h: print this help\n");
    printf("  -l <NLOOPS>: number of loops, default 10^6\n");
    printf("  -r <NRANDS>: number of random jumps, default 100, max 2^14(16K)\n");
    exit(1);
}
int main(int argc, char *argv[]) {
    uint64_t sum = 0;
    uint64_t nloops = 1000000;
    uint64_t nrands = 100;

    int opt;
    while ((opt = getopt(argc, argv, "hl:r:")) != -1) {
        switch (opt) {
        default:
        case 'h':
            usage();
        break;
        break;
        case 'l':
            nloops = strtoul(optarg, NULL, 0);
        break;
        case 'r':
            nrands = strtoul(optarg, NULL, 0);
        break;
        }
    }

    uint64_t randaddrs[1<<14];
    void *prandaddrs = &randaddrs;
    for (uint64_t i=0; i<(1<<14); i++) {
        randaddrs[i] = ((uint64_t)prets) + 8*i; // inc, jmp: 8 bytes
    }
    srand(time(NULL));
    std::random_shuffle(randaddrs, randaddrs+(1<<14));
    /* for (uint64_t i=nrands; i>0; i--) { */
    /*     printf("%016lx\n", randaddrs[i]); */
    /* } */

    asm volatile (
        "xor %%rax, %%rax\n\t"

        "mov %1, %%rdx\n\t" // nloops
        "outer_loop:\n\t"
        "dec %%rdx\n\t" // nloops
        "mov %3, %%rbx\n\t" // nrands
        "inner_loop:\n\t"
        "dec %%rbx\n\t" // nrands

        // prepare address
        "mov (%2, %%rbx, 8), %%rcx\n\t" // prandaddrs
        // indirect call
        "jmp *%%rcx\n\t"
        "ret_addr:\n\t"

        "test %%rbx, %%rbx\n\t" // nrands
        "jnz inner_loop\n\t"
        "test %%rdx, %%rdx\n\t" // nloops
        "jnz outer_loop\n\t"

        "mov %%rax, %0\n\t"
        : "=r"(sum)
        : "r"(nloops), "r"(prandaddrs), "r"(nrands)
        : "rax", "rbx", "rcx", "rdx"
    );
    printf("%s loop %ld sum %ld\n", argv[0], nloops*nrands, sum);

    return 0;
}
