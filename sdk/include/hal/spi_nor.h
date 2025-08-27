#ifndef _HAL_SPI_NOR_H_
#define _HAL_SPI_NOR_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "osal/mutex.h"
#include "osal/atomic.h"

/**
  * @brief spi nor mode
  */
enum spi_nor_mode {
    SPI_NOR_NORMAL_SPI_MODE,
    SPI_NOR_DUAL_SPI_MODE,
    SPI_NOR_QUAD_SPI_MODE,
    SPI_NOR_QPI_MODE,
    SPI_NOR_XIP_MODE,
};

struct spi_nor_flash;

struct spi_nor_bus {
    void (*open)(struct spi_nor_flash *flash);
    void (*close)(struct spi_nor_flash *flash);
    void (*read)(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
    void (*program)(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
    void (*sector_erase)(struct spi_nor_flash *flash, uint32 addr);
    void (*block_erase)(struct spi_nor_flash *flash, uint32 addr);
    void (*chip_erase)(struct spi_nor_flash *flash);
    void (*enter_power_down)(struct spi_nor_flash *flash);
    void (*release_power_down)(struct spi_nor_flash *flash);
    void (*erase_security_reg)(struct spi_nor_flash *flash, uint32 addr);
    void (*program_security_reg)(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
    void (*read_security_reg)(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
    void (*custom_read)(struct spi_nor_flash *flash, uint32_t param);
    void (*custom_write)(struct spi_nor_flash *flash, uint32_t param);
    void (*custom_erase)(struct spi_nor_flash *flash, uint32_t param);
    int32 (*ioctl)(struct spi_nor_flash *flash, uint32_t cmd,uint32_t param1,uint32_t param2);
};

struct spi_nor_flash {
    struct dev_obj dev;
    struct spi_device *spidev;
    struct {
        uint32 clk, clk_mode, wire_mode, cs;
    } spi_config;
    uint32 size, sector_size, page_size,block_size;
    uint32 vendor_id, product_id;
    enum spi_nor_mode mode;
    struct os_mutex lock;
    atomic_t ref;
    const struct spi_nor_bus *bus;
};

const struct spi_nor_bus *spi_nor_bus_get(enum spi_nor_mode mode);
void spi_nor_standard_read_jedec_id(struct spi_nor_flash *flash, uint8 *buf);
int32 spi_nor_open(struct spi_nor_flash *flash);
void spi_nor_close(struct spi_nor_flash *flash);
void spi_nor_read(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len);
void spi_nor_write(struct spi_nor_flash *flash, uint32 addr, uint8 *data, uint32 len);
void spi_nor_sector_erase(struct spi_nor_flash *flash, uint32 sector_addr);
void spi_nor_block_erase(struct spi_nor_flash *flash, uint32 block_addr);
void spi_nor_chip_erase(struct spi_nor_flash *flash);
void spi_nor_erase_security_reg(struct spi_nor_flash *flash, uint32 addr);
void spi_nor_program_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
void spi_nor_read_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
int32 spi_nor_attach(struct spi_nor_flash *flash, uint32 dev_id);
void spi_nor_enter_power_down(struct spi_nor_flash *flash);
void spi_nor_release_power_down(struct spi_nor_flash *flash);
void spi_nor_custom_read(struct spi_nor_flash *flash, uint32_t param);
void spi_nor_custom_write(struct spi_nor_flash *flash, uint32_t param);
void spi_nor_custom_erase(struct spi_nor_flash *flash, uint32_t param);
int32 spi_nor_ioctl(struct spi_nor_flash *flash, uint32_t cmd,uint32_t param1,uint32_t param2);

#ifdef __cplusplus
}
#endif
#endif

