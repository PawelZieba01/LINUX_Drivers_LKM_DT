#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define MCP4921_CFG_BITS (0x03 << 12)   /* Kanal A, Unbuffered Vref, 
                                        Gain=1, DAC Enable */
#define BINARY_VAL_FOR_1mV 0x9ee0       /* unsigned Q1.15 -> 1,2412     
                                        (1mV to wartość 1,2412 
                                        dla przetwornika) */


struct mcp4921_data {
        struct spi_device *dev;
        long int voltage_mV;
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
        unsigned int voltage_12bit = (((unsigned long)voltage_mV *
                                     (unsigned long)BINARY_VAL_FOR_1mV) >> 15);
        return mcp4921_set(dev, voltage_12bit);
}


static ssize_t voltage_mV_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
        int err;
        struct mcp4921_data *data = dev_get_drvdata(dev);

        err = kstrtol(buf, 0, &data->voltage_mV);
        if (err) {
                dev_err(dev, "Can't convert str to int\n");
                return -EINVAL;
        }
        
        if (data->voltage_mV < 0  ||  data->voltage_mV > 3300) {
                dev_err(dev, "Bad voltage value\n");
                return -EINVAL;
        }
                
        err = mcp4921_set_mV(data->dev, data->voltage_mV);
        if (err) {
               dev_err(dev, "Can't communicate with mcp4921\n");
               return err;
        }

        return count;
}

DEVICE_ATTR_WO(voltage_mV);
/*---------------------------------------------------*/


static int mcp4921_probe(struct spi_device *dev)
{
        int err;
        struct mcp4921_data *data;

        dev_info(&dev->dev, "SPI DAC Driver Probed\n");
        
        /* Utworzenie pliku reprezentującego atrybut urządzenia */
        err = device_create_file(&dev->dev, &dev_attr_voltage_mV);
        if (err) {
               dev_err(&dev->dev, "Can't create device attribute file\n");
               goto err_attr;
        }

        /* Zarezerwowanie pamięci dla danych urządzenia */
        data = devm_kzalloc(&dev->dev, sizeof(struct mcp4921_data), GFP_KERNEL);
        if (IS_ERR(data)) {
               dev_err(&dev->dev, "Can't allocate device data\n");
               goto err_alloc;
        }

        data->dev = dev;
        dev_set_drvdata(&dev->dev, data);

        return 0;

err_alloc:
        device_remove_file(&dev->dev, &dev_attr_voltage_mV);
err_attr:
        return -1;
}


static void mcp4921_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
        device_remove_file(&dev->dev, &dev_attr_voltage_mV);
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
