# BÁO CÁO TIẾN ĐỘ & KIỂM THỬ TÍCH HỢP DRIVER ĐIỀU KHIỂN QUẠT PWM

**Đồ án:** Linux Kernel Driver Điều Khiển Tốc Độ Quạt Tản Nhiệt Bằng PWM Tự Động Theo Nhiệt Độ CPU  
**Giảng viên hướng dẫn:** Thầy Trương Ngọc Sơn  
**Học phần:** Hệ thống nhúng (Embedded Systems)  

---

## I. THÔNG TIN NHÓM & PHÂN CHIA CÔNG VIỆC

Nhóm thực hiện gồm 03 thành viên với bảng phân công công việc chi tiết, khoa học, tập trung nhiệm vụ cốt lõi và phức tạp nhất cho Nhóm trưởng. 

*(Bảng dưới đây đã được tối ưu hóa hiển thị trên GitHub bằng thẻ HTML để tránh vỡ khung và căn chỉnh thẳng hàng tuyệt đối)*

| STT | Thành viên | MSSV | Vai trò | Công việc phân công chi tiết | Trọng số đóng góp |
| :---: | :--- | :---: | :---: | :--- | :---: |
| 1 | **Võ Trần Đăng Khoa** | **24119051** | **Nhóm trưởng** | <ul><li>Nghiên cứu cấu trúc Kernel Module, thiết kế Driver <code>pwm_fan_driver.c</code>.</li><li>Lập trình ánh xạ địa chỉ vật lý sang địa chỉ ảo (<code>ioremap</code>), cấu hình thanh ghi GPIO và PWM của chip Broadcom.</li><li>Nghiên cứu và sửa lỗi tương thích hàm <code>class_create</code> giữa Kernel mới (v7.0) và cũ.</li><li>Trực tiếp quản trị Git/GitHub, giải quyết các lỗi xung đột nghiêm trọng (<code>non-fast-forward</code>, <code>force push</code>).</li><li>Khắc phục sự cố bảo mật <b>GitHub Push Protection</b> (rò rỉ mã Token bí mật tại dòng 241 file code C).</li></ul> | **45%**<br>*(Nặng nhất)* |
| 2 | **Huỳnh Anh Tuấn** | **24119096** | **Thành viên** | <ul><li>Thiết lập môi trường máy ảo Oracle VM VirtualBox và Ubuntu OS Host.</li><li>Cài đặt bộ biên dịch chéo <code>arm-linux-gnueabihf-gcc</code> và các thư viện hỗ trợ xây dựng hệ thống (<code>build-essential</code>, <code>bc</code>, <code>bison</code>, <code>flex</code>).</li><li>Tải và thiết lập mã nguồn Kernel Raspberry Pi OS từ GitHub, thực hiện cấu hình phần cứng mặc định (<code>bcmrpi_defconfig</code>).</li><li>Hỗ trợ chạy thử nghiệm nạp driver cục bộ (<code>insmod</code>, <code>rmmod</code>) trên máy ảo Ubuntu.</li></ul> | **30%** |
| 3 | **Phạm Trần Huy Hoàng** | **24119039** | **Thành viên** | <ul><li>Tổ chức cấu trúc thư mục đồ án chuẩn khoa học trên GitHub (<code>source_code/</code>, <code>Docs/</code>, <code>weekly_reports/</code>).</li><li>Thiết lập kịch bản và ma trận kiểm thử (Test Matrix) phục vụ việc chạy thực nghiệm.</li><li>Thử nghiệm phân quyền thiết bị ảo dưới <code>/dev/pwm_fan</code>, gửi tín hiệu mô phỏng tốc độ quạt (<code>echo "75" > /dev/pwm_fan</code>) và theo dõi log hệ thống (<code>dmesg</code>).</li><li>Tổng hợp dữ liệu, viết báo cáo tuần và hoàn thiện tài liệu hướng dẫn Markdown (<code>README.md</code>).</li></ul> | **25%** |

---

## II. QUY TRÌNH THIẾT LẬP MÔI TRƯỜNG BIÊN DỊCH (UBUNTU HOST)

### 1. Cài đặt các thư viện nền tảng và Git
Để chuẩn bị máy ảo Ubuntu cho việc biên dịch hệ thống, nhóm đã thực hiện cài đặt Git và các công cụ dịch phần cứng bằng các lệnh:
```bash
sudo apt update
sudo apt install -y build-essential bc bison flex libncurses5-dev libssl-dev git
```
*   **Sự cố đã giải quyết**: Khắc phục lỗi `Command 'git' not found` trên một số phiên bản máy ảo rút gọn bằng cách cập nhật cơ sở dữ liệu gói và cài đặt gói `git` sạch từ kho Ubuntu.

