#include <linux/of.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>

#define MY_DEV_NAME "pcf8574"
#define MAX_WRITE_SIZE 10
#define MAX_READ_SIZE 7


struct pcf8574_data {
        struct i2c_client *i2cdev;
        struct regmap *rmap;
        struct miscdevice mdev;
        int port_state;
};

/*------------------ Obsługa urządzenia znakowego ------------------*/

static ssize_t pcf8574_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        int err;
        struct pcf8574_data *data = filp->private_data;
        struct device *dev = &data->i2cdev->dev; 
        char kspace_buffer[MAX_WRITE_SIZE] = "";

        dev_info(dev, "Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                dev_err(dev, "Bad input number.\n");
                return -ERANGE;
        }

        err = copy_from_user(kspace_buffer, buf, count);
        if (err) {
                dev_err(dev,
                        "Can't copy data from user space.\n");
                return -EFAULT;		
        }

        err = kstrtol(kspace_buffer, 0, (long*)&data->port_state);
        if (err) {
                dev_err(dev,
                        "Can't convert data to integer.\n");
                return err;
        }

        if (data->port_state < 0x00  ||  data->port_state > 0xff) {
                dev_err(dev, "Bad voltage value.\n");
                return -EINVAL;
        }

        err = regmap_write(data->rmap, 0x00, data->port_state);
        if (err) {
                dev_err(dev, "I2C communication error.\n");
                return err;
        }

        return count;
}


static ssize_t pcf8574_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        int err;
        struct pcf8574_data *data = filp->private_data;
        struct device *dev = &data->i2cdev->dev; 
        unsigned char port_buf[MAX_READ_SIZE];

        dev_info(dev, "Read from device file.\n");

        err = regmap_read(data->rmap, 0xff, &data->port_state);
        if (err) {
                dev_err(dev, "I2C communication error.\n");
                return err;
        }

        sprintf(port_buf, "0x%X\n", data->port_state);
     
        err = copy_to_user(buf, port_buf, strlen(port_buf));
        if (err) {
                dev_err(dev, "Can't copy data to uder space.\n");
                return -EIO;
        }
                                
        return count;
}


int pcf8574_open(struct inode *node, struct file *filp)
{       
        struct pcf8574_data *data = container_of(filp->private_data,
                                                 struct pcf8574_data,
                                                 mdev);

        dev_info(&data->i2cdev->dev, "Driver file open.\n");
        
        filp->private_data = data;
        return 0;
}


int pcf8574_release(struct inode *node, struct file *filp)
{
        struct pcf8574_data *data = filp->private_data;

        dev_info(&data->i2cdev->dev, "Driver file close.\n");

        filp->private_data = NULL;
        return 0;
}


static struct file_operations fops = 
{
        .owner = THIS_MODULE,
        .write = pcf8574_write,
        .read = pcf8574_read,
        .open = pcf8574_open,
        .release = pcf8574_release 
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
        int err;
        struct pcf8574_data *data;
        struct regmap_config reg_conf;

        dev_info(&client->dev, "I2C IO Driver Probed.\n");
        
        data = devm_kzalloc(&client->dev, sizeof(struct pcf8574_data),
                            GFP_KERNEL);
        if (IS_ERR(data)) {
               dev_err(&client->dev, "Can't allocate memory for device data.\n");
               goto err_alloc;
        }            

        data->mdev.minor = MISC_DYNAMIC_MINOR;
        data->mdev.name = MY_DEV_NAME;
        data->mdev.fops = &fops;
        err = misc_register(&data->mdev);
        if (err) {
                dev_err(&client->dev, "Misc device registration failed.\n");
                goto err_misc;
        }

        /* regmap config */
        memset(&reg_conf, 0, sizeof(reg_conf));
        reg_conf.reg_bits = 8;
        reg_conf.val_bits = 8;
        reg_conf.writeable_reg = writeable_reg;
        reg_conf.readable_reg = readable_reg;
;

        /* regmap init */
        data->rmap = devm_regmap_init_i2c(client, &reg_conf);
        if (IS_ERR(data->rmap)) {
                dev_err(&client->dev, "Can't init spi regmap.\n");
                goto err_rmap;
        }

        data->i2cdev = client;
        dev_set_drvdata(&client->dev, data);
        
        return 0;


err_rmap:
err_misc:
err_alloc:
        return -1;
}


static void pcf8574_remove(struct i2c_client *client)
{
        struct pcf8574_data *data = dev_get_drvdata(&client->dev);
        dev_info(&client->dev, "I2C IO Driver Removed\n");
        misc_deregister(&data->mdev);
}


static const struct of_device_id pcf8574_of_id[] = {
        { .compatible = "microchip,pcf8574" },
        {},
};
MODULE_DEVICE_TABLE(of, pcf8574_of_id); 


static struct i2c_driver pcf8574_driver = {
        .probe = pcf8574_probe,
        .remove = pcf8574_remove,
        .driver = {
                .name = "pcf8574",
                .of_match_table = pcf8574_of_id,
                .owner = THIS_MODULE,
        },
};
module_i2c_driver(pcf8574_driver);

MODULE_LICENSE("GPL v2");
