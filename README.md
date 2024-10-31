# Sterownik znakowy przetwornika DAC MCP4921

## Cel zadania
Stworzyć sterownik znakowy dla przetwornika DAC MCP4921 z obsługa interface'u SPI


## Opis kodu

**W nowszych wydaniach jądra nie ma funkcji `spi_busnum_to_master()`, przez co aby stworzyć sterownik `SPI` należy użyć mechanizmu DeviceTree (będzie on omówiony w kolejnych przykładach). Z tego powodu to ćwiczenie wykonano dla wersji jądra 5.10.27**

Przed załadowaniem modułu do jądra za pomocą polecenia `insmod`, należy jednorazowo skompilować i załadować nakładkę device tree `spidev_disabler.dts`.

```C
/dts-v1/;
/plugin/;
/* Device Tree overlay for disabling spidev */
/* Source: https://forums.raspberrypi.com/viewtopic.php?t=151423 */

/ {
    compatible = "brcm,bcm2708";

    fragment@0 {
        target = <&spidev0>;

        __overlay__ {
            status = "disabled";
        };
    };
};
```

Dzięki temu zasoby kontrolera SPI0 zostaną zwolnione i będzie możliwe ich użycie za pośrednictwem sterownika znakowego.

Kompilacja nakładki: `dtc spidev_disabler.dts -O dtb >spidev_disabler.dtbo`   
Załadowanie nakładki: `sudo dtoverlay -d . spidev_disabler`

W buforze diagnostycznym pojawi się informacja podobna do tej poniżej:   
`OF: overlay: WARNING: memory leak will occur if overlay removed, property: /soc/spi@7e204000/spidev@0/status`


***Opis powstanie w docelowym pliku .odt***