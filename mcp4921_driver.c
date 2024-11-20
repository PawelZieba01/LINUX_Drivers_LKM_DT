#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define MCP4921_CFG_BITS (0x03 << 12)   /* Kanal A, Unbuffered Vref, 
                                        Gain=1, DAC Enable */
#define BINARY_VAL_FOR_1mV 0x9ee0       /* unsigned Q1.15 -> 1,2412     
                                        (1mV to wartość 1,2412 
                                        dla przetwornika) */


static struct spi_device *dac_mcp4921_dev;


void DAC_MCP4921_Set(unsigned int voltage_12bit)
{        
        unsigned int data = MCP4921_CFG_BITS | (voltage_12bit & 0x0fff);
        unsigned char spi_buff[2];
        
        spi_buff[0] = ( (data & 0xff00) >> 8 );
        spi_buff[1] = ( data & 0x00ff );

        pr_info("Send buffer: %x %x", spi_buff[0], spi_buff[1]);

        spi_write(dac_mcp4921_dev, spi_buff, 2);
}


void DAC_MCP4921_Set_mV(unsigned int voltage_mV)
{
         /* Konwersja Q16.15 do Q16.0 */
        unsigned int voltage_12bit = (((unsigned long)voltage_mV *
                                     (unsigned long)BINARY_VAL_FOR_1mV) >> 15);
        
        pr_info("Set voltage to %d [mV]\n", voltage_mV);
        pr_info("Set voltage to %d [12bit]\n", voltage_12bit);

        DAC_MCP4921_Set(voltage_12bit);
}


/*----- Parametr modułu przeznaczony do zapisu -----*/
static long int dac_voltage_mV = 0;

static ssize_t dac_voltage_mV_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
        int res;

        res = kstrtol(buf, 0, &dac_voltage_mV);
        if (res)
                return res;
        
        if (dac_voltage_mV < 0  ||  dac_voltage_mV > 3300)
                return -EINVAL;
        
        DAC_MCP4921_Set_mV(dac_voltage_mV);
        return count;
}

DEVICE_ATTR_WO(dac_voltage_mV);
/*---------------------------------------------------*/


static int mtm_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

        dac_mcp4921_dev = dev;
        
        /* Utworzenie pliku reprezentującego atrybut urządzenia */
        device_create_file(&dev->dev, &dev_attr_dac_voltage_mV);

        return 0;
}

static int mtm_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
        device_remove_file(&dev->dev, &dev_attr_dac_voltage_mV);
        return 0;
}


static const struct of_device_id mtm_of_id[] = {
        { .compatible = "microchip,my_dac" },
        {},
};
MODULE_DEVICE_TABLE(of, mtm_of_id); 


static struct spi_driver mtm_driver = {
        //.id_table = my_dac;
        .probe = (void*) mtm_probe,
        .remove = (void*) mtm_remove,
        .driver = {
                .name = "my_dac",
                .of_match_table = mtm_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mtm_driver);

MODULE_LICENSE("GPL v2");
