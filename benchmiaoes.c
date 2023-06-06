#define __GNU_SOURCE

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

asm (
    "___d:\n\t"
    ".8byte 0xabcedf0123456789\n\t"
);

#define PAGE_SIZE (1 << 20)

#define DUMMY_SIZE 16384
static inline double second(){
    struct timespec _t;
    clock_gettime(CLOCK_REALTIME, &_t);
    return _t.tv_sec + _t.tv_nsec/1.0e9;
}

static inline long long nano_second(){
    struct timespec _t;
    clock_gettime(CLOCK_REALTIME, &_t);
    return _t.tv_sec * 1000000000ll + _t.tv_nsec;
}

#define FIVE ONE ONE ONE ONE ONE
#define TEN FIVE FIVE
#define FIFITY TEN TEN TEN TEN TEN
#define HUNDRED FIFITY FIFITY
#define FHUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED
#define THOUSAND FHUNDRED FHUNDRED

#define FTHOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND
#define TENTHOUSAND FTHOUSAND FTHOUSAND

#ifndef __INNER_COUNT_NUM
#define __INNER_COUNT_NUM               FIFITY
#endif

#ifndef __INNER_COUNT
#define __INNER_COUNT                      50ULL
#endif

// How do I turn a macro into a string using cpp?
// https://stackoverflow.com/questions/6852920/
#define _STR(str) #str
#define STRING(str) _STR(str)

// #ifndef __LOOP_COUNT
// #define __LOOP_COUNT 2000000000ULL
// #endif

static size_t __LOOP_COUNT = 20000000;


uint8_t dummy[DUMMY_SIZE] __attribute__ ((aligned (16384)));

size_t self_pointer = (size_t)&self_pointer;
size_t self_pointer2 = (size_t)&self_pointer;

// 1 freq
uint64_t __attribute__ ((noinline)) freq(size_t cnt){
    uint64_t res;
#undef ONE
#define ONE "add %2, %2\n\t"
    asm volatile(
        ".align 256 \n\t"
        "1: \n\t"
        __INNER_COUNT_NUM
        "dec %1 \n\t"
        "jne 1b \n\t"
        "movq %2, %0\n\t"
        :"=r"(res)
        :"r"(cnt), "r"(self_pointer)
        :"memory");
    return res;
}

uint64_t freq_test() {
    freq(10000);
    double begin = second();
    uint64_t res = freq(__LOOP_COUNT);
    double end = second();
    double f = (__LOOP_COUNT * __INNER_COUNT) / 1000000000.0 / (end - begin);
    printf("frequency:%f GHz, one cycle:%f ns\n", f, 1/f);
    return res;
}

// 2 bcc
uint64_t bcc(void){
    uint64_t res;
#undef ONE
#if defined(__x86_64__)
    asm volatile(
        "1: \n\t"
        "add %2, %2\n\t"
        "sub $1, %1 \n\t"
        "jnz 1b \n\t"
        "mov %2, %0\n\t"
        : "=r"(res)
        : "r"(__INNER_COUNT * __LOOP_COUNT), "r"(self_pointer)
        :   "rax",
            "memory");
#endif
    return res;
}

