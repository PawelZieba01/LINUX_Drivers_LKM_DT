# Prosty moduł jądra linux - LKM

## Cel zadania
Stworzyć prosty sterownik znakowy z funkcjami open i close (na razie bez zapisu i odczytu).
Pokazać sposób rejestrowania urządzenia w systemie.
Numery MAJOR i MINOR.



## Opis kodu

### Urządzenia znakowe
W linuxie rozróżniamy dwie grupy urządzeń specjanych:
- urządzenie znakowe (*ang. character device*)
- urządzenie blokowe (*ang. block device*)

Urządzenia znakowe charakteryzują się tym, że zazwyczaj są wolniejsze i przetwarzają dane znak po znaku (bajt po bajcie). Takimi urządzeniami mogą być klawiatury, myszki, terminale znakowe itd.  
Natomiast urządzenia blokowe są szybsze i przetwarzają dane blokami a nie po jednym bajcie. Przykładami takich urządzeń mogą być np. dyski twarde, stacje dysków CD-ROM itd. 






### Funkcje open i close

Na podstawie poprzedniego modułu przygotowano jego rozszerzenie. W tym przykładzie dodano dwie funkcje służące jako callback dla funkcjonalności wywołań systemowych `open` i `close`, które można wykonać na pliku.

```C
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
```

Dzięki temu, próba odczytania pliku za pomocą np. programu `cat` powoduje jednocześnie jego otwarcie (uruchamiane jest wywołanie systemowe `open()`, *ang. system call*)






### Struktura file_operations
Wskaźniki do powyższych funkcji muszą zostać umieszczone w struktirze typu `file_operations`, która opisuje powiązania wywołań systemowych (*ang. system calls) z funkcjami pełniącymi rolę callbacków w naszym module.
Pole `.owner` wskazuje na bierzący moduł jądra.    
Dzięki temu np.: system bokuje możliwość usunięcia modułu z jądra, jeśli jego plik został otwarty (system call `open()`) i nie został jeszcze zamknięty (system call `close()`).

```C
/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
    .owner=THIS_MODULE,
    .open=ModuleOpen,
    .release=ModuleClose
};
```




### Rejestrowanie urządzenia znakowego - MAJOR i MINOR

W systemie linux, urządzenia są reprezentowane jako unikatowe numery nazywane `MAJOR` i `MINOR`.
Numer `MAJOR` to numer identyfikujący urządzenie w systemie, natomiast `MINOR` służy sterownikowi do rozpoznawania urządzenia.   

Przykład:   
Sterownik pamięci EEPROM, która jest podzielona na 8 bloków:
Podczas inicjalizacji moduł rejestruje i tworzy 8 plików urządzeń (w przestrzeni użytkownika) o numerze `MAJOR` równym `18` i o kolejnych numerach `MINOR` - `0, 1, 2, ..., 7`. Dzięki temu każdy blok pamięci jest dostępny jako osobny plik w folderze `/dev`, a sterownik rozróżnia poszcególne bloki za pomocą numerów `MINOR`.

Zajęte numery `MAJOR` można sprawdzić następującym poleceniem:

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo cat /proc/devices
Character devices:
  1 mem
  4 /dev/vc/0
  4 tty
  4 ttyS
  5 /dev/tty
  5 /dev/console
  5 /dev/ptmx
  5 ttyprintk
  7 vcs
 10 misc
 13 input
...
```

Poniższy fragment kodu przedstawia funkcję inicalizującą modułu, w której rejestrowane jest urządzenie.

```C
#define MY_MAJOR 0
#define MY_DEV_NAME "my_open_close_dev"

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
```



Funkcja `register_chrdrv` rejestruje urządzenie znakowe w jądrze systemu. Jej deklaracja wygląda następująco:   
`int register_chrdev(unsigned int major, const char *name, struct file_operations *fops);`   
  
- `unsigned int major` - numer MAJOR, nie może się powtarzać. Jeśli wybrany zostanie numer `0`, to jądro samo przydzieli dynamicznie niezajętą liczbę.
- `const char *name` - nazwa urządzenia, która będzie widoczna w `/proc/devices`.
- `struct file_operations` - struktura typu file_operations zawierająca powiązania funkcji wywołań systemowych.
Funkcja zwraca numer `MAJOR`, który zarezerowowała (w przypadku gdy podano `0` w argumencie `major`), wartość ujemną, jeśli operacja się nie powiedzie, oraz wartość `0` jeśli uda się zarezerwować podany w argumencie numer `MINOR`.






## Weryfikacja działania modułu

Aby zweryfikować czy funkcje `ModuleOpen()` i `ModuleClose` poprawnie się wykonują, należy najpierw stworzyć plik specjalny, który będzie dostępny w przestrzeni użytkownika. Służy do tego polecenie `mknod`:

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo mknod /dev/my_open_close_dev c 30 0
```








```console
pi@pi:~/LINUX_Drivers_LKM_DT $ cat /proc/devices | grep my
 30 my_open_close_dev
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo mknod /dev/my_open_close_dev c 30 0
pi@pi:~/LINUX_Drivers_LKM_DT $ ls /dev/my_open_close_dev -al
crw-r--r-- 1 root root 30, 0 Oct 21 18:06 /dev/my_open_close_dev
pi@pi:~/LINUX_Drivers_LKM_DT $ cat /dev/my_open_close_dev
cat: /dev/my_open_close_dev: Invalid argument
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo rmmod my_open_close_dev 
pi@pi:~/LINUX_Drivers_LKM_DT $ dmesg | tail
...
[ 9216.109707] my_open_close: Module init.
[ 9216.109732] my_open_close: Zarejestrowano urządzenie o numerze MAJOR: 30
[ 9544.969588] my_open_close: Module exit.
[ 9551.959353] my_open_close: Module init.
[ 9551.959379] my_open_close: Zarejestrowano urządzenie o numerze MAJOR: 30
[ 9689.865726] my_open_close: Module device file open.
[ 9689.868118] my_open_close: Module device file close.
[ 9766.854713] my_open_close: Module exit.
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo rm -f /dev/my_open_close_dev 
```


```console
pi@pi:~/LINUX_Drivers_LKM_DT $ dmesg | tail
...
[ 9820.267041] my_open_close: Module init.
[ 9820.267086] my_open_close: Zarejestrowano urządzenie o numerze MAJOR: 236
```


