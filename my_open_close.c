/* Dzięki tej definicji, funkcja pr_info doklei na początku każdej wiadomości nazwę naszego modułu */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty moduł z obsługą wywołań systemowych open() i close() ");


#define MY_MAJOR 30
#define MY_DEV_NAME "my_open_close_dev"


static int my_dev_major;


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


/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
        .owner=THIS_MODULE,
        .open=device_open,
        .release=device_close
};


/* Funkcja wykonywana podczas ładowania modułu do jądra linux */
static int __init on_init(void)
{
        int ret;

        pr_info("Module init.\n");

        /* 
        *  Rejestracja urządzenia w jądrze linux
        *  Podanie 0 jako numer major, spowoduje dynamiczne pozyskanie tego numeru, wtedy funkcja go zwróci
        */
        ret = register_chrdev(MY_MAJOR, MY_DEV_NAME, &fops);
        if (ret < 0) {
                pr_info("Nie udało zarejestrować urządzenia\n");
                return -1;
        } else if (ret > 0) {            /* Udało się dynamicznie zarejestrować urządzenie*/
                my_dev_major = ret;
        } else {                            /* Jeśli funkcja zwróci 0, to udało się zarejestrować urządzenie pod podanym numerem*/
                my_dev_major = MY_MAJOR;
        }

        pr_info("Zarejestrowano urządzenie o numerze MAJOR: %d\n", my_dev_major);
        return 0;
}


/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit on_exit(void)
{
        pr_info("Module exit.\n"); 
        
        /* Zwolnienie numeru major i usunięcie urządzenia*/
        unregister_chrdev(my_dev_major, MY_DEV_NAME);
}

module_init(on_init);
module_exit(on_exit);
