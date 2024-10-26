# Prosty sterownik znakowy

## Cel zadania
Dodać do poprzedniego przykładu możliwość odczytu i zapisu pliku urządzenia znakowego.
Dodać rejestrację urządzenia i klasy (tym razem przykład z dynamicznym przydzieleniem numerów MAJOR i MINOR)
Dodać tworzenie pliku urządzenia w przestrzeni użytkownika.




## Opis kodu

### Struktura file_operations

Aby dodać możliwość wpisywania znaków do naszego sterownika, uzupełniono strukturę `fops` o dodatkowe przypisania funkcji callback.

```C
/* Struktura przechowująca informacje o operacjach możliwych do wykonania na pliku urządzenia */
static struct file_operations fops = {
    .owner=THIS_MODULE,
    .open=DeviceOpen,
    .release=DeviceClose,
    .write=DeviceWrite,     //  <-------
    .read=DeviceRead        //  <-------
};
```

### Funkcja wpisująca DeviceWrite()

Dwie nowe funkcje mają za zadanie pośredniczyć w wymianie danych z przestrzenią użytkownika (np. podczas wywołania programu `echo <text> >...` lub `cat`).

```C
#define BUFF_LENGTH 16
static char dev_buffer[BUFF_LENGTH];

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
```

Funkcja `DeviceWrite()` odpowiada za przechwycenie danych wpisywanych przez program użytkownika i sprawdzenie ich poprawności (długość, adres itd.)

Argumenty funkcji:

- `struct file *filp` - wskaźnik na strukturę reprezentującą deskryptor otwartego pliku. Przechowuje informacje o otwartym pliku urządzenia (typ, ścieżka, fops, itd.)
- `const char *buf` - adres bufora w przestrzeni użytkownika, zawiera wpisywane dane
- `size_t count` - ilość bajtów do wpisania
- `loff_t *f_pos` - pozycja, od której ma zacząć się wpisywanie znaków 

Funkcja powinna zwrócić liczbę poprawnie zapisanch/przetworzonych znaków.   


### Funkcja odczytująca DeviceRead()


Funkcja odczytująca z pliku urządzenia znakowego jest analogiczna.
Następuje w niej sprawdzenie poprawności żądania danych i skopiowanie zapisanego bufora do przestrzeni użytkownika.
Na koniec licznik pozycji jest zwiększany o liczbę 'odesłanych' bajtów, dzięki temu, przy kolejnej próbie odczytu, zwrócone zostaną pozostałe bajty, których nie udało się odczytać poprzednio.

Przykład:   
Program `cat` odczytuje dane z pliku do momentu przeczytania znaku `EOF` (end of file). Zwiększenie licznika pozycji `f_pos` powoduje, że przy kolejnej próbie odczytu pliku, zostanie zwrócony znak `EOF`, ponieważ licznik będzie wskazywał na koniec bufora.   
Gdyby takiego zabezpieczenia nie było, program `cat` odczytywałby plik w nieskończoność.


```C
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
```





</br>

#### Kopiowanie danych z/do przestrzeni użytkownia

Przestrzenie adresowe jądra i użytkownika są oddzielone, dlatego adres pochodzący od aplikacji użytkownika (`buf`) nie wskazuje na poprawne dane z punktu widzenia jądra. Aby skopiować dane pomiędzy różnymi przestrzeniami adresowymi należy użyć specjalnej funkcji.

Funkcja `copy_from_user()` kopiuje dane z przestrzeni użytkownika do przestrzeni adresowej jądra. Jej deklaracja wygląda następująco:

```C
unsigned long copy_from_user (void * to, const void __user * from, unsigned long n);
```

- `void * to` - adres docelowy w przestrzeni jądra
- `const void __user * from` - adres źródłowy (atrybut `__user` oznacza przestrzeń użytkownika)
- `unsigned long n` - liczba bajtów do skopiowania

\
Analogicznie w przypadku przekazywania danych w drugą stronę - do przerstrzeni użytkownika, należy użyć funkcji `copy_to_user()`. Jej deklaracja wygląda jak poniżej:

```C
unsigned long copy_to_user (void __user * to, const void * from, unsigned long n);
```

- `void __user * to` - adres docelowy w przestrzeni użytkownika
- `const void * from` - adres źródłowy w przestrzeni jądra
- `unsigned long n` - liczba bajtów do skopiowania


