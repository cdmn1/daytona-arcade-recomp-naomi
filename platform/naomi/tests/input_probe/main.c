/*
 * Naomi JVS input probe. Explicitly enables KOS's MIE/JVS driver and reports
 * native cabinet inputs; Maple gamepad input is intentionally out of scope.
 */
#include <stdio.h>
#include <string.h>
#include <kos.h>
#include <kos/init.h>
#include <kos/dbgio.h>
#include <dc/fb_console.h>
#include <dc/video.h>
#include <dc/maple.h>
#include <dc/maple/mie.h>

/* KOS requires INIT_MIE explicitly; INIT_DEFAULT alone does not enable it. */
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MIE);

static void clear_screen(void) {
    int width = vid_mode ? vid_mode->width : 640;
    int height = vid_mode ? vid_mode->height : 480;

    memset(vram_s, 0, (size_t)width * (size_t)height * sizeof(uint16_t));
    dbgio_fb_set_target(NULL, width, height, 32, 32);
}

int main(void) {
    /* Naomi has a KOS framebuffer console backed by its built-in minifont. */
    dbgio_dev_select("fb");

    puts("Daytona Naomi input probe");
#if defined(_arch_sub_naomi)
    puts("KOS sub-architecture: Naomi");
#else
    puts("WARNING: not compiled for Naomi");
#endif

    for(;;) {
        int found_mie = 0;
        int i;
        maple_device_t *dev;

        clear_screen();
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
                   (unsigned)mie->jvs.p1.raw, (unsigned)mie->jvs.p2.raw,
                   (unsigned)mie->jvs.panel.dip.raw,
                   (unsigned)mie->jvs.panel.psw.raw);
            printf("  coins=%u,%u raw=%04x,%04x\n",
                   (unsigned)mie->jvs.coin[0].count,
                   (unsigned)mie->jvs.coin[1].count,
                   (unsigned)mie->jvs.coin[0].raw,
                   (unsigned)mie->jvs.coin[1].raw);
            printf("  coin pulse=%02x fault=%02x system=%02x\n",
                   (unsigned)mie->jvs.coin_pulse,
                   (unsigned)mie->jvs.coin_fault,
                   (unsigned)mie->jvs.system.raw);
        }
#else
        puts("MIE/JVS details require the Naomi KOS sub-architecture");
#endif

        if(!found_mie) {
            puts("MIE/JVS: not detected by KOS");
            puts("Check INIT_MIE and port A JVS detection");
        } else {
            puts("MIE/JVS device detected; native values above");
        }

        puts("Refreshes every 250 ms.");
        thd_sleep(250);
    }
    return 0;
}
