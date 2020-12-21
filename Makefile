.PHONY: all clean

ROOTDIR = /opt/n64
GCCN64PREFIX = mips64-elf-
CHKSUM64PATH = chksum64
MKDFSPATH = mkdfs
N64TOOL = n64tool
LINK_FLAGS = -G0 -L$(ROOTDIR)/mips64-elf/lib --start-group -ldragon \
		-lc -lm -ldragonsys -Tn64ld.x \
		/opt/n64/lib/gcc/mips64-elf/8.2.0/libgcc.a --end-group
CFLAGS = -O3 -march=vr4300 -G0 -Wall -Wextra -I$(ROOTDIR)/mips64-elf/include
ASFLAGS = -mtune=vr4300 -march=vr4300
CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy

PROG_NAME = linux

ROM_EXTENSION = .z64
N64_FLAGS = -l 2M -h header -o $(PROG_NAME)$(ROM_EXTENSION) $(PROG_NAME).bin

FILES = $(wildcard *.c)
OBJS = $(FILES:.c=.o)

all: $(PROG_NAME)$(ROM_EXTENSION)

-include $(wildcard *.d)

$(PROG_NAME)$(ROM_EXTENSION): $(PROG_NAME).elf
	@$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	@rm -f $(PROG_NAME)$(ROM_EXTENSION)
	@util/size2bin vmlinux.32 size.bin
	@$(N64TOOL) -t "Linux" $(N64_FLAGS) -s 1048572B size.bin -s 1M vmlinux.32
	@$(CHKSUM64PATH) $(PROG_NAME)$(ROM_EXTENSION) > /dev/null
	@rm -f $(PROG_NAME).bin

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)

clean:
	rm -f *.v64 *.z64 *.elf $(OBJS) *.bin

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@$(CC) -MM -MP $(CFLAGS) $(CPPFLAGS) $*.c -o $*.d > /dev/null &2>&1
