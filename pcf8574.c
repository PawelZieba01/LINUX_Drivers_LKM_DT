#include <linux/of.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>

#define MY_DEV_NAME "io_pcf8574"
#define MAX_WRITE_SIZE 10
#define MAX_READ_SIZE 7


static struct i2c_client *pcf8574_i2c_client;
static struct regmap *pcf8574_regmap;


/*------------------ Obsługa urządzenia znakowego ------------------*/

static ssize_t pcf8574_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        char kspace_buffer[MAX_WRITE_SIZE] = "";
        int port_value, res;

        dev_info(&pcf8574_i2c_client->dev, "Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                dev_err(&pcf8574_i2c_client->dev, "Bad input number.\n");
                return -ERANGE;
        }

        res = copy_from_user(kspace_buffer, buf, count);
        if (res) {
                dev_err(&pcf8574_i2c_client->dev,
                        "Can't copy data from user space.\n");
                return -EFAULT;		
        }

        res = kstrtol(kspace_buffer, 0, (long*)&port_value);
        if (res) {
                dev_err(&pcf8574_i2c_client->dev,
                        "Can't convert data to integer.\n");
                return res;
        }

        if (port_value < 0x00  ||  port_value > 0xff) {
                dev_err(&pcf8574_i2c_client->dev, "Bad voltage value.\n");
                return -EINVAL;
        }

        res = regmap_write(pcf8574_regmap, 0x00, port_value);
        if (res) {
                dev_err(&pcf8574_i2c_client->dev,
                        "I2C communication error.\n");
                return res;
        }

        return count;
}


static ssize_t pcf8574_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        int res;
        unsigned int port_value; 
        unsigned char port_buf[MAX_READ_SIZE];

       dev_info(&pcf8574_i2c_client->dev, "Read from device file.\n");

        res = regmap_read(pcf8574_regmap, 0xff, &port_value);
        if (res) {
                dev_err(&pcf8574_i2c_client->dev,
                        "I2C communication error.\n");
                return res;
        }

        sprintf(port_buf, "0x%X\n", port_value);
     
        res = copy_to_user(buf, port_buf, strlen(port_buf));
        if (res) {
                dev_err(&pcf8574_i2c_client->dev,
                        "Can't copy data to uder space.\n");
                return -EIO;
        }
                
                
        return count;
}


static struct file_operations fops = 
{
        .owner = THIS_MODULE,
        .write = pcf8574_write,
        .read = pcf8574_read
};


struct miscdevice pcf8574_device = 
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = MY_DEV_NAME,
    .fops = &fops,
};
/*------------------------------------------------------------------*/


/*----------------------------- REGMAP -----------------------------*/
static bool writeable_reg(struct device *dev, unsigned int reg)
{
        if (reg == 0x00)
                return true;
        return false;
}
  
static bool readable_reg(struct device *dev, unsigned int reg)
{
        if (reg == 0xff)
                return true;
        return false;
}
/*------------------------------------------------------------------*/


static int pcf8574_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
        int res;
        struct regmap_config reg_conf;

        dev_info(&client->dev, "i2c IO Driver Probed\n");
        
        pcf8574_i2c_client = client;              

        /* misc device create */
        res = misc_register(&pcf8574_device);
        if (res) {
                dev_err(&pcf8574_i2c_client->dev,
                        "Misc device registration failed!");
                return res;
        }

        /* regmap config */
        memset(&reg_conf, 0, sizeof(reg_conf));
        reg_conf.reg_bits = 8;
        reg_conf.val_bits = 8;
        reg_conf.writeable_reg = writeable_reg;
        reg_conf.readable_reg = readable_reg;

        /* regmap init */
        pcf8574_regmap = devm_regmap_init_i2c(client, &reg_conf);
        return 0;
}


static void pcf8574_remove(struct i2c_client *client)
{
        dev_info(&client->dev, "i2c IO Driver Removed\n");
        misc_deregister(&pcf8574_device);
        //regmap jest usuwany automatycznie!!!
}


static const struct of_device_id pcf8574_of_id[] = {
        { .compatible = "microchip,pcf8574_io" },
        {},
};
MODULE_DEVICE_TABLE(of, pcf8574_of_id); 


static struct i2c_driver pcf8574_driver = {
        .probe = pcf8574_probe,
        .remove = pcf8574_remove,
        .driver = {
                .name = "pcf8574_io",
                .of_match_table = pcf8574_of_id,
                .owner = THIS_MODULE,
        },
};
module_i2c_driver(pcf8574_driver);

MODULE_LICENSE("GPL v2");
