#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>


/*----- Parametr modułu przeznaczony do odczytu i zapisu -----*/
struct mtm_dev_data {
        long int param;
};

static ssize_t mtm_param_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
        struct mtm_dev_data *dev_data = dev_get_drvdata(dev);
        return sprintf(buf, "%lu\n", dev_data->param);
}


static ssize_t mtm_param_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
        int res;
        struct mtm_dev_data *dev_data;

        dev_data = dev_get_drvdata(dev);

        res = kstrtol(buf, 0, &dev_data->param);
        if (res)
                return res;
        return count;
}
DEVICE_ATTR_RW(mtm_param);
/*------------------------------------------------------------*/



static int mtm_probe(struct platform_device *dev)
{
        struct mtm_dev_data *dev_data;

        dev_info(&dev->dev, "probed\n");

        /* Utworzenie pliku reprezentującego atrybut */
        device_create_file(&dev->dev, &dev_attr_mtm_param);

        dev_data = kzalloc(sizeof(struct mtm_dev_data), GFP_KERNEL);
        dev_data->param++;

        dev_set_drvdata(&dev->dev, dev_data);
        
        dev_info(&dev->dev, "param: %lu\n", dev_data->param);
        return 0;
}


static int mtm_remove(struct platform_device *dev)
{
        struct mtm_dev_data *dev_data = (struct mtm_dev_data *)dev->dev.driver_data;

        dev_info(&dev->dev, "removed\n");

        kfree(dev_data);

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
