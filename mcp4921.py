from time import sleep
import array, fcntl, struct, termios, os

GENERIC_WRITE = 0x40000000

FILE_PATH = "/dev/dac_mcp4921"

CMD_RESET       = ord("k") << 8 | 0
CMD_ENABLE      = ord("k") << 8 | 1
CMD_DISABLE     = ord("k") << 8 | 2

#               ioctl write code   size
CMD_GAIN        = GENERIC_WRITE | 4 << 16 | ord("k") << 8 | 3
CMD_VREF_BUFF   = GENERIC_WRITE | 4 << 16 | ord("k") << 8 | 4


fd = open(FILE_PATH, "wb")
#res = fcntl.ioctl(fd, CMD_RESET, 0)
# print(res)
# res = fcntl.ioctl(fd, CMD_ENABLE, 0)
# print(res)
# res = fcntl.ioctl(fd, CMD_DISABLE, 0)
# print(res)
buf = array.array('l', [1])
res = fcntl.ioctl(fd, CMD_GAIN, buf)
print(res)
#res = fcntl.ioctl(fd, CMD_VREF_BUFF, 1)
#print(res)
#fd.write('0')
fd.close()
