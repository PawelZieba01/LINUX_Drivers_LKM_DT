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
MODULE_DESCRIPTION("Sterownik znakowy do obsługi ekspandera IO MCP23S09");

/* Dzięki tej definicji, funkcja pr_info doklei na początku każdej wiadomości nazwę naszego modułu */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define MY_DEV_NAME "io_mcp23s09"
#define MY_CLASS_NAME "my_spi_devices"
#define SPI_BUS 0

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09

static struct class *my_class;
static struct cdev my_cdev;
static dev_t my_devt;

static struct spi_device *io_mcp23s09_dev;

void IO_MCP23S09_Set(unsigned char port_value)
{
    unsigned char spi_buff[6] = {
        IO_WRITE_OPCODE,
        IO_DIR_REG,
        0x00,               /* Ustawienie pinów jako wyjścia */
        IO_WRITE_OPCODE,
        IO_GPIO_REG,
        port_value
    };
    
    pr_info("Set port value to %x \n", port_value);


    spi_write(io_mcp23s09_dev, &spi_buff[0], 3);    /* Ustawienie pinów jako wyjścia */
    spi_write(io_mcp23s09_dev, &spi_buff[3], 3);    /* Ustawienie wartości pinów */
}




int IO_MCP23S09_Get(void)
{
    unsigned char spi_tx_buff[5] = {
        IO_WRITE_OPCODE,
        IO_DIR_REG,
        0xff,               /* Ustawienie pinów jako wejścia */
        IO_READ_OPCODE,
        IO_GPIO_REG
    };

    unsigned char spi_rx_buff[] = {0};

    pr_info("Get port value.\n");

    spi_write_then_read(io_mcp23s09_dev, &spi_tx_buff[0], 3, 0, 0);                 /* Ustawienie pinów jako wejścia */
    spi_write_then_read(io_mcp23s09_dev, &spi_tx_buff[3], 2, &spi_rx_buff[0], 1);   /* Odczyt danych */

    return spi_rx_buff[0];
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

/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t DeviceRead(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    #define BUFF_LEN 6
    
    unsigned char port_value; 
    unsigned char port_buf[BUFF_LEN];

    pr_info("Read from device file.\n");

    port_value = IO_MCP23S09_Get();
    if(port_value < 0)
    {
        pr_info("Can't get port value.\n");
        return port_value;
    }

    sprintf(port_buf, "0x%X\n", port_value);

    if(*f_pos >= BUFF_LEN)
    {
        return 0; /*EOF*/
    }

    if(*f_pos + count > BUFF_LEN)
    {
        count = BUFF_LEN - *f_pos;
    }

    if( copy_to_user(buf, port_buf, BUFF_LEN) != 0)
    {
        return -EIO;
    }
   
    *f_pos += count;
    return count;
}

/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t DeviceWrite(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    #define MAX_WRITE_SIZE 5    /* 4 znaki na liczbę i jeden znak Null */

    char kspace_buffer[MAX_WRITE_SIZE];
    int port_value, ret_val;

    pr_info("Write to device file.\n");

    if(count > MAX_WRITE_SIZE)    
    {
        pr_info("Bad input number.\n");
        return -ERANGE;
    }

    if( copy_from_user(kspace_buffer, buf, count) )
    {
        pr_info("Can't copy data from user space.\n");
        return -EFAULT;		
    }

    ret_val = kstrtol(kspace_buffer, 0, (long*)&port_value);
    if(ret_val)
    {
        pr_info("Can't convert data to integer.\n");
        return ret_val;
    }

    if(port_value < 0    ||   port_value > 255)
    {
        pr_info("Bad port value.\n");
        return -EINVAL;
    }

    IO_MCP23S09_Set(port_value);

    return count;
}


/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
    .owner=THIS_MODULE,
    .open=DeviceOpen,
    .release=DeviceClose,
    .write=DeviceWrite,
    .read=DeviceRead
};


/* Funkcja wywoływana podczas ładowania modułu do jądra linux */
static int __init ModuleInit(void)
{
    struct spi_master *master;

    /* Struktura zawierająca konfigurację spi*/
    struct spi_board_info spi_dev_info = {
        .modalias = "mcp23s09",
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
        pr_info("Cannot find spi bus with number %d.\n", SPI_BUS);
        return -EIO;
    }

    /*Utworzenie urządzenia SPI*/
    io_mcp23s09_dev = spi_new_device(master, &spi_dev_info);
    if(!io_mcp23s09_dev)
    {
        pr_info("Cannot create device.\n");
        return -EIO;
    }

    io_mcp23s09_dev->bits_per_word = 8;

    /* Skonfigurowanie magistrali spi do komunikacji z urządzeniem*/
    if(spi_setup(io_mcp23s09_dev) != 0)
    {
        pr_info("Can't set bus configuration.");
        spi_unregister_device(io_mcp23s09_dev);
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


    pr_info("Alocated device MAJOR number: %d.\n", MAJOR(my_devt));
    return 0;
}

/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit ModuleExit(void)
{
    pr_info("Module exit.\n"); 

    /* Usunięcie urządzenia spi */
    if(io_mcp23s09_dev)
    {
        spi_unregister_device(io_mcp23s09_dev);
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

