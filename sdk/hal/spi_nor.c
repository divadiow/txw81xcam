#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/mutex.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"

int32 spi_nor_open(struct spi_nor_flash *flash)
{
    int32 ret = RET_OK;

    ASSERT(flash->bus);
    if(atomic_read(&flash->ref) == 0){
        ret = spi_open(flash->spidev, flash->spi_config.clk, SPI_MASTER_MODE,
                       flash->spi_config.wire_mode, flash->spi_config.clk_mode);
        ASSERT(!ret);
        if (flash->bus->open) {
            flash->bus->open(flash);
        }
    }
    atomic_inc(&flash->ref);
    return ret;
}

void spi_nor_close(struct spi_nor_flash *flash)
{
    ASSERT(flash->bus);

    if(atomic_dec_and_test(&flash->ref)){
        if (flash->bus->close) {
            flash->bus->close(flash);
        }
        spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
        spi_close(flash->spidev);
    }
}

void spi_nor_read(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->read(flash, addr, buf, len);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_write(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len)
{
    uint32 remain_size;
    ASSERT(flash->bus);
    
    os_mutex_lock(&flash->lock, osWaitForever);
    /* Flash is written according to page */
    remain_size = flash->page_size - (addr % flash->page_size);
    if(remain_size && (len > remain_size)) {
        flash->bus->program(flash, addr, buf, remain_size);
        addr += remain_size;
        buf  += remain_size;
        len  -= remain_size;
    }
    while(len > flash->page_size) {
        flash->bus->program(flash, addr, buf, flash->page_size);
        addr += flash->page_size;
        buf  += flash->page_size;
        len  -= flash->page_size;
    }
    
    flash->bus->program(flash, addr, buf, len);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_sector_erase(struct spi_nor_flash *flash, uint32 sector_addr)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->sector_erase(flash, sector_addr);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_block_erase(struct spi_nor_flash *flash, uint32 block_addr)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->block_erase(flash, block_addr);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_chip_erase(struct spi_nor_flash *flash)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->chip_erase(flash);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_erase_security_reg(struct spi_nor_flash *flash, uint32 addr)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->erase_security_reg(flash, addr);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_program_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->program_security_reg(flash, addr, buf, size);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_read_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size)
{
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->read_security_reg(flash, addr, buf, size);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_custom_read(struct spi_nor_flash *flash, uint32_t param)
{
    ASSERT(flash->bus);
    return flash->bus->custom_read(flash, param);
}

void spi_nor_custom_write(struct spi_nor_flash *flash, uint32_t param)
{
    ASSERT(flash->bus);
    return flash->bus->custom_write(flash, param);
}

void spi_nor_custom_erase(struct spi_nor_flash *flash, uint32_t param)
{
    ASSERT(flash->bus);
    return flash->bus->custom_erase(flash, param);
}

__init int32 spi_nor_attach(struct spi_nor_flash *flash, uint32 dev_id)
{
    int32 ret = 0;

    ASSERT(flash->size && flash->sector_size);
    os_mutex_init(&flash->lock);
    flash->bus = spi_nor_bus_get(flash->mode);
    ASSERT(flash->bus);
    ret = dev_register(dev_id, (struct dev_obj *)flash);
    return ret;
}

