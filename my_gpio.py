from time import sleep


LED_PATH = "/sys/devices/platform/led"


def led_get_value(led):
        path = LED_PATH + str(led) + "/value"
        file = open(path, "r")
        value = file.readline()
        file.close()

        return value


def led_set_value(led, value):
        path = LED_PATH + str(led) + "/value"
        file = open(path, "w")
        file.write(str(value))
        file.close()

def leds_off():
        for i in range(4):
                led_set_value(i, 0)

def led_on(led):
        leds_off()
        led_set_value(led, 1)



for i in range(4):
        print("LED_", i, " : ", led_get_value(1))



led_ctr = 0

while True:
        led_on(led_ctr % 4)
        led_ctr += 1
        sleep(0.15)