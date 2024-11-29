#include <linux/of.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"

struct gpio_desc *led, *btn;

static int gpio_probe(struct platform_device *dev)
{
        int btn_state;

        dev_info(&dev->dev, "probed\n");

        led = devm_gpiod_get(&dev->dev, "led", GPIOD_OUT_LOW);
        btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);

        btn_state = gpiod_get_value(btn);
        gpiod_set_value(led, btn_state);

        dev_info(&dev->dev, "BTN state is %d\n", btn_state);

        return 0;
}


static int gpio_remove(struct platform_device *dev)
{
        dev_info(&dev->dev, "removed\n");
        dev_info(&dev->dev, "led off\n");
        gpiod_set_value(led, 0);
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