Obie funkcje zwracają liczbę bajtów, których nie udało się skopiować. W przypadku powodzenia, zwracaną wartością jest liczba `0`.



### Działanie zapisu i odczytu

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo insmod my_read_write.ko 
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo sh -c "echo 'hello kernel' > /dev/my_read_write_dev" 
pi@pi:~/LINUX_Drivers_LKM_DT $ sudo cat /dev/my_read_write_dev 
hello kernel
```

```console
pi@pi:~/LINUX_Drivers_LKM_DT $ dmesg | tail
[14489.826934] my_read_write: Module init.
[14489.827243] my_read_write: Alocated device MAJOR number: 236
[14502.008872] my_read_write: Module device file open.
[14502.008971] my_read_write: Write to device file.
[14502.008999] my_read_write: Module device file close.
[14505.589724] my_read_write: Module device file open.
[14505.589824] my_read_write: Read from device file.
[14505.589916] my_read_write: Read from device file.
[14505.589994] my_read_write: Module device file close.
```

Jak widać, w buforze diagnostycznym widnieją dwa wpisy `Read from device file`, co oznacza, że program `cat` ponowił próbę odczytania pliku, ale za drugim razem funkcja zwróciła `EOF`, co poinformowało program odczytujący o końcu pliku. 


## Inicjalizacja sterownika

W tym przykładzie rozbudowano część kodu odpowiadającą za inicjalizację sterownika. Dodano automatyczne tworzenie i rejestrowanie pliku urządzenia znakowego, skojarzonego ze sterownikiem. Dodano też automatyczne utowrzenie klasy urządzenia.

```C
#define MY_DEV_NAME "my_read_write_dev"
#define MY_CLASS_NAME "my_device_class"
static struct class *my_class;
static struct cdev my_cdev;
static dev_t my_devt;

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

    /* Dodanie urządzenia do jądra */
    cdev_add(&my_cdev, my_devt, 1);

    /* Stworzenie pliku w przestrzeni użytkownika (w /dev), reprezentującego urządzenie */
    device_create(my_class, NULL, my_devt, NULL, MY_DEV_NAME);


    pr_info("Alocated device MAJOR number: %d\n", MAJOR(my_devt));
    return 0;
}
```

Klasy urządzeń grupują poszczególne urządzenia na podstawie ich przeznaczenia, np. urządzenia sterujące GPIO są skojarzone z klasą `GPIO`.

Funkcja `class_create()` tworzy klasę o podanej w argumencie nazwie, przypisaną do tego modułu. Podczas tworzenia urządzenia, przypisywana jest do niego stowrzona klasa.

Funkcja `cdev_init()` inicjalizuje strukturę `cdev`, podpinając jednocześnie callbacki ze struktury `fops`.

Dzięki `cdev_add()` urządzenie znakowe zostaje dodane do jądra i będzie identyfikowane za pomocą numerów `MAJOR` i `MINOR`, przekazanych przez argument `my_devt`. Liczba `1` określa liczbę numerów `MINOR` skojarzonych z urządzeniem.

Funkcja `device_create()` tworzy urządzenie i dodaje do systemu plików sprcjalny plik urządzenia znakowego. Od tego momentu w folderze `/dev` widoczny będzie plik `my_read_write_dev`. Deklaracja funkcji wygląda następująco:

```C
struct device * device_create(struct class * class, struct device * parent, dev_t devt, void * drvdata, const char * fmt, ...);
```

- `struct class * class` - wskaźnik na klasę, która została zarejestrowana wcześniej
- `struct device * parent` - wskaźnik na strukturę urządzenia nadrzędnego, jeśli istnieje
- `dev_t devt` - zmienna przechowująca numer `MAJOR` i `MINOR`
- `void * drvdata` - wskaźnik na dodatkowe dane związane z urządzeniem
- `const char * fmt` - nazwa urządzenia znakowego




## Zamykanie sterownika

W momencie, usunięcia modułu z jądra, wywoływana jest poniższa funkcja, która kolejno usuwa urządzenie i plik specjalny z systemu, usuwa urząddzenie znakowe, niszczy utworzoną wcześniej klasę i zwalnia numery `MAJOR` i `MINOR` 

```C
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
```

