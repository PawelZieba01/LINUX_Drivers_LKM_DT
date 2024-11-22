#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>


static int mtm_probe(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Probed\n");
        return 0;
}

static void mtm_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
}


static const struct of_device_id mtm_of_id[] = {
        { .compatible = "microchip,my_dac" },
        {},
};

MODULE_DEVICE_TABLE(of, mtm_of_id);                     //platform - dopasowuje sterownik po compatible


// static const struct spi_device_id my_dac[] = {
//         {"my_dac", 0},
//         {},
// };

// MODULE_DEVICE_TABLE(spi, my_dac);                    //spi - dopasowuje sterownik po nazwie urządzenia z tablicy typu spi_device_id


static struct spi_driver mtm_driver = {
        //.id_table = my_dac;
        .probe = mtm_probe,
        .remove = mtm_remove,
        .driver = {
                .name = "my_dac",
                .of_match_table = mtm_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mtm_driver);

MODULE_LICENSE("GPL v2");

/* Powyższe komentarze dotyczące mechanizmu dopasowania sterowników i urządzeń dt zostaną usunięte - wykorzystano mechanizm of, dopasowujący poprzez 'compatible' */