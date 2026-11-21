// *** flash.h ********************************************************

void
flash_erase_pages(uint32 *addr, uint32 npages);

void
flash_write_words(uint32 *addr, const uint32 *data, uint32 nwords);

void
flash_upgrade(void);

// N.B. runs from RAM; the __longramfunc__ attributes are on its definition,
// so only call it after that (as flash_upgrade() does)
void
__attribute__((noreturn))
flash_upgrade_ram(void);

void
flash_initialize(void);

