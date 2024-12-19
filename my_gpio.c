#include <linux/of.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"
#include "linux/timer.h"

struct my_gpio {
        struct gpio_desc *led;
        struct gpio_desc *btn;
        struct timer_list timer; 
};

static void my_gpio_timer_callback(struct timer_list * timer)
{
        struct my_gpio *my_gpio = container_of(timer, struct my_gpio, timer);
        int btn_state = gpiod_get_value(my_gpio->btn);

        gpiod_set_value(my_gpio->led, btn_state);
        mod_timer(timer, jiffies + msecs_to_jiffies(100));
}


static int my_gpio_probe(struct platform_device *dev)
{    
        int err = 0;
        struct my_gpio *my_gpio;

        my_gpio = devm_kzalloc(&dev->dev, sizeof(struct my_gpio),
                            GFP_KERNEL);
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

        my_gpio->btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);
        if (IS_ERR(my_gpio->btn)) {
               dev_err(&dev->dev, "GPIO descriptor acquisition failed.\n");
               err = PTR_ERR(my_gpio->btn);
               goto out_ret_err;
        }  

        timer_setup(&my_gpio->timer, &my_gpio_timer_callback, 0);
        mod_timer(&my_gpio->timer, jiffies + msecs_to_jiffies(100));
             
        dev_set_drvdata(&dev->dev, my_gpio);
        dev_info(&dev->dev, "probed\n");

out_ret_err:
        return err;
}


static int my_gpio_remove(struct platform_device *dev)
{
        struct my_gpio *my_gpio = dev_get_drvdata(&dev->dev);
        gpiod_set_value(my_gpio->led, 0);
        del_timer_sync(&my_gpio->timer);

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
