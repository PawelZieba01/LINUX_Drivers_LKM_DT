#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include "mcp4921_commands.h"


#define MY_DEV_NAME "mcp4921"
#define MCP4921_REF_VOLTAGE_mV 3300
#define MCP4921_CFG_BITS (0x03 << 12)           /* Kanal A, Unbuffered Vref,
                                                   Gain=1, DAC Enable */
#define BINARY_VAL_FOR_1mV 0x9ee0               /* unsigned Q1.15 -> 1,2412
                                                   (1mV to wartość 1,2412 
                                                   dla przetwornika) */
#define MAX_WRITE_SIZE 10                       /* 4 znaki na liczbę i jeden 
                                                   znak Null */   

struct mcp4921_data {
        struct spi_device *spidev;
        struct miscdevice mdev;
        bool vref_buf_bit;
        bool gain_bit;
        bool enable_bit;
};


int mcp4921_set(struct mcp4921_data *dev_data, unsigned int voltage_12bit)
{

        unsigned int cfg_bits = (dev_data->vref_buf_bit << 14) |
                                (dev_data->gain_bit << 13) |
                                (dev_data->enable_bit << 12);
        unsigned int data = cfg_bits | (voltage_12bit & 0x0fff);
        unsigned char spi_buff[2];
        
        spi_buff[0] = ( (data & 0xff00) >> 8 );
        spi_buff[1] = ( data & 0x00ff );

        return spi_write(dev_data->spidev, spi_buff, 2);
}


int mcp4921_set_mV(struct mcp4921_data *dev_data, unsigned int voltage_mV)
{
        /* Konwersja Q16.15 do Q16.0 */
        unsigned int voltage_12bit = (((unsigned long)voltage_mV *
                                     (unsigned long)BINARY_VAL_FOR_1mV) >> 15);

        return mcp4921_set(dev_data, voltage_12bit);
}


/*-------------------- Obsługa urządzenia znakowego --------------------*/
static ssize_t mcp4921_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        struct mcp4921_data *data = filp->private_data;
        struct device *dev = &data->spidev->dev;
        char kspace_buffer[MAX_WRITE_SIZE] = "";
        int voltage_mV, err;

        dev_info(dev, "Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                dev_err(dev, "Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                dev_err(dev, "Can't copy data from user space.\n");
                return -EFAULT;		
        }

        err = kstrtol(kspace_buffer, 0, (long*)&voltage_mV);
        if (err) {
                dev_err(dev, "Can't convert data to integer.\n");
                return err;
        }

        if (voltage_mV < 0  ||  voltage_mV > MCP4921_REF_VOLTAGE_mV) {
                dev_err(dev, "Bad voltage value.\n");
                return -EINVAL;
        }

        mcp4921_set_mV(data, voltage_mV);

        return count;
}


static long mcp4921_ioctl(struct file *filp, unsigned int cmd,
                          unsigned long arg)
{
        int value;
        struct mcp4921_data *data = filp->private_data;
        struct device *dev = &data->spidev->dev;

        switch(cmd) {
        case MCP4921_RESET:
                dev_info(dev, "MCP4921_RESET command\n");

                data->vref_buf_bit = 0;
                data->gain_bit = 1;
                data->enable_bit = 1;
                break;

        case MCP4921_ENABLE:
                dev_info(dev, "MCP4921_ENABLE command\n");

                value = *(unsigned long*)arg;
                if(value != 0 && value != 1) {
                        dev_err(dev, "ENABLE bit wrong value!\n");
                        return -EINVAL;
                }

                data->enable_bit = value;
                break;

        case MCP4921_GAIN:
                dev_info(dev, "MCP4921_GAIN command\n");

                value = *(unsigned long*)arg;
                if(value != 0 && value != 1) {
                        dev_err(dev, "GAIN bit wrong value!\n");
                        return -EINVAL;
                }

                dev_info(dev, "Set GAIN bit to: %d\n", value);
                data->gain_bit = value;
                break;

        case MCP4921_VREF_BUFF:
                dev_info(dev, "MCP4921_VREF_BUFF command\n");

                value = *(unsigned long*)arg;
                if(value != 0 && value != 1) {
                        dev_err(dev, "VREF_BUFF bit wrong value!\n");
                        return -EINVAL;
                }

                dev_info(dev, "Set VREF_BUFF bit to: %d\n", value);
                data->vref_buf_bit = value;
                break;

        default:
                break;
        }
        return 0;
}


int mcp4921_open(struct inode *node, struct file *filp)
{       
        struct mcp4921_data *data = container_of(filp->private_data,
                                                 struct mcp4921_data,
                                                 mdev);

        dev_info(&data->spidev->dev, "Driver file open\n");
        
        filp->private_data = data;
        return 0;
}


int mcp4921_release(struct inode *node, struct file *filp)
{
        struct mcp4921_data *data = filp->private_data;

        dev_info(&data->spidev->dev, "Driver file close\n");

        filp->private_data = NULL;
        return 0;
}


static struct file_operations fops = {
        .owner=THIS_MODULE,
        .write=mcp4921_write,
        .unlocked_ioctl = mcp4921_ioctl,
        .open = mcp4921_open,
        .release = mcp4921_release
};
/*----------------------------------------------------------------------*/




static int mcp4921_probe(struct spi_device *dev)
{
        int err;
        //struct miscdevice *mdev;
        struct mcp4921_data *data;
        
        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

        data = devm_kzalloc(&dev->dev, sizeof(struct mcp4921_data), GFP_KERNEL);
        if (IS_ERR(data)) {
               dev_err(&dev->dev, "Can't allocate memory for device data");
               goto err_alloc;
        }

        data->mdev.minor = MISC_DYNAMIC_MINOR;
        data->mdev.name = MY_DEV_NAME;
        data->mdev.fops = &fops;

        err = misc_register(&data->mdev);
        if (err) {
                pr_err("Misc device registration failed!");
                goto err_misc;
        }

        data->spidev = dev;
        data->gain_bit = 1;
        data->enable_bit = 1;
        dev_set_drvdata(&dev->dev, data);
                
        return 0;

err_misc:
err_alloc:
        return -1;
}


static void mcp4921_remove(struct spi_device *dev)
{
        struct mcp4921_data *data = dev_get_drvdata(&dev->dev);
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
        misc_deregister(&data->mdev);        
}


static const struct of_device_id mcp4921_of_id[] = {
        { .compatible = "microchip,mcp4921" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp4921_of_id);   


static struct spi_driver mcp4921_driver = {
        .probe = mcp4921_probe,
        .remove = mcp4921_remove,
        .driver = {
                .name = "mcp4921",
                .of_match_table = mcp4921_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp4921_driver);

MODULE_LICENSE("GPL v2");
