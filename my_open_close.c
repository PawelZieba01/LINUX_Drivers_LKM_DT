#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>

/*
* Metadane
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty moduł z obsługą wywołań systemowych open() i close() ");

/* Dzięki tej definicji, funkcja pr_info doklei na początku każdej wiadomości nazwę naszego modułu */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

static int my_dev_major;

/* Funkcja wywoływana podczas otwierania pliku urządzenia */
static int ModuleOpen(struct inode *device_file, struct file *instance)
{
    pr_info("Module device file open.\n");
    return 0;
}

/* Funkcja wywoływana podczas zamykania pliku urządzenia */
static int ModuleClose(struct inode *device_file, struct file *instance)
{
    pr_info("Module device file close.\n");
    return 0;
}


/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
    .owner=THIS_MODULE,
    .open=ModuleOpen,
    .release=ModuleClose
};

#define MY_MAJOR 0
#define MY_DEV_NAME "my_dev"

/* Funkcja wywoływana podczas ładowania modułu do jądra linux */
static int __init ModuleInit(void)
{
    int status;

    pr_info("Module init.\n");

    /* Rejestracja urządzenia w jądrze linux
    *  Podanie 0 jako numer major, spowoduje dynamiczne pozyskanie tego numeru, wtedy funkcja go zwróci
    */
    status = register_chrdev(MY_MAJOR, MY_DEV_NAME, &fops);
    if(status < 0)
    {
        pr_info("Nie udało zarejestrować urządzenia\n");
        return -1;
    }
    else if(status > 0)     /* Udało się dynamicznie zarejestrować urządzenie*/
    {
        my_dev_major = status;
    }
    else    /* Jeśli funkcja zwróci 0, to udało się zarejestrować urządzenie pod podanym numerem*/
    {
        my_dev_major = MY_MAJOR;
    }

    pr_info("Zarejestrowano urządzenie o numerze MAJOR: %d\n", my_dev_major);
    return 0;
}

/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit ModuleExit(void)
{
    pr_info("Module exit.\n"); 
    
    /* Zwolnienie numeru major i usunięcie urządzenia*/
    unregister_chrdev(my_dev_major, MY_DEV_NAME);
}


module_init(ModuleInit);
module_exit(ModuleExit);

