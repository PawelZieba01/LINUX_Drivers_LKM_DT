#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty moduł z obsługą wywołań systemowych open(), close(), read() i write()");


#define BUFF_LENGTH 128
#define MY_DEV_NAME "my_read_write_dev"
#define MY_CLASS_NAME "my_device_class"

static char dev_buffer[BUFF_LENGTH];

static struct class *my_class;
static struct device *my_device;
static struct cdev my_cdev;
static dev_t my_devt;


/* Funkcja wywoływana podczas otwierania pliku urządzenia */
static int device_open(struct inode *device_file, struct file *instance)
{
        pr_info("Module device file open.\n");
        return 0;
}


/* Funkcja wywoływana podczas zamykania pliku urządzenia */
static int device_close(struct inode *device_file, struct file *instance)
{
        pr_info("Module device file close.\n");
        return 0;
}


/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t device_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
        pr_info("Write to device file.\n");
       
        if (copy_from_user(dev_buffer, buf, count) != 0)
                return -EFAULT;
        dev_buffer[count] = '\0';
        
        return count;
}


/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t device_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
        pr_info("Read from device file.\n");

        if (copy_to_user(buf, dev_buffer, strlen(dev_buffer)) != 0)
                return -EIO;

        return count;
}


/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = device_open,
        .release = device_close,
        .write = device_write,
        .read = device_read
};


/* Funkcja wykonywana podczas ładowania modułu do jądra linux */
static int __init on_init(void)
{
        int ret;
        pr_info("Module init.\n");

        /* Zaalokowanie numerów MAJOR i MINOR dla urządzenia */
        ret = alloc_chrdev_region(&my_devt, 0, 1, MY_DEV_NAME);
        if (ret) {
                pr_err("Can't allocate chrdev.\n");
                goto err_alloc;
        }
        
        /* Stworzenie klasy urządzeń, widocznej w /sys/class */
        my_class = class_create(THIS_MODULE, MY_CLASS_NAME);
        if (IS_ERR(my_class)) {
                pr_err("Can't create class.\n");
                goto err_class;
        }

        /* Inicjalizacja urządzenia znakowego - podpięcie funkcji do operacji na plikach (file operations) */
        cdev_init(&my_cdev, &fops);

        /* Dodanie urządzenia do systemu */
        ret = cdev_add(&my_cdev, my_devt, 1);
        if (ret) {
                pr_err("Can't add device to system.\n");
                goto err_cdev;
        }

        /* Stworzenie pliku w przestrzeni użytkownika (w /dev), reprezentującego urządzenie */
        my_device = device_create(my_class, NULL, my_devt, NULL, MY_DEV_NAME);
        if (IS_ERR(my_device)) {
                pr_err("Can't create device.\n");
                goto err_dev;
        }

        pr_info("Alocated device MAJOR number: %d\n", MAJOR(my_devt));
        return 0;


err_dev:
        cdev_del(&my_cdev);
err_cdev:
        class_unregister(my_class);
        class_destroy(my_class);
err_class:
        unregister_chrdev_region(my_devt, 1);
err_alloc:
        return -1;
}


/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit on_exit(void)
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


module_init(on_init);
module_exit(on_exit); 

