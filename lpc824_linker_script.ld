/* useful reference: www.linuxselfhelp.com/gnu/ld/html_chapter/ld_toc.html */

MEMORY
{
    flash : org = 0x00000000, len = 32k
    ram : org = 0x10000000, len = 8k
}

/* define stack size and heap size here */
stack_size = 1024;
heap_size = 1024;

/* define beginning and ending of stack */
PROVIDE(_stack_start = ORIGIN(ram)+LENGTH(ram));
PROVIDE(_stack_end = _stack_start - stack_size);
PROVIDE(_heap_start = _stack_end - heap_size);

GROUP(libgcc.a libc_nano.a libm.a)
/* GROUP(libgcc.a libc.a libm.a) */
/* GROUP(libgcc.a libc_nano.a) */

SECTIONS
{
	. = ORIGIN(flash);

    .vectors :
    {
        KEEP(*o(.vectors_))
    } >flash
	
	.text :
	{
       . = ALIGN(4);
        _start_text = .;
        /* *(.vectors);  The interrupt vectors */
		*(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        _end_text = .;
	} >flash

    .ARM.extab :
    {
		*(.ARM.extab* .gnu.linkonce.armextab.*)
    } > flash

    __exidx_start = .;
    .ARM.exidx :
    {
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash
    __exidx_end = .;

	. = ORIGIN(ram);
	. = ALIGN (4);
	.data :
	{
		INIT_DATA_VALUES = LOADADDR(.data);
		INIT_DATA_START = .;
		*(.data)
        *(.data*)
		INIT_DATA_END = .;
	} >ram AT>flash
	
    . = ALIGN (4);
    BSS_START = .;
	.bss : {
		__bss_start__ = .;
        *(.bss) 
		*(.bss*)
		*(COMMON)
		_ebss = .;         /* define a global symbol at bss end */
		__bss_end__ = .; 
	} > ram
	BSS_END = .;
	
/*   . = ALIGN(4);*/
/*   .heap : {*/
/*       _heap_start = .;*/
/*       . = . + heap_size;*/
/*       _heap_end = .;*/
/*   } > ram*/
}
