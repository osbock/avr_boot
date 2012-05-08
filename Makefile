#------------------------------------------------------------------
# Makefile for stand-alone MMC boot strap loader
#------------------------------------------------------------------
# Change these three defs for the target device

MCU_TARGET  = atmega328p	# Target device to be used (32K or lager)
BOOT_ADR    = 0x7000	# Boot loader start address [byte]
F_CPU       =16000000	# CPU clock frequency [Hz]

#------------------------------------------------------------------

TARGET      = avr_boot
CSRC        = main.c pff.c mmc.c
ASRC        = asmfunc.S
OPTIMIZE    = -Os -mcall-prologues
DEFS        = -DBOOT_ADR=$(BOOT_ADR) -DF_CPU=$(F_CPU)
LIBS        =
DEBUG       = dwarf-2

ASFLAGS     = -Wa,-adhlns=$(<:.S=.lst),-gstabs $(DEFS)
ALL_ASFLAGS = -mmcu=$(MCU_TARGET) -I. -x assembler-with-cpp $(ASFLAGS)
CFLAGS      = -g$(DEBUG) -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS     = -Wl,-Map,$(TARGET).map -Wl,--section-start,.text=$(BOOT_ADR)
OBJ         = $(CSRC:.c=.o) $(ASRC:.S=.o)

CC          = avr-gcc
OBJCOPY     = avr-objcopy
OBJDUMP     = avr-objdump
SIZE        = avr-size


all:	$(TARGET).elf lst text size

$(TARGET).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)


clean:
	rm -rf *.o $(TARGET).elf *.eps *.bak *.a
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)
	rm -rf $(TARGET).hex

size: $(TARGET).elf
	$(SIZE) -C --mcu=$(MCU_TARGET) $(TARGET).elf

lst:  $(TARGET).lst
%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

%.o : %.S
	$(CC) -c $(ALL_ASFLAGS) $< -o $@


text: $(TARGET).hex

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -j .fuse -O ihex $< $@
