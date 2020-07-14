/* -*- mode:Fundamental; tab-width: 4 -*- */

OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc)
ENTRY("SecondaryLoaderMain")

SEARCH_DIR(/lib)
SEARCH_DIR(/usr/lib)
SEARCH_DIR(/usr/local/lib)
SEARCH_DIR(/usr/ppc-linux-elf/lib)

SECTIONS {

	text 7M: {

		LoadBase = .;

		/* A relative branch instruction to get to our "main" from the first byte */
		LONG(0x48000000 + SecondaryLoaderMain - LoadBase);

		*(.text) *(.rodata)
		*(.data)
		*(.bss) *(.sdata) *(.sbss) *(COMMON)
		end = .;
	}

	/* The symbols and comments just waste precious space */
	"/DISCARD/": {
		*(.stabstr) *(.stab) *(.comment)
	}
}
