
BOARD=L432KC
BOARD_FLASH=262144  # 256 KB
BOARD_RAM=65536  # 64 KB

MBED_DEFINES = -DMBED_HEAP_STATS_ENABLED=1 -DMBED_STACK_STATS_ENABLED=1

# INFINITE_LOOP_AFTER_TESTS
# DISABLE_TESTS
CUSTOM_DEFINES = -DIAM_QBOUGE

MBED_COMPILE_OPTIONS = -N out -m NUCLEO_$(BOARD) -t GCC_ARM  $(MBED_DEFINES) $(CUSTOM_DEFINES)

.PHONY: all re build upload

all: build upload

build:
	mbed compile $(MBED_COMPILE_OPTIONS)
	python ../memstats_free.py "$(BOARD)" "$(BOARD_FLASH)" "$(BOARD_RAM)"

stats:
	# nm --print-size --size-sort .build/NUCLEO_$(BOARD)/GCC_ARM/out.elf | tail
	# python mbed-os/tools/memap.py .build/NUCLEO_$(BOARD)/GCC_ARM/out.map -t GCC_ARM -d
	python ../memstats_free.py "$(BOARD)" "$(BOARD_FLASH)" "$(BOARD_RAM)"

soft_re:
	rm -rf .build/NUCLEO_$(BOARD)/GCC_ARM/libs/Galib
	rm -f .build/NUCLEO_$(BOARD)/GCC_ARM/main*
	mbed compile $(MBED_COMPILE_OPTIONS)
	python ../memstats_free.py "$(BOARD)" "$(BOARD_FLASH)" "$(BOARD_RAM)"

re:
	mbed compile -c $(MBED_COMPILE_OPTIONS)
	python ../memstats_free.py "$(BOARD)" "$(BOARD_FLASH)" "$(BOARD_RAM)"

upload:
	st-flash --reset write .build/NUCLEO_$(BOARD)/GCC_ARM/out.bin 0x8000000
	# cf. https://github.com/texane/stlink
	#cp .build/NUCLEO_$(BOARD)/GCC_ARM/out.bin /media/nodraak/NODE_$(BOARD)/

debug_openocd:
	# openocd -f board/st_nucleo_f401re.cfg -c "gdb_memory_map enable" -c "gdb_flash_program enable"
	openocd -f ../Openocd/st_nucleo_f303k8.cfg -c "gdb_memory_map disable"
# -c "gdb_flash_program enable"

debug_gdb:
	/home/nodraak/Documents/Etudes/ECE_ING4/Deoxys/stm32-mbed/gcc-arm-none-eabi-5_4-2016q2/bin/arm-none-eabi-gdb
