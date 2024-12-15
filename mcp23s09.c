#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09


struct mcp23s09_data {
        struct spi_device *spidev;
        int port_state;
};


int mcp23s09_set_port(struct spi_device *dev, unsigned int port_value)
{
        int err;
        unsigned char spi_buff[6] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0x00,
                IO_WRITE_OPCODE,
                IO_GPIO_REG,
                port_value
        };
        
        pr_info("Set port value to %x \n", port_value);

        err = spi_write(dev, &spi_buff[0], 3); /* Ustawienie 
                                                  kierunku portu */
        if (err) {
                dev_err(&dev->dev, "Can't communicate with device\n");
                return -EIO;
        }

        err = spi_write(dev, &spi_buff[3], 3); /* Ustawienie stanu
                                                  portu */
        if (err) {
                dev_err(&dev->dev, "Can't communicate with device\n");
                return -EIO;
        }
        return 0;
}


int mcp23s09_get_port(struct spi_device *dev, unsigned int *value)
{
        int err;
        unsigned char spi_tx_buff[5] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0xff,
                IO_READ_OPCODE,
                IO_GPIO_REG
        };

        pr_info("Get port value.\n");

        err = spi_write_then_read(dev, &spi_tx_buff[0], 3, 0, 0);
        if (err) {
                dev_err(&dev->dev, "Can't communicate with device\n");
                return -EIO;
        }

        err = spi_write_then_read(dev, &spi_tx_buff[3], 2, value, 1);
        if (err) {
                dev_err(&dev->dev, "Can't communicate with device\n");
                return -EIO;
        }
        return 0;
}


/*----- Parametr modułu przeznaczony do zapisu -----*/
static ssize_t port_state_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count)
{
        int err;
        struct mcp23s09_data *data = dev_get_drvdata(dev);

        err = kstrtol(buf, 0, (long *)&data->port_state);
        if (err)
                return err;
               
        if (data->port_state < 0x00  ||  data->port_state > 0xff)
                return -EINVAL;
        
        mcp23s09_set_port(data->spidev, data->port_state);
        return count;
}


static ssize_t port_state_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
        int err;
        struct mcp23s09_data *data = dev_get_drvdata(dev);

        err = mcp23s09_get_port(data->spidev, &data->port_state);
        if (err) {
               dev_err(dev, "Can't get port value\n");
               return err;
        }
        return sprintf(buf, "0x%02X\n", data->port_state);
}
DEVICE_ATTR_RW(port_state);
/*---------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        int err;
        struct mcp23s09_data *data;

        dev_info(&dev->dev, "SPI IO Driver Probed\n");
        
        err = device_create_file(&dev->dev, &dev_attr_port_state);
        if (err) {
                dev_err(&dev->dev, "Can't create device file\n");
                goto err_attr;
        }

        data = devm_kzalloc(&dev->dev, sizeof(struct mcp23s09_data),
                            GFP_KERNEL);
        if (IS_ERR(data)) {
                dev_err(&dev->dev, "Can't allocate device data\n");
                goto err_alloc;
        }

        data->spidev = dev;
        dev_set_drvdata(&dev->dev, data);

        return 0;

err_alloc:
        device_remove_file(&dev->dev, &dev_attr_port_state);
err_attr:
        return -1;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Removed\n");
        device_remove_file(&dev->dev, &dev_attr_port_state);
}


static const struct of_device_id mcp23s09_of_id[] = {
        { .compatible = "microchip,mcp23s09" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp23s09_of_id); 


static struct spi_driver mcp23s09_driver = {
        .probe =  mcp23s09_probe,
        .remove = mcp23s09_remove,
        .driver = {
                .name = "mcp23s09",
                .of_match_table = mcp23s09_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp23s09_driver);

MODULE_LICENSE("GPL v2");
