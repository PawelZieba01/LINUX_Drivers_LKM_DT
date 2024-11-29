#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"

struct gpio_dev_data {
        struct gpio_desc *pin;
};


static ssize_t value_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
        int val;
        struct gpio_dev_data *dev_data = dev_get_drvdata(dev);
        
        val = gpiod_get_value(dev_data->pin);

        return sprintf(buf, "%lu\n", (unsigned long)val);
}


static ssize_t value_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
        int res, val;
        struct gpio_dev_data *dev_data;

        dev_data = dev_get_drvdata(dev);

        res = kstrtol(buf, 0, (unsigned long*)&val);
        if (res)
                return res;

        gpiod_set_value(dev_data->pin, val);

        return count;
}
DEVICE_ATTR_RW(value);


static int gpio_probe(struct platform_device *dev)
{    
        struct gpio_dev_data *dev_data;

        dev_info(&dev->dev, "probed\n");

        device_create_file(&dev->dev, &dev_attr_value);

        dev_data = kzalloc(sizeof(struct gpio_dev_data), GFP_KERNEL);
        dev_data->pin = devm_gpiod_get(&dev->dev, "led", GPIOD_OUT_LOW);
        dev_set_drvdata(&dev->dev, dev_data);
        
        return 0;
}


static int gpio_remove(struct platform_device *dev)
{
        struct gpio_dev_data *dev_data;

        dev_info(&dev->dev, "removed\n");

        dev_data = dev_get_drvdata(&dev->dev);

        kfree(dev_data);

        device_remove_file(&dev->dev, &dev_attr_value);

        return 0;
}


static const struct of_device_id gpio_of_id[] = {
        { .compatible = "mtm,gpio" },
        { },
};
MODULE_DEVICE_TABLE(of, gpio_of_id);


static struct platform_driver gpio_driver = {
        .probe = gpio_probe,
        .remove = gpio_remove,
        .driver = {
                .name = "mtm_gpio",
                .of_match_table = gpio_of_id,
                .owner = THIS_MODULE,
        },
};
module_platform_driver(gpio_driver);


MODULE_LICENSE("GPL v2");
