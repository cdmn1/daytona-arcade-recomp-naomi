ifeq ($(KOS_SUBARCH),)
$(error KOS_SUBARCH is unset. Source the Naomi KOS environ.sh first)
endif
ifneq ($(KOS_SUBARCH),naomi)
$(error Wrong KOS target '$(KOS_SUBARCH)'. This test program requires KOS_SUBARCH=naomi)
endif
ifeq ($(KOS_BASE),)
$(error KOS_BASE is unset. Source the Naomi KOS environ.sh first)
endif
ifeq ($(RECORD_DATA_DIR),)
$(error RECORD_DATA_DIR is required (generated race_record_data.h))
endif

ROOT := ../../../../platform/dreamcast-native
AICAFLOW_ROOT ?= ../../../../extern/aicaflow
SH4ZAM_ROOT ?= ../../../../extern/sh4zam
AICAFLOW_LIB := $(AICAFLOW_ROOT)/driver/sh4/libaicaflow_host.a
SH4ZAM_LIB := $(SH4ZAM_ROOT)/lib/libsh4zam.a
TARGET := daytona_game_test.elf
TEST_ROM := daytona_game_test.bin
OBJS := test_main.o save_data.o save_store.o naomi_save.o naomi_cart.o naomi_romfs.o sound.o sfx.o nm_mem.o
NAOMIBINTOOL ?= $(KOS_BASE)/utils/naomibintool/naomibintool
# The Game Test uses the same streamed announcer pack as the main Naomi game.
# Its sound driver and phrase reader are built into the BIOS-launched test executable.
CFLAGS += -DNM_SFX_VOICE_STREAM=1 -I$(ROOT)/include -I$(RECORD_DATA_DIR) -I../../include -I$(AICAFLOW_ROOT)/driver/include -I$(AICAFLOW_ROOT)/driver/format/include -I$(AICAFLOW_ROOT)/driver/sh4/include -I$(SH4ZAM_ROOT)/include --embed-dir=$(AICAFLOW_ROOT)/firmware -D__DREAMCAST__=1 -std=gnu11 -Wall -Wextra
LDFLAGS += -Wl,--wrap=snd_init -Wl,--wrap=snd_shutdown
LDLIBS += $(AICAFLOW_LIB) $(SH4ZAM_LIB)

all: $(TARGET)

$(TARGET): $(OBJS) $(AICAFLOW_LIB) $(SH4ZAM_LIB)
	kos-cc $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(AICAFLOW_LIB):
	$(MAKE) -C $(AICAFLOW_ROOT)/driver/sh4

$(SH4ZAM_LIB):
	@test -f $@ || { echo "Missing SH4ZAM library: run platform/dreamcast-native/scripts/fetch_sh4zam.sh"; exit 1; }

test_main.o: test_main.c
	kos-cc $(CFLAGS) -c $< -o $@

save_data.o: $(ROOT)/code/save/save_data.c
	kos-cc $(CFLAGS) -c $< -o $@

save_store.o: $(ROOT)/code/save/save_store.c
	kos-cc $(CFLAGS) -c $< -o $@

naomi_save.o: ../../code/naomi_save.c
	kos-cc $(CFLAGS) -c $< -o $@

naomi_cart.o: ../../code/naomi_cart.c
	kos-cc $(CFLAGS) -c $< -o $@

naomi_romfs.o: ../../code/naomi_romfs.c
	kos-cc $(CFLAGS) -c $< -o $@

sound.o: $(ROOT)/code/audio/sound.c
	kos-cc $(CFLAGS) -c $< -o $@

sfx.o: $(ROOT)/code/audio/sfx.c
	kos-cc $(CFLAGS) -c $< -o $@

nm_mem.o: $(ROOT)/code/nm_mem.c
	kos-cc $(CFLAGS) -DNDEBUG -c $< -o $@

clean:
	-rm -f $(TARGET) $(TEST_ROM) $(OBJS)

.PHONY: all clean
