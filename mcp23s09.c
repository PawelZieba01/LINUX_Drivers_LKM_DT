#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09

#define MY_DEV_NAME "mcp23s09"

#define MAX_WRITE_SIZE 10
#define MAX_READ_SIZE 7


struct mcp23s09_data {
        struct spi_device *spidev;
        struct miscdevice mdev;
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
        
        dev_info(&dev->dev, "Set port value to %x \n", port_value);

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

        dev_info(&dev->dev, "Get port value.\n");

        err = spi_write_then_read(dev, &spi_tx_buff[0], 3, 0, 0);
        if (err) {
                dev_err(&dev->dev, "Can't communicate with device\n");
                return -EIO;
        }

        err = spi_write_then_read(dev, &spi_tx_buff[3], 2,
                                  value, 1);
        if (err) {
                dev_err(&dev->dev, "Can't communicate with device\n");
                return -EIO;
        }
        return 0;
}


/*------------------ Obsługa urządzenia znakowego ------------------*/
/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t mcp23s09_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        int err;
        struct mcp23s09_data *data = filp->private_data;
        struct device *dev = &data->spidev->dev; 
        char kspace_buffer[MAX_WRITE_SIZE] = "";

       dev_info(dev, "Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                dev_err(dev, "Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                dev_err(dev, "Can't copy data from user space\n");
                return -EFAULT;		
        }

        err = kstrtol(kspace_buffer, 0, (long*)&data->port_state);
        if (err) {
                dev_err(dev, "Can't convert data to integer\n");
                return err;
        }

        if (data->port_state < 0x00  ||  data->port_state > 0xff) {
                dev_err(dev, "Bad voltage value\n");
                return -EINVAL;
        }

        err = mcp23s09_set_port(data->spidev, data->port_state);
        if (err) {
               dev_err(dev, "Can't set port\n");
               return -EIO;
        }
        return count;
}


/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t mcp23s09_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        int err;
        unsigned char port_buf[MAX_READ_SIZE];
        struct mcp23s09_data *data = filp->private_data;
        struct device *dev = &data->spidev->dev; 

        dev_info(dev, "Read from device file.\n");

        err = mcp23s09_get_port(data->spidev, &data->port_state);
        if (err) {
               dev_err(dev, "Can't get port value\n");
               return err;
        }

        sprintf(port_buf, "0x%X\n", data->port_state);

        err = copy_to_user(buf, port_buf, strlen(port_buf));
        if (err) {
                dev_err(dev, "Can't send data to userspace\n");
                return -EIO;
        }
                
        return count;
}


int mcp23s09_open(struct inode *node, struct file *filp)
{       
        struct mcp23s09_data *data = container_of(filp->private_data,
                                                 struct mcp23s09_data,
                                                 mdev);

        dev_info(&data->spidev->dev, "Driver file open\n");
        
        filp->private_data = data;
        return 0;
}


int mcp23s09_release(struct inode *node, struct file *filp)
{
        struct mcp23s09_data *data = filp->private_data;

        dev_info(&data->spidev->dev, "Driver file close\n");

        filp->private_data = NULL;
        return 0;
}

/* Struktura przechowująca informacje o operacjach możliwych do
   wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = mcp23s09_write,
        .read = mcp23s09_read,
        .open = mcp23s09_open,
        .release = mcp23s09_release 
};
/*------------------------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        int err;
        struct mcp23s09_data *data;

        dev_info(&dev->dev, "SPI IO Driver Probed\n");
        
        data = devm_kzalloc(&dev->dev, sizeof(struct mcp23s09_data), GFP_KERNEL);
        if (IS_ERR(data)) {
               dev_err(&dev->dev, "Can't allocate memory for device data/n");
               goto err_alloc;
        }            

        data->mdev.minor = MISC_DYNAMIC_MINOR;
        data->mdev.name = MY_DEV_NAME;
        data->mdev.fops = &fops;

        err = misc_register(&data->mdev);
        if (err) {
                dev_err(&dev->dev, "Misc device registration failed!/n");
                goto err_misc;
        }

        data->spidev = dev;
        dev_set_drvdata(&dev->dev, data);
        
        return 0;

err_misc:
err_alloc:
        return -1;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        struct mcp23s09_data *data = dev_get_drvdata(&dev->dev);
        dev_info(&dev->dev, "SPI IO Driver Removed\n");
        misc_deregister(&data->mdev);
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
