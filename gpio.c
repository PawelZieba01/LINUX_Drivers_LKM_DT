#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"
#include "linux/timer.h"


static struct gpio_descs *leds;
static struct gpio_desc *btn;
static struct timer_list timer;

enum servo_state {IDLE, STEP_LEFT, STEP_RIGHT};

struct servo {
        enum servo_state state;
        int current_pos;
        int desired_pos;
        int led_ctr;
};

static struct servo servo_data = {
        .state = IDLE,
        .current_pos = 0,
        .desired_pos = 0,
        .led_ctr = 0,
};

static ssize_t servo_desired_pos_store(struct device *dev, 
                                       struct device_attribute *attr, 
                                       const char *buf, size_t count)
{
        int res;
        
        res = kstrtol(buf, 0, (long int*)(&servo_data.desired_pos));

        if (res)
                return res;

        return count;
}
DEVICE_ATTR_WO(servo_desired_pos);


static void set_leds(int val)
{
        int led_ctr = 0;
        for (led_ctr=0 ; led_ctr<4 ; led_ctr++)
                gpiod_set_value(leds->desc[led_ctr], ((val>>led_ctr)&0x01));
}

static void set_led(int led)
{
        set_leds(0x01 << led);
}


static void step_left(void)
{
        servo_data.led_ctr--;
        if (servo_data.led_ctr < 0)
                servo_data.led_ctr = 3;
}


static void step_right(void)
{
        servo_data.led_ctr++;
        if (servo_data.led_ctr > 3)
                servo_data.led_ctr = 0;
}


static void timer_callback(struct timer_list * timer)
{
        switch (servo_data.state) {
                case IDLE:
                        if (servo_data.current_pos > servo_data.desired_pos)
                                servo_data.state = STEP_LEFT;
                        else if (servo_data.current_pos < servo_data.desired_pos)
                                servo_data.state = STEP_RIGHT;
                        else
                                servo_data.state = IDLE;
                        break;

                case STEP_LEFT:
                        if (servo_data.current_pos > servo_data.desired_pos) {
                                step_left();
                                servo_data.current_pos--;
                                servo_data.state = STEP_LEFT;
                        } else {
                                servo_data.state = IDLE;
                        }     
                        break;

                case STEP_RIGHT:
                        if(servo_data.current_pos < servo_data.desired_pos) {
                                step_right();
                                servo_data.current_pos++;
                                servo_data.state = STEP_RIGHT;
                        } else {
                                servo_data.state = IDLE;
                        }
                        break;

                default:
                        break;
        }

        set_led(servo_data.led_ctr);
        mod_timer(timer, jiffies + msecs_to_jiffies(100));
}


static int gpio_probe(struct platform_device *dev)
{    
        timer_setup(&timer, &timer_callback, 0);
        mod_timer(&timer, jiffies + msecs_to_jiffies(100));

        dev_info(&dev->dev, "probed\n");

        device_create_file(&dev->dev, &dev_attr_servo_desired_pos);
        
        leds = devm_gpiod_get_array(&dev->dev, "leds", GPIOD_OUT_LOW);
        btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);
             
        return 0;
}


static int gpio_remove(struct platform_device *dev)
{
        dev_info(&dev->dev, "removed\n");

        set_leds(0);
        del_timer_sync(&timer);
        device_remove_file(&dev->dev, &dev_attr_servo_desired_pos);

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
