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
TARGET := daytona_game_test.elf
TEST_ROM := daytona_game_test.bin
OBJS := test_main.o save_data.o save_store.o naomi_save.o
NAOMIBINTOOL ?= $(KOS_BASE)/utils/naomibintool/naomibintool
CFLAGS += -I$(ROOT)/include -I$(RECORD_DATA_DIR) -I../../include -std=gnu11 -Wall -Wextra

all: $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)

test_main.o: test_main.c
	kos-cc $(CFLAGS) -c $< -o $@

save_data.o: $(ROOT)/code/save/save_data.c
	kos-cc $(CFLAGS) -c $< -o $@

save_store.o: $(ROOT)/code/save/save_store.c
	kos-cc $(CFLAGS) -c $< -o $@

naomi_save.o: ../../code/naomi_save.c
	kos-cc $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(TARGET) $(TEST_ROM) $(OBJS)

.PHONY: all clean
