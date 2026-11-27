/*
 * htd_elfdl 模块链接脚本
 *
 * loader 安装时把整个文件按 vaddr == file_offset 的恒等映射拷进程序空间,
 * 所以模块必须是单段 LOAD, 且 p_offset == p_vaddr, 且不能有 bss.
 * 链接参数: -nostdlib -shared -N -Wl,-z,max-page-size=4 -T mod.lds
 */
ENTRY(elfdl_entry)
SECTIONS
{
  . = SIZEOF_HEADERS;
  .hash    : { *(.hash) }
  .dynsym  : { *(.dynsym) }
  .dynstr  : { *(.dynstr) }
  .gnu.version   : { *(.gnu.version) }
  .gnu.version_r : { *(.gnu.version_r) }
  .rel.dyn : { *(.rel.dyn) }
  .rel.plt : { *(.rel.plt) }
  .init_array : { *(.init_array) }
  .fini_array : { *(.fini_array) }
  .plt     : { *(.plt) }
  .text    : { *(.text .text.*) }
  .rodata  : { *(.rodata .rodata.*) }
  .dynamic : { *(.dynamic) }
  .got     : { *(.got .got.*) }
  .data    : { *(.data .data.*) }
  .bss     : { *(.bss .bss.*) *(COMMON) }

  /DISCARD/ : { *(.note*) *(.comment) *(.ARM.attributes) *(.eh_frame*) }
}
