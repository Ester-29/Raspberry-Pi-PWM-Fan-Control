#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define DEVICE_NAME "pwm_fan"
#define CLASS_NAME "pwm_fan_class"

/* Địa chỉ vật lý nền của BCM2835 (Sử dụng cho Raspberry Pi Zero W) */
#define BCM2835_PERI_BASE 0x20000000
#define GPIO_BASE (BCM2835_PERI_BASE + 0x200000)
#define PWM_BASE  (BCM2835_PERI_BASE + 0x20C000)
#define CLK_BASE  (BCM2835_PERI_BASE + 0x101000)

/* Các thanh ghi GPIO */
#define GPFSEL1 0x04  /* Thanh ghi cấu hình chức năng cho chân GPIO từ 10 đến 19 */

/* Các thanh ghi PWM của BCM2835 */
#define PWM_CTL  0x00  /* PWM Control */
#define PWM_STA  0x04  /* PWM Status */
#define PWM_RNG1 0x10  /* PWM Channel 1 Range */
#define PWM_DAT1 0x14  /* PWM Channel 1 Data */

/* Các thanh ghi điều khiển PWM Clock trong Clock Manager */
#define CM_PWMCTL 0xA0  /* PWM Clock Control */
#define CM_PWMDIV 0xA4  /* PWM Clock Divisor */

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class = NULL;
static struct device *my_device = NULL;

/* Con trỏ ánh xạ bộ nhớ ảo trong Kernel Space */
static void __iomem *gpio_regs;
static void __iomem *pwm_regs;
static void __iomem *clk_regs;

/* Nguyên mẫu các hàm file operations */
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
};

/* Hàm cấu hình phần cứng: GPIO 18 (ALT5), Clock, và PWM Controller */
static void init_pwm_hardware(void) {
    uint32_t val;

    /* 1. Cấu hình chân GPIO 18 sang chức năng ALT5 (Hardware PWM0 Channel 1)
     * Chân GPIO 18 nằm trong thanh ghi GPFSEL1 (quản lý GPIO 10 -> 19).
     * Mỗi GPIO pin chiếm 3 bit trong thanh ghi. Chân 18 bắt đầu từ bit 24 (18-10 = 8; 8 * 3 = 24).
     * Chức năng ALT5 tương ứng với mã nhị phân 010 (giá trị thập phân là 2).
     */
    val = ioread32(gpio_regs + GPFSEL1);
    val &= ~(7 << 24); /* Xóa 3 bit cấu hình của GPIO 18 (bit 26-24) */
    val |= (2 << 24);  /* Đặt chức năng ALT5 (010 nhị phân) */
    iowrite32(val, gpio_regs + GPFSEL1);

    /* 2. Cấu hình bộ chia tần số PWM Clock (PWM Clock Generator)
     * Thao tác với các thanh ghi Clock Manager cần sử dụng mã bảo vệ (Password) 0x5A000000 ở byte cao.
     */
    // Dừng PWM Clock trước khi cấu hình
    iowrite32(0x5A000001, clk_regs + CM_PWMCTL);
    // Chờ cho đến khi PWM Clock dừng hẳn (Kiểm tra bit BUSY - bit 7 bằng 0)
    while (ioread32(clk_regs + CM_PWMCTL) & 0x80) {
        cpu_relax();
    }
    // Thiết lập hệ số chia tần số (Divisor). Ví dụ chia cho 16 để tần số Clock PWM là 19.2MHz / 16 = 1.2MHz
    // Hệ số chia nằm ở byte cao, dịch trái 12 bit cho phần nguyên.
    iowrite32(0x5A000000 | (16 << 12), clk_regs + CM_PWMDIV);
    // Bật PWM Clock với nguồn cấp là thạch anh nội Oscillator (mã nguồn clock = 19.2MHz, giá trị nguồn = 1)
    // 0x11 = bật clock (0x10) + chọn nguồn Oscillator (0x01)
    iowrite32(0x5A000011, clk_regs + CM_PWMCTL);

    /* 3. Cấu hình Bộ điều khiển PWM (PWM Controller) */
    // Dừng kênh PWM 1 trước khi cấu hình
    iowrite32(0, pwm_regs + PWM_CTL);
    // Thiết lập Range (Chu kỳ xung). Sử dụng giá trị 1024 để có độ phân giải Duty Cycle mịn từ 0 đến 1024
    iowrite32(1024, pwm_regs + PWM_RNG1);
    // Đặt giá trị Data ban đầu bằng 0 (quạt tắt)
    iowrite32(0, pwm_regs + PWM_DAT1);
    // Bật kênh PWM 1 ở chế độ M/S Mode (Mark-Space) giúp xung chạy đều và mượt mà hơn
    // Bit 0: PWEN1 (Bật kênh 1)
    // Bit 7: MSEN1 (Bật chế độ Mark-Space)
    // 0x81 = 0x80 + 0x01
    iowrite32(0x81, pwm_regs + PWM_CTL);
}

