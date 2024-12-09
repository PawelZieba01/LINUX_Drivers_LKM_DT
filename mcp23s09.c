#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09

#define MY_DEV_NAME "io_mcp23s09"

#define MAX_WRITE_SIZE 10
#define MAX_READ_SIZE 7


static struct spi_device *mcp23s09_spi_dev;
static struct regmap *mcp23s09_regmap;


int mcp23s09_set_port(unsigned int port_value)
{               
        int res;
        dev_info(&mcp23s09_spi_dev->dev, "Set port value.\n");

        /* Ustawienie kierunku portu jako wyjście */
        res = regmap_write(mcp23s09_regmap, (IO_WRITE_OPCODE << 8 |
                                             IO_DIR_REG), 0x00);  
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "SPI Communication write error.\n");
                return res;
        }

        /* Ustawienie wartości portu */                     
        res = regmap_write(mcp23s09_regmap, (IO_WRITE_OPCODE << 8 | 
                                             IO_GPIO_REG), port_value);
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "SPI Communication write error.\n");
                return res;
        }

        return 0;
}


int mcp23s09_get_port(unsigned int *value)
{
        int res;
        unsigned int spi_buff;

        dev_info(&mcp23s09_spi_dev->dev, "Get port value.\n");

        /* Ustawienie kierunku portu jako wejście */
        res = regmap_write(mcp23s09_regmap, (IO_WRITE_OPCODE << 8 |
                                             IO_DIR_REG), 0xff);
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "SPI Communication write error.\n");
                return res;
        }

        /* Odczyt wartości portu */  
        res = regmap_read(mcp23s09_regmap, (IO_READ_OPCODE << 8 | IO_GPIO_REG),
                          &spi_buff);  
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "SPI Communication read error.\n");
                return res;
        }

        *value = spi_buff;
        return 0;
}


/*------------------ Obsługa urządzenia znakowego ------------------*/
/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t mcp23s09_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{       
        int res;
        char kspace_buffer[MAX_WRITE_SIZE] = "";
        unsigned int port_value;
        
        dev_info(&mcp23s09_spi_dev->dev, "Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                dev_err(&mcp23s09_spi_dev->dev, "Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                dev_err(&mcp23s09_spi_dev->dev, "Can't copy data from user space\n");
                return -EFAULT;		
        }

        res = kstrtol(kspace_buffer, 0, (long*)&port_value);
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "Can't convert data to integer\n");
                return res;
        }

        if (port_value < 0x00  ||  port_value > 0xff) {
                dev_err(&mcp23s09_spi_dev->dev, "Bad voltage value\n");
                return -EINVAL;
        }

        res = mcp23s09_set_port(port_value);
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "Can't set value to port.\n");
                return res;
        }

        return count;
}


/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t mcp23s09_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        int res;
        unsigned int port_value; 
        unsigned char port_buf[MAX_READ_SIZE];

        dev_info(&mcp23s09_spi_dev->dev, "Read from device file.\n");

        res = mcp23s09_get_port(&port_value);
        if (res) {
                dev_err(&mcp23s09_spi_dev->dev, "Can't get port value.\n");
                return res;
        }

        sprintf(port_buf, "0x%X\n", port_value);

        if (copy_to_user(buf, port_buf, strlen(port_buf)) != 0)
                return -EIO;
                
        return count;
}


/* Struktura przechowująca informacje o operacjach możliwych do
   wykonania na pliku urządzenia */
static struct file_operations fops = 
{
        .owner = THIS_MODULE,
        .write = mcp23s09_write,
        .read = mcp23s09_read
};


struct miscdevice mcp23s09_device = 
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = MY_DEV_NAME,
    .fops = &fops,
};
/*------------------------------------------------------------------*/


/*----------------------------- REGMAP -----------------------------*/
static bool writeable_reg(struct device *dev, unsigned int reg)
{
        switch (reg) {
                case (IO_WRITE_OPCODE << 8 | IO_DIR_REG):
                case (IO_WRITE_OPCODE << 8 | IO_GPIO_REG):
                        return true;
        }
        return false;
}
  
static bool readable_reg(struct device *dev, unsigned int reg)
{
        if (reg == (IO_READ_OPCODE << 8 | IO_GPIO_REG))
                return true;
        return false;
}
/*------------------------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        int res;
        struct regmap_config reg_conf;

        dev_info(&dev->dev, "SPI IO Driver Probed\n");
        
        mcp23s09_spi_dev = dev;              

        /* misc device create */
        res = misc_register(&mcp23s09_device);
        if (res) {
                dev_err(&dev->dev, "Misc device registration failed!");
                return res;
        }

        /* regmap config */
        memset(&reg_conf, 0, sizeof(reg_conf));
        reg_conf.reg_bits = 16;
        reg_conf.val_bits = 8;
        reg_conf.writeable_reg = writeable_reg;
        reg_conf.readable_reg = readable_reg;

        /* regmap init */
        mcp23s09_regmap = devm_regmap_init_spi(dev, &reg_conf);
        return 0;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Removed\n");
        misc_deregister(&mcp23s09_device);
        //regmap jest usuwany automatycznie!!!
}


static const struct of_device_id mcp23s09_of_id[] = {
        { .compatible = "microchip,mcp23s09_io" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp23s09_of_id); 


static struct spi_driver mcp23s09_driver = {
        .probe = mcp23s09_probe,
        .remove = mcp23s09_remove,
        .driver = {
                .name = "mcp23s09_io",
                .of_match_table = mcp23s09_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp23s09_driver);

MODULE_LICENSE("GPL v2");
