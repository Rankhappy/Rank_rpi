#ifndef ARM_H
#define ARM_H

#define PSR_MODE_MASK   (0x1f)
#define PSR_MODE_USER   (0x10)
#define PSR_MODE_FIQ    (0x11) 
#define PSR_MODE_IRQ    (0x12)
#define PSR_MODE_SVC    (0x13)
#define PSR_MODE_MON    (0x16) 
#define PSR_MODE_ABT    (0x17)
#define	PSR_MODE_HYP    (0x1a) 
#define PSR_MODE_UND    (0x1b) 
#define PSR_MODE_SYS    (0x1f) 

#define PSR_F_BIT       (1<<6)
#define PSR_I_BIT       (1<<7)
#define PSR_A_BIT       (1<<8)

#define SCTLR_M         (1 << 0)
#define SCTLR_A         (1 << 1)
#define SCTLR_C         (1 << 2)
#define SCTLR_SW        (1 << 10)
#define SCTLR_Z         (1 << 11)
#define SCTLR_I         (1 << 12)
#define SCTLR_V         (1 << 13)
#define SCTLR_RR        (1 << 14)
#define SCTLR_HA        (1 << 17)
#define SCTLR_EE        (1 << 25)
#define SCTLR_TRE       (1 << 28)
#define SCTLR_AFE       (1 << 29)
#define SCTLR_TE        (1 << 30)     

#define allign_mask(a) ((1<<a)-1)
#define allign_up(x, a) ((x+((1<<a)-1))&(~((1<<a)-1)))
#define allign_down(x, a) (x&(~((1<<a)-1)))

#define PAGE_SHIFT 12
#define SECTION_SHIFT 20
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define SECTION_SIZE (1 << SECTION_SHIFT)

#define PAGE_MASK allign_mask(PAGE_SHIFT)
#define SECTION_MASK allign_mask(SECTION_SHIFT)

#endif

