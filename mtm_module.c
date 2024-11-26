#include <linux/of.h>
#include <linux/platform_device.h>

/*----- Parametr modułu przeznaczony do odczytu -----*/
static unsigned long int mtm_param = 64;

static ssize_t mtm_param_show(struct device *dev, struct device_attribute *attr,
                              char *buf)
{
        return sprintf(buf, "%lu\n", mtm_param);
}
DEVICE_ATTR_RO(mtm_param);
/*---------------------------------------------------*/


static int mtm_probe(struct platform_device *dev)
{
        struct device_node * sMyNode = dev->dev.of_node;
        uint32_t param;

        dev_info(&dev->dev, "probed\n");

        /* Odczyt własności urządzenia z DT */
        of_property_read_u32(sMyNode, "int_param", &param);
        pr_info("int_param: %d\n", param);

        /* Utworzenie pliku reprezentującego atrybut w przestrzeni użytkownika */
        device_create_file(&dev->dev, &dev_attr_mtm_param);
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
