#ifndef _HGIC_SYSCFG_H_
#define _HGIC_SYSCFG_H_

#define SYSCFG_MAGIC (0x1234)

enum SYSCFG_ERASE_MODE{
    SYSCFG_ERASE_MODE_SECTOR,
    SYSCFG_ERASE_MODE_BLOCK,
    SYSCFG_ERASE_MODE_CHIP,
};

struct syscfg_info {
    struct spi_nor_flash *flash1, *flash2;
    uint32 addr1, addr2;
    uint32 size;
    uint8  erase_mode;
};

int32 syscfg_init(void *cfg, uint32 size);
int32 syscfg_read(void *cfg, uint32 size);
int32 syscfg_write(void *cfg, uint32 size);
int32 syscfg_info_get(struct syscfg_info *pinfo);
void syscfg_loaddef(void);
int32 syscfg_save(void);

#endif
