# Device Tree Basic

Ten kod przedstawia problem użycia globalnej zmiennej przez wiele urządzeń obsługiwanych przez jeden sterownik.

Tworzymy dodatkowy węzeł, w Device Tree, a każde urządzenie w funkcji probe inkrementuje licznik instancji sterownika.

W tej modyfikacji inkrementowany parametr znajduje się w strukturze danych sterownika `driver_data`, która została zaalokowana w pamięci dla każdego węzła (urządzenia) Device Tree.



Dodać:

- #include <linux/slab.h>
- węzeł w DT
- funkcje _show i _store
- alokowanie pamięci
- dev_set_drvdata(&dev->dev, dev_data); get_set_drvdata(&dev->dev);