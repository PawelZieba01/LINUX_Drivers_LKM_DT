#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09


static struct spi_device * mcp23s09_spi_dev;


void mcp23s09_set_port(unsigned char port_value)
{
        unsigned char spi_buff[6] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0x00,
                IO_WRITE_OPCODE,
                IO_GPIO_REG,
                port_value
        };
        
        pr_info("Set port value to %x \n", port_value);

        spi_write(mcp23s09_spi_dev, &spi_buff[0], 3);    /* Ustawienie pinów 
                                                           jako wyjścia */
        spi_write(mcp23s09_spi_dev, &spi_buff[3], 3);    /* Ustawienie wartości
                                                           pinów */
}


unsigned char mcp23s09_get_port(void)
{
        unsigned char spi_tx_buff[5] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0xff,
                IO_READ_OPCODE,
                IO_GPIO_REG
        };

        unsigned char spi_rx_buff[] = {0};

        pr_info("Get port value.\n");

        spi_write_then_read(mcp23s09_spi_dev, &spi_tx_buff[0], 3, 0, 0);
        spi_write_then_read(mcp23s09_spi_dev, &spi_tx_buff[3], 2,
                            &spi_rx_buff[0], 1);

        return spi_rx_buff[0];
}


/*----- Parametr modułu przeznaczony do zapisu -----*/
static long int mcp23s09_port_state = 0;

static ssize_t mcp23s09_port_state_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count)
{
        int res;

        res = kstrtol(buf, 0, &mcp23s09_port_state);
        if (res)
                return res;
               
        if (mcp23s09_port_state < 0x00  ||  mcp23s09_port_state > 0xff)
                return -EINVAL;
        
        mcp23s09_set_port(mcp23s09_port_state);
        return count;
}


static ssize_t mcp23s09_port_state_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
        mcp23s09_port_state = (unsigned char)mcp23s09_get_port();
        return sprintf(buf, "0x%02X\n", (unsigned int)mcp23s09_port_state);
}
DEVICE_ATTR_RW(mcp23s09_port_state);
/*---------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Probed\n");

        mcp23s09_spi_dev = dev;
        
        /* Utworzenie pliku reprezentującego atrybut urządzenia */
        device_create_file(&dev->dev, &dev_attr_mcp23s09_port_state);

        return 0;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Removed\n");
        device_remove_file(&dev->dev, &dev_attr_mcp23s09_port_state);
}


static const struct of_device_id mcp23s09_of_id[] = {
        { .compatible = "microchip,mcp23s09_io" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp23s09_of_id); 


static struct spi_driver mcp23s09_driver = {
        .probe =  mcp23s09_probe,
        .remove = mcp23s09_remove,
        .driver = {
                .name = "mcp23s09_io",
                .of_match_table = mcp23s09_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp23s09_driver);

MODULE_LICENSE("GPL v2");
