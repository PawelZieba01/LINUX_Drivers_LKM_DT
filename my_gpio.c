#include <linux/of.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"

struct my_gpio {
        struct gpio_desc *led;
        struct platform_device *pdev;
};


static ssize_t my_gpio_value_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
        int value;
        struct my_gpio *my_gpio = dev_get_drvdata(dev);
        
        value = gpiod_get_value(my_gpio->led);
        return sprintf(buf, "%lu\n", (unsigned long)value);
}


static ssize_t my_gpio_value_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
        int err = 0;
        int value;
        struct my_gpio *my_gpio = dev_get_drvdata(dev);

        err = kstrtol(buf, 0, (unsigned long*)&value);
        if (err) {
                dev_err(dev, "Bad input number.\n");
                return err;
        }
                
        gpiod_set_value(my_gpio->led, value);
        return count;
}
DEVICE_ATTR_RW(my_gpio_value);


static int my_gpio_probe(struct platform_device *dev)
{    
        int err = 0;
        struct my_gpio *my_gpio;

        my_gpio = devm_kzalloc(&dev->dev, sizeof(struct my_gpio), GFP_KERNEL);
        if (IS_ERR(my_gpio)) {
               dev_err(&dev->dev, "Memory allocation failed.\n");
               err = PTR_ERR(my_gpio);
               goto out_ret_err;
        }  

        my_gpio->led = devm_gpiod_get(&dev->dev, "led", GPIOD_OUT_LOW);
        if (IS_ERR(my_gpio->led)) {
               dev_err(&dev->dev, "GPIO descriptor acquisition failed.\n");
               err = PTR_ERR(my_gpio->led);
               goto out_ret_err;
        }

        err = device_create_file(&dev->dev, &dev_attr_my_gpio_value);
        if (err) {
               dev_err(&dev->dev, "Device file creation failed.\n");
               goto out_ret_err;
        }

        my_gpio->pdev = dev;
        dev_set_drvdata(&dev->dev, my_gpio);

        dev_info(&dev->dev, "probed\n");
        
out_ret_err:
        return err;
}


static int my_gpio_remove(struct platform_device *dev)
{
        device_remove_file(&dev->dev, &dev_attr_my_gpio_value);
        dev_info(&dev->dev, "removed\n");
        return 0;
}

static const struct of_device_id my_gpio_of_id[] = {
        { .compatible = "mtm,my_gpio" },
        { },
};
MODULE_DEVICE_TABLE(of, my_gpio_of_id);


static struct platform_driver my_gpio_driver = {
        .probe = my_gpio_probe,
        .remove = my_gpio_remove,
        .driver = {
                .name = "my_gpio",
                .of_match_table = my_gpio_of_id,
                .owner = THIS_MODULE,
        },
};
module_platform_driver(my_gpio_driver);


MODULE_LICENSE("GPL v2");
