#!/usr/bin/python

from time import sleep
import array, fcntl, sys

args = sys.argv
if(len(args) < 2):
        sys.exit("Bad number of arguments")

GENERIC_WRITE           = 0x40000000
GENERIC_INT_SIZE        = 0x00040000

FILE_PATH = "/dev/mcp4921"

CMD_RESET       = ord("k") << 8 | 0
#              ioctl write code  size
CMD_ENABLE      = GENERIC_WRITE | GENERIC_INT_SIZE | ord("k") << 8 | 1
CMD_GAIN        = GENERIC_WRITE | GENERIC_INT_SIZE | ord("k") << 8 | 2
CMD_VREF_BUFF   = GENERIC_WRITE | GENERIC_INT_SIZE | ord("k") << 8 | 3


fd = open(FILE_PATH, "w")

if (args[1] == "EN"):
        value = int(args[2])
        buf = array.array('l', [value])
        fcntl.ioctl(fd, CMD_ENABLE, buf)
elif (args[1] == "RESET"):
        fcntl.ioctl(fd, CMD_RESET, 0)
elif (args[1] == "GAIN"):
        value = int(args[2])
        buf = array.array('l', [value])
        fcntl.ioctl(fd, CMD_GAIN, buf)
elif (args[1] == "VREF"):
        value = int(args[2])
        buf = array.array('l', [value])
        fcntl.ioctl(fd, CMD_VREF_BUFF, buf)
elif (args[1] == "SET"):
        value = int(args[2])
        fd.write(str(value))
else:
        print("Bad argument")

fd.close()
