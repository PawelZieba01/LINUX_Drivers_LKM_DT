#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09


static struct spi_device * io_mcp23s09_dev;


void IO_MCP23S09_Set(unsigned char port_value)
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

        spi_write(io_mcp23s09_dev, &spi_buff[0], 3);    /* Ustawienie pinów 
                                                           jako wyjścia */
        spi_write(io_mcp23s09_dev, &spi_buff[3], 3);    /* Ustawienie wartości
                                                           pinów */
}


unsigned char IO_MCP23S09_Get(void)
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

        spi_write_then_read(io_mcp23s09_dev, &spi_tx_buff[0], 3, 0, 0);
        spi_write_then_read(io_mcp23s09_dev, &spi_tx_buff[3], 2,
                            &spi_rx_buff[0], 1);

        return spi_rx_buff[0];
}


/*----- Parametr modułu przeznaczony do zapisu -----*/
static long int io_port_state = 0;

static ssize_t io_port_state_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count)
{
        int res;

        res = kstrtol(buf, 0, &io_port_state);
        if (res)
                return res;
               
        if (io_port_state < 0x00  ||  io_port_state > 0xff)
                return -EINVAL;
        
        IO_MCP23S09_Set(io_port_state);
        return count;
}


static ssize_t io_port_state_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
        io_port_state = (unsigned char)IO_MCP23S09_Get();
        return sprintf(buf, "0x%02X\n", (unsigned int)io_port_state);
}
DEVICE_ATTR_RW(io_port_state);
/*---------------------------------------------------*/


static int mtm_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Probed\n");

        io_mcp23s09_dev = dev;
        
        /* Utworzenie pliku reprezentującego atrybut urządzenia */
        device_create_file(&dev->dev, &dev_attr_io_port_state);

        return 0;
}


static int mtm_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Removed\n");
        device_remove_file(&dev->dev, &dev_attr_io_port_state);
        return 0;
}


static const struct of_device_id mtm_of_id[] = {
        { .compatible = "microchip,my_io" },
        {},
};
MODULE_DEVICE_TABLE(of, mtm_of_id); 


static struct spi_driver mtm_driver = {
        .probe = (void*) mtm_probe,
        .remove = (void*) mtm_remove,
        .driver = {
                .name = "my_io",
                .of_match_table = mtm_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mtm_driver);

MODULE_LICENSE("GPL v2");
