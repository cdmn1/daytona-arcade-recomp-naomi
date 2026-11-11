#ifndef NAOMI_HOST_STUB_KOS_IRQ_H
#define NAOMI_HOST_STUB_KOS_IRQ_H
#include <stdint.h>
typedef uint32_t irq_mask_t;
static inline irq_mask_t irq_disable(void) { return 0; }
static inline void irq_restore(irq_mask_t state) { (void)state; }
#endif
