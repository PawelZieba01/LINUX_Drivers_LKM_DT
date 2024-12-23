#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>


struct mcp4921 {
        struct spi_device *dev;
};


static int mcp4921_probe(struct spi_device *dev)
{
        int err = 0;
        struct mcp4921 *mcp4921;

        mcp4921 = devm_kzalloc(&dev->dev, sizeof(struct mcp4921), GFP_KERNEL);
        if (IS_ERR(mcp4921)) {
               dev_err(&dev->dev, "Device data allocation failed.\n");
               err = PTR_ERR(mcp4921);
               goto out_ret_err;
        }
        
        mcp4921->dev = dev;
        dev_set_drvdata(&dev->dev, mcp4921);

        dev_info(&dev->dev, "SPI DAC Driver Probed\n");

out_ret_err:
        return err;
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
