#include <linux/of.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"
#include "linux/timer.h"


#define LEDS_NUMBER 4


struct my_gpio {
        struct gpio_descs *leds;
        struct gpio_desc *btn;
        struct platform_device *pdev;
        struct timer_list timer; 
        int leds_value;
};


static ssize_t my_gpio_leds_value_store(struct device *dev, struct device_attribute *attr, 
                           const char *buf, size_t count)
{
        int err;
        struct my_gpio *my_gpio = dev_get_drvdata(dev);
        
        err = kstrtol(buf, 0, (long int*)(&my_gpio->leds_value));
        if (err) {
                dev_err(dev, "Bad input number.\n");
                return err;
        }
                
        return count;
}
DEVICE_ATTR_WO(my_gpio_leds_value);


static void my_gpio_set_leds(struct my_gpio *my_gpio, int value)
{
        int led_ctr = 0;

        for (led_ctr = 0; led_ctr < LEDS_NUMBER; led_ctr++)
                gpiod_set_value(my_gpio->leds->desc[led_ctr], value); 
}


static void my_gpio_timer_callback(struct timer_list * timer)
{
        struct my_gpio *my_gpio = container_of(timer, struct my_gpio, timer);
        int btn_state = gpiod_get_value(my_gpio->btn);
     
        if (btn_state == 1)
                my_gpio_set_leds(my_gpio, my_gpio->leds_value);
        else
                my_gpio_set_leds(my_gpio, 0);

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
        
        my_gpio->leds = devm_gpiod_get_array(&dev->dev, "leds", GPIOD_OUT_LOW);
        if (IS_ERR(my_gpio->leds)) {
               dev_err(&dev->dev, "GPIO descriptor acquisition failed.\n");
               err = PTR_ERR(my_gpio->leds);
               goto out_ret_err;
        }  

        my_gpio->btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);
        if (IS_ERR(my_gpio->btn)) {
               dev_err(&dev->dev, "GPIO descriptor acquisition failed.\n");
               err = PTR_ERR(my_gpio->btn);
               goto out_ret_err;
        }  

        err = device_create_file(&dev->dev, &dev_attr_my_gpio_leds_value);
        if (err) {
               dev_err(&dev->dev, "Device file creation failed.\n");
               goto out_ret_err;
        }

        timer_setup(&my_gpio->timer, &my_gpio_timer_callback, 0);
        mod_timer(&my_gpio->timer, jiffies + msecs_to_jiffies(100));
        
        my_gpio->pdev = dev;
        dev_set_drvdata(&dev->dev, my_gpio);

        dev_info(&dev->dev, "probed\n");

out_ret_err:
        return err;
}


static int my_gpio_remove(struct platform_device *dev)
{
        struct my_gpio *my_gpio = dev_get_drvdata(&dev->dev);
        del_timer_sync(&my_gpio->timer);
        device_remove_file(&dev->dev, &dev_attr_my_gpio_leds_value);

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
