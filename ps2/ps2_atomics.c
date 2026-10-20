// PS2 (R5900, single core) shim for the libatomic-style builtins GCC cannot
// inline on this CPU. Atomicity is provided by masking interrupts, which is
// sufficient on a single-core machine (preemption only happens via interrupts).
#include <kernel.h>
#include <stdbool.h>
#include <stdint.h>

#define ATOMIC_BEGIN int __ints_were_on = DIntr()
#define ATOMIC_END   do { if (__ints_were_on) EIntr(); } while (0)

bool __atomic_test_and_set(volatile void *ptr, int memorder) {
    (void)memorder;
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    ATOMIC_BEGIN;
    unsigned char old = *p;
    *p = 1;
    ATOMIC_END;
    return old != 0;
}

#define DEFINE_ATOMICS(N, T)                                                              \
T __atomic_load_##N(const volatile void *p, int mo) {                                     \
    (void)mo; ATOMIC_BEGIN; T v = *(const volatile T *)p; ATOMIC_END; return v; }         \
void __atomic_store_##N(volatile void *p, T v, int mo) {                                  \
    (void)mo; ATOMIC_BEGIN; *(volatile T *)p = v; ATOMIC_END; }                           \
T __atomic_exchange_##N(volatile void *p, T v, int mo) {                                  \
    (void)mo; ATOMIC_BEGIN; T o = *(volatile T *)p; *(volatile T *)p = v; ATOMIC_END; return o; } \
bool __atomic_compare_exchange_##N(volatile void *p, void *expected, T desired,           \
                                   bool weak, int s, int f) {                             \
    (void)weak; (void)s; (void)f; ATOMIC_BEGIN;                                           \
    T cur = *(volatile T *)p; bool ok = (cur == *(T *)expected);                          \
    if (ok) *(volatile T *)p = desired; else *(T *)expected = cur;                        \
    ATOMIC_END; return ok; }                                                              \
T __atomic_fetch_add_##N(volatile void *p, T v, int mo) {                                 \
    (void)mo; ATOMIC_BEGIN; T o = *(volatile T *)p; *(volatile T *)p = o + v; ATOMIC_END; return o; } \
T __atomic_fetch_sub_##N(volatile void *p, T v, int mo) {                                 \
    (void)mo; ATOMIC_BEGIN; T o = *(volatile T *)p; *(volatile T *)p = o - v; ATOMIC_END; return o; } \
T __atomic_fetch_and_##N(volatile void *p, T v, int mo) {                                 \
    (void)mo; ATOMIC_BEGIN; T o = *(volatile T *)p; *(volatile T *)p = o & v; ATOMIC_END; return o; } \
T __atomic_fetch_or_##N(volatile void *p, T v, int mo) {                                  \
    (void)mo; ATOMIC_BEGIN; T o = *(volatile T *)p; *(volatile T *)p = o | v; ATOMIC_END; return o; } \
T __atomic_fetch_xor_##N(volatile void *p, T v, int mo) {                                 \
    (void)mo; ATOMIC_BEGIN; T o = *(volatile T *)p; *(volatile T *)p = o ^ v; ATOMIC_END; return o; }

DEFINE_ATOMICS(1, uint8_t)
DEFINE_ATOMICS(2, uint16_t)
DEFINE_ATOMICS(4, uint32_t)
DEFINE_ATOMICS(8, uint64_t)
