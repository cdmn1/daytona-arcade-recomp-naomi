/*
 * ROM-free Naomi input probe.
 *
 * KOS INIT_DEFAULT includes INIT_MIE on the Naomi sub-architecture through
 * INIT_MAPLE_ALL. The probe prints both native MIE/JVS state and ordinary
 * Maple controllers; it does not depend on game assets or enDjinn.
 */
#include <stdio.h>
#include <kos.h>
#include <kos/init.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/mie.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

int main(void) {
    puts("Daytona Naomi input probe");
#if defined(_arch_sub_naomi)
    puts("KOS sub-architecture: Naomi");
#else
    puts("WARNING: this binary was not compiled for the Naomi sub-architecture");
#endif

    for(;;) {
        int found_mie = 0;
        int found_pad = 0;

        printf("\033[2J\033[H");
        puts("Daytona Naomi input probe");
        printf("Port A mode: %s\n",
               mie_port0_mode() == MIE_PORT0_JVS ? "JVS" :
               mie_port0_mode() == MIE_PORT0_MAPLE ? "Maple" : "unknown");

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MIE, mie_state_t, mie)
            if(mie) {
                found_mie = 1;
                printf("MIE/JVS port %u unit %u\n",
                       (unsigned)mie->port, (unsigned)mie->unit);
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
        MAPLE_FOREACH_END()

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, pad)
            if(pad) {
                found_pad = 1;
                printf("Maple controller port %u unit %u\n",
                       (unsigned)pad->port, (unsigned)pad->unit);
                printf("  buttons=%08lx joy=(%d,%d) triggers=(%d,%d)\n",
                       (unsigned long)pad->buttons, pad->joyx, pad->joyy,
                       pad->ltrig, pad->rtrig);
            }
        MAPLE_FOREACH_END()

        if(!found_mie) puts("MIE/JVS: not detected");
        if(!found_pad) puts("Maple gamepad: not detected");
        puts("Refreshes every 250 ms. Check serial/netboot console output.");
        thd_sleep(250);
    }

    return 0;
}
