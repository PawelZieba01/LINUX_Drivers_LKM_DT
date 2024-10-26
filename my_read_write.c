#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

/*
* Metadane
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty moduł z obsługą wywołań systemowych open(), close(), read() i write()");

/* Dzięki tej definicji, funkcja pr_info doklei na początku każdej wiadomości nazwę naszego modułu */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define BUFF_LENGTH 16
#define MY_DEV_NAME "my_read_write_dev"
#define MY_CLASS_NAME "my_device_class"

static char dev_buffer[BUFF_LENGTH];

static struct class *my_class;
static struct cdev my_cdev;
static dev_t my_devt;

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
    pr_info("Write to device file.\n");

    if(*f_pos >= BUFF_LENGTH)
    {
        return -EINVAL;
    }

    if(*f_pos + count > BUFF_LENGTH)
    {
        count = BUFF_LENGTH - *f_pos;
    }

    if( copy_from_user(dev_buffer, buf, count) != 0 )
    {
        return -EFAULT;
    } 

    return count;
}

/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t DeviceRead(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    pr_info("Read from device file.\n");

    if(*f_pos >= BUFF_LENGTH)
    {
        return 0; /*EOF*/
    }

    if(*f_pos + count > BUFF_LENGTH)
    {
        count = BUFF_LENGTH - *f_pos;
    }

    if( copy_to_user(buf, &dev_buffer[*f_pos], count) != 0)
    {
        return -EIO;
    }
   
    *f_pos += count;
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
    pr_info("Module init.\n");

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

