#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>

#define MCP23S09_WRITE_OPCODE 0x40
#define MCP23S09_READ_OPCODE 0x41
#define MCP23S09_DIR_REG 0x00
#define MCP23S09_GPIO_REG 0x09

#define MCP23S09_MAX_WRITE_SIZE 7
#define MCP23S09_MAX_READ_SIZE 7


struct mcp23s09 {
        struct spi_device *spidev;
        struct miscdevice mdev;
        int port_state;
};


int mcp23s09_set_port(struct spi_device *dev, unsigned int port_value)
{
        int err = 0;
        unsigned char spi_buff[6] = {
                MCP23S09_WRITE_OPCODE,
                MCP23S09_DIR_REG,
                0x00,
                MCP23S09_WRITE_OPCODE,
                MCP23S09_GPIO_REG,
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
                MCP23S09_WRITE_OPCODE,
                MCP23S09_DIR_REG,
                0xff,
                MCP23S09_READ_OPCODE,
                MCP23S09_GPIO_REG
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


/*------------------ Obsługa urządzenia znakowego ------------------*/
/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t mcp23s09_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        int err = 0;
        struct mcp23s09 *mcp23s09 = container_of(filp->private_data,
                                                 struct mcp23s09, mdev);
        char kspace_buffer[MCP23S09_MAX_WRITE_SIZE] = "";
        int port_state;

        dev_info(&mcp23s09->spidev->dev, "Write to device file.\n");

        if (count > MCP23S09_MAX_WRITE_SIZE) {
                dev_err(&mcp23s09->spidev->dev, "Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                dev_err(&mcp23s09->spidev->dev,
                        "Getting data from userspace failed.\n");
                return -EFAULT;		
        }

        err = kstrtol(kspace_buffer, 0, (long *) &port_state);
        if (err) {
                dev_err(&mcp23s09->spidev->dev,
                        "Bad input number.\n");
                return err;
        }

        if (port_state < 0x00  ||  port_state > 0xff) {
                dev_err(&mcp23s09->spidev->dev, "Bad voltage value\n");
                return -EINVAL;
        }

        err = mcp23s09_set_port(mcp23s09->spidev, port_state);
        if (err) {
               dev_err(&mcp23s09->spidev->dev, "Setting port value failed.\n");
               return -EIO;
        }

        return count;
}



/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t mcp23s09_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        int err = 0;
        unsigned char port_buf[MCP23S09_MAX_READ_SIZE];
        struct mcp23s09 *mcp23s09 = container_of(filp->private_data,
                                             struct mcp23s09, mdev);

        dev_info(&mcp23s09->spidev->dev, "Read from device file.\n");

        err = mcp23s09_get_port(mcp23s09->spidev, &mcp23s09->port_state);
        if (err) {
               dev_err(&mcp23s09->spidev->dev, "Getting port value failed.\n");
               return err;
        }

        sprintf(port_buf, "0x%X\n", mcp23s09->port_state);

        err = copy_to_user(buf, port_buf, strlen(port_buf));
        if (err) {
                dev_err(&mcp23s09->spidev->dev,
                        "Sending data to userspace failed\n");
                return -EIO;
        }
                
        return count;
}





/* Struktura przechowująca informacje o operacjach możliwych do
   wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = mcp23s09_write,
        .read = mcp23s09_read
};
/*------------------------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        int err;
        struct mcp23s09 *mcp23s09;
              
        mcp23s09 = devm_kzalloc(&dev->dev, sizeof(struct mcp23s09),
                                GFP_KERNEL);
        if (IS_ERR(mcp23s09)) {
               dev_err(&dev->dev, "Device data allocation failed.\n");
               err = PTR_ERR(mcp23s09);
               goto out_ret_err;
        }          

        mcp23s09->mdev.minor = MISC_DYNAMIC_MINOR;
        mcp23s09->mdev.name = "mcp23s09";
        mcp23s09->mdev.fops = &fops;

        err = misc_register(&mcp23s09->mdev);
        if (err) {
                dev_err(&dev->dev, "Misc device registration failed!/n");
                goto out_ret_err;
        }

        mcp23s09->spidev = dev;
        dev_set_drvdata(&dev->dev, mcp23s09);

        dev_info(&dev->dev, "SPI IO Driver Probed\n");
        
out_ret_err:
        return err;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        struct mcp23s09 *mcp23s09 = dev_get_drvdata(&dev->dev);
        misc_deregister(&mcp23s09->mdev);
        dev_info(&dev->dev, "SPI IO Driver Removed\n");
}


static const struct of_device_id mcp23s09_of_id[] = {
        { .compatible = "microchip,mcp23s09" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp23s09_of_id); 


static struct spi_driver mcp23s09_driver = {
        .probe = mcp23s09_probe,
        .remove = mcp23s09_remove,
        .driver = {
                .name = "mcp23s09",
                .of_match_table = mcp23s09_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp23s09_driver);

MODULE_LICENSE("GPL v2");