static int dev_open(struct inode *inode, struct file *file) {
    pr_info("pwm_fan: Thao tac Mo file thiet bi /dev/pwm_fan\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
    pr_info("pwm_fan: Thao tac Dong file thiet bi /dev/pwm_fan\n");
    return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    char k_buf[16];
    unsigned int duty_cycle = 0;
    uint32_t dat_val;

    if (len > sizeof(k_buf) - 1) {
        len = sizeof(k_buf) - 1;
    }

    // Truyền dữ liệu an toàn từ User-space vào Kernel-space
    if (copy_from_user(k_buf, buf, len)) {
        return -EFAULT;
    }
    k_buf[len] = '\0';

    // Chuyển đổi chuỗi ký tự nhận được thành số nguyên không dấu
    if (kstrtouint(k_buf, 10, &duty_cycle) != 0) {
        return -EINVAL;
    }

    // Giới hạn giá trị Duty Cycle trong khoảng 0% - 100%
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }

    // Ánh xạ tuyến tính tỉ lệ phần trăm (0-100) sang thanh ghi PWM DAT1 (0-1024)
    dat_val = (duty_cycle * 1024) / 100;
    iowrite32(dat_val, pwm_regs + PWM_DAT1);

    pr_info("pwm_fan: Da thay doi toc do quat -> Duty Cycle: %u%% (DAT1 = %u)\n", duty_cycle, dat_val);

    return len;
}

static int __init pwm_fan_init(void) {
    int ret;

    pr_info("pwm_fan: Bat dau khoi tao module\n");

    // 1. Cấp phát động thiết bị ký tự (Character Device Number)
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("pwm_fan: Cap phat dev_num that bai\n");
        return ret;
    }

    // 2. Khoi tao va dang ky cdev
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("pwm_fan: Them thiet bi cdev that bai\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    // 3. Tao Class thiet bi de udev tu dong tao file node duoi /dev
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    my_class = class_create(CLASS_NAME);
#else
    my_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(my_class)) {
        pr_err("pwm_fan: Tao class thiet bi that bai\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    // 4. Tao device node /dev/pwm_fan
    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("pwm_fan: Tao thiet bi device node that bai\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }

    // 5. Anh xa dia chi vat ly cua cac thanh ghi BCM2835 sang khong gian ao cua Kernel Space
    gpio_regs = ioremap(GPIO_BASE, 0xB4);
    pwm_regs = ioremap(PWM_BASE, 0x28);
    clk_regs = ioremap(CLK_BASE, 0xA8);

    if (!gpio_regs || !pwm_regs || !clk_regs) {
        pr_err("pwm_fan: Ánh xa bo nho ioremap cho thiet bi that bai\n");
        if (gpio_regs) iounmap(gpio_regs);
        if (pwm_regs) iounmap(pwm_regs);
        if (clk_regs) iounmap(clk_regs);
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    // 6. Cau hinh phan cung ban dau
    init_pwm_hardware();

    pr_info("pwm_fan: Driver da duoc nap thanh cong tai /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit pwm_fan_exit(void) {
    pr_info("pwm_fan: Bat dau go bo driver khoi hệ thong\n");

    // Dung hoat dong cua xung PWM (Quat dung lai)
    iowrite32(0, pwm_regs + PWM_CTL);

    // Giai phong vung nho anh xa ioremap
    iounmap(gpio_regs);
    iounmap(pwm_regs);
    iounmap(clk_regs);

    // Xoa device node va class thiet bi
    device_destroy(my_class, dev_num);
    class_destroy(my_class);

    // Giai phong character device
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("pwm_fan: Driver da duoc giai phong hoan toan\n");
}

module_init(pwm_fan_init);
module_exit(pwm_fan_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Khoa - embedded-system-student");
MODULE_DESCRIPTION("Hardware PWM driver for Raspberry Pi Zero fan control (BCM2835)");
MODULE_VERSION("1.0");


