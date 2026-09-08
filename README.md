# BÁO CÁO TIẾN ĐỘ ĐỒ ÁN MỖI TUẦN
**Đồ án:** Xây Dựng Linux Driver Điều Khiển Tốc Độ Quạt Bằng PWM Trên Raspberry Pi
**Giảng viên hướng dẫn:** Thầy Trương Ngọc Sơn
**Thành viên thực hiện:** Võ Trần Đăng Khoa

## 1. MỤC TIÊU KIỂM THỬ TUẦN NÀY
Trọng tâm tuần này là hoàn thiện việc cài đặt môi trường biên dịch chéo, viết driver điều khiển quạt và nạp thành công module driver vào Kernel.

## 2. SƠ ĐỒ ĐẤU NỐI CHI TIẾT (WIRING DIAGRAM)
Sử dụng mã khối ký tự để vẽ sơ đồ phân tầng nguồn điện:
                  [ Nguồn Tổ Ong 12V ]
                           │
        ┌──────────────────┴──────────────────┐
        ▼                                     ▼
[Động cơ quạt 12V]                    [Mạch Hạ Áp LM2596] │                                     │ ▼                            (Hạ áp xuống đúng 5V) [Cực C Transistor]                           │ ▼ [Cấp nguồn Pi]

## 3. BẢNG GÁN CHÂN GPIO (PINOUT TABLE)
| Linh kiện | Chân linh kiện | GPIO trên Pi Zero W | Chức năng kỹ thuật |
| :--- | :--- | :--- | :--- |
| **Quạt tản nhiệt** | Dây cấp nguồn 12V | GPIO 18 (PWM0) | Điều khiển tốc độ quạt bằng xung PWM. |
| **Transistor Đệm** | Chân Base (B) | GPIO 18 | Nhận xung điều khiển kích mở dòng cho quạt. |

## 4. QUY TRÌNH TRIỂN KHAI & BẢN MÃ GIẢ (PSEUDOCODE)
```c
// Đăng ký Driver Thiết bị:
Khởi tạo Module:
    Cấp phát động số Major/Minor cho Driver
    Ánh xạ địa chỉ vật lý GPIO và PWM sang địa chỉ ảo (ioremap)
    Cấu hình chân GPIO 18 làm chức năng phát xung PWM
5. MA TRẬN KẾT QUẢ KIỂM THỬ KỲ VỌNG (TEST MATRIX)
STT
Hạng mục kiểm thử
Điều kiện kiểm thử
Kết quả kỳ vọng (Expected)
Trạng thái thực tế
1
Biên dịch chéo
Chạy lệnh make trên Ubuntu
Tạo ra file pwm_fan_driver.ko thành công.
ĐẠT
2
Nạp Driver vào Pi
Chạy lệnh sudo insmod trên Pi
Log kernel xuất hiện thông báo Driver đã nạp thành công.
ĐẠT
6. CÁC NGUYÊN TẮC AN TOÀN TRONG QUÁ TRÌNH THỰC NGHIỆM
Tuyệt đối không đấu trực tiếp cực dương của quạt 12V vào chân GPIO của Raspberry Pi để tránh dòng rò làm cháy chip vi xử lý.
Luôn cấu hình chung chân mát (Common GND) giữa nguồn nuôi ngoài và chân GND của Raspberry Pi.
