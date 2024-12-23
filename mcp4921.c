#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>


#define MCP4921_REF_VOLTAGE_mV 3300
#define MCP4921_CFG_BITS (0x03 << 12)           /* Kanal A, Unbuffered Vref,
                                                   Gain=1, DAC Enable */
#define BINARY_VAL_FOR_1mV 0x9ee0               /* unsigned Q1.15 -> 1,2412
                                                   (1mV to wartość 1,2412 
                                                   dla przetwornika) */
#define MCP4921_MAX_WRITE_SIZE 8


struct mcp4921 {
        struct spi_device *spidev;
        struct miscdevice mdev;
        int voltage_mV;
};


int mcp4921_set(struct spi_device *dev, unsigned int voltage_12bit)
{
        unsigned int data = MCP4921_CFG_BITS | (voltage_12bit & 0x0fff);
        unsigned char spi_buff[2];
        
        spi_buff[0] = ( (data & 0xff00) >> 8 );
        spi_buff[1] = ( data & 0x00ff );

        return spi_write(dev, spi_buff, 2);
}


int mcp4921_set_mV(struct spi_device *dev, unsigned int voltage_mV)
{
        /* Konwersja Q16.15 do Q16.0 */
        unsigned int voltage_12bit = ((voltage_mV * BINARY_VAL_FOR_1mV) >> 15);
        
        dev_info(&dev->dev, "Set voltage to %d [mV]\n", voltage_mV);
        dev_info(&dev->dev, "Set voltage to %d [12bit]\n", voltage_12bit);
        
        return mcp4921_set(dev, voltage_12bit);
}


/*-------------------- Obsługa urządzenia znakowego --------------------*/
static ssize_t mcp4921_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        int err = 0;
        char kspace_buffer[MCP4921_MAX_WRITE_SIZE] = "";
        int voltage_mV;
        struct mcp4921 *mcp4921 = container_of(filp->private_data,
                                               struct mcp4921, mdev);

        dev_info(&mcp4921->spidev->dev, "Write to device file.\n");

        if (count > MCP4921_MAX_WRITE_SIZE) {
                dev_err(&mcp4921->spidev->dev, "Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                dev_err(&mcp4921->spidev->dev,
                        "Getting data from userspace failed.\n");
                return -EFAULT;		
        }

        err = kstrtol(kspace_buffer, 0, (unsigned long *) &voltage_mV);
        if (err) {
                dev_err(&mcp4921->spidev->dev, "Bad input value.\n");
                return err;
        }

        if (voltage_mV < 0  ||  voltage_mV > MCP4921_REF_VOLTAGE_mV) {
                dev_err(&mcp4921->spidev->dev, "Bad voltage value.\n");
                return -EINVAL;
        }

        mcp4921->voltage_mV = voltage_mV;
        mcp4921_set_mV(mcp4921->spidev, voltage_mV);

        return count;
}


static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = mcp4921_write
};
/*----------------------------------------------------------------------*/


static int mcp4921_probe(struct spi_device *dev)
{
        int err = 0;
        struct mcp4921 *mcp4921;
        
        mcp4921 = devm_kzalloc(&dev->dev, sizeof(struct mcp4921), GFP_KERNEL);
        if (IS_ERR(mcp4921)) {
               dev_err(&dev->dev, "Device data allocation failed.\n");
               err = PTR_ERR(mcp4921);
               goto out_ret_err;
        }

        /* miscdevice config */
        mcp4921->mdev.minor = MISC_DYNAMIC_MINOR;
        mcp4921->mdev.name = "mcp4921";
        mcp4921->mdev.fops = &fops;

        err = misc_register(&mcp4921->mdev);
        if (err) {
                dev_err(&dev->dev, "Misc device registration failed.\n");
                goto out_ret_err;
        }

        mcp4921->spidev = dev;
        dev_set_drvdata(&dev->dev, mcp4921);
                
        dev_info(&dev->dev, "SPI DAC Driver Probed.\n");

out_ret_err:
        return err;
}


static void mcp4921_remove(struct spi_device *dev)
{
        struct mcp4921 *mcp4921 = dev_get_drvdata(&dev->dev);
        misc_deregister(&mcp4921->mdev);
        dev_info(&dev->dev, "SPI DAC Driver Removed.\n"); 
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
