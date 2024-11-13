# Device Tree sterownik MCP4921

Plik atrybutu urządzenia jest dostępny tutaj:   
`/sys/bus/spi/drivers/my_dac/spi0.0/dac_voltage_mV`

Zmiana uprawnień do pliku, aby można było zapisać wartość za pomocą `echo`:   
`sudo chmod 666 /sys/bus/spi/drivers/my_dac/spi0.0/dac_voltage_mV`

Ustawienie napięcia wyjściowego przetwornika:   
`echo 1200 > /sys/bus/spi/drivers/my_dac/spi0.0/dac_voltage_mV`