### 2. Cài đặt và kiểm tra bộ biên dịch chéo (Cross-compiler)
Để dịch mã nguồn x86_64 của máy tính Host sang mã máy ARM 32-bit (chạy trên Raspberry Pi Zero W), nhóm đã cài đặt bộ công cụ biên dịch chéo:
```bash
sudo apt-get install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
```
*   **Kết quả kiểm tra thực tế**:
    ```bash
    arm-linux-gnueabihf-gcc --version
    ```
    Hệ thống phản hồi thành công phiên bản: `arm-linux-gnueabihf-gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0`.

### 3. Tải và cấu hình thư mục nhân Linux (Linux Kernel Source)
Để có các file thư viện hệ thống (Headers) đồng bộ, nhóm đã tiến hành tải mã nguồn nhân Linux Raspberry Pi trực tiếp từ GitHub:
```bash
mkdir -p ~/workspace && cd ~/workspace
git clone --depth=1 --branch rpi-5.10.y https://github.com/raspberrypi/linux.git
```
Cấu hình phần cứng mặc định dành riêng cho dòng chip Broadcom trên Raspberry Pi Zero W:
```bash
cd ~/workspace/linux
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcmrpi_defconfig
```
*Kết quả tạo ra tệp cấu hình phần cứng ẩn `.config` thành công.*

---

## III. QUY TRÌNH LẬP TRÌNH VÀ SỬA LỖI TƯƠNG THÍCH LINUX DRIVER

### 1. Mã nguồn Driver và Makefile cấu hình thông minh
Mã nguồn driver `pwm_fan_driver.c` được thiết kế để đăng ký một lớp thiết bị ảo dưới `/dev/pwm_fan`, lắng nghe tín hiệu Duty Cycle từ User-space thông qua hàm `copy_from_user` để điều chế xung PWM cấp cho transistor kích quạt tản nhiệt.

#### Giải quyết lỗi tương thích phiên bản Kernel (`class_create`):
Khi biên dịch thử nghiệm cục bộ trên máy ảo Ubuntu đang chạy Kernel phiên bản cực kỳ mới **v7.0.0-31-generic**, trình biên dịch báo lỗi nghiêm trọng do thay đổi cấu trúc hàm `class_create`:
*   *Trước Kernel v6.4*: Hàm yêu cầu 2 tham số: `class_create(THIS_MODULE, CLASS_NAME);`
*   *Từ Kernel v6.4 trở đi*: Tham số `THIS_MODULE` bị loại bỏ, chỉ còn 1 tham số: `class_create(CLASS_NAME);`

**Giải pháp mã hóa thông minh bằng biên dịch có điều kiện do Nhóm trưởng thực hiện:**
```c
#include <linux/version.h>

// Trong hàm pwm_fan_init():
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    my_class = class_create(CLASS_NAME);
#else
    my_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
```
*   **Ý nghĩa**: Giúp mã nguồn tương thích hoàn toàn khi biên dịch cục bộ trên máy ảo Ubuntu Host (Kernel 7.0) lẫn biên dịch chéo sang Raspberry Pi (Kernel 5.10).

---

## IV. QUY TRÌNH NẠP, KIỂM THỬ VÀ ĐÁNH GIÁ (TESTING MATRIX)

Vì không có kit phần cứng vật lý tại chỗ, nhóm đã áp dụng phương pháp kiểm thử cục bộ (Local Build & Test) ngay trên nhân Linux của máy ảo Ubuntu nhằm giả lập và đánh giá toàn diện hoạt động của Driver.

### 1. Nhật ký thực thi kiểm thử trên Terminal
1.  **Biên dịch cục bộ tạo Driver thành phẩm**:
    ```bash
    cd ~/workspace/pwm_fan
    make
    ```
    *Hệ thống biên dịch thành công và tạo ra tệp tin `pwm_fan_driver.ko`.*
2.  **Nạp Driver vào nhân Linux**:
    ```bash
    sudo insmod ./pwm_fan_driver.ko
    ```
3.  **Xem log Kernel xác nhận nạp thành công**:
    ```bash
    sudo dmesg | tail -n 10
    ```
    *Log hệ thống hiển thị trực quan*:
    ```text
    [ 3110.166402] pwm_fan: Bat dau khoi tao module
    [ 3110.172822] pwm_fan: Driver da duoc nap thanh cong tai /dev/pwm_fan
    ```
4.  **Phân quyền và giả lập gửi tín hiệu điều khiển**:
    ```bash
    sudo chmod 666 /dev/pwm_fan
    echo "75" > /dev/pwm_fan
    ```
    *Log dmesg hiển thị kết quả ghi nhận từ User-space xuống Driver*:
    ```text
    pwm_fan: Da nhan Duty Cycle = 75
    ```

### 2. Ma trận kết quả kiểm thử thực tế (Test Matrix)

Dựa theo cấu trúc báo cáo tích hợp chuẩn, nhóm đã xây dựng bảng ma trận kiểm thử thực nghiệm:

