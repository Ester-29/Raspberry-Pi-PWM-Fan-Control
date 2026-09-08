# Tên file đối tượng đầu ra của Driver
obj-m += pwm_fan_driver.o

# Đường dẫn mặc định đến thư mục chứa mã nguồn nhân Linux (Kernel Source) trên máy ảo Host Ubuntu
# Nếu bạn đã tải mã nguồn của Raspberry Pi xuống một thư mục khác, hãy sửa đường dẫn bên dưới
# Hoặc truyền biến KDIR khi chạy lệnh make (ví dụ: make KDIR=~/linux_source)
KDIR ?= /lib/modules/$(shell uname -r)/build

# Lấy đường dẫn thư mục hiện tại làm không gian build cho driver
PWD := $(shell pwd)

# Mục tiêu biên dịch mặc định
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Mục tiêu dọn dẹp các file trung gian sau khi biên dịch
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# ==============================================================================
# HƯỚNG DẪN BIÊN DỊCH CHO BẠN:
# ==============================================================================
# 1. Biên dịch trực tiếp trên Raspberry Pi (Local Compile):
#    $ make
#
# 2. Biên dịch chéo trên máy ảo Host Ubuntu (Cross-compilation) cho Raspberry Pi Zero:
#    $ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KDIR=/duong/dan/thu/muc/linux/kernel/cua/pi
# ==============================================================================
