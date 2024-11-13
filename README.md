# Device Tree sterownik MCP4921
Sterownik udostępniający do przestrzeni użytkownika plik atrybutu, dzięki któremu można sterować napięciem na wyjściu DAC'a

Plik atrybutu urządzenia jest dostępny tutaj:   
`/sys/bus/spi/drivers/my_dac/spi0.0/dac_voltage_mV`

Zmiana uprawnień do pliku, aby można było zapisać wartość za pomocą `echo`:   
`sudo chmod 666 /sys/bus/spi/drivers/my_dac/spi0.0/dac_voltage_mV`   
lub   
`/devices/platform/soc/fe204000.spi/spi_master/spi0/spi0.0/dac_voltage_mV'`

Ustawienie napięcia wyjściowego przetwornika:   
`echo 1200 > /sys/bus/spi/drivers/my_dac/spi0.0/dac_voltage_mV`