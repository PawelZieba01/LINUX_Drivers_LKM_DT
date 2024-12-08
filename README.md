# Device Tree sterownik MCP23S09 - urządzenie znakowe
Sterownik udostępniający do przestrzeni użytkownika plik urządzenia znakowego, dzięki któremu można sterować portem ekspandera IO.

Jeśli `insmod` nie załaduje modułu, to należy użyć `modprobe`:
- przekopiować plik `.ko` do filderu`/lib/modules/<wersja kernela>`
- uruchomić `depmod`
- załadować moduł za pomocą `modprobe`
  
  `insmod` nie ładuje modułu bo moduł używa `regmap_i2c` (to też jest moduł). `Modprobe` automatycznie ładuje zależności wymagane przez nasz moduł.

  Układ PCF8574 wymaga wysłania:
  - przy odczycie - `0xff`, aby ustawić kierunek pinów
  - przy zapisie - `0x00`, ponieważ regmap zawsze wysyła zmienną `reg`, więc wyłączamy cały port, po czym kolejny bajt ustawia odpowiednie wartości wyjść.