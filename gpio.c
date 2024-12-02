#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"
#include "linux/timer.h"


static struct gpio_descs *leds;
static struct gpio_desc *btn;
static struct timer_list timer;

static int leds_values;

static ssize_t value_store(struct device *dev, 
                                       struct device_attribute *attr, 
                                       const char *buf, size_t count)
{
        int res;
        
        res = kstrtol(buf, 0, (long int*)(&leds_values));

        if (res)
                return res;

        return count;
}
DEVICE_ATTR_WO(value);


static void set_leds(int val)
{
        int led_ctr = 0;
        for (led_ctr=0 ; led_ctr<4 ; led_ctr++)
                gpiod_set_value(leds->desc[led_ctr], ((val>>led_ctr)&0x01));
}


static void timer_callback(struct timer_list * timer)
{
        int btn_state = gpiod_get_value(btn);

        if(btn_state == 1)
                set_leds(leds_values);
        else
                set_leds(0);

        mod_timer(timer, jiffies + msecs_to_jiffies(100));
}


static int gpio_probe(struct platform_device *dev)
{    
        timer_setup(&timer, &timer_callback, 0);
        mod_timer(&timer, jiffies + msecs_to_jiffies(100));

        dev_info(&dev->dev, "probed\n");

        device_create_file(&dev->dev, &dev_attr_value);
        
        leds = devm_gpiod_get_array(&dev->dev, "leds", GPIOD_OUT_LOW);
        btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);
             
        return 0;
}


static int gpio_remove(struct platform_device *dev)
{
        dev_info(&dev->dev, "removed\n");

        set_leds(0);
        del_timer_sync(&timer);
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
