#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

static struct spi_device * dac_mcp4921_dev;

void DAC_MCP4921_Set(unsigned int voltage_12bit)
{
    #define MCP4921_CFG_BITS (0x03 << 12)                //Kanal A, Unbuffered Vref, Gain=1, DAC Enable
    #define DATA_BUF_LEN 2

    unsigned int data = MCP4921_CFG_BITS | (voltage_12bit & 0x0fff);
    unsigned char spi_buff[DATA_BUF_LEN];
    
    spi_buff[0] = ( (data & 0xff00) >> 8 );
    spi_buff[1] = ( data & 0x00ff );

    pr_info("Send buffer: %x %x", spi_buff[0], spi_buff[1]);

    spi_write(dac_mcp4921_dev, spi_buff, DATA_BUF_LEN);
}


void DAC_MCP4921_Set_mV(unsigned int voltage_mV)
{
    #define BINARY_VAL_FOR_1mV 0x9ee0       /* unsigned Q1.15 -> 1,2412     (1mV to wartość 1,2412 dla przetwornika) */
    unsigned int voltage_12bit = ( ((unsigned long)voltage_mV * (unsigned long)BINARY_VAL_FOR_1mV)>>15 );    /* Konwersja Q16.15 do Q16.0 */
    
    pr_info("Set voltage to %d [mV]\n", voltage_mV);
    pr_info("Set voltage to %d [12bit]\n", voltage_12bit);

    DAC_MCP4921_Set(voltage_12bit);
}


/*----- Parametr modułu przeznaczony do zapisu -----*/
static long int dac_voltage_mV = 0;

static ssize_t dac_voltage_mV_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int res;

    res = kstrtol(buf, 0, &dac_voltage_mV);
    if(res)     //Jeśli wystąpi bład podczas konwersji
    {
        return res;
    }
    
    if(dac_voltage_mV < 0   ||   dac_voltage_mV > 3300)
    {
        return -EINVAL;
    }

    DAC_MCP4921_Set_mV(dac_voltage_mV);
    return count;
}

DEVICE_ATTR_WO(dac_voltage_mV);
/*---------------------------------------------------*/




static int mtm_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

        dac_mcp4921_dev = dev;
        
        /* Utworzenie pliku reprezentującego atrybut w przestrzeni użytkownika */
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

MODULE_DEVICE_TABLE(of, mtm_of_id);                     //platform - dopasowuje sterownik po compatible


// static const struct spi_device_id my_dac[] = {
//         {"my_dac", 0},
//         {},
// };

// MODULE_DEVICE_TABLE(spi, my_dac);                    //spi - dopasowuje sterownik po nazwie urządzenia z tablicy typu spi_device_id


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

/* Powyższe komentarze dotyczące mechanizmu dopasowania sterowników i urządzeń dt zostaną usunięte - wykorzystano mechanizm of, dopasowujący poprzez 'compatible' */