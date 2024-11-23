#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>

#define IO_WRITE_OPCODE 0x40
#define IO_READ_OPCODE 0x41
#define IO_DIR_REG 0x00
#define IO_GPIO_REG 0x09

#define MY_DEV_NAME "io_mcp23s09"
#define MY_CLASS_NAME "spi_io_devices"

#define MAX_WRITE_SIZE 10
#define MAX_READ_SIZE 6

static struct class *my_class;
static struct cdev my_cdev;
static dev_t my_devt;

static struct spi_device *mcp23s09_spi_dev;


void mcp23s09_set_port(unsigned char port_value)
{
        unsigned char spi_buff[6] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0x00,
                IO_WRITE_OPCODE,
                IO_GPIO_REG,
                port_value
        };
        
        pr_info("Set port value to %x \n", port_value);

        spi_write(mcp23s09_spi_dev, &spi_buff[0], 3);    /* Ustawienie pinów 
                                                           jako wyjścia */
        spi_write(mcp23s09_spi_dev, &spi_buff[3], 3);    /* Ustawienie wartości
                                                           pinów */
}


unsigned char mcp23s09_get_port(void)
{
        unsigned char spi_tx_buff[5] = {
                IO_WRITE_OPCODE,
                IO_DIR_REG,
                0xff,
                IO_READ_OPCODE,
                IO_GPIO_REG
        };

        unsigned char spi_rx_buff[] = {0};

        pr_info("Get port value.\n");

        spi_write_then_read(mcp23s09_spi_dev, &spi_tx_buff[0], 3, 0, 0);
        spi_write_then_read(mcp23s09_spi_dev, &spi_tx_buff[3], 2,
                            &spi_rx_buff[0], 1);

        return spi_rx_buff[0];
}


/*------------------ Obsługa urządzenia znakowego ------------------*/
/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t mcp23s09_write(struct file *filp, const char *buf,
                           size_t count, loff_t *f_pos) 
{
        char kspace_buffer[MAX_WRITE_SIZE] = "";
        int port_value, ret;

        pr_info("Write to device file.\n");

        if (count > MAX_WRITE_SIZE) {
                pr_err("Bad input number.\n");
                return -ERANGE;
        }

        if (copy_from_user(kspace_buffer, buf, count)) {
                pr_err("Can't copy data from user space\n");
                return -EFAULT;		
        }

        ret = kstrtol(kspace_buffer, 0, (long*)&port_value);
        if (ret) {
                pr_err("Can't convert data to integer\n");
                return ret;
        }

        if (port_value < 0x00  ||  port_value > 0xff) {
                pr_err("Bad voltage value\n");
                return -EINVAL;
        }

        mcp23s09_set_port(port_value);

        return count;
}


/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t mcp23s09_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos)
{
        unsigned char port_value; 
        unsigned char port_buf[MAX_READ_SIZE];

        pr_info("Read from device file.\n");

        port_value = mcp23s09_get_port();
        sprintf(port_buf, "0x%X\n", port_value);
     
        if (*f_pos >= MAX_READ_SIZE)
                return 0; /*EOF*/

        if (*f_pos + count > MAX_READ_SIZE)
                count = MAX_READ_SIZE - *f_pos;

        if (copy_to_user(buf, port_buf, MAX_READ_SIZE) != 0)
                return -EIO;
                
        *f_pos += count;
        return count;
}


/* Struktura przechowująca informacje o operacjach możliwych do
   wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = mcp23s09_write,
        .read = mcp23s09_read
};
/*------------------------------------------------------------------*/


static int mcp23s09_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Probed\n");

        /* Zapisanie wskaźnika do urządzenia SPI */
        mcp23s09_spi_dev = dev;

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

        dev_info(&dev->dev, "Alocated device MAJOR number: %d\n",
                 MAJOR(my_devt));
        
        return 0;
}


static void mcp23s09_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI IO Driver Removed\n");

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
