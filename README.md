# Misc Device

Znamy już koncept urządzenia znakowego i umiemy napisać do niego sterownik, teraz pora na urządzenie `Miscellaneous  Device`

`Misc Device` jest bardzo podobne do `Character Device` z dwiema istotnymi różnicami:

- `Misc Device` domyślnie mają przypisany numer `Major` równy `10`. Użytkownik może wybrać numer `Minor` z puli dostępnych numerów lub pozwolić aby jądro dynamicznie przydzieliło jeden z nich.
- Plik urządzenia `Misc Device` jest tworzony automatycznie przy zarejestrowaniu utworzeniu w systemie. NAtomiast `Character Device` wymaga od użytkownika użycia następujących funkcji:  `cdev_init`, `cdev_add`, `class_create`, oraz `device_create`.