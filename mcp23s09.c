#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09


struct mcp23s09 {
        struct spi_device *spidev;
        int port_state;
};


int mcp23s09_set_port(struct spi_device *dev, unsigned int port_value)
{
        int err = 0;
        unsigned char spi_buff[6] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0x00,
                IO_WRITE_OPCODE,
                IO_GPIO_REG,
                port_value
        };
        
        dev_info(&dev->dev, "Set port value.\n");

        err = spi_write(dev, &spi_buff[0], 3); /* Ustawienie kierunku portu */
        if (err) {
                dev_err(&dev->dev, "Communication with device failed.\n");
                return -EIO;
        }

        err = spi_write(dev, &spi_buff[3], 3); /* Ustawienie stanu portu */
        if (err) {
                dev_err(&dev->dev, "Communication with device failed.\n");
                return -EIO;
        }
        return 0;
}


int mcp23s09_get_port(struct spi_device *dev, unsigned int *value)
{
        int err = 0;
        unsigned char spi_tx_buff[5] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0xff,
                IO_READ_OPCODE,
                IO_GPIO_REG
        };

        dev_info(&dev->dev, "Get port value.\n");

        err = spi_write_then_read(dev, &spi_tx_buff[0], 3, 0, 0);
        if (err) {
                dev_err(&dev->dev, "Communication with device failed.\n");
                return -EIO;
        }

        err = spi_write_then_read(dev, &spi_tx_buff[3], 2, value, 1);
        if (err) {
                dev_err(&dev->dev, "Communication with device failed.\n");
                return -EIO;
        }
        
        return 0;
}


/*----- Parametr modułu przeznaczony do zapisu -----*/
static ssize_t mcp23s09_port_state_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count)
{
        int err = 0;
        int port_state;
        struct mcp23s09 *mcp23s09 = dev_get_drvdata(dev);

        err = kstrtol(buf, 0, (long *)&port_state);
        if (err) {
                dev_err(dev, "Bad input number\n");
                return err;
        }
                               
        if (port_state < 0x00  ||  port_state > 0xff) {
                dev_err(dev, "Bad input value\n");
                return -EINVAL;
        }

        mcp23s09->port_state = port_state;                
        mcp23s09_set_port(mcp23s09->spidev, port_state);
        return count;
}


static ssize_t mcp23s09_port_state_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
        int err = 0;
        struct mcp23s09 *mcp23s09 = dev_get_drvdata(dev);

        err = mcp23s09_get_port(mcp23s09->spidev, &mcp23s09->port_state);
        if (err) {
               dev_err(dev, "Can't get port value.\n");
               return err;
        }
        return sprintf(buf, "0x%02X\n", mcp23s09->port_state);
}
DEVICE_ATTR_RW(mcp23s09_port_state);
/*---------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        int err = 0;
        struct mcp23s09 *mcp23s09;

        mcp23s09 = devm_kzalloc(&dev->dev, sizeof(struct mcp23s09),
                                GFP_KERNEL);
        if (IS_ERR(mcp23s09)) {
               dev_err(&dev->dev, "Device data allocation failed.\n");
               err = PTR_ERR(mcp23s09);
               goto out_ret_err;
        }

        err = device_create_file(&dev->dev, &dev_attr_mcp23s09_port_state);
        if (err) {
                dev_err(&dev->dev, "Can't create device file.\n");
                goto out_ret_err;
        }

        mcp23s09->spidev = dev;
        dev_set_drvdata(&dev->dev, mcp23s09);

        dev_info(&dev->dev, "SPI IO Driver Probed.\n");

out_ret_err:
        return err;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        device_remove_file(&dev->dev, &dev_attr_mcp23s09_port_state);
        dev_info(&dev->dev, "SPI IO Driver Removed.\n");
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
