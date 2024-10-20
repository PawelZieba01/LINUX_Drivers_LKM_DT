#include <linux/module.h>
#include <linux/init.h>

/*
* Metadane
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty moduł jądra linux");

/* Funkcja wywoływana podczas ładowania modułu do jądra linux */
static int __init ModuleInit(void)
{
    printk("Hello kernel !!!\n");
    return 0;
}

/* Funkcja wykonywana podczas usuwania modułu z jądra linux */
static void __exit ModuleExit(void)
{
    printk("Goodbye kernel !!!\n"); 
}


module_init(ModuleInit);
module_exit(ModuleExit);

