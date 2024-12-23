#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define MCP4921_CFG_BITS (0x03 << 12)   /* Kanal A, Unbuffered Vref, 
                                        Gain=1, DAC Enable */
#define MCP4921_BIN_VAL_FOR_1mV 0x9ee0  /* unsigned Q1.15 -> 1,2412     
                                        (1mV to wartość 1,2412 
                                        dla przetwornika) */


struct mcp4921 {
        struct spi_device *dev;
        unsigned int voltage_mV;
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
        unsigned int voltage_12bit = ((voltage_mV *
                                      MCP4921_BIN_VAL_FOR_1mV) >> 15);
        return mcp4921_set(dev, voltage_12bit);
}


static ssize_t mcp4921_voltage_mV_store(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count)
{
        int err = 0;
        struct mcp4921 *mcp4921 = dev_get_drvdata(dev);

        err = kstrtol(buf, 0, (unsigned long *)&mcp4921->voltage_mV);
        if (err) {
                dev_err(dev, "Bad input value.\n");
                return -EINVAL;
        }
        
        if (mcp4921->voltage_mV < 0  ||  mcp4921->voltage_mV > 3300) {
                dev_err(dev, "Bad voltage value\n");
                return -EINVAL;
        }
                
        err = mcp4921_set_mV(mcp4921->dev, mcp4921->voltage_mV);
        if (err) {
               dev_err(dev, "Communication with mcp4921 failed\n");
               return err;
        }

        return count;
}

DEVICE_ATTR_WO(mcp4921_voltage_mV);
/*---------------------------------------------------*/


static int mcp4921_probe(struct spi_device *dev)
{
        int err = 0;
        struct mcp4921 *mcp4921;

        /* Zarezerwowanie pamięci dla danych urządzenia */
        mcp4921 = devm_kzalloc(&dev->dev, sizeof(struct mcp4921), GFP_KERNEL);
        if (IS_ERR(mcp4921)) {
               dev_err(&dev->dev, "Device data allocation failed.\n");
               err = PTR_ERR(mcp4921);
               goto out_ret_err;
        }
        
        /* Utworzenie pliku reprezentującego atrybut urządzenia */
        err = device_create_file(&dev->dev, &dev_attr_mcp4921_voltage_mV);
        if (err) {
               dev_err(&dev->dev, "Device attribute file creation failed.\n");
               goto out_ret_err;
        }
      
        mcp4921->dev = dev;
        dev_set_drvdata(&dev->dev, mcp4921);

        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

out_ret_err:
        return err;
}


static void mcp4921_remove(struct spi_device *dev)
{
        device_remove_file(&dev->dev, &dev_attr_mcp4921_voltage_mV);
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
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
