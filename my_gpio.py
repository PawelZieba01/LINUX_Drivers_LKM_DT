#!/usr/bin/python

from time import sleep
import curses
from math import floor

curses.KEY_ENTER = 10

class led_driver:
        def __init__(self, led):
                self.LED_PATH = "/sys/devices/platform/led" + str(led) + "/my_gpio_value"
        
        def __set_value(self, value):
                file = open(self.LED_PATH, "w")
                file.write(str(value))
                file.close()

        def __all_off(self):
                for i in range(4):
                        self.__set_value(i, 0)

        def on(self):
                self.__set_value(1)
        
        def off(self):
                self.__set_value(0)

        def get_value(self):
                file = open(self.LED_PATH, "r")
                value = int(file.readline())
                file.close()
                return value
        
        def toggle(self):
                if (self.get_value()):
                        self.off()
                        return 0
                else:
                        self.on()
                        return 1

class box:
        def __init__(self, screen, x, y, str, w, h):
                self.screen = screen
                self.x = x
                self.y = y
                self.str = str
                self.w = w
                self.h = h
                self.color = curses.color_pair(0)

        def draw(self):
                for line in range(self.h):
                        for char in range(self.w):
                                self.screen.addch(self.y+line, self.x+char, " ", self.color)

                str_len = len(self.str)          
                self.screen.addstr(floor(self.y+self.h/2), floor(self.x+(self.w-str_len)/2), self.str, self.color | curses.A_BOLD)

        def set_color(self, color):
                self.color = color

        def set_text(self, str):
                self.str = str

class led_box(box):
        def __init__(self, screen, x, y, str, w=7, h=3):
                super().__init__(screen, x, y, str, w, h)
                self.set_color(curses.color_pair(2))

class led_pointer(box):
        def __init__(self, screen, x, y, str, w=7, h=3):
                super().__init__(screen, x, y, str, w, h)
                self.set_color(curses.color_pair(0))

class led:
        def __init__(self, screen, x, y, str):
                self.led_b = led_box(screen, x, y, str, 11, 3)
                self.pointer = led_pointer(screen, x+15, y+1, "", 6, 1)
                self.activate()

        def draw(self):                       
                self.led_b.draw()
                self.pointer.draw()

        def activate(self):
                self.pointer.set_text("<---")

        def deactivate(self):
                self.pointer.set_text("")
        
        def on(self):
                self.led_b.set_color(curses.color_pair(1))

        def off(self):
                self.led_b.set_color(curses.color_pair(2))
         


def main(stdscr):
        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_GREEN)
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_RED)
        curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_MAGENTA, curses.COLOR_BLACK)


        leds = [led(stdscr, 5, 3, "LED_0"),
                led(stdscr, 5, 7, "LED_1"),
                led(stdscr, 5, 11, "LED_2"),
                led(stdscr, 5, 15, "LED_3")]
        
        leds_drv = [led_driver(0),
                    led_driver(1),
                    led_driver(2),
                    led_driver(3)]
        
        led_pointer = 0
        user_char = None

        curses.curs_set(0)

        while True:
                # debug
                stdscr.addstr(0, 50, "Debug data:", curses.A_BOLD | curses.color_pair(3))
                stdscr.addstr(1, 50, "led_pointer: " + str(led_pointer), curses.color_pair(3))
                stdscr.addstr(2, 50, "last user key: " + str(user_char), curses.color_pair(3))

                # legend
                stdscr.addstr(5, 50, "CONTROLS:", curses.A_BOLD | curses.color_pair(3))
                stdscr.addstr(6, 50, "ARROW UP,  ARROW DOWN,  ENTER", curses.color_pair(3))

                # Title
                stdscr.addstr(0, 0, "Simple terminal app to control gpio driver", curses.A_BOLD | curses.color_pair(4))

                # Led pointer controll
                for i, led_i in enumerate(leds):
                        if(led_pointer == i):
                                led_i.activate()
                        else:
                                led_i.deactivate()

                        led_i.draw()


                # User input controll
                user_char = stdscr.getch()

                if (user_char == curses.KEY_DOWN):
                        led_pointer += 1
                        if (led_pointer == len(leds)): led_pointer = 0
                elif (user_char == curses.KEY_UP):
                        led_pointer -= 1
                        if (led_pointer < 0): led_pointer = len(leds)-1
                elif (user_char == curses.KEY_ENTER):
                        state = leds_drv[led_pointer].toggle()
                        if (state):
                                leds[led_pointer].on()
                        else:
                                leds[led_pointer].off()
                
                
                stdscr.erase()
                
curses.wrapper(main)