| STT | Kịch bản kiểm thử | Thao tác thực tế | Kết quả kỳ vọng (Expected) | Trạng thái thực tế |
| :---: | :--- | :--- | :--- | :---: |
| 1 | **Biên dịch chéo** | Chạy `make ARCH=arm...` | Tạo ra tệp nhị phân `pwm_fan_driver.ko` cho ARM. | **ĐẠT** |
| 2 | **Biên dịch cục bộ** | Chạy `make` trên Ubuntu | Giải quyết lỗi tương thích `class_create`, tạo file `.ko` chạy trên x86_64. | **ĐẠT** |
| 3 | **Nạp Kernel Module** | `sudo insmod ./pwm_fan_driver.ko` | Đăng ký thành công thiết bị ảo dưới `/dev/pwm_fan`. | **ĐẠT** |
| 4 | **Quyền truy cập Log** | <code>sudo dmesg \| tail</code> | Vượt qua hàng rào chặn bảo mật `Operation not permitted` của Kernel v7.0. | **ĐẠT** |
| 5 | **Giao tiếp User-Space** | <code>echo "75" > /dev/pwm_fan</code> | Hàm `write` nhận diện chính xác giá trị duty cycle "75" từ người dùng. | **ĐẠT** |

---

## V. QUY TRÌNH THIẾT LẬP KHO LƯU TRỮ GITHUB & GIẢI QUYẾS SỰ CỐ BẢO MẬT

Nhóm đã xây dựng kho lưu trữ GitHub tại địa chỉ: `https://github.com/Ester-29/Raspberry-Pi-PWM-Fan-Control`. Quá trình đẩy code lên đã trải qua các bước giải quyết sự cố kỹ thuật phức tạp:

### 1. Đồng bộ hóa lịch sử xung đột Git (`non-fast-forward`)
*   **Vấn đề**: Khi push code lên bị báo lỗi do trên GitHub đã có sẵn tệp `README.md` trống từ bước khởi tạo, trong khi máy ảo là một lịch sử trắng.
*   **Cách xử lý chuyên nghiệp**:
    ```bash
    git pull origin main --allow-unrelated-histories
    git push origin main
    ```
    *(Gộp lịch sử khác biệt thành công và đẩy các tệp tin lên an toàn).*

### 2. Dập tắt lỗi bảo mật nghiêm trọng (GitHub Push Protection)
*   **Vấn đề**: GitHub chặn đứng lệnh push và báo lỗi `GH013: Repository rule violations found (Push cannot contain secrets)`.
*   **Nguyên nhân**: Hệ thống bảo mật quét thấy mã bí mật **GitHub Personal Access Token** (`ghp_...`) bị dán nhầm vào file **`source_code/pwm_fan_driver.c`** ở dòng **`241`**.
*   **Quy trình dập dịch và làm sạch lịch sử của Nhóm trưởng**:
    1.  Dùng trình soạn thảo `nano` định vị chính xác dòng 241, xóa sạch mã Token bị lộ và lưu lại file `pwm_fan_driver.c` sạch.
    2.  Hủy bỏ commit chứa lỗi bảo mật nhưng giữ nguyên code hiện tại:
        ```bash
        git reset --soft HEAD~1
        git reset
        ```
    3.  Thêm lại file code sạch và sử dụng cờ hiệu chỉnh commit (`--amend`) nhằm **ghi đè và xóa hoàn toàn dấu vết mã Token** khỏi lịch sử lưu trữ của Git cục bộ:
        ```bash
        git add source_code/pwm_fan_driver.c
        git commit --amend --no-edit
        ```
    4.  Đẩy lại lên GitHub an toàn 100%:
        ```bash
        git push origin main
        ```

---

## VI. NGUYÊN TẮC AN TOÀN TRONG QUÁ TRÌNH THỰC NGHIỆM

Để đảm bảo hệ thống vận hành bền bỉ và không gây hỏng hóc thiết bị, nhóm luôn tuân thủ các nguyên tắc cốt lõi:
1.  **Chung mát (Common GND)**: GND động lực của nguồn quạt 12V và GND tín hiệu của Raspberry Pi/máy ảo phải được kết nối chụm chung một điểm để tránh hiện tượng dòng rò điện áp cao làm nhiễu loạn hoặc cháy các chân GPIO điều khiển.
2.  **Cách ly công suất bằng Transistor**: Tuyệt đối không đấu trực tiếp quạt công suất lớn vào chân GPIO của Pi. Bắt buộc phải sử dụng transistor đệm kích dòng (như TIP122 hoặc các dòng MOSFET) để bảo vệ mạch vi điều khiển khỏi dòng cảm ứng ngược của cuộn dây quạt.
3.  **Tản nhiệt liên tục**: Khi chạy sò nóng lạnh Peltier hoặc động cơ công suất lớn, các quạt tản nhiệt mặt nóng phải hoạt động đồng thời để tránh hiện tượng quá nhiệt gây cháy hỏng linh kiện bán dẫn.
