#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"
#include "linux/timer.h"


static struct gpio_desc *led;
static struct gpio_desc *btn;
static struct timer_list timer;


static void timer_callback(struct timer_list * timer)
{
        int btn_state = gpiod_get_value(btn);
        gpiod_set_value(led, btn_state);
        mod_timer(timer, jiffies + msecs_to_jiffies(100));
}


static int gpio_probe(struct platform_device *dev)
{    
        timer_setup(&timer, &timer_callback, 0);
        mod_timer(&timer, jiffies + msecs_to_jiffies(100));

        dev_info(&dev->dev, "probed\n");
        
        led = devm_gpiod_get(&dev->dev, "led", GPIOD_OUT_LOW);
        btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);
             
        return 0;
}


static int gpio_remove(struct platform_device *dev)
{
        dev_info(&dev->dev, "removed\n");

        gpiod_set_value(led, 0);
        del_timer_sync(&timer);

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
