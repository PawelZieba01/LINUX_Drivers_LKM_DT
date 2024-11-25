#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty sterownik misc device");

/* Dzięki tej definicji, funkcja pr_info doklei na początku każdej wiadomości nazwę naszego modułu */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define BUFF_LENGTH 128
#define MY_DEV_NAME "my_misc_dev"

static char dev_buffer[BUFF_LENGTH];


/* Funkcja wywoływana podczas otwierania pliku urządzenia */
static int misc_open(struct inode *device_file, struct file *instance)
{
        pr_info("Misc device file open.\n");
        return 0;
}


/* Funkcja wywoływana podczas zamykania pliku urządzenia */
static int misc_close(struct inode *device_file, struct file *instance)
{
        pr_info("Misc device file close.\n");
        return 0;
}


/* Funkcja wywoływana podczas zapisywania do pliku urządzenia */
static ssize_t misc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
        pr_info("Write to device file.\n");
       
        if (copy_from_user(dev_buffer, buf, count) != 0)
                return -EFAULT;
        dev_buffer[count] = '\0';
        
        return count;
}


/* Funkcja wywoływana podczas czytania z pliku urządzenia */
static ssize_t misc_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
        pr_info("Read from device file.\n");

        if (copy_to_user(buf, dev_buffer, strlen(dev_buffer)) != 0)
                return -EIO;

        return strlen(dev_buffer);
}


/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner=THIS_MODULE,
        .open=misc_open,
        .release=misc_close,
        .write=misc_write,
        .read=misc_read
};


struct miscdevice misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MY_DEV_NAME,
    .fops = &fops,
};


/* Funkcja wykonywana podczas ładowania modułu do jądra linux */
static int __init on_init(void)
{
        int res;

        pr_info("Module init.\n");

        res = misc_register(&misc_device);

        if (res) {
                pr_err("Misc device registration failed!");
                return res;
        }
        
        return 0;
}


/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit on_exit(void)
{
        pr_info("Module exit.\n"); 
        
        misc_deregister(&misc_device);
}


module_init(on_init);
module_exit(on_exit); 

