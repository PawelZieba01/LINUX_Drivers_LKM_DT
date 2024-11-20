#include <linux/spi/spi.h>
#include <linux/cdev.h>

#define MY_DEV_NAME "dac_mcp4921"
#define MY_CLASS_NAME "spi_dac_devices"
#define SPI_BUS 0
#define DAC_MCP4921_REF_VOLTAGE_mV 3300
#define MCP4921_CFG_BITS (0x03 << 12)           /* Kanal A, Unbuffered Vref,
                                                   Gain=1, DAC Enable */
#define BINARY_VAL_FOR_1mV 0x9ee0               /* unsigned Q1.15 -> 1,2412
                                                   (1mV to wartość 1,2412 
                                                   dla przetwornika) */
#define MAX_WRITE_SIZE 10                       /* 4 znaki na liczbę i jeden 
                                                   znak Null */                                                   

static struct class *my_class;
static struct cdev my_cdev;
static dev_t my_devt;
static struct spi_device *dac_mcp4921_dev;


void DAC_MCP4921_Set(unsigned int voltage_12bit)
{
        unsigned int data = MCP4921_CFG_BITS | (voltage_12bit & 0x0fff);
        unsigned char spi_buff[2];
        
        spi_buff[0] = ( (data & 0xff00) >> 8 );
        spi_buff[1] = ( data & 0x00ff );

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


/*---------- Obsługa urządzenia znakowego ----------*/

/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t device_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        char kspace_buffer[MAX_WRITE_SIZE] = "";
        int voltage_mV, ret_val;

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

        ret_val = kstrtol(kspace_buffer, 0, (long*)&voltage_mV);
        if (ret_val) {
                pr_err("Can't convert data to integer\n");
                return ret_val;
        }

        if (voltage_mV < 0  ||  voltage_mV > DAC_MCP4921_REF_VOLTAGE_mV) {
                pr_err("Bad voltage value\n");
                return -EINVAL;
        }

        DAC_MCP4921_Set_mV(voltage_mV);

        return count;
}


/* Struktura przechowująca informacje o operacjach możliwych do
   wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner=THIS_MODULE,
        .write=device_write
};

/*---------------------------------------------------*/




static int mtm_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

        /* Zapisanie wskaźnika do urządzenia SPI */
        dac_mcp4921_dev = dev;

        /* Zaalokowanie numerów MAJOR i MINOR dla urządzenia */
        alloc_chrdev_region(&my_devt, 0, 1, MY_DEV_NAME);

        /* Stworzenie klasy urządzeń, widocznej w /sys/class */
        my_class = class_create(THIS_MODULE, MY_CLASS_NAME);

        /* Inicjalizacja urządzenia znakowego - podpięcie funkcji
           do operacji na plikach (file operations) */
        cdev_init(&my_cdev, &fops);

        /* Dodanie urządzenia do systemu */
        cdev_add(&my_cdev, my_devt, 1);

        /* Stworzenie pliku w przestrzeni użytkownika (w /dev),
           reprezentującego urządzenie */
        device_create(my_class, NULL, my_devt, NULL, MY_DEV_NAME);

        pr_info("Alocated device MAJOR number: %d\n", MAJOR(my_devt));
        
        return 0;
}

static int mtm_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");

         /* Usunięcie pliku urządzenia z przestrzeni użytkownika */
        device_destroy(my_class, my_devt);

        /* Usunięcie urządzenia z systemu */
        cdev_del(&my_cdev);

        /* Usunięcie klasy urządzenia */
        class_unregister(my_class);
        class_destroy(my_class);

        /* Zwolnienie przypisanych numerów MAJOR i MINOR */
        unregister_chrdev_region(my_devt, 1);
        
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
