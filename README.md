# Device Tree sterownik MCP4921
Sterownik udostępniający do przestrzeni użytkownika plik atrybutu, dzięki któremu można sterować wyjściem ekspadnera IO - MCP23S09

Plik atrybutu urządzenia jest dostępny tutaj:   
`/sys/bus/spi/drivers/my_io/spi0.0/io_port_state`

Zmiana uprawnień do pliku, aby można było zapisać wartość za pomocą `echo`:   
`sudo chmod 666 /sys/bus/spi/drivers/my_io/spi0.0/io_port_state`

Ustawienie stanu portu:   
`echo 0xff > /sys/bus/spi/drivers/my_io/spi0.0/io_port_state`   

Odczyt stanu portu:   
`cat /sys/bus/spi/drivers/my_io/spi0.0/io_port_state`