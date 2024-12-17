#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/regmap.h>

#define PCF8574_WRITE_BUFF_SIZE 6
#define PCF8574_READ_BUFF_SIZE 6
#define PCF8574_PORT_MAX_VALUE 0xff
#define PCF8574_PORT_MIN_VALUE 0x00


struct pcf8574 {
        struct i2c_client *client;
        struct regmap *rmap;
        struct miscdevice mdev;
        int port_state;
};

/*------------------ Obsługa urządzenia znakowego ------------------*/

static ssize_t pcf8574_write(struct file *filp, const char *buf,
                             size_t count, loff_t *f_pos) 
{
        int err = 0;
        int port_state;
        char kbuff[PCF8574_WRITE_BUFF_SIZE] = "";
        struct pcf8574 *pcf8574 = container_of(filp->private_data,
                                                 struct pcf8574, mdev);

        dev_info(&pcf8574->client->dev, "Write to device file.\n");

        if (count > PCF8574_WRITE_BUFF_SIZE) {
                dev_err(&pcf8574->client->dev, "Bad input number.\n");
                return -ERANGE;
        }

        err = copy_from_user(kbuff, buf, count);
        if (err) {
                dev_err(&pcf8574->client->dev,
                        "Can't copy data from user space.\n");
                return -EFAULT;		
        }

        err = kstrtol(kbuff, 0, (long *) &port_state);
        if (err) {
                dev_err(&pcf8574->client->dev,
                        "Can't convert data to integer.\n");
                return err;
        }

        if (port_state < PCF8574_PORT_MIN_VALUE  ||
            port_state > PCF8574_PORT_MAX_VALUE) {
                dev_err(&pcf8574->client->dev, "Bad voltage value.\n");
                return -EINVAL;
        }

        pcf8574->port_state = port_state;
        err = regmap_write(pcf8574->rmap, PCF8574_PORT_MIN_VALUE, port_state);
        if (err) {
                dev_err(&pcf8574->client->dev, "I2C communication error.\n");
                return err;
        }

        return count;
}


static ssize_t pcf8574_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        int err = 0;
        unsigned char port_buf[PCF8574_READ_BUFF_SIZE] = "";
        struct pcf8574 *pcf8574 = container_of(filp->private_data,
                                                 struct pcf8574, mdev);
        

        dev_info(&pcf8574->client->dev, "Read from device file.\n");

        err = regmap_read(pcf8574->rmap, PCF8574_PORT_MAX_VALUE,
                          &pcf8574->port_state);
        if (err) {
                dev_err(&pcf8574->client->dev, "I2C communication error.\n");
                return err;
        }

        sprintf(port_buf, "0x%X\n", pcf8574->port_state);
     
        err = copy_to_user(buf, port_buf, strlen(port_buf));
        if (err) {
                dev_err(&pcf8574->client->dev,"Can't copy data to uder space.\n");
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
/*------------------------------------------------------------------*/


/*----------------------------- REGMAP -----------------------------*/
static bool writeable_reg(struct device *dev, unsigned int reg)
{
        return (reg == PCF8574_PORT_MIN_VALUE);
}
  
static bool readable_reg(struct device *dev, unsigned int reg)
{
        return (reg == PCF8574_PORT_MAX_VALUE);
}
/*------------------------------------------------------------------*/


static int pcf8574_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
        int err = 0;
        struct pcf8574 *pcf8574;
        struct regmap_config reg_conf;
        
        /* pcf8574 data allocation */
        pcf8574 = devm_kzalloc(&client->dev, sizeof(struct pcf8574),
                            GFP_KERNEL);
        if (IS_ERR(pcf8574)) {
               dev_err(&client->dev, "Memory allocation failed.\n");
               err = PTR_ERR(pcf8574->rmap);
               goto out_ret_err;
        }         

        pcf8574->client = client;   
        
        /* miscdevice config */
        pcf8574->mdev.minor = MISC_DYNAMIC_MINOR;
        pcf8574->mdev.name = "pcf8574";
        pcf8574->mdev.fops = &fops;

        /* regmap config */
        memset(&reg_conf, 0, sizeof(reg_conf));
        reg_conf.reg_bits = 8;
        reg_conf.val_bits = 8;
        reg_conf.writeable_reg = writeable_reg;
        reg_conf.readable_reg = readable_reg;

        /* miscdevice init */
        err = misc_register(&pcf8574->mdev);
        if (err) {
                dev_err(&client->dev, "Misc device registration failed.\n");
                goto out_ret_err;
        }
        
        /* regmap init */
        pcf8574->rmap = devm_regmap_init_i2c(client, &reg_conf);
        if (IS_ERR(pcf8574->rmap)) {
                dev_err(&client->dev, "Can't init spi regmap.\n");
                err = PTR_ERR(pcf8574->rmap);
                goto out_deregister_misc;
        }

        dev_set_drvdata(&client->dev, pcf8574);
        
        dev_info(&client->dev, "I2C IO Driver Probed.\n");
        goto out_ret_err;


out_deregister_misc:
        misc_deregister(&pcf8574->mdev);
out_ret_err:
        return err;
}


static void pcf8574_remove(struct i2c_client *client)
{
        struct pcf8574 *pcf8574 = dev_get_drvdata(&client->dev);
        misc_deregister(&pcf8574->mdev);
        dev_info(&client->dev, "I2C IO Driver Removed\n");
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
