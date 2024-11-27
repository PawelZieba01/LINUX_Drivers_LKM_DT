# Device Tree Basic

Ten kod przedstawia problem użycia globalnej zmiennej przez wiele urządzeń obsługiwanych przez jeden sterownik.

Tworzymy dodatkowy węzeł, w Device Tree, a każde urządzenie w funkcji probe inkrementuje licznik instancji sterownika.