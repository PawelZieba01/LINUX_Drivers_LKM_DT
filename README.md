# Device Tree sterownik MCP23S09 - urządzenie znakowe
Sterownik udostępniający do przestrzeni użytkownika plik urządzenia znakowego, dzięki któremu można sterować portem ekspandera IO.

Jeśli `insmod` nie załaduje modułu, to należy użyć `modprobe`:
- przekopiować plik `.ko` do filderu`/lib/modules/<wersja kernela>`
- uruchomić `depmod`
- załadować moduł za pomocą `modprobe`
  
  `insmod` nie ładuje modułu bo moduł używa `regmap_spi` (to też jest moduł). `Modprobe` automatycznie ładuje zależności wymagane przez nasz moduł.

  Zapis:   
  - adres 16-bitowy, ponieważ pierwszy bajt to `WRITE_OPCODE`, a drugi bajt to wartość adres rejestru. Jako 3 wysłany jest bajt z wartością rejestru.

Odczyt:
- adres 16-bitowy. Pierwszy bajt to `READ_OPCODE`, drugi bajt - adres rejestru. 