uint64_t bcc_test(void) {
    double begin = second();
    uint64_t res = bcc();
    double end = second();
    printf("simple add/bcc:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 3 jr
uint64_t jr(void){
    uint64_t res;
#undef ONE
#define ONE "0: \n\t" \
            "add %2, %2\n\t" \
            "lea 0f(%%rip), %%rax\n\t" \
            "jmp *%%rax\n\t"

    asm volatile(
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %1 \n\t"
        "jnz 1b \n\t"
        "mov %2, %0\n\t"
        : "=r"(res)
        : "r"(__LOOP_COUNT), "r"(self_pointer)
        :   "rax",
            "memory");
    return res;
}

uint64_t jr_test(void) {
    double begin = second();
    uint64_t res = jr();
    double end = second();
    printf("simple jr:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}


// 4 call_ret
asm (
    "foo_ret: \n\t"
    "add %rax, %rax\n\t"
    "ret \n\t"
);

uint64_t call_ret(void){
    uint64_t res;
#undef ONE
#define ONE "call foo_ret\n\t"

    asm volatile(
        "mov %2, %%rax\n\t"
        "mov %1, %%rbx\n\t"
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %%rbx \n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r"(res)
        : "r"(__LOOP_COUNT), "r"(self_pointer)
        : "memory", "rax", "rbx");
    return res;
}

uint64_t call_ret_test(void) {
    double begin = second();
    uint64_t res = call_ret();
    double end = second();
    printf("simple call_ret:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 14 indirect call, call_reg_ret
asm (
    "foo_ret1: \n\t"
    "add %rax, %rax\n\t"
    "ret \n\t"
);

uint64_t call_reg_ret(void){
    uint64_t res;
#undef ONE
#define ONE "call *%%rcx\n\t"

    asm volatile(
        "mov $foo_ret1, %%rcx\n\t"
        "mov %2, %%rax\n\t"
        "mov %1, %%rbx\n\t"
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %%rbx \n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r"(res)
        : "r"(__LOOP_COUNT), "r"(self_pointer)
        : "memory", "rax", "rbx", "rcx");
    return res;
}

uint64_t call_reg_ret_test(void) {
    double begin = second();
    uint64_t res = call_reg_ret();
    double end = second();
    printf("simple call_ret reg:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 5 base_index_imm
#define base_index_imm       \
    asm volatile(            \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0" \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT), "r"(base), "r"(index1), "r"(index2), "r"(index3)  \
        : "rbx", "memory");

uint64_t base_index_imm_test(long arg1) {
    uint64_t res;
    long base = (long)aligned_alloc(64, 1 << 20);
    long index1 = 1 << 10;
    long index2 = base / 4 + 64;
    long index3 = base + 64;
    double begin = second();
    switch (arg1)
    {
    case 0: {
        
#undef ONE  // 4i__
#define ONE "sub $1, %4\n\tadd $1, %4\n\tmov (,%4,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 1: {
#undef ONE  // 4i_d
#define ONE "sub $1, %4\n\tadd $1, %4\n\tmov 64(,%4,4), %%rbx\n\t\n\t"
        base_index_imm
        break;
    }
    case 2: {
#undef ONE  // __b_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov (%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 3: {
#undef ONE  // __bd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov 64(%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 4: {
#undef ONE  // 4ib_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov (%2,%3,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 5: {
#undef ONE  // 4ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov 64(%2,%3,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 6: {
#undef ONE  // _ib_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov (%2,%3), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 7: {
#undef ONE  // _ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov 64(%2,%3), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 8: {
#undef ONE  // _i_d
#define ONE "sub $1, %5\n\tadd $1, %5\n\tmov 64(,%5), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 9: {
#undef ONE  // _i_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov (,%5), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 10: {
#undef ONE  // 8ib_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov (%2,%3,8), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 11: {
#undef ONE  // 8ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tmov 64(%2,%3,8), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 12: {
#undef ONE  // 4ib_ with disturb
#define ONE "sub $1, %2\n\tadd $1, %2\n\t" \
            "sub $1, %3\n\tadd $1, %3\n\t" \
            "mov (%2,%3,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 13: {
#undef ONE  // 8ib_ with disturb
#define ONE "sub $1, %2\n\tadd $1, %2\n\t" \
            "sub $1, %3\n\tadd $1, %3\n\t" \
            "mov (%2,%3,8), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 14: {
#undef ONE  // ___d
#define ONE "\n\t" \
            "sub $1, %2\n\tadd $1, %2\n\tmov ___d, %%rbx\n\t"
        base_index_imm
        break;
    }

    // use add to test
    case 15: {
#undef ONE  // _i_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd (,%5), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 16: {
#undef ONE  // __b_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd (%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 17: {
#undef ONE  // ___d
#define ONE "\n\t" \
            "sub $1, %2\n\tadd $1, %2\n\tadd ___d, %%rbx\n\t"
        base_index_imm
        break;
    }
    case 18: {
#undef ONE  // 4i__
#define ONE "sub $1, %4\n\tadd $1, %4\n\tadd (,%4,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 19: {
#undef ONE  // _ib_
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd (%2,%3), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 20: {
#undef ONE  // _i_d
#define ONE "sub $1, %5\n\tadd $1, %5\n\tadd 64(,%5), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 21: {
#undef ONE  // __bd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 64(%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 22: {
#undef ONE  // 4ib_ with disturb
#define ONE "sub $1, %2\n\tadd $1, %2\n\t" \
            "sub $1, %3\n\tadd $1, %3\n\t" \
            "add (%2,%3,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 23: {
#undef ONE  // 8ib_ with disturb
#define ONE "sub $1, %2\n\tadd $1, %2\n\t" \
            "sub $1, %3\n\tadd $1, %3\n\t" \
            "add (%2,%3,8), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 24: {
#undef ONE  // 4i_d
#define ONE "sub $1, %4\n\tadd $1, %4\n\tadd 64(,%4,4), %%rbx\n\t\n\t"
        base_index_imm
        break;
    }
    case 25: {
#undef ONE  // _ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 64(%2,%3), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 26: {
#undef ONE  // 4ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 64(%2,%3,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 27: {
#undef ONE  // 8ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 64(%2,%3,8), %%rbx\n\t"
        base_index_imm
        break;
    }

    // 32bit disp test
    case 28: {
        base += 0x3ef;
#undef ONE  // __bd disp=0xfffffc11
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd -0x3ef(%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 29: {
        base += 0x43ef;
#undef ONE  // __bd disp=0xffffbc11
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd -0x43ef(%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 30: {
        base += 0x83ef;
#undef ONE  // __bd disp=0xffff7c11
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd -0x83ef(%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 31: {
        base += 0x512343ef;
#undef ONE  // __bd disp=0xaedcbc11
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd -0x512343ef(%2), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 32: {
#undef ONE  // 4ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 0x12364(%2,%3,4), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 33: {
#undef ONE  // 4ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 0x4(%%rip), %%rbx\n\t"
        base_index_imm
        break;
    }
    case 34: {
#undef ONE  // 4ibd
#define ONE "sub $1, %2\n\tadd $1, %2\n\tadd 0x12345(%%rip), %%rbx\n\t"
        base_index_imm
        break;
    }
    default:    
        break;
    }
    double end = second();
    printf("simple base_index_imm_%ld:%f ns\n", arg1, (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 18 cmp_jcc
#define cmp_jcc       \
    asm volatile(            \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0" \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT), "r"(base), "r"(index1), "r"(index2)  \
        : "rbx", "memory");

uint64_t cmp_jcc_test(long arg1) {
    uint64_t res;
    long base = (long)aligned_alloc(64, 1 << 20);
    long index1 = 1 << 10;
    long index2 = base / 4 + 64;
    double begin = second();
    switch (arg1)
    {
    case 0: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jae 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 1: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jbe 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 2: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jge 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 3: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jle 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 4: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp 0x1004(,%4,4), %%rax\n\t" \
            "jo 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 5: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "js 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 6: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jp 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 7: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jae 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 8: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "cmp (,%4,4), %%rax\n\t" \
            "jne 2f\n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 9: {
        
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "cmp $10, %%al\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 10: {
        
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "cmp $10, %%ax\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    case 11: {
        
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $20, %%rbx\n\t" \
            "cmp %%eax, %%ebx\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        cmp_jcc
        break;
    }
    default:
        break;
    }
    double end = second();
    printf("simple cmp_jcc_%ld:%f ns\n", arg1, (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 6 push_pop
uint64_t push_pop(void){
    uint64_t res;
#undef ONE
#define ONE \
    "push %%rax\n\t" \
    "jmp 3f\n\t" \
    "3: \n\t" \
    "add %%rsp, %%rax\n\t" \
    "pop  %%rax\n\t" \
    "jmp 2f\n\t" \
    "2: \n\t"

    asm volatile(
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %1 \n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res)
        : "r"(__LOOP_COUNT)
        :   "rax",
            "memory");
    return res;
}

uint64_t push_pop_test(void) {
    double begin = second();
    uint64_t res = push_pop();
    double end = second();
    printf("simple push_pop:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 7 lahf
uint64_t lahf(void){
    uint64_t res;
#undef ONE
#define ONE "add %%rax, %%rbx\n\t" "lahf \n\t"
    asm volatile(
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %1 \n\t"
        "jnz 1b \n\t"
        "mov %%rbx, %0\n\t"
        : "=r"(res)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "memory");
    return res;
}

uint64_t lahf_test(void) {
    double begin = second();
    uint64_t res = lahf();
    double end = second();
    printf("simple lahf:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 8 partial_reg
#undef ONE
#define _partial_reg \
    asm volatile(                       \
        "1: \n\t"                       \
        __INNER_COUNT_NUM               \
        "0: sub $1, %1 \n\t"            \
        "jnz 1b \n\t"                   \
        "mov %%rax, %0\n\t"             \
        : "=r"(res)                     \
        : "r"(__LOOP_COUNT)             \
        :   "rax", "rbx", "memory");
uint64_t partial_reg(long arg1){
    uint64_t res;
    switch(arg1) {
    case 0: {
#undef  ONE
#define ONE "add %%bh, %%al\n\t"
        _partial_reg
        break;
    }
    case 1: {
#undef  ONE
#define ONE "add %%bl, %%ah\n\t"
        _partial_reg
        break;
    }
    case 2: {
#undef  ONE
#define ONE "add %%bh, %%ah\n\t"
        _partial_reg
        break;
    }
    case 3: {
#undef  ONE
#define ONE "add %%ah, %%ah\n\t"
        _partial_reg
        break;
    }
    case 4: {
#undef  ONE
#define ONE "add %%al, %%ah\n\t"
        _partial_reg
        break;
    }
    case 5: {
#undef  ONE
#define ONE "add %%bx, %%ax\n\t"
        _partial_reg
        break;
    }
    case 6: {
#undef  ONE
#define ONE "add %%ebx, %%eax\n\t"
        _partial_reg
        break;
    }
    case 7: {
#undef  ONE
#define ONE "add $2, %%ax\n\t"
        _partial_reg
        break;
    }
    case 8: {
#undef  ONE
#define ONE "add $2, %%ah\n\t"
        _partial_reg
        break;
    }
    }
    return res;
}

uint64_t partial_reg_test(long arg1) {
    double begin = second();
    uint64_t res = partial_reg(arg1);
    double end = second();
    printf("simple partial_reg:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 9 load_imm32
#define _load_imm32             \
    asm volatile(               \
        "1: \n\t"               \
        __INNER_COUNT_NUM       \
        "0: sub $1, %1 \n\t"    \
        "jnz 1b \n\t"           \
        "mov %%rax, %0\n\t"     \
        : "=r"(res)             \
        : "r"(__LOOP_COUNT)     \
        : "rax", "rbx", "memory");
uint64_t load_imm32(long arg1){
    uint64_t res;
    switch (arg1) {
    case 0: {
#undef ONE
#define ONE "add $0xfffffc11, %%eax\n\t"
        _load_imm32
        break;
    }
    case 1: {
#undef ONE
#define ONE "add $0xffffbc11, %%eax\n\t"
        _load_imm32
        break;
    }
    case 2: {
#undef ONE
#define ONE "add $0xaedcbc11, %%eax\n\t"
        _load_imm32
        break;
    }
    case 3: {
#undef ONE
#define ONE "add $0xffff7c11, %%eax\n\t"
        _load_imm32
        break;
    }
    }
    return res;
}

uint64_t load_imm32_test(long arg1) {
    double begin = second();
    uint64_t res = load_imm32(arg1);
    double end = second();
    printf("simple load_imm32:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 10 load_imm64
#define _load_imm64                 \
    asm volatile(                   \
        "1: \n\t"                   \
        __INNER_COUNT_NUM           \
        "0: sub $1, %1 \n\t"        \
        "jnz 1b \n\t"               \
        "mov %%rbx, %0\n\t"         \
        : "=r"(res)                 \
        : "r"(__LOOP_COUNT)         \
        :   "rax", "rbx", "memory");
uint64_t load_imm64(long arg1){
    uint64_t res;
    switch (arg1) {
    case 0: {
#undef ONE
#define ONE "movabs $0xfffffffffffff704, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 1: {
#undef ONE
#define ONE "movabs $0xffffffffffffa304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 2: {
#undef ONE
#define ONE "movabs $0xffffffffff720304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 3: {
#undef ONE
#define ONE "movabs $0xffffffff73020304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 4: {
#undef ONE
#define ONE "movabs $0xfffffff701020304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 5: {
#undef ONE
#define ONE "movabs $0xffff7c1101020304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 6: {
#undef ONE
#define ONE "movabs $0xfff7bc1101020304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 7: {
#undef ONE
#define ONE "movabs $0xabfabc1101020304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 8: { // zero hole in [15:0] for AArch64
#undef ONE
#define ONE "movabs $0xabfabc1153040000, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 9: { // zero hole in [11:0] for LoongArch
#undef ONE
#define ONE "movabs $0xabfabc1153045000, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 10: { // zero hole in [31:16] for AArch64
#undef ONE
#define ONE "movabs $0xabfabc1100005304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 11: { // zero hole in [31:12] for LoongArch
#undef ONE
#define ONE "movabs $0xabfabc1100000304, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 12: { // simple bitmask pattern for AArch64
#undef ONE
#define ONE "movabs $0x3333333333333333, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 13: { // complex bitmask pattern for AArch64
#undef ONE
// can be load by two AArch64 instructions
//   andi tmp, 0x3333333333333333, zero;
//   xor  tmp, 0xFF00007FFF00007F, tmp;
#define ONE "movabs $0xCC33334CCC33334C, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    case 14: { // complex bitmask pattern for AArch64
#undef ONE
// can be load by two AArch64 instructions
//   andi tmp, 0x3333333333333333, zero;
//   xor  tmp, 0x7FFF00007FFF0000, tmp;
#define ONE "movabs $0x4CCC33334CCC3333, %%rax\n\tadd %%rax, %%rbx\n\t"
        _load_imm64
        break;
    }
    }
    return res;
}

uint64_t load_imm64_test(long arg1) {
    double begin = second();
    uint64_t res = load_imm64(arg1);
    double end = second();
    printf("simple load_imm64:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 11 extension
#define _extension           \
    asm volatile(            \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0"      \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT), "r"(num) \
        : "rax", "rbx", "memory");
uint64_t extension_test(long arg1) {
    uint64_t res;
    uint64_t num = 0xabcdef9876543210ull;
    double begin = second();
    switch (arg1) {
    case 0: {
#undef ONE // low -> 16
#define ONE "movsx %b2, %%ax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 1: {
#undef ONE // high -> 16
#define ONE "movsx %h2, %%ax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 2: {
#undef ONE // low -> 32
#define ONE "movsx %b2, %%eax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 3: {
#undef ONE // high -> 32
#define ONE "movsx %h2, %%eax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 4: {
#undef ONE // 16 -> 32
#define ONE "movsx %w2, %%eax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 5: {
#undef ONE // 16 -> 64
#define ONE "movsx %w2, %%rax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 6: {
#undef ONE // 32 -> 64
#define ONE "movsxd %k2, %%rax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 7: {
#undef ONE // low -> 16
#define ONE "movzx %b2, %%ax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 8: {
#undef ONE // high -> 16
#define ONE "movzx %h2, %%ax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 9: {
#undef ONE // low -> 32
#define ONE "movzx %b2, %%eax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 10: {
#undef ONE // high -> 32
#define ONE "movzx %h2, %%eax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 11: {
#undef ONE // low -> 64
#define ONE "movzx %b2, %%rax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    /* illegal encoding */
    /* case 12: { */
/* #undef ONE // high -> 64 */
/* #define ONE "movzx %h2, %%rax\n\tadd %%rax, %%rbx\n\t" */
    /* _extension */
    /*     break; */
    /* } */
    case 12: {
#undef ONE // 16 -> 32
#define ONE "movzx %w2, %%eax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    case 13: {
#undef ONE // 16 -> 64
#define ONE "movzx %w2, %%rax\n\tadd %%rax, %%rbx\n\t"
    _extension
        break;
    }
    }
    double end = second();
    printf("simple extension:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 12 regs map
uint64_t regs_map(void) {
    uint64_t regs[16];
    int i;
    for (i = 0; i < 16; i++) {
        regs[i] = 0;
    }
    asm volatile(
        "mov $1, %%eax\n\t"
        "mov $2, %%ebx\n\t"
        "mov $3, %%ecx\n\t"
        "mov $4, %%edx\n\t"
        "mov $5, %%esi\n\t"
        "mov $6, %%edi\n\t"
        // "mov $1, %%ebp\n\t"
        // "mov $1, %%esp\n\t"
        "mov $8, %%r8\n\t"
        "mov $9, %%r9\n\t"
        "mov $10, %%r10\n\t"
        "mov $11, %%r11\n\t"
        "mov $12, %%r12\n\t"
        "mov $13, %%r13\n\t"
        "mov $14, %%r14\n\t"
        "mov $15, %%r15\n\t"
        "mov %%eax, %0\n\t"
        "mov %%ebx, %1\n\t"
        "mov %%ecx, %2\n\t"
        "mov %%edx, %3\n\t"
        "mov %%esi, %4\n\t"
        "mov %%edi, %5\n\t"
        "mov %%r8, %6\n\t"
        "mov %%r9, %7\n\t"
        "mov %%r10, %8\n\t"
        "mov %%r11, %9\n\t"
        "mov %%r12, %10\n\t"
        "mov %%r13, %11\n\t"
        "mov %%r14, %12\n\t"
        "mov %%r15, %13\n\t"
        "mov %%ebp, %14\n\t"
        "mov %%esp, %15\n\t"
        : "=m"(regs[0]), "=m"(regs[1]), "=m"(regs[2]), "=m"(regs[3]), "=m"(regs[4]),
          "=m"(regs[5]), "=m"(regs[6]), "=m"(regs[7]), "=m"(regs[8]), "=m"(regs[9]),
          "=m"(regs[10]), "=m"(regs[11]), "=m"(regs[12]), "=m"(regs[13]), "=m"(regs[14]),"=m"(regs[15])
        : : "eax", "ebx", "ecx", "edx", "esi", "edi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    );
    // printf("After store one to regs:\n");
    // for (i = 0; i < 16; i++) {
    //     printf("Reg %d value: %lu\n", i, regs[i]);
    // }
}


uint64_t regs_map_test() {
    double begin = second();
    uint64_t res;
    for (size_t i = 0; i < 100000000; i++)
    // for (size_t i = 0; i < 1; i++)
    {
        res = regs_map();
    }
    double end = second();
    printf("regs_map:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 13 jcc
#undef ONE
#define _jcc \
    asm volatile(                       \
        "1: \n\t"                       \
        __INNER_COUNT_NUM               \
        "0: sub $1, %1 \n\t"            \
        "jnz 1b \n\t"                   \
        "mov %%rax, %0\n\t"             \
        : "=r"(res)                     \
        : "r"(__LOOP_COUNT)             \
        :   "rax", "rbx", "memory");
uint64_t jcc(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {   // jz, ZF
#undef  ONE
#define ONE "2: \n\t" \
        "mov $10, %%rax\n\t" \
        "cmp $10, %%rax\n\t" \
        "jz 2f \n\t" \
        "2: \n\t" 
        _jcc
        break;
        }
        case 1: {   // jo, OF
#undef ONE
#define ONE "mov $0x7fffffffffffffff, %%rax\n\t" \
            "add $1,  %%rax\n\t" \
            "jo  2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 2: {   // jc, CF
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "add $2, %%rax\n\t" \
            "jc 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 3: {   // jna, CF or ZF
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $20, %%rbx\n\t" \
            "cmp %%rbx,  %%rax\n\t" \
            "jna  2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 4: {   // js, SF
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "sub $1, %%rax\n\t" \
            "js  2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 5: {   // jp, PF
#undef ONE
#define ONE "mov $0x12, %%ah\n\t" \
            "add $0x34, %%ah\n\t" \
            "jp  2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 6: {   // jl, SF!=OF
#undef ONE
#define ONE "mov $0x80000000, %%eax\n\t" \
            "add $0x80000000, %%eax\n\t" \
            "jl  2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 7: {   // jng, SF!=OF or ZF=1
#undef ONE
#define ONE "mov $0x80000000, %%eax\n\t" \
            "add $0x80000000, %%eax\n\t" \
            "jng  2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        // further test add/sub jc, together with case 2
        case 8: {   // add jnc, !CF
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "add $2, %%rax\n\t" \
            "jnc 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 9: {   // sub jc, CF
#undef ONE
#define ONE "mov $1, %%rax\n\t" \
            "sub $2, %%rax\n\t" \
            "jc 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 10: {   // sub jnc, CF
#undef ONE
#define ONE "mov $1, %%rax\n\t" \
            "sub $2, %%rax\n\t" \
            "jnc 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        // further test add/sub jna
        case 11: {   // add jna, CF or ZF
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "add $2, %%rax\n\t" \
            "jna 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 12: {   // add ja, !(CF or ZF)
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "add $2, %%rax\n\t" \
            "ja 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 13: {   // sub jna, CF or ZF
#undef ONE
#define ONE "mov $0x1, %%rax\n\t" \
            "sub $2, %%rax\n\t" \
            "jna 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
        case 14: {   // sub ja, !(CF or ZF)
#undef ONE
#define ONE "mov $0x1, %%rax\n\t" \
            "sub $2, %%rax\n\t" \
            "ja 2f\n\t" \
            "2: \n\t"
        _jcc
        break;
        }
    }
    return res;
}

uint64_t jcc_test(long arg1) {
    double begin = second();
    uint64_t res = jcc(arg1);
    double end = second();
    printf("simple jcc:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 15 mul
#define _mul \
    asm volatile( \
        "mov %1, %%rcx\n\t" \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %%rcx\n\t" \
        "jnz 1b \n\t" \
        "mov %2, %0\n\t" \
        : "=r"(res) \
        : "r"(__LOOP_COUNT), "r"(self_pointer) \
        : "rax", "rbx", "rcx", "rdx", "memory");
uint64_t mul(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef ONE
#define ONE "mov $7, %%eax \n\t" \
            "mov $13, %%ebx\n\t" \
            "mul %%ebx\n\t"
        _mul
        break;
        }
        case 1: {
#undef ONE
#define ONE "mov $7, %%rax \n\t" \
            "mov $13, %%rbx\n\t" \
            "mul %%rbx\n\t"
        _mul
        break;
        }
        // if not use rdx, Rosetta won't calc high 64bit of mul
        case 2: {
#undef ONE
#define ONE "mov $7, %%rax\n\t" \
            "mov $13, %%rbx\n\t" \
            "mul %%rbx\n\t" \
            "add %%rdx, %%rax\n\t"
        _mul
        break;
        }
    }

    return res;
}

uint64_t mul_test(long arg1) {
    double begin = second();
    uint64_t res = mul(arg1);
    double end = second();
    printf("simple mul:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 16 sse
#define _addps \
    asm volatile( \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %2 \n\t" \
        "jnz 1b \n\t" \
        "mov %3, %0\n\t" \
        : "=r"(res), "=m"(y) \
        : "r"(__LOOP_COUNT), "r"(self_pointer), "m"(x1), "m"(x2) \
        :   "rax", "rbx", "xmm0" ,"xmm1", "memory"); \
    printf("y = %f, %f, %f, %f\n", y[0], y[1], y[2], y[3]);
uint64_t add_vf(long arg1){
    uint64_t res;
    float x1[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float x2[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float y[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    switch(arg1) {
        case 0: {
#undef ONE
#define ONE "movups %4, %%xmm0\n\t" \
            "movups %5, %%xmm1\n\t" \
            "addps  %%xmm1, %%xmm0\n\t" \
            "movups %%xmm0, %1\n\t"
        _addps
        break;
        }
        case 1: {
#undef ONE
#define ONE "movups %4, %%xmm0\n\t" \
            "addps  %%xmm0, %%xmm0\n\t" \
            "movups %%xmm0, %1\n\t"
        _addps
        break;
        }
        case 2: {
#undef ONE
#define ONE "addpd  %%xmm0, %%xmm0\n\t" \
            "movupd %%xmm0, %1\n\t"
        _addps
        break;
        }
        case 3: {
#undef ONE
#define ONE "addss  %%xmm0, %%xmm0\n\t" \
            "movss %%xmm0, %1\n\t"
        _addps
        break;
        }
        case 4: {
#undef ONE
#define ONE "addsd  %%xmm0, %%xmm0\n\t" \
            "movsd %%xmm0, %1\n\t"
        _addps
        break;
        }
        case 5: {
#undef ONE
#define ONE "movss %1, %%xmm0\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 6: {
#undef ONE
#define ONE "movsd %1, %%xmm0\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 7: {
#undef ONE
#define ONE "movss %1, %%xmm1\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 8: {
#undef ONE
#define ONE "addps %%xmm0, %%xmm0\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 9: { // test latx's strange behaviour
#undef ONE
#define ONE "movss %1, %%xmm1\n\t" \
            "addss %%xmm1, %%xmm0\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 10: { // test latx's strange behaviour
#undef ONE
#define ONE "movss %%xmm0, %%xmm1\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 11: { // test latx's strange behaviour
#undef ONE
#define ONE "movsd %1, %%xmm1\n\t" \
            "addss %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
        case 12: {
#undef ONE
#define ONE "movsd %1, %%xmm0\n\t" \
            "addsd %%xmm1, %%xmm0\n\t"
        _addps
        break;
        }
    }

    return res;
}

uint64_t add_vf_test(long arg1) {
    double begin = second();
    uint64_t res = add_vf(arg1);
    double end = second();
    printf("simple sse:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 17 comiss
#define _comiss \
    asm volatile( \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t" \
        "mov %2, %0\n\t" \
        : "=r"(res) \
        : "r"(__LOOP_COUNT), "r"(self_pointer), "m"(x), "m"(y[0]) \
        : "rax", "rbx", "xmm0", "xmm1", "memory");
uint64_t comiss(long arg1){
    float x[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "ja 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 1: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "jz 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 2: {
#undef ONE
#define ONE "addps %%xmm1, %%xmm0\n\t" \
            "comiss %%xmm1, %%xmm0\n\t" \
            "jz 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 3: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comisd %4, %%xmm0\n\t" \
            "jz 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 4: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "ucomiss %4, %%xmm0\n\t" \
            "jz 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 5: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "ucomisd %4, %%xmm0\n\t" \
            "jz 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 6: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "jo 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 7: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "jc 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 8: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "js 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 9: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "jp 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 10: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "jl 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
        case 11: {
#undef ONE
#define ONE "movups %3, %%xmm0\n\t" \
            "comiss %4, %%xmm0\n\t" \
            "jle 2f\n\t" \
            "2: \n\t"
            // "seta %%al\n\t"
        _comiss
        break;
        }
    }

    return res;
}

uint64_t comiss_test(long arg1) {
    double begin = second();
    uint64_t res = comiss(arg1);
    double end = second();
    printf("simple comiss:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 19 cmovcc
#undef ONE
#define _cmovcc \
    asm volatile(                       \
        "1: \n\t"                       \
        __INNER_COUNT_NUM               \
        "0: sub $1, %1 \n\t"            \
        "jnz 1b \n\t"                   \
        "mov %%rax, %0\n\t"             \
        : "=r"(res)                     \
        : "r"(__LOOP_COUNT)             \
        :   "rax", "rbx", "memory");
uint64_t cmovcc(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {   // cmovz, ZF
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "cmp $10, %%rax\n\t" \
            "cmovz %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 1: {   // cmovo, OF
#undef ONE
#define ONE "mov $0x7fffffffffffffff, %%rax\n\t" \
            "add $1,  %%rax\n\t" \
            "cmovo %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 2: {   // cmovc, CF
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "add $2, %%rax\n\t" \
            "cmovc %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 3: {   // cmovna, CF or ZF
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $20, %%rbx\n\t" \
            "cmp %%rbx,  %%rax\n\t" \
            "cmovna %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 4: {   // cmovs, SF
#undef ONE
#define ONE "mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t" \
            "sub $1, %%rax\n\t" \
            "cmovs %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 5: {   // cmovp, PF
#undef ONE
#define ONE "mov $0x12, %%ah\n\t" \
            "add $0x34, %%ah\n\t" \
            "cmovp %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 6: {   // cmovl, SF!=OF
#undef ONE
#define ONE "mov $0x80000000, %%eax\n\t" \
            "add $0x80000000, %%eax\n\t" \
            "cmovl %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 7: {   // cmovng, SF!=OF or ZF=1
#undef ONE
#define ONE "mov $0x80000000, %%eax\n\t" \
            "add $0x80000000, %%eax\n\t" \
            "cmovng %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 8: {   // cmovz, ZF
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovz %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 9: {   // cmovo, OF
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovo %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 10: {   // cmovc, CF
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovc %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 11: {   // cmovs, SF
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovs %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 12: {   // cmovp, PF
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovp %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 13: {   // cmovl, S!=O
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovl %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 14: {   // cmovle, (S!=O)|Z
#undef  ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $10, %%rbx\n\t" \
            "cmp %%rbx, %%rax\n\t" \
            "cmovle %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 15: {   // cmovz, ZF
#undef ONE
#define ONE "mov $0x80000000, %%eax\n\t" \
            "add $0x80000000, %%eax\n\t" \
            "cmovz %1, %%rbx\n\t"
        _cmovcc
        break;
        }
        case 16: {   // cmovbe, C|Z
#undef ONE
#define ONE "mov $0x80000000, %%eax\n\t" \
            "add $0x80000000, %%eax\n\t" \
            "cmovbe %1, %%rbx\n\t"
        _cmovcc
        break;
        }
    }
    return res;
}

uint64_t cmovcc_test(long arg1) {
    double begin = second();
    uint64_t res = cmovcc(arg1);
    double end = second();
    printf("simple cmovcc:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 20 ldst_pair
#undef ONE
#define _ldst_pair \
    asm volatile(                       \
        "1: \n\t"                       \
        "mov %3, %%rcx\n\t"             \
        __INNER_COUNT_NUM               \
        "0: sub $1, %1 \n\t"            \
        "jnz 1b \n\t"                   \
        "mov %%rax, %0\n\t"             \
        : "=r"(res)                     \
        : "r"(__LOOP_COUNT), "r"(base), "r"(index1), "r"(index2)  \
        : "rax", "rbx", "rcx", "xmm0", "xmm1", "memory");
uint64_t ldst_pair(long arg1){
    long base = (long)aligned_alloc(64, 1 << 20);
    long index1 = 1 << 10;
    long index2 = base / 4 + 64;
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef  ONE
#define ONE "mov 64(%2), %%ebx\n\tmov 68(%2), %%eax\n\t"
        _ldst_pair
        break;
        }
        case 1: {
#undef  ONE
#define ONE "mov 64(%2), %%rbx\n\tmov 72(%2), %%rax\n\t"
        _ldst_pair
        break;
        }
        case 2: {
#undef  ONE
#define ONE "mov 64(%2,%%rcx,4), %%ebx\n\tmov 68(%2,%%rcx,4), %%eax\n\t"
        _ldst_pair
        break;
        }
        case 3: {
#undef  ONE
#define ONE "mov 64(%2,%%rcx,4), %%ebx\n\tinc %%rcx\n\tmov 64(%2,%%rcx,4), %%eax\n\t"
        _ldst_pair
        break;
        }
        case 4: {
#undef  ONE
#define ONE "add 64(%2,%%rcx,4), %%ebx\n\tadd 68(%2,%%rcx,4), %%eax\n\t"
        _ldst_pair
        break;
        }
        case 5: {
#undef  ONE
#define ONE "add 64(%2,%%rcx,4), %%ebx\n\tinc %%rcx\n\tadd 64(%2,%%rcx,4), %%eax\n\t"
        _ldst_pair
        break;
        }
        case 6: {
#undef  ONE
#define ONE "movups 64(%2), %%xmm0\n\tmovups 80(%2), %%xmm1\n\t"
        _ldst_pair
        break;
        }
        case 7: {
#undef  ONE
#define ONE "mov 64(%2,%%rcx,4), %%ebx\n\tinc %2\n\tmov 64(%2,%%rcx,4), %%eax\n\tdec %2\n\t"
        _ldst_pair
        break;
        }
        case 8: {
#undef  ONE
#define ONE "add 64(%2,%%rcx,4), %%ebx\n\tinc %2\n\tadd 64(%2,%%rcx,4), %%eax\n\tdec %2\n\t"
        _ldst_pair
        break;
        }
    }
    return res;
}

uint64_t ldst_pair_test(long arg1) {
    double begin = second();
    uint64_t res = ldst_pair(arg1);
    double end = second();
    printf("ldst_pair :%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 21 pushpop_pair
uint64_t pushpop_pair(void){
    uint64_t res;
#undef ONE
#define ONE \
    "push %%rax\n\t" \
    "push %%rbx\n\t" \
    "jmp 3f\n\t" \
    "3: \n\t" \
    "add %%rsp, %%rax\n\t" \
    "pop  %%rbx\n\t" \
    "pop  %%rax\n\t" \
    "jmp 2f\n\t" \
    "2: \n\t"

    asm volatile(
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %1 \n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "memory");
    return res;
}

uint64_t pushpop_pair_test(void) {
    double begin = second();
    uint64_t res = pushpop_pair();
    double end = second();
    printf("simple pushpop_pair:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 22 div
uint64_t div1(void){
    uint64_t res;
#undef ONE
#define ONE \
        "mov $7, %%eax \n\t" \
        "mov $13, %%ebx\n\t" \
        "mov $0, %%edx\n\t" \
        "idiv %%ebx\n\t"

    asm volatile(
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %1 \n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "memory");
    return res;
}

uint64_t div_test(void) {
    double begin = second();
    uint64_t res = div1();
    double end = second();
    printf("simple div test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 23 movss
uint64_t movss(void){
    uint64_t res;
    float a = 1.0f, b = 2.0f;
#undef ONE
#define ONE \
        "movss %2, %%xmm2 \n\t" \
        "movss %1, %2 \n\t" \
        "movss %%xmm2, %1 \n\t" \

    asm volatile(
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %3 \n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res), "+x" (a), "+x" (b)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "xmm2", "memory");
    return res;
}

uint64_t movss_test(void) {
    double begin = second();
    uint64_t res = movss();
    double end = second();
    printf("simple movss test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 24 jmp_direct
#undef  ONE
#define _jmp_direct \
    asm volatile( \
        "mov %1, %%rbx\n\t" \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t" \
        "mov %%rax, %0\n\t" \
        : "=r" (res) \
        : "r"(__LOOP_COUNT) \
        :   "rax", "rbx", "memory");
uint64_t jmp_direct(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef ONE
#define ONE "add $1,%%rax\n\tjmp 2f\n\t2:\n\t"
        _jmp_direct
        break;
        }
        case 1: { // for latx, cannot reduce the eflags
#undef ONE
#define ONE "jmp 2f\n\t2:\n\t"
        _jmp_direct
        break;
        }
    }
    return res;
}

uint64_t jmp_direct_test(long arg1) {
    double begin = second();
    uint64_t res = jmp_direct(arg1);
    double end = second();
    printf("jmp direct test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 25 mov
uint64_t mov(void){
    uint64_t res;
#undef ONE
#define ONE "mov %%rbx, %%rax\n\t"
    asm volatile(
        "mov %1, %%rbx\n\t"
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %%rbx\n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "memory");
    return res;
}

uint64_t mov_test(void) {
    double begin = second();
    uint64_t res = mov();
    double end = second();
    printf("simple mov test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 26 lea
#undef ONE
#define _lea \
    asm volatile( \
        "mov %1, %%rbx\n\t" \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t" \
        "mov %%rax, %0\n\t" \
        : "=r" (res) \
        : "r"(__LOOP_COUNT) \
        :   "rax", "rbx", "memory");
uint64_t lea(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef  ONE
#define ONE "lea 64(%%rax, %%rax, 4), %%rax\n\t"
        _lea
        break;
        }
        case 1: {
#undef  ONE
#define ONE "lea 0x12345(%%rax, %%rax, 4), %%rax\n\t"
        _lea
        break;
        }
    }
    return res;
}

uint64_t lea_test(long arg1) {
    double begin = second();
    uint64_t res = lea(arg1);
    double end = second();
    printf("simple lea test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 27 and
uint64_t and(void){
    uint64_t res;
#undef ONE
#define ONE "and %%rbx, %%rax\n\t"
    asm volatile(
        "mov %1, %%rbx\n\t"
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %%rbx\n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "memory");
    return res;
}

uint64_t and_test(void) {
    double begin = second();
    uint64_t res = and();
    double end = second();
    printf("simple and test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 28 shl
#undef ONE
#define _shl \
    asm volatile( \
        "mov %1, %%rbx\n\t" \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t" \
        "mov %%rax, %0\n\t" \
        : "=r" (res) \
        : "r"(__LOOP_COUNT) \
        :   "rax", "rbx", "memory");
uint64_t shl(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef  ONE
#define ONE "shl $1, %%eax\n\t"
        _shl
        break;
        }
        case 1: {
#undef  ONE
#define ONE "shl $1, %%rax\n\t"
        _shl
        break;
        }
    }
    return res;
}

uint64_t shl_test(long arg1) {
    double begin = second();
    uint64_t res = shl(arg1);
    double end = second();
    printf("simple shl test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 29 imul
#define _imul \
    asm volatile( \
        "1: \n\t" \
        __INNER_COUNT_NUM \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t" \
        "mov %2, %0\n\t" \
        : "=r"(res) \
        : "r"(__LOOP_COUNT), "r"(self_pointer) \
        :   "rax", "rbx", \
            "memory");
uint64_t imul(long arg1){
    uint64_t res;
    switch(arg1) {
        case 0: {
#undef ONE
#define ONE "mov $7, %%eax \n\t" \
            "mov $13, %%ebx\n\t" \
            "imul %%ebx, %%eax\n\t"
        _imul
        break;
        }
        case 1: {
#undef ONE
#define ONE "mov $7, %%rax \n\t" \
            "mov $13, %%rbx\n\t" \
            "imul %%rbx, %%rax\n\t"
        _imul
        break;
        }
    }
    return res;
}

uint64_t imul_test(long arg1) {
    double begin = second();
    uint64_t res = imul(arg1);
    double end = second();
    printf("simple imul:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 30 movs
#define _movs \
    asm volatile( \
        "mov %1, %%rbx\n\t" \
        "1: \n\t" \
        "mov %2, %%rsi\n\t" \
        "mov %3, %%rdi\n\t" \
        "mov $" STRING(__INNER_COUNT) ", %%rcx\n\t" \
        MOVS \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t" \
        "mov %%rax, %0\n\t" \
        : "=r" (res) \
        : "r"(__LOOP_COUNT), "r"(src), "r"(dst) \
        :   "rax", "rbx", "rcx", "rsi", "rdi", "memory");
uint64_t movs(long arg1){
    uint64_t res;
    uint64_t src[__INNER_COUNT];
    uint64_t dst[__INNER_COUNT];
    switch(arg1) {
        case 0: {
#undef  MOVS
#define MOVS "rep movsq\n\t"
        _movs
        break;
        }
        case 1: {
#undef  MOVS
#define MOVS "rep movsl\n\t"
        _movs
        break;
        }
        case 2: {
#undef  MOVS
#define MOVS "rep movsw\n\t"
        _movs
        break;
        }
        case 3: {
#undef  MOVS
#define MOVS "rep movsb\n\t"
        _movs
        break;
        }
    }
    return res;
}

uint64_t movs_test(long arg1) {
    double begin = second();
    uint64_t res = movs(arg1);
    double end = second();
    printf("simple movs test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 31 movs1024
// ExaGear will combine 4/8/16/1 rep movs for 64bit 32bit 16bit 8bit version
// Movs1024 will unleash ExaGear's full power!
#define _movs1024 \
    asm volatile( \
        "mov %1, %%rbx\n\t" \
        "1: \n\t" \
        "mov %2, %%rsi\n\t" \
        "mov %3, %%rdi\n\t" \
        "mov $1024, %%rcx\n\t" \
        MOVS1024 \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t" \
        "mov %%rax, %0\n\t" \
        : "=r" (res) \
        : "r"(__LOOP_COUNT), "r"(src), "r"(dst) \
        :   "rax", "rbx", "rcx", "rsi", "rdi", "memory");
uint64_t movs1024(long arg1){
    uint64_t res;
    uint64_t src[1024];
    uint64_t dst[1024];
    switch(arg1) {
        case 0: {
#undef  MOVS1024
#define MOVS1024 "rep movsq\n\t"
        _movs1024
        break;
        }
        case 1: {
#undef  MOVS1024
#define MOVS1024 "rep movsl\n\t"
        _movs1024
        break;
        }
        case 2: {
#undef  MOVS1024
#define MOVS1024 "rep movsw\n\t"
        _movs1024
        break;
        }
        case 3: {
#undef  MOVS1024
#define MOVS1024 "rep movsb\n\t"
        _movs1024
        break;
        }
    }
    return res;
}

uint64_t movs1024_test(long arg1) {
    double begin = second();
    uint64_t res = movs1024(arg1);
    double end = second();
    printf("simple movs1024 test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 32 cmp_jcc
#define cmp_jcc_reg       \
    asm volatile(            \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0" \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT), "r"(base), "r"(index1), "r"(index2)  \
        : "rbx", "memory");

uint64_t cmp_jcc_reg_test(long arg1) {
    uint64_t res;
    long base = (long)aligned_alloc(64, 1 << 20);
    long index1 = 1 << 10;
    long index2 = base / 4 + 64;
    double begin = second();
    switch (arg1) {
    case 0: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "je 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 1: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "jo 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 2: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "jc 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 3: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "ja 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 4: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "js 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 5: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "jp 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 6: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "jge 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    case 7: {
#undef ONE
#define ONE "cmp %%rbx, %%rax\n\t" \
            "jng 2f\n\t" \
            "2: \n\t"
        cmp_jcc_reg
        break;
    }
    }
    double end = second();
    printf("simple cmp_jcc_reg_%ld:%f ns\n", arg1, (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 33 mov_fp
#define mov_fp \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT), "r"(src), "r"(dest)    \
        :   "rax", "rbx", "memory", "r15", "xmm2");

uint64_t movfp(long arg1, const double *src, double *dest){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movupd (%%r15, %%rax), %%xmm2\n\t" \
            "movupd %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    case 1:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movupd (%%r15, %%rax, 2), %%xmm2\n\t" \
            "movupd %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
        case 2:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movsd -0x261c(%%rip), %%xmm2\n\t" \
            "movupd %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    case 3:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movups (%%r15, %%rax, 2), %%xmm2\n\t" \
            "movups %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    case 4:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movss (%%r15, %%rax, 2), %%xmm2\n\t" \
            "movss %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    case 5:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movsd (%%r15, %%rax, 2), %%xmm2\n\t" \
            "movsd %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    case 6:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movaps (%%r15, %%rax, 2), %%xmm2\n\t" \
            "movaps %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    case 7:
#undef ONE
#define ONE "mov %2, %%r15\n\t" \
            "mov $0, %%rax\n\t" \
            "movapd (%%r15, %%rax, 2), %%xmm2\n\t" \
            "movapd %%xmm2, (%%r15)\n\t"
        mov_fp
        break;
    default:
        break;
    }

    return res;
}

uint64_t movfp_test(long arg1) {
    double src[2] = {1.0, 2.0};
    double dest[2] = {0.0, 0.0};
    double begin = second();
    uint64_t res = movfp(arg1, src, dest);
    double end = second();
    printf("simple movfp test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 34 div_vf
#define _div_vf \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "movsd (%2), %%xmm1\n\t" \
        "movss (%3), %%xmm2\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT), "r"(dbl), "r"(sng)    \
        :   "rax", "rbx", "memory", "r15", "xmm0", "xmm1", "xmm2");

uint64_t div_vf(long arg1, const double *dbl, float *sng){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "divsd %%xmm1, %%xmm0\n\t"
        _div_vf
        break;
    case 1:
#undef ONE
#define ONE "divss %%xmm2, %%xmm0\n\t"
        _div_vf
        break;
    case 2:
#undef ONE
#define ONE "divpd %%xmm1, %%xmm0\n\t"
        _div_vf
        break;
    case 3:
#undef ONE
#define ONE "divps %%xmm2, %%xmm0\n\t"
        _div_vf
        break;
    default:
        break;
    }

    return res;
}

uint64_t div_vf_test(long arg1) {
    double dbl[2] = {1.0, 2.0};
    float sng[2] = {1.0, 2.0};
    double begin = second();
    uint64_t res = div_vf(arg1, dbl, sng);
    double end = second();
    printf("simple movfp test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 35 idiv
uint64_t idiv(void){
    uint64_t res;
#undef ONE
#define ONE "idiv %%rbx\n\t"

    asm volatile(
        "mov %1, %%rcx\n\t"
        "mov $1, %%rax\n\t"
        "mov $1, %%rbx\n\t"
        "mov $0, %%rdx\n\t"
        "1: \n\t"
        __INNER_COUNT_NUM
        "0: sub $1, %%rcx\n\t"
        "jnz 1b \n\t"
        "mov %%rax, %0\n\t"
        : "=r" (res)
        : "r"(__LOOP_COUNT)
        :   "rax", "rbx", "rcx", "memory");
    return res;
}

uint64_t idiv_test(void) {
    double begin = second();
    uint64_t res = idiv();
    double end = second();
    printf("simple idiv test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 36 pxor
#define _pxor \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "movsd (%2), %%xmm1\n\t" \
        "movss (%3), %%xmm2\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT), "r"(dbl), "r"(sng)    \
        :   "rax", "rbx", "memory", "r15", "xmm0", "xmm1", "xmm2");

uint64_t pxor(long arg1, const double *dbl, float *sng){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "pxor %%xmm1, %%xmm0\n\t"
        _pxor
        break;
    default:
        break;
    }

    return res;
}

uint64_t pxor_test(long arg1) {
    double dbl[2] = {1.0, 2.0};
    float sng[2] = {1.0, 2.0};
    double begin = second();
    uint64_t res = pxor(arg1, dbl, sng);
    double end = second();
    printf("simple pxor test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 37 cvt_cf
#define _cvt_vf \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "movsd (%2), %%xmm1\n\t" \
        "movss (%3), %%xmm2\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT), "r"(dbl), "r"(sng)    \
        :   "rax", "rbx", "memory", "r15", "xmm0", "xmm1", "xmm2");

uint64_t cvt_vf(long arg1, const double *dbl, float *sng){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "cvtdq2pd %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 1:
#undef ONE
#define ONE "cvtdq2ps %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 2:
#undef ONE
#define ONE "cvtpd2ps %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 3:
#undef ONE
#define ONE "cvtps2pd %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 4:
#undef ONE
#define ONE "cvtsd2ss %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 5:
#undef ONE
#define ONE "cvtsi2sd %%rbx, %%xmm0\n\t"
        _cvt_vf
        break;
    case 6:
#undef ONE
#define ONE "cvtsi2ss %%rbx, %%xmm0\n\t"
        _cvt_vf
        break;
    case 7:
#undef ONE
#define ONE "cvtss2sd %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 8:
#undef ONE
#define ONE "cvttpd2dq %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 9:
#undef ONE
#define ONE "cvttps2dq %%xmm1, %%xmm0\n\t"
        _cvt_vf
        break;
    case 10:
#undef ONE
#define ONE "cvttsd2si %%xmm0, %%rax\n\t"
        _cvt_vf
        break;
    case 11:
#undef ONE
#define ONE "cvttss2si %%xmm0, %%rax\n\t"
        _cvt_vf
        break;
    default:
        break;
    }

    return res;
}

uint64_t cvt_vf_test(long arg1) {
    double dbl[2] = {1.0, 2.0};
    float sng[2] = {1.0, 2.0};
    double begin = second();
    uint64_t res = cvt_vf(arg1, dbl, sng);
    double end = second();
    printf("simple cvt_vf test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 38 mul_vf
#define _mul_vf \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "movsd (%2), %%xmm1\n\t" \
        "movss (%3), %%xmm2\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT), "r"(dbl), "r"(sng)    \
        :   "rax", "rbx", "memory", "r15", "xmm0", "xmm1", "xmm2");

uint64_t mul_vf(long arg1, const double *dbl, float *sng){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "mulpd %%xmm1, %%xmm0\n\t"
        _mul_vf
        break;
    case 1:
#undef ONE
#define ONE "mulps %%xmm1, %%xmm0\n\t"
        _mul_vf
        break;
    case 2:
#undef ONE
#define ONE "mulsd %%xmm1, %%xmm0\n\t"
        _mul_vf
        break;
    case 3:
#undef ONE
#define ONE "mulss %%xmm1, %%xmm0\n\t"
        _mul_vf
        break;
    case 4:
#undef ONE
#define ONE "divpd %%xmm1, %%xmm0\n\t"
        _div_vf
        break;
    case 5:
#undef ONE
#define ONE "divps %%xmm1, %%xmm0\n\t"
        _div_vf
        break;
    case 6:
#undef ONE
#define ONE "divsd %%xmm1, %%xmm0\n\t"
        _div_vf
        break;
    case 7:
#undef ONE
#define ONE "divss %%xmm1, %%xmm0\n\t"
        _div_vf
        break;
    default:
        break;
    }

    return res;
}

uint64_t mul_vf_test(long arg1) {
    double dbl[2] = {1.0, 2.0};
    float sng[2] = {1.0, 2.0};
    double begin = second();
    uint64_t res = mul_vf(arg1, dbl, sng);
    double end = second();
    printf("simple mul_vf test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 39 madd_vf
#define _madd_vf \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "movsd (%2), %%xmm1\n\t" \
        "movss (%3), %%xmm2\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT), "r"(dbl), "r"(sng), "m"(dbl), "m"(sng) \
        :   "rax", "rbx", "memory", "r15", "xmm0", "xmm1", "xmm2");

uint64_t madd_vf(long arg1, const double *dbl, float *sng){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "mulpd %%xmm1, %%xmm0\n\taddpd %%xmm2, %%xmm0\n\t"
        _madd_vf
        break;
    case 1:
#undef ONE
#define ONE "mulps %%xmm1, %%xmm0\n\taddps %%xmm2, %%xmm0\n\t"
        _madd_vf
        break;
    case 2:
#undef ONE
#define ONE "mulsd %%xmm1, %%xmm0\n\taddsd %%xmm2, %%xmm0\n\t"
        _madd_vf
        break;
    case 3:
#undef ONE
#define ONE "mulss %%xmm1, %%xmm0\n\taddss %%xmm2, %%xmm0\n\t"
        _madd_vf
        break;
    case 4:
#undef ONE
#define ONE "movss %4, %%xmm0\n\t" \
            "mulss %%xmm1, %%xmm0\n\taddss %%xmm2, %%xmm0\n\t"
        _madd_vf
        break;
    default:
        break;
    }

    return res;
}

uint64_t madd_vf_test(long arg1) {
    double dbl[2] = {1.0, 2.0};
    float sng[2] = {1.0, 2.0};
    double begin = second();
    uint64_t res = madd_vf(arg1, dbl, sng);
    double end = second();
    printf("simple madd_vf test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 40 shift
#define _shift \
    asm volatile(   \
        "mov %1, %%rbx\n\t" \
        "1: \n\t"           \
        __INNER_COUNT_NUM   \
        "0: sub $1, %%rbx\n\t" \
        "jnz 1b \n\t"       \
        "mov %%rax, %0\n\t" \
        : "=r" (res)        \
        : "r"(__LOOP_COUNT)   \
        :   "rax", "rbx");


uint64_t shift(long arg1){
    uint64_t res;
    switch (arg1)
    {
    case 0:
#undef ONE
#define ONE "mov $128, %%rdx\n\t" \
            "shr $7, %%dl\n\t" \
            "add $1, %%rdx\n\t"
        _shift
        break;
    case 1:
#undef ONE
#define ONE "mov $128, %%rdx\n\t" \
            "add $7, %%dl\n\t" \
            "add $1, %%rdx\n\t"
        _shift
        break;
    case 2:
#undef ONE
#define ONE "mov $128, %%rdx\n\t" \
            "shr $7, %%edx\n\t" \
            "add $1, %%rdx\n\t"
        _shift
        break;
    }
    return res;
}

uint64_t shift_test(long arg1) {
    double begin = second();
    uint64_t res = shift(arg1);
    double end = second();
    printf("simple shift test:%f ns\n", (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 41 test_jcc
#define test_jcc       \
    asm volatile(            \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %1 \n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0" \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT), "r"(base), "r"(index1), "r"(index2)  \
        : "rbx", "memory");

uint64_t test_jcc_test(long arg1) {
    uint64_t res;
    long base = (long)aligned_alloc(64, 1 << 20);
    long index1 = 1 << 10;
    long index2 = base / 4 + 64;
    double begin = second();
    switch (arg1)
    {
    case 0: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jae 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 1: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jbe 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 2: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jge 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 3: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jle 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 4: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test 0x1004(,%4,4), %%rax\n\t" \
            "jo 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 5: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "js 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 6: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jp 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 7: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jae 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 8: {
        
#undef ONE
#define ONE "sub $1, %4\n\tadd $1, %4\n\t" \
            "test (,%4,4), %%rax\n\t" \
            "jne 2f\n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 9: {
        
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "test $10, %%al\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 10: {
        
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "test $10, %%ax\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 11: {
        
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "mov $20, %%rbx\n\t" \
            "test %%eax, %%ebx\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    case 12: {
#undef ONE
#define ONE "mov $10, %%rax\n\t" \
            "test $10, %%rax\n\t" \
            "jz 2f \n\t" \
            "2: \n\t"
        test_jcc
        break;
    }
    default:
        break;
    }
    double end = second();
    printf("simple test_jcc_%ld:%f ns\n", arg1, (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 42 test_jcc_reg
#define test_jcc_reg       \
    asm volatile(            \
        "mov %1, %%rcx\n\t"    \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %%rcx\n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0" \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT), "r"(base), "r"(index1), "r"(index2)  \
        : "rbx", "rcx", "memory");

uint64_t test_jcc_reg_test(long arg1) {
    uint64_t res;
    long base = (long)aligned_alloc(64, 1 << 20);
    long index1 = 1 << 10;
    long index2 = base / 4 + 64;
    double begin = second();
    switch (arg1) {
    case 0: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "je 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 1: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "jo 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 2: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "jc 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 3: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "ja 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 4: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "js 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 5: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "jp 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 6: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "jge 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 7: {
#undef ONE
#define ONE "test %%rbx, %%rax\n\t" \
            "jng 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 8: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "je 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 9: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "jo 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 10: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "jc 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 11: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "ja 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 12: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "js 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 13: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "jp 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 14: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "jge 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    case 15: {
#undef ONE
#define ONE "dec %%rbx\n\t" \
            "test %%rbx, %%rax\n\t" \
            "jng 2f\n\t" \
            "2: \n\t"
        test_jcc_reg
        break;
    }
    }
    double end = second();
    printf("simple test_jcc_reg_%ld:%f ns\n", arg1, (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

// 43 nop
#define _nop      \
    asm volatile(            \
        "mov %1, %%rcx\n\t"    \
        "1: \n\t"            \
        __INNER_COUNT_NUM    \
        "0: sub $1, %%rcx\n\t" \
        "jnz 1b \n\t"        \
        "mov %%rbx, %0" \
        : "=r"(res)          \
        : "r"(__LOOP_COUNT) \
        : "rbx", "rcx", "memory");

uint64_t nop_test(long arg1) {
    uint64_t res;
    double begin = second();
    switch (arg1) {
    case 0: {
#undef ONE
#define ONE ".byte 0x0f, 0x1f, 0x00\n\t"
        _nop
        break;
    }
    case 1: {
#undef ONE
#define ONE "endbr32\n\t"
        _nop
        break;
    }
    }
    double end = second();
    printf("simple nop_%ld:%f ns\n", arg1, (end - begin) / ((__LOOP_COUNT * __INNER_COUNT) / 1000000000.0));
    return res;
}

int main(int argc, char* argv[]) {
    if ((size_t)dummy % 4096) {
        fprintf(stderr, "dummy align error\n");
        abort();
    }
#if defined (__APPLE__) && defined (__MACH__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    // pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0);
#endif

    *(size_t*)dummy = (size_t)dummy;

    // prepare();
    if (argc < 2) {
        printf("env N for loop_cnt\n");
        printf("Usage: freq_test       ./a.out 1\n");
        printf("Usage: ld_tp_test      ./a.out 2\n");
        printf("Usage: pointer_chasing ./a.out 3 disp(default 0)\n");
        printf("Usage: st2ld           ./a.out 4\n");
        return 0;
    }
    char* loop_cnt = getenv("N");
    if(loop_cnt) {
        __LOOP_COUNT = atol(loop_cnt);
    }

    uint64_t test_item = atoll(argv[1]);
    uint64_t arg1 = 0, arg2 = 0;
    if (argc > 2) {arg1 = atoll(argv[2]);}
    if (argc > 3) {arg2 = atoll(argv[3]);}
    uint64_t result;
    switch (test_item)
    {
    case 1:
        result = freq_test();
        break;
    case 2:
        result = bcc_test();
        break;
    case 3:
        result = jr_test();
        break;
    case 4:
        result = call_ret_test();
        break;
    case 5:
        if (argc <= 2) {abort();}
        result = base_index_imm_test(arg1);
        break;
    case 6:
        result = push_pop_test();
        break;
    case 7:
        result = lahf_test();
        break;
    case 8:
        if (argc <= 2) {abort();}
        result = partial_reg_test(arg1);
        break;
    case 9:
        if (argc <= 2) {abort();}
        result = load_imm32_test(arg1);
        break;
    case 10:
        if (argc <= 2) {abort();}
        result = load_imm64_test(arg1);
        break;
    case 11:
        if (argc <= 2) {abort();}
        result = extension_test(arg1);
        break;
    case 12:
        result = regs_map_test();
        break;
    case 13:
        if (argc <= 2) {abort();}
        result = jcc_test(arg1);
        break;
    case 14:
        result = call_reg_ret_test();
        break;
    case 15:
        if (argc <= 2) {abort();}
        result = mul_test(arg1);
        break;
    case 16:
        if (argc <= 2) {abort();}
        result = add_vf_test(arg1);
        break;
    case 17:
        if (argc <= 2) {abort();}
        result = comiss_test(arg1);
        break;
    case 18:
        if (argc <= 2) {abort();}
        result = cmp_jcc_test(arg1);
        break;
    case 19:
        if (argc <= 2) {abort();}
        result = cmovcc_test(arg1);
        break;
    case 20:
        if (argc <= 2) {abort();}
        result = ldst_pair_test(arg1);
        break;
    case 21:
        result = pushpop_pair_test();
        break;
    case 22:
        result = div_test();
        break;
    case 23:
        result = movss_test();
        break;
    case 24:
        if (argc <= 2) {abort();}
        result = jmp_direct_test(arg1);
        break;
    case 25:
        result = mov_test();
        break;
    case 26:
        if (argc <= 2) {abort();}
        result = lea_test(arg1);
        break;
    case 27:
        result = and_test();
        break;
    case 28:
        if (argc <= 2) {abort();}
        result = shl_test(arg1);
        break;
    case 29:
        if (argc <= 2) {abort();}
        result = imul_test(arg1);
        break;
    case 30:
        if (argc <= 2) {abort();}
        result = movs_test(arg1);
        break;
    case 31:
        if (argc <= 2) {abort();}
        result = movs1024_test(arg1);
        break;
    case 32:
        if (argc <= 2) {abort();}
        result = cmp_jcc_reg_test(arg1);
        break;
    case 33:
        if (argc <= 2) {abort();}
        result = movfp_test(arg1);
        break;
    case 34:
        if (argc <= 2) {abort();}
        result = div_vf_test(arg1);
        break;
    case 35:
        result = idiv_test();
        break;
    case 36:
        if (argc <= 2) {abort();}
        result = pxor_test(arg1);
        break;
    case 37:
        if (argc <= 2) {abort();}
        result = cvt_vf_test(arg1);
        break;
    case 38:
        if (argc <= 2) {abort();}
        result = mul_vf_test(arg1);
        break;
    case 39:
        if (argc <= 2) {abort();}
        result = madd_vf_test(arg1);
        break;
    case 40:
        if (argc <= 2) {abort();}
        result = shift_test(arg1);
        break;
    case 41:
        if (argc <= 2) {abort();}
        result = test_jcc_test(arg1);
        break;
    case 42:
        if (argc <= 2) {abort();}
        result = test_jcc_reg_test(arg1);
        break;
    case 43:
        if (argc <= 2) {abort();}
        result = nop_test(arg1);
        break;
    default:
        break;
    }

    return 0;
}
