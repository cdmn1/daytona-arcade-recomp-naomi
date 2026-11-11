/*
 * ROM-free Naomi input probe. Reads KOS Maple status directly so it can show
 * both the MIE/JVS bridge and independent standard Maple controllers.
 */
#include <stdio.h>
#include <kos.h>
#include <kos/init.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/mie.h>

/* INIT_DEFAULT includes MIE automatically on the Naomi sub-architecture. */
KOS_INIT_FLAGS(INIT_DEFAULT);

int main(void) {
    puts("Daytona Naomi input probe");
#if defined(_arch_sub_naomi)
    puts("KOS sub-architecture: Naomi");
#else
    puts("WARNING: not compiled for Naomi");
#endif

    for(;;) {
        int found_mie = 0;
        int found_pad = 0;
        int i;
        maple_device_t *dev;

        printf("\033[2J\033[H");
        puts("Daytona Naomi input probe");
#if defined(_arch_sub_naomi)
        printf("Port A mode: %s\n",
               mie_port0_mode() == MIE_PORT0_JVS ? "JVS" :
               mie_port0_mode() == MIE_PORT0_MAPLE ? "Maple" : "unknown");

        for(i = 0; (dev = maple_enum_type(i, MAPLE_FUNC_MIE)) != NULL; ++i) {
            mie_state_t *mie = (mie_state_t *)maple_dev_status(dev);
            if(!mie) continue;
            found_mie = 1;
            printf("MIE/JVS port %u unit %u\n",
                   (unsigned)dev->port, (unsigned)dev->unit);
            printf("  mapped buttons=%08lx joyx=%d ltrig=%d rtrig=%d\n",
                   (unsigned long)mie->cont.buttons, mie->cont.joyx,
                   mie->cont.ltrig, mie->cont.rtrig);
            printf("  wheel=%u accel=%u brake=%u\n",
                   (unsigned)mie->jvs.wheel, (unsigned)mie->jvs.accel,
                   (unsigned)mie->jvs.brake);
            printf("  p1=%04x p2=%04x panel=%02x/%02x\n",
                   mie->jvs.p1.raw, mie->jvs.p2.raw,
                   mie->jvs.panel.dip.raw, mie->jvs.panel.psw.raw);
            printf("  coins=%u,%u raw=%04x,%04x\n",
                   (unsigned)mie->jvs.coin[0].count,
                   (unsigned)mie->jvs.coin[1].count,
                   mie->jvs.coin[0].raw, mie->jvs.coin[1].raw);
        }
#else
        puts("MIE/JVS details require the Naomi KOS sub-architecture");
#endif

        for(i = 0; (dev = maple_enum_type(i, MAPLE_FUNC_CONTROLLER)) != NULL; ++i) {
            cont_state_t *pad = (cont_state_t *)maple_dev_status(dev);
            if(!pad) continue;
            found_pad = 1;
            printf("Maple controller port %u unit %u\n",
                   (unsigned)dev->port, (unsigned)dev->unit);
            printf("  buttons=%08lx joy=(%d,%d) triggers=(%d,%d)\n",
                   (unsigned long)pad->buttons, pad->joyx, pad->joyy,
                   pad->ltrig, pad->rtrig);
        }

        if(!found_mie) puts("MIE/JVS: not detected");
        if(!found_pad) puts("Maple gamepad: not detected");
        puts("Refreshes every 250 ms; see the serial/netboot console.");
        thd_sleep(250);
    }
    return 0;
}
