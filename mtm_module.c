#include <linux/of.h>
#include <linux/platform_device.h>

/*----- Parametr modułu przeznaczony do odczytu i zapisu -----*/
static unsigned long int mtm_param_instances = 0;

static ssize_t mtm_param_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%lu\n", mtm_param_instances);
}
DEVICE_ATTR_RO(mtm_param);
/*------------------------------------------------------------*/


static int mtm_probe(struct platform_device *dev)
{
        dev_info(&dev->dev, "probed\n");

        /* Utworzenie pliku reprezentującego atrybut */
        device_create_file(&dev->dev, &dev_attr_mtm_param);

        mtm_param_instances++;
        dev_info(&dev->dev, "mtm_param: %lu\n", mtm_param_instances);
        return 0;
}


static int mtm_remove(struct platform_device *dev)
{
        dev_info(&dev->dev, "removed\n");

        /* Usunięcie pliku reprezentującego atrybut urządzenia*/
        device_remove_file(&dev->dev, &dev_attr_mtm_param);
        return 0;
}


static const struct of_device_id mtm_of_id[] = {
        { .compatible = "mtm" },
        { },
};
MODULE_DEVICE_TABLE(of, mtm_of_id);


static struct platform_driver mtm_driver = {
        .probe = mtm_probe,
        .remove = mtm_remove,
        .driver = {
                .name = "mtm_device",
                .of_match_table = mtm_of_id,
                .owner = THIS_MODULE,
        },
};
module_platform_driver(mtm_driver);


MODULE_LICENSE("GPL v2");
