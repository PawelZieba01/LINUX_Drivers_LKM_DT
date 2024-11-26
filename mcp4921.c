#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>


#define MY_DEV_NAME "dac_mcp4921"
#define SPI_BUS 0
#define DAC_MCP4921_REF_VOLTAGE_mV 3300
#define MCP4921_CFG_BITS (0x03 << 12)           /* Kanal A, Unbuffered Vref,
                                                   Gain=1, DAC Enable */
#define BINARY_VAL_FOR_1mV 0x9ee0               /* unsigned Q1.15 -> 1,2412
                                                   (1mV to wartość 1,2412 
                                                   dla przetwornika) */
#define MAX_WRITE_SIZE 10                       /* 4 znaki na liczbę i jeden 
                                                   znak Null */                                                   


static struct spi_device *mcp4921_spi_dev;


void mcp4921_set(unsigned int voltage_12bit)
{
        unsigned int data = MCP4921_CFG_BITS | (voltage_12bit & 0x0fff);
        unsigned char spi_buff[2];
        
        spi_buff[0] = ( (data & 0xff00) >> 8 );
        spi_buff[1] = ( data & 0x00ff );

        spi_write(mcp4921_spi_dev, spi_buff, 2);
}


void mcp4921_set_mV(unsigned int voltage_mV)
{
        /* Konwersja Q16.15 do Q16.0 */
        unsigned int voltage_12bit = (((unsigned long)voltage_mV *
                                     (unsigned long)BINARY_VAL_FOR_1mV) >> 15);
        
        pr_info("Set voltage to %d [mV]\n", voltage_mV);
        pr_info("Set voltage to %d [12bit]\n", voltage_12bit);

        mcp4921_set(voltage_12bit);
}


/*-------------------- Obsługa urządzenia znakowego --------------------*/
static ssize_t mcp4921_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        char kspace_buffer[MAX_WRITE_SIZE] = "";
        int voltage_mV, ret;

        pr_info("Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                pr_err("Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                pr_err("Can't copy data from user space\n");
                return -EFAULT;		
        }

        pr_info("%s\n", kspace_buffer);

        ret = kstrtol(kspace_buffer, 0, (long*)&voltage_mV);
        if (ret) {
                pr_err("Can't convert data to integer\n");
                return ret;
        }

        if (voltage_mV < 0  ||  voltage_mV > DAC_MCP4921_REF_VOLTAGE_mV) {
                pr_err("Bad voltage value\n");
                return -EINVAL;
        }

        mcp4921_set_mV(voltage_mV);

        return count;
}


static struct file_operations fops = {
        .owner=THIS_MODULE,
        .write=mcp4921_write
};


struct miscdevice mcp4921_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MY_DEV_NAME,
    .fops = &fops,
};
/*----------------------------------------------------------------------*/




static int mcp4921_probe(struct spi_device *dev)
{
        int res;
        
        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

        mcp4921_spi_dev = dev;

        res = misc_register(&mcp4921_device);
        if (res) {
                pr_err("Misc device registration failed!");
                return res;
        }
        
        return 0;
}


static void mcp4921_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
        misc_deregister(&mcp4921_device);
        
}




static const struct of_device_id mcp4921_of_id[] = {
        { .compatible = "microchip,mcp4921_dac" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp4921_of_id);   


static struct spi_driver mcp4921_driver = {
        .probe = mcp4921_probe,
        .remove = mcp4921_remove,
        .driver = {
                .name = "mcp4921_dac",
                .of_match_table = mcp4921_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp4921_driver);

MODULE_LICENSE("GPL v2");
