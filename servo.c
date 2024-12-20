#include <linux/of.h>
#include <linux/platform_device.h>
#include "linux/gpio/consumer.h"
#include "linux/timer.h"


#define LEDS_NUMBER 4

enum servo_state {IDLE, STEP_LEFT, STEP_RIGHT};

struct servo {
        struct gpio_descs *leds;
        struct gpio_desc *btn;
        struct platform_device *pdev;
        struct timer_list timer; 

        enum servo_state state;
        int current_pos;
        int desired_pos;
        int led_ctr;
};





// static struct servo servo_data = {
//         .state = IDLE,
//         .current_pos = 0,
//         .desired_pos = 0,
//         .led_ctr = 0,
// };

static ssize_t servo_desired_pos_store(struct device *dev, 
                                       struct device_attribute *attr, 
                                       const char *buf, size_t count)
{
        int err;
        struct servo *servo = dev_get_drvdata(dev);
        
        err = kstrtol(buf, 0, (long int*)(&servo->desired_pos));
        if (err) {
                dev_err(dev, "Bad input number.\n");
                return err;
        }
                
        return count;
}
DEVICE_ATTR_WO(servo_desired_pos);


static void servo_set_leds(struct servo *servo, int value)
{
        int led_ctr = 0;
        for (led_ctr = 0; led_ctr < LEDS_NUMBER; led_ctr++)
                gpiod_set_value(servo->leds->desc[led_ctr], ((value >> led_ctr) & 0x01)); 
}


static void servo_set_led(struct servo *servo, int led)
{
        servo_set_leds(servo, (1 << led));
}


static void servo_step_left(struct servo *servo)
{
        servo->led_ctr--;
        if (servo->led_ctr < 0)
                servo->led_ctr = 3;
}


static void servo_step_right(struct servo *servo)
{
        servo->led_ctr++;
        if (servo->led_ctr > 3)
                servo->led_ctr = 0;
}


static void servo_timer_callback(struct timer_list * timer)
{
        struct servo *servo = container_of(timer, struct servo, timer);
        switch (servo->state) {
                case IDLE:
                        if (servo->current_pos > servo->desired_pos)
                                servo->state = STEP_LEFT;
                        else if (servo->current_pos < servo->desired_pos)
                                servo->state = STEP_RIGHT;
                        else
                                servo->state = IDLE;
                        break;

                case STEP_LEFT:
                        if (servo->current_pos > servo->desired_pos) {
                                servo_step_left(servo);
                                servo->current_pos--;
                                servo->state = STEP_LEFT;
                        } else {
                                servo->state = IDLE;
                        }     
                        break;

                case STEP_RIGHT:
                        if(servo->current_pos < servo->desired_pos) {
                                servo_step_right(servo);
                                servo->current_pos++;
                                servo->state = STEP_RIGHT;
                        } else {
                                servo->state = IDLE;
                        }
                        break;

                default:
                        break;
        }

        servo_set_led(servo, servo->led_ctr);
        mod_timer(timer, jiffies + msecs_to_jiffies(100));
}


static int servo_probe(struct platform_device *dev)
{    
        int err = 0;
        struct servo *servo;

        servo = devm_kzalloc(&dev->dev, sizeof(struct servo),
                            GFP_KERNEL);
        if (IS_ERR(servo)) {
               dev_err(&dev->dev, "Memory allocation failed.\n");
               err = PTR_ERR(servo);
               goto out_ret_err;
        }  
        
        servo->leds = devm_gpiod_get_array(&dev->dev, "leds", GPIOD_OUT_LOW);
        if (IS_ERR(servo->leds)) {
               dev_err(&dev->dev, "GPIO descriptor acquisition failed.\n");
               err = PTR_ERR(servo->leds);
               goto out_ret_err;
        }  

        servo->btn = devm_gpiod_get(&dev->dev, "btn", GPIOD_IN);
        if (IS_ERR(servo->btn)) {
               dev_err(&dev->dev, "GPIO descriptor acquisition failed.\n");
               err = PTR_ERR(servo->btn);
               goto out_ret_err;
        }  

        err = device_create_file(&dev->dev, &dev_attr_servo_desired_pos);
        if (err) {
               dev_err(&dev->dev, "Device file creation failed.\n");
               goto out_ret_err;
        }

        timer_setup(&servo->timer, &servo_timer_callback, 0);
        mod_timer(&servo->timer, jiffies + msecs_to_jiffies(100));
        
        servo->pdev = dev;
        dev_set_drvdata(&dev->dev, servo);

        dev_info(&dev->dev, "probed\n");

out_ret_err:
        return err;
}


static int servo_remove(struct platform_device *dev)
{
        struct servo *servo = dev_get_drvdata(&dev->dev);

        del_timer_sync(&servo->timer);
        device_remove_file(&dev->dev, &dev_attr_servo_desired_pos);
        servo_set_leds(servo, 0);

        dev_info(&dev->dev, "removed\n");
        return 0;
}


static const struct of_device_id servo_of_id[] = {
        { .compatible = "mtm,servo" },
        { },
};
MODULE_DEVICE_TABLE(of, servo_of_id);


static struct platform_driver servo_driver = {
        .probe = servo_probe,
        .remove = servo_remove,
        .driver = {
                .name = "servo",
                .of_match_table = servo_of_id,
                .owner = THIS_MODULE,
        },
};
module_platform_driver(servo_driver);


MODULE_LICENSE("GPL v2");
