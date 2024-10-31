#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>


/*
* Metadane
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Sterownik znakowy do obsługi przetwornika DAC MCP4921");

/* Dzięki tej definicji, funkcja pr_info doklei na początku każdej wiadomości nazwę naszego modułu */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define MY_DEV_NAME "dac_mcp4921"
#define MY_CLASS_NAME "my_spi_devices"
#define SPI_BUS 0
#define DAC_MCP4921_REF_VOLTAGE_mV 3300

static struct class *my_class;
static struct cdev my_cdev;
static dev_t my_devt;

static struct spi_device *dac_mcp4921_dev;

void DAC_MCP4921_Set(unsigned int voltage_12bit)
{
    #define MCP4921_CFG_BITS (0x03 << 12)                //Kanal A, Unbuffered Vref, Gain=1, DAC Enable
    #define DATA_BUF_LEN 2

    unsigned int data = MCP4921_CFG_BITS | (voltage_12bit & 0x0fff);
    unsigned char spi_buff[DATA_BUF_LEN];
    
    pr_info("Set voltage to %d (12bit)\n", voltage_12bit);

    
    spi_buff[0] = ( (data & 0xff00) >> 8 );
    spi_buff[1] = ( data & 0x00ff );

    pr_info("Send buffer: %x %x", spi_buff[0], spi_buff[1]);

    spi_write(dac_mcp4921_dev, spi_buff, DATA_BUF_LEN);
}


void DAC_MCP4921_Set_mV(unsigned int voltage_mV)
{
    #define BINARY_VAL_FOR_1mV 0x9ee0       /* unsigned Q1.15 -> 1,2412     (1mV to wartość 1,2412 dla przetwornika) */
    unsigned int voltage_12bit = ( ((unsigned long)voltage_mV * (unsigned long)BINARY_VAL_FOR_1mV)>>15 );    /* Konwersja Q16.15 do Q16.0 */
    pr_info("%d", voltage_12bit);
    DAC_MCP4921_Set(voltage_12bit);
}


/* Funkcja wywoływana podczas otwierania pliku urządzenia */
static int DeviceOpen(struct inode *device_file, struct file *instance)
{
    pr_info("Module device file open.\n");
    return 0;
}

/* Funkcja wywoływana podczas zamykania pliku urządzenia */
static int DeviceClose(struct inode *device_file, struct file *instance)
{
    pr_info("Module device file close.\n");
    return 0;
}

/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t DeviceWrite(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    #define MAX_WRITE_SIZE 5    /* 4 znaki na liczbę i jeden znak Null */

    char kspace_buffer[MAX_WRITE_SIZE];
    int voltage_mV, ret_val;

    pr_info("Write to device file.\n");

    if(count > MAX_WRITE_SIZE)    
    {
        pr_info("Bad input number.\n");
        return -ERANGE;
    }

    if( copy_from_user(kspace_buffer, buf, count) )
    {
        pr_info("Can't copy data from user space\n");
        return -EFAULT;		
    }

    ret_val = kstrtol(kspace_buffer, 0, (long*)&voltage_mV);
    if(ret_val)
    {
        pr_info("Can't convert data to integer\n");
        return ret_val;
    }

    if(voltage_mV < 0    ||   voltage_mV > DAC_MCP4921_REF_VOLTAGE_mV)
    {
        pr_info("Bad voltage value\n");
        return -EINVAL;
    }

    DAC_MCP4921_Set_mV(voltage_mV);

    return count;
}


/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
    .owner=THIS_MODULE,
    .open=DeviceOpen,
    .release=DeviceClose,
    .write=DeviceWrite
};


/* Funkcja wywoływana podczas ładowania modułu do jądra linux */
static int __init ModuleInit(void)
{
    struct spi_master *master;

    /* Struktura zawierająca konfigurację spi*/
    struct spi_board_info spi_dev_info = {
        .modalias = "mcp4921",
        .max_speed_hz = 100000,
        .bus_num = SPI_BUS,
        .chip_select = 0,
        .mode = 3
    };

    pr_info("Module init.\n");

    /* Pozyskanie mastera skojarzonego z podanym numerem interfejsu SPI*/
    master = spi_busnum_to_master(SPI_BUS);
    if(!master)
    {
        pr_info("Cannot find spi bus with number %d\n", SPI_BUS);
        return -EIO;
    }

    /*Utworzenie urządzenia SPI*/
    dac_mcp4921_dev = spi_new_device(master, &spi_dev_info);
    if(!dac_mcp4921_dev)
    {
        pr_info("Cannot create device\n");
        return -EIO;
    }

    dac_mcp4921_dev->bits_per_word = 8;

    /* Skonfigurowanie magistrali spi do komunikacji z urządzeniem*/
    if(spi_setup(dac_mcp4921_dev) != 0)
    {
        pr_info("Cannot set bus configuration");
        spi_unregister_device(dac_mcp4921_dev);
        return -EIO;
    }

    /* Zaalokowanie numerów MAJOR i MINOR dla urządzenia */
    alloc_chrdev_region(&my_devt, 0, 1, MY_DEV_NAME);

    /* Stworzenie klasy urządzeń, widocznej w /sys/class */
    my_class = class_create(THIS_MODULE, MY_CLASS_NAME);

    /* Inicjalizacja urządzenia znakowego - podpięcie funkcji do operacji na plikach (file operations) */
    cdev_init(&my_cdev, &fops);

    /* Dodanie urządzenia do systemu */
    cdev_add(&my_cdev, my_devt, 1);

    /* Stworzenie pliku w przestrzeni użytkownika (w /dev), reprezentującego urządzenie */
    device_create(my_class, NULL, my_devt, NULL, MY_DEV_NAME);


    pr_info("Alocated device MAJOR number: %d\n", MAJOR(my_devt));
    return 0;
}

/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit ModuleExit(void)
{
    pr_info("Module exit.\n"); 

    /* Usunięcie urządzenia spi */
    if(dac_mcp4921_dev)
    {
        spi_unregister_device(dac_mcp4921_dev);
    }
    
    /* Usunięcie pliku urządzenia z przestrzeni użytkownika */
    device_destroy(my_class, my_devt);

    /* Usunięcie urządzenia z systemu */
    cdev_del(&my_cdev);

    /* Usunięcie klasy urządzenia */
    class_unregister(my_class);
    class_destroy(my_class);

    /* Zwolnienie przypisanych numerów MAJOR i MINOR */
    unregister_chrdev_region(my_devt, 1);
}


module_init(ModuleInit);
module_exit(ModuleExit); 

