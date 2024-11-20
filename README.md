# Prosty moduł jądra linux - LKM

## Cel zadania
Stworzyć prosty moduł, który przedstawia działanie funkcji init i exit.

## Opis kodu

```C
/*
* Metadane
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paweł Zięba  AGH UST  2024");
MODULE_DESCRIPTION("Prosty moduł jądra linux");
```

Metadane zawierają podstawowe informacje o module, takie jak autor modułu, oraz licencja modułu.
Istotne jest aby moduł zawierał informacje o licencji, w przeiwnym wypadku proces budowania modułu nie powiedzie się i otrymamy następującą informację:

```
ERROR: modpost: missing MODULE_LICENSE() in /home/pi/LINUX_Drivers_LKM_DT/hello_kernel.o
```

>[!NOTE]
>Licencja GPL (General Public License) pozwala na korzystanie, modyfikowanie i rozpowrzechnianie (na tej samej licencji) zasobów oprogramowania.

```C
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
```

Główną częścią modułu są dwie funkcje: `ModuleInit` oraz `ModuleExit`. Pierwsza jest wywoływana podczas procesu ładowania modułu (np. poleceniem `insmod`), natomast druga wykonuje się w momencie usuwania modułu z jądra (np. polecenie `remmod`).

Funkcja `printk` odpowiada za wpisanie wiadomości do bufora diagnostycznego warstwy jądra.

Atrybuty `__init` oraz `__exit` służą do poinstruowania linkera, aby umieścił kod funkcji w specjalnej sekcji pliku objektowego. Mają one zastosowanie w przypadku modułów wbudowanych, które są budowane razem z jądrem systemu. Oznacza to, że w trakcie działania systemu taki moduł jest ładowany tylko jeden raz, więc można zwolnić pamięć zajmowaną przez funkcję z atrybutem `__init`. W przypadku atrybutu `__exit`, funkcje nie są umieszczane w obrazie systemu.

```C
module_init(ModuleInit);
module_exit(ModuleExit);
```

Powyższe funkcje służą do wskazania tak zwanego 'driver initialization entry point' oraz 'driver exit entry point'.   
Ustalają one, które funkcję będą pełnić rolę 'init' i 'exit' dla modułu.


## Weryfikacja działania modułu

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ make
make -C /lib/modules/6.1.21-v8+/build M=/home/pi/LINUX_Drivers_LKM_DT modules
make[1]: Entering directory '/usr/src/linux-headers-6.1.21-v8+'
  CC [M]  /home/pi/LINUX_Drivers_LKM_DT/hello_kernel.o
  MODPOST /home/pi/LINUX_Drivers_LKM_DT/Module.symvers
  CC [M]  /home/pi/LINUX_Drivers_LKM_DT/hello_kernel.mod.o
  LD [M]  /home/pi/LINUX_Drivers_LKM_DT/hello_kernel.ko
make[1]: Leaving directory '/usr/src/linux-headers-6.1.21-v8+'
``` 

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo insmod hello_kernel.ko 
pi@pi:~/LINUX_Drivers_LKM_DT $ dmesg
...
[ 5612.575352] hello_kernel: loading out-of-tree module taints kernel.
[ 5612.576601] Hello kernel !!!
``` 

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo rmmod hello_kernel 
pi@pi:~/LINUX_Drivers_LKM_DT $ dmesg
...
[ 5612.575352] hello_kernel: loading out-of-tree module taints kernel.
[ 5612.576601] Hello kernel !!!
[ 5793.825293] Goodbye kernel !!!
``` 