/*  n64bootloader, a Linux bootloader for the N64
    Copyright (C) 2020 Lauri Kasanen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>

typedef uint64_t u64;
typedef unsigned int u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef signed int s32;
typedef int16_t s16;
typedef int8_t s8;

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t  Elf32_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;

#define EI_NIDENT (16)

#define EI_CLASS        4               /* File class byte index */
#define ELFCLASSNONE    0               /* Invalid class */
#define ELFCLASS32      1               /* 32-bit objects */
#define ELFCLASS64      2               /* 64-bit objects */
#define ELFCLASSNUM     3

typedef struct
{
  unsigned char e_ident[EI_NIDENT];     /* Magic number and other info */
  Elf32_Half    e_type;                 /* Object file type */
  Elf32_Half    e_machine;              /* Architecture */
  Elf32_Word    e_version;              /* Object file version */
  Elf32_Addr    e_entry;                /* Entry point virtual address */
  Elf32_Off     e_phoff;                /* Program header table file offset */
  Elf32_Off     e_shoff;                /* Section header table file offset */
  Elf32_Word    e_flags;                /* Processor-specific flags */
  Elf32_Half    e_ehsize;               /* ELF header size in bytes */
  Elf32_Half    e_phentsize;            /* Program header table entry size */
  Elf32_Half    e_phnum;                /* Program header table entry count */
  Elf32_Half    e_shentsize;            /* Section header table entry size */
  Elf32_Half    e_shnum;                /* Section header table entry count */
  Elf32_Half    e_shstrndx;             /* Section header string table index */
} Elf32_Ehdr;

typedef struct
{
  Elf32_Word    p_type;                 /* Segment type */
  Elf32_Off     p_offset;               /* Segment file offset */
  Elf32_Addr    p_vaddr;                /* Segment virtual address */
  Elf32_Addr    p_paddr;                /* Segment physical address */
  Elf32_Word    p_filesz;               /* Segment size in file */
  Elf32_Word    p_memsz;                /* Segment size in memory */
  Elf32_Word    p_flags;                /* Segment flags */
  Elf32_Word    p_align;                /* Segment alignment */
} Elf32_Phdr;

volatile u32 frames;

display_context_t lockVideo(int wait)
{
	display_context_t dc;

	if (wait)
		while (!(dc = display_lock()));
	else
		dc = display_lock();
	return dc;
}

void unlockVideo(display_context_t dc)
{
	if (dc)
		display_show(dc);
}

/* text functions */
void printText(display_context_t dc, const char *msg, int x, int y)
{
	if (dc)
		graphics_draw_text(dc, x*8, y*8, msg);
}

/* vblank callback */
void vblCallback(void)
{
	frames++;
}

void delay(u32 cnt)
{
	u32 then = frames + cnt;
	while (then > frames);
}

/* initialize console hardware */
void init_n64(void)
{
	/* enable interrupts (on the CPU) */
	init_interrupts();

	/* Initialize peripherals */
	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);

	register_VI_handler(vblCallback);
}

extern int __bootcic;

static display_context_t disp;

static void err(const char msg[]) {
	u32 color;

	disp = lockVideo(1);
	color = graphics_make_color(0xCC, 0xCC, 0xCC, 0xFF);
	graphics_fill_screen(disp, color);

	color = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
	graphics_set_color(color, 0);
	printText(disp, msg, 4, 5);
	unlockVideo(disp);

	while (1);
}

static u8 hdrbuf[256] __attribute__((aligned(8)));

static const char * const args[] = { "hello",
					(const char *) hdrbuf,
					(const char *) hdrbuf + 128 };
static const char * const env[] = { NULL };

/* main code entry point */
int main(void)
{
	init_n64();

	const int osMemSize = (__bootcic != 6105) ? (*(int*)0xA0000318) : (*(int*)0xA00003F0);
	char buf[64];

	u32 color;

	disp = lockVideo(1);
	color = graphics_make_color(0xCC, 0xCC, 0xCC, 0xFF);
	graphics_fill_screen(disp, color);

	color = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
	graphics_set_color(color, 0);

	if (osMemSize / 1024 / 1024 != 8) {
		printText(disp, "Expansion pak required", 4, 4);

		unlockVideo(disp);

		while (1);
	}

	u32 kernelsize __attribute__((aligned(8)));
	u32 disksize __attribute__((aligned(8)));
	data_cache_hit_writeback_invalidate(&kernelsize, 4);
	dma_read(&kernelsize, 0xB0101000 - 4, 4);

	data_cache_hit_writeback_invalidate(&disksize, 4);
	dma_read(&disksize, 0xB0101000 - 8, 4);

	if (!kernelsize) {
		printText(disp, "No kernel configured", 4, 4);

		unlockVideo(disp);

		while (1);
	}

	sprintf(buf, "Booting kernel %u kb, %u kb", kernelsize / 1024,
			disksize / 1024);
	printText(disp, buf, 4, 4);

	unlockVideo(disp);
	delay(1);

	disable_interrupts();
	set_VI_interrupt(0, 0);

	Elf32_Ehdr * const ptr = (Elf32_Ehdr *) hdrbuf;

	dma_read(ptr, 0xB0101000, 256);
	data_cache_hit_invalidate(ptr, 256);

	if (ptr->e_ident[1] != 'E' ||
		ptr->e_ident[2] != 'L' ||
		ptr->e_ident[3] != 'F')
		err("Not an ELF kernel?");

	if (ptr->e_ident[EI_CLASS] != ELFCLASS32)
		err("Not a 32-bit kernel?");

	// Where is it wanted?
	const Elf32_Phdr *phdr = (Elf32_Phdr *) (hdrbuf + ptr->e_phoff);
	while (phdr->p_type != 1) phdr++;

	// Put it there
	dma_read((void *) phdr->p_paddr, 0xB0101000 + phdr->p_offset,
			(phdr->p_filesz + 1) & ~1);
	data_cache_hit_invalidate((void *) phdr->p_paddr, (phdr->p_filesz + 3) & ~3);

	// Zero any extra memory desired
	if (phdr->p_filesz < phdr->p_memsz) {
		memset((void *) (phdr->p_paddr + phdr->p_filesz), 0,
			phdr->p_memsz - phdr->p_filesz);
	}

	void (*funcptr)(int, const char * const *, const char * const *, int *) = (void *) ptr->e_entry;

	// Fill out our disk info
	sprintf((char *) hdrbuf, "n64cart.start=%u", 0xB0101000 + ((kernelsize + 4095) & ~4095));
	sprintf((char *) hdrbuf + 128, "n64cart.size=%u", disksize);

	funcptr(sizeof(args) / sizeof(args[0]), args, env, NULL /* unused */);

	return 0;
}
