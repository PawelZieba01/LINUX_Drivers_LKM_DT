#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>


struct mcp4921_data {
        struct spi_device *dev;
};


static int mcp4921_probe(struct spi_device *dev)
{
        struct mcp4921_data *data;

        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

        data = devm_kzalloc(&dev->dev, sizeof(struct mcp4921_data), GFP_KERNEL);
        if (IS_ERR(data)) {
               dev_err(&dev->dev, "Can't allocate device data\n");
               goto err_alloc;
        }
        
        data->dev = dev;
        dev_set_drvdata(&dev->dev, data);

        return 0;

err_alloc:
        return -1;
}


static void mcp4921_remove(struct spi_device *dev)
{
        dev_info(&dev->dev, "SPI DAC Driver Removed\n");
}


static const struct of_device_id mcp4921_of_id[] = {
        { .compatible = "microchip,mcp4921" },
        {},
};
MODULE_DEVICE_TABLE(of, mcp4921_of_id);


static struct spi_driver mcp4921_driver = {
        .probe = mcp4921_probe,
        .remove = mcp4921_remove,
        .driver = {
                .name = "mcp4921",
                .of_match_table = mcp4921_of_id,
                .owner = THIS_MODULE,
        },
};
module_spi_driver(mcp4921_driver);

MODULE_LICENSE("GPL v2");
