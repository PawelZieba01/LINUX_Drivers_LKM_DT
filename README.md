# Device Tree sterownik MCP23S09 - urządzenie znakowe
Sterownik udostępniający do przestrzeni użytkownika plik urządzenia znakowego, dzięki któremu można sterować portem ekspandera IO.

Jeśli `insmod` nie załaduje modułu, to należy użyć `modprobe`:
- przekopiować plik `.ko` do filderu`/lib/modules/<wersja kernela>`
- uruchomić `depmod`
- załadować moduł za pomocą `modprobe`
  
  `insmod` nie ładuje modułu bo moduł używa `regmap_spi` (to też jest moduł). `Modprobe` automatycznie ładuje zależności wymagane przez nasz moduł.