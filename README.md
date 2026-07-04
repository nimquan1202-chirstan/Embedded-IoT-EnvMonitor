# 🌡️ IoT Environment Monitor — STM32 + ESP32 + Web Dashboard

<div align="center">

**Đại học Quốc gia TP.HCM · Trường Đại học Khoa học Tự nhiên · Khoa Vật lý học**

---

![STM32](https://img.shields.io/badge/MCU-STM32F103C8T6-blue?style=for-the-badge&logo=stmicroelectronics&logoColor=white)
![ESP32](https://img.shields.io/badge/Gateway-ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-03234B?style=for-the-badge&logo=stmicroelectronics)
![Arduino](https://img.shields.io/badge/Firmware-Arduino%20IDE-00979D?style=for-the-badge&logo=arduino&logoColor=white)
![C++](https://img.shields.io/badge/Backend-C%2B%2B%2017%20Crow-F89820?style=for-the-badge&logo=cplusplus)
![JavaScript](https://img.shields.io/badge/Frontend-JavaScript%20%2B%20Chart.js-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black)
![ThingsBoard](https://img.shields.io/badge/Cloud-ThingsBoard-7B68EE?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-22c55e?style=for-the-badge)

### *"Thiết Kế Hệ Thống Nhúng Thu Thập Dữ Liệu Vi Khí Hậu Kết Nối Đa Vi Điều Khiển Qua Giao Thức Nối Tiếp UART Và Đồng Bộ Điện Toán Đám Mây ThingsBoard Cloud"*

**Sinh viên:** Nìm Nguyễn Quán Quân &nbsp;·&nbsp; Khoa Vật lý học &nbsp;·&nbsp; 2026

[📐 Kiến trúc](#-kiến-trúc-hệ-thống) &nbsp;·&nbsp;
[🔌 Phần cứng](#-phần-cứng--kết-nối) &nbsp;·&nbsp;
[📁 Cấu trúc thư mục](#-cấu-trúc-thư-mục) &nbsp;·&nbsp;
[⚙️ Cài đặt](#️-hướng-dẫn-cài-đặt) &nbsp;·&nbsp;
[📡 API](#-api-reference) &nbsp;·&nbsp;
[🧮 Thuật toán](#-thuật-toán)

</div>

---

## 📌 Tổng quan dự án

Hệ thống IoT hoàn chỉnh theo kiến trúc **Edge-to-Cloud 3 tầng**, thực hiện thu thập, xử lý, hiển thị và cảnh báo dữ liệu vi khí hậu (nhiệt độ, độ ẩm) theo thời gian thực.

**Luồng dữ liệu chính:**

```
DHT11 ──Single-Bus──► STM32F103C8T6 ──UART 9600──► ESP32 ──HTTP POST──► Crow C++ Server ──► Web Dashboard
                            │                          │
                      LED 3 màu                  ThingsBoard Cloud (MQTT)
                      (PA1/PA4/PA7)              (Access Token Auth)
```

> **Điểm đặc biệt:** Toàn bộ thuật toán AI, phân tích dữ liệu và API đều chạy trên **Web Server C++ nội bộ** — không phụ thuộc Cloud để hoạt động. ThingsBoard chỉ là kênh đồng bộ song song.

---

## 🏗️ Kiến trúc hệ thống

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      TẦNG 1 — THU THẬP DỮ LIỆU                             │
│                                                                             │
│   ┌──────────┐   1-Wire Single Bus     ┌───────────────────────────────┐    │
│   │  DHT11   │ ────────── PB9 ───────► │     STM32F103C8T6 (Master)   │    │
│   │ Sensor   │   (Open-Drain + PullUp) │   LQFP48 · 72 MHz · 64KB    │    │
│   └──────────┘                         │                               │    │
│                                        │  ┌─ GPIO OUTPUT_PP ─────┐    │    │
│                                        │  │ PA1 → LED Đỏ  🔴     │    │    │
│                                        │  │ PA4 → LED Xanh Lá 🟢 │    │    │
│                                        │  │ PA7 → LED Xanh Dương🔵│    │    │
│                                        │  └──────────────────────┘    │    │
│                                        │  TIM1: Prescaler=71→1µs/tick │    │
│                                        │  USART2: PA2(TX) · PA3(RX)  │    │
│                                        └───────────┬───────────────────┘    │
└───────────────────────────────────────────────────┼─────────────────────────┘
                                                    │ UART · 9600 baud · 8N1
                                                    │ PA2(TX) → GPIO16(RX2)
                                                    │ Logic: 3.3V ↔ 3.3V ✅
┌───────────────────────────────────────────────────▼─────────────────────────┐
│                      TẦNG 2 — GATEWAY & XỬ LÝ                               │
│                                                                             │
│                      ┌──────────────────────────────┐                       │
│                      │        ESP32 DevKit v1        │                       │
│                      │   Xtensa LX6 · Wi-Fi 2.4GHz  │                       │
│                      │                              │                       │
│                      │  Serial2.begin(115200,…,16,17)│                      │
│                      │  Bóc tách: indexOf(',')       │                       │
│                      │  HTTPClient → HTTP POST JSON  │                       │
│                      │  Chống spam: 3000ms cooldown  │                       │
│                      └───────────┬──────────────────┘                       │
└──────────────────────────────────┼──────────────────────────────────────────┘
                                   │
                    ┌──────────────┴──────────────┐
                    │ HTTP POST                   │ MQTT (mở rộng)
                    │ /api/telemetry              │ Port 1883
                    ▼                             ▼
┌───────────────────────────────┐   ┌─────────────────────────────────┐
│   WEB SERVER C++ (Crow)       │   │      THINGSBOARD CLOUD          │
│   localhost:8080              │   │   thingsboard.cloud             │
│                               │   │                                 │
│ • 5 REST API endpoints        │   │ • Telemetry Time-series         │
│ • KNN Classifier (K=3)        │   │ • Access Token: jG4GYT...       │
│ • Humidex · Dew Point JEDEC   │   │ • Dashboard realtime            │
│ • Adaptive Sampling (dT/dt)   │   │ • Offline → flush on reconnect  │
│ • Fire Detection Rolling Win  │   └─────────────────────────────────┘
│ • Offline Buffer (CSV+Queue)  │
│ • Web Dashboard (JS+Chart.js) │
└───────────────────────────────┘
```

---

## 🔌 Phần cứng & Kết nối

### Linh kiện sử dụng

| Linh kiện | Thông số | Vai trò |
|---|---|---|
| **STM32F103C8T6** (Blue Pill) | ARM Cortex-M3, 72 MHz, 64KB Flash, LQFP48 | Master: đọc DHT11, điều khiển LED, truyền UART |
| **DHT11** | 0–50°C ±2°C · 20–90%RH ±5% · 1-Wire | Cảm biến vi khí hậu |
| **ESP32 DevKit v1** | Xtensa LX6 Dual-core, Wi-Fi 802.11b/g/n | Gateway: bóc tách UART, HTTP POST |
| **LED Đỏ** × 1 | GPIO Output Push-Pull, 3.3V | Báo nhiệt độ ≥ 40°C |
| **LED Xanh Lá** × 1 | GPIO Output Push-Pull, 3.3V | Báo nhiệt độ bình thường 20–40°C |
| **LED Xanh Dương** × 1 | GPIO Output Push-Pull, 3.3V | Báo nhiệt độ < 20°C |
| **Điện trở 330Ω** × 3 | — | Hạn dòng LED |
| **ST-Link V2** | SWD Debug Interface | Nạp & debug firmware STM32 |

---

### Sơ đồ đấu nối chi tiết

#### STM32 ↔ ESP32 (UART)

| STT | Chân STM32 | Chân ESP32 | Kiểu nối | Chức năng |
|:---:|---|---|:---:|---|
| 1 | **GND** | **GND** | Song song — bắt buộc | Đồng bộ mốc điện áp tham chiếu 0V |
| 2 | **PA2** (USART2_TX) | **GPIO 16** (RX2) | Chéo TX → RX | Truyền chuỗi `Temp: X.X C, Humid: Y.Y %` |
| 3 | **PA3** (USART2_RX) | **GPIO 17** (TX2) | Chéo RX ← TX | Nhận lệnh từ ESP32 (mở rộng) |

> ✅ **Không cần Level Shifter:** STM32 và ESP32 đều dùng mức logic **3.3V** — kết nối TX-RX trực tiếp an toàn.

#### STM32 ↔ DHT11 (Single-Bus)

| Chân STM32 | Chân DHT11 | Ghi chú |
|---|---|---|
| **PB9** (GPIO Open-Drain + Pull-Up) | **DATA** | Single-Bus 1-Wire · Điện trở kéo lên 10KΩ |
| **3.3V** | **VCC** | Nguồn cấp DHT11 |
| **GND** | **GND** | Mass chung |

#### STM32 → LED (GPIO Output)

| Chân STM32 | Màu LED | Kích hoạt khi | Điện trở |
|---|---|---|---|
| **PA1** | 🔴 Đỏ | `T ≥ 40.0°C` | 330Ω |
| **PA4** | 🟢 Xanh Lá | `20.0°C ≤ T < 40.0°C` | 330Ω |
| **PA7** | 🔵 Xanh Dương | `T < 20.0°C` | 330Ω |

#### Sơ đồ ASCII tổng hợp

```
        DHT11                   STM32F103C8T6 (Blue Pill)
       ┌──────┐                ┌──────────────────────────┐
       │ VCC  │──── 3.3V ─────┤ 3.3V                     │
       │ DATA │──[10kΩ]──3.3V─┤ PB9 (Open-Drain)         │
       │ GND  │──── GND ──────┤ GND                      │
       └──────┘                │                          │
                               │ PA1 ──[330Ω]──► LED ĐỎ  │
                               │ PA4 ──[330Ω]──► LED XANH LÁ│
                               │ PA7 ──[330Ω]──► LED XANH DƯƠNG│
                               │                          │
                               │ PA2 (TX) ─────────────────┼──► GPIO16 (RX2)┐
                               │ PA3 (RX) ◄─────────────────┼─── GPIO17 (TX2)│  ESP32
                               │ GND ───────────────────────┼─── GND         │
                               │ PA13 (SWDIO) ──► ST-Link   │
                               │ PA14 (SWDCLK) ─► ST-Link   │
                               └──────────────────────────┘
                                                            │
                                               Wi-Fi 802.11b/g/n
                                               HTTP POST → Crow Server
                                               (Mở rộng: MQTT → ThingsBoard)
```

---

## 📁 Cấu trúc thư mục

Dự án được tổ chức thành **2 thư mục chính độc lập**: phần cứng firmware và phần mềm web server.

```
IoT-Environment-Monitor/
│
├── 📁 firmware/                        ← PHẦN CỨNG (STM32 + ESP32)
│   │
│   ├── 📁 STM32_DHT11/                 ← Project STM32CubeIDE
│   │   ├── 📁 Core/
│   │   │   ├── 📁 Src/
│   │   │   │   ├── main.c              ← Vòng lặp chính: DHT11 + UART + LED
│   │   │   │   ├── dht11.c            ← Driver: DHT11_Start() + DHT11_Read()
│   │   │   │   └── stm32f1xx_hal_msp.c
│   │   │   └── 📁 Inc/
│   │   │       ├── main.h
│   │   │       └── dht11.h
│   │   ├── 📁 Drivers/                 ← STM32 HAL Library (auto-generated)
│   │   ├── dht11project.ioc           ← ⚙️ CubeMX config (chip, GPIO, TIM, UART)
│   │   ├── dht11project_Debug.cfg     ← OpenOCD debug config (ST-Link DAP SWD)
│   │   ├── STM32F103C8TX_FLASH.ld    ← Linker script (Flash layout)
│   │   ├── .project                   ← Eclipse/CubeIDE project descriptor
│   │   ├── .cproject                  ← CDT build config (GCC, optimization)
│   │   └── .mxproject                 ← STM32CubeMX project metadata
│   │
│   └── 📁 ESP32_Gateway/              ← Sketch Arduino IDE
│       └── esp32_gateway.ino          ← WiFi + UART parse + HTTP POST
│
└── 📁 Iot_Web_Project/                 ← PHẦN MỀM (Backend C++ + Frontend Web)
    │
    ├── 📁 src/                     ← Web Server C++ (Visual Studio Code)
    │   ├── main.cpp                    ← Crow HTTP Server · 5 API endpoints
    │   ├── MonitorSystem.h             ← Class: Thresholds, AppMode enum
    │   ├── MonitorSystem.cpp           ← KNN, Humidex, Mode recommendations
    │   └── crow_all.h                  ← Crow framework (header-only, ~1MB)
    │
    ├── 📁 frontend/                    ← Web Dashboard (Visual Studio Code)
    │   ├── index.html                  ← UI dark IoT theme · Tabler Icons
    │   └── app.js                      ← Realtime polling · Chart · Email alert
    │
    └── 📁 buid/                        ← Dữ liệu sinh tự động khi chạy
        ├── history_data.csv           ← Lịch sử đo (tự sinh)
        └── offline_queue.txt          ← Hàng đợi ThingsBoard offline (tự sinh)
```

---

## ⚙️ Hướng dẫn cài đặt

### Yêu cầu hệ thống

| Công cụ | Phiên bản | Dùng cho |
|---|---|---|
| STM32CubeIDE | 1.x | Build & flash firmware STM32 |
| STM32CubeMX | 6.15.0 | Sinh mã cấu hình (file `.ioc`) |
| Arduino IDE | 2.x | Flash firmware ESP32 |
| g++ / MinGW | C++17 | Build Crow Web Server |
| Visual Studio Code | — | Phát triển backend & frontend |

---

### Bước 1 — Flash firmware STM32

#### 1.1. Mở project trong STM32CubeIDE

```
File → Import → Existing Projects into Workspace
→ Chọn thư mục: firmware/STM32_DHT11/
→ Import → Build (Ctrl + B)
```

#### 1.2. Cấu hình đã có sẵn trong `dht11project.ioc`

| Peripheral | Cấu hình |
|---|---|
| **Chip** | STM32F103C8T6 · LQFP48 |
| **Clock** | HSE → PLL × 9 → **SYSCLK = 72 MHz** |
| **TIM1** | Prescaler = **71** → 1 MHz → **1µs/tick** (đếm micro giây cho DHT11) |
| **USART2** | Asynchronous · **9600 baud** · 8N1 · PA2(TX) / PA3(RX) |
| **PA1** | GPIO_Output_PP · Khởi tạo HIGH → **LED Đỏ** |
| **PA4** | GPIO_Output_PP · Khởi tạo HIGH → **LED Xanh Lá** |
| **PA7** | GPIO_Output_PP · Khởi tạo HIGH → **LED Xanh Dương** |
| **PB9** | GPIO_Output_OD + Pull-Up → **DHT11 Single-Bus Data** |
| **PA13/PA14** | SWD Interface → ST-Link Debug |
| **Debug** | ST-Link DAP · SWD · GDB Port 3333 · Clock 8 MHz |

#### 1.3. Thêm DHT11 driver thủ công

Tạo file `Core/Src/dht11.c` và `Core/Inc/dht11.h`, thêm các hàm:

```c
uint8_t DHT11_Start(void);   // Gửi xung Start Signal, đợi ACK từ DHT11
uint8_t DHT11_Read(void);    // Đọc 1 byte (8 bit) theo giao thức 1-Wire
```

> **Lưu ý:** Các hàm này sử dụng `TIM1->CNT` để đếm thời gian chính xác từng µs. Bắt buộc gọi `HAL_TIM_Base_Start(&htim1)` trước khi dùng.

#### 1.4. Flash lên board

```
Run → Debug Configuration → ST-Link (DAP SWD) → Debug
hoặc: Run → Run (Ctrl + F11)
```

---

### Bước 2 — Flash firmware ESP32

#### 2.1. Cài đặt thư viện trong Arduino IDE

```
Tools → Manage Libraries → Tìm và cài:
  ✅ (không cần ThingsBoard — version ESP32 này dùng HTTPClient)
  ✅ WiFi         (built-in ESP32)
  ✅ HTTPClient   (built-in ESP32)
```

#### 2.2. Cấu hình `esp32_gateway.ino`

```cpp
// ===== ĐIỀN THÔNG TIN CỦA BẠN =====

#define SSID       "Tên_WiFi_2.4GHz"     // Chỉ dùng băng tần 2.4GHz
#define PASSWORD   "Mật_khẩu_WiFi"

// Lấy IP máy tính: mở CMD → gõ "ipconfig" → tìm "IPv4 Address"
#define SERVER_URL "http://192.168.x.x:8080/api/telemetry"

// Serial2: GPIO16 = RX2 (nhận từ PA2 STM32), GPIO17 = TX2 (gửi về PA3 STM32)
// Baudrate PHẢI khớp với STM32: 9600 baud
Serial2.begin(9600, SERIAL_8N1, 16, 17);
```

> ⚠️ File gốc `esp32_gateway.ino` đang set `Serial2.begin(115200,...)` nhưng STM32 đang cấu hình **9600 baud** trong `MX_USART2_UART_Init()`. Cần đồng bộ 2 bên về **cùng một tốc độ**.

#### 2.3. Upload

```
Tools → Board: "ESP32 Dev Module"
Tools → Port: COMx (Windows) / /dev/ttyUSB0 (Linux)
Tools → Upload Speed: 921600
Sketch → Upload (Ctrl + U)
```

#### 2.4. Kiểm tra trên Serial Monitor (115200 baud)

```
Bat dau ket noi wifi den mang: Redmi Note 9S
....
Da gui ve Server C++ -> Phản hồi từ Server: 200
Thông số mạch nhận -> Temp: 29 C | Hum: 72 %
```

---

### Bước 3 — Build & chạy Web Server C++

```bash
# Di chuyển vào thư mục backend
cd webserver/backend/

# ── Linux / macOS ──
g++ -std=c++17 -O2 -o server main.cpp MonitorSystem.cpp \
    -lpthread -lboost_system -I.
./server

# ── Windows (MinGW64 / MSYS2) ──
g++ -std=c++17 -O2 -o server.exe main.cpp MonitorSystem.cpp \
    -lws2_32 -lwsock32 -lmswsock -I.
server.exe
```

Server khởi động tại: **`http://localhost:8080`** (đa luồng — multithreaded)

> File `history_data.csv` và `offline_queue.txt` sẽ tự sinh trong thư mục `webserver/data/` khi có dữ liệu đầu tiên.

---

### Bước 4 — Mở Web Dashboard

```
Trình duyệt → http://localhost:8080
```

---

## 📡 API Reference

| Method | Endpoint | Mô tả | Gọi từ |
|:---:|---|---|---|
| `GET` | `/` | Trả về `index.html` — Web Dashboard | Trình duyệt |
| `GET` | `/app.js` | Trả về JavaScript frontend | Trình duyệt |
| `GET` | `/api/status` | Trạng thái realtime: T, H, Dew Point, KNN AI, cảnh báo | Frontend (3s/lần) |
| `POST` | `/api/telemetry` | ESP32 push dữ liệu · nhận lệnh điều chỉnh tần số quét | ESP32 |
| `GET` | `/api/history?date=YYYY-MM-DD` | 50 bản ghi lịch sử của ngày từ CSV | Frontend |
| `POST` | `/api/settings` | Cập nhật chế độ vận hành + ngưỡng cảnh báo | Frontend |

**Payload ESP32 → `/api/telemetry`:**
```json
{ "temperature": 29, "humidity": 72 }
```

**Response Server → ESP32** (điều khiển ngược tần số quét):
```json
{ "status": "acknowledged", "next_interval_sec": 5 }
```

**Response `/api/status`:**
```json
{
  "temperature": 29.0,
  "humidity": 72.0,
  "dew_point": 23.1,
  "is_connected": true,
  "is_alert": false,
  "is_fire_risk": false,
  "is_condensation_risk": false,
  "is_cloud_online": true,
  "offline_count": 0,
  "threshold_temp": 35.0,
  "threshold_hum": 80.0,
  "medical_rec": "🟡 Cảnh báo oi bức (Humidex: 34). Gây cảm giác khó chịu nhẹ.",
  "mode_rec": "🤖 EDGE AI (KNN): [OI BỨC] -> Độ ẩm bão hòa. Bật chế độ DRY.",
  "sampling_interval": 5
}
```

---

## 🧮 Thuật toán

### 1. Logic điều khiển LED — STM32

```c
if (tCelsius >= 40.0f) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);    // 🔴 Đỏ SÁNG
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // Xanh Lá TẮT
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // Xanh Dương TẮT
} else if (tCelsius < 20.0f) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);  // Đỏ TẮT
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // Xanh Lá TẮT
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);    // 🔵 Xanh Dương SÁNG
} else { /* 20 ≤ T < 40 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);  // Đỏ TẮT
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // 🟢 Xanh Lá SÁNG
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // Xanh Dương TẮT
}
HAL_Delay(2000);  // Bắt buộc: ổn định phần cứng DHT11 (max 1Hz)
```

### 2. Đọc DHT11 — Giao thức 5-byte + Checksum

```c
// Đọc 5 byte tuần tự qua Single-Bus
RHI = DHT11_Read();  // Byte 1: Phần nguyên độ ẩm
RHD = DHT11_Read();  // Byte 2: Phần thập phân độ ẩm
TCI = DHT11_Read();  // Byte 3: Phần nguyên nhiệt độ
TCD = DHT11_Read();  // Byte 4: Phần thập phân nhiệt độ
SUM = DHT11_Read();  // Byte 5: Checksum

// Xác thực toàn vẹn dữ liệu
if (RHI + RHD + TCI + TCD == SUM) {
    tCelsius = (float)TCI + (float)TCD / 10.0f;
    RH       = (float)RHI + (float)RHD / 10.0f;
    // → Độ phân giải: 0.1°C và 0.1%RH
}
```

### 3. Điểm sương — Magnus Formula (chuẩn JEDEC)

```
α(T, RH) = (17.27 × T) / (237.7 + T) + ln(RH / 100)
Td        = (237.7 × α) / (17.27 − α)

Cảnh báo đọng sương JEDEC: (T − Td) ≤ 2.5°C
→ Nguy cơ hơi nước ngưng tụ lên bề mặt linh kiện điện tử
```

### 4. Chỉ số Humidex — Cảm nhận nhiệt thực tế

```
e       = 6.11 × exp(17.27 × Td / (237.7 + Td))
Humidex = T + 0.5555 × (e − 10)
```

| Humidex | Mức | Khuyến nghị y tế |
|:---:|---|---|
| < 30 | 🟢 An toàn | Môi trường lý tưởng |
| 30 – 39 | 🟡 Oi bức | Gây khó chịu nhẹ, uống nước thường xuyên |
| 40 – 45 | 🟠 Nguy hiểm | Mất nước nhanh, tránh vận động mạnh |
| ≥ 46 | 🔴 Cực kỳ nguy hiểm | Nguy cơ đột quỵ nhiệt |

### 5. KNN Classifier (K=3) — Edge AI phân loại môi trường

Tập huấn luyện **20 điểm** nhúng trực tiếp trong C++:

| Label | Phân loại | Vùng T (°C) | Vùng RH (%) | Khuyến nghị |
|:---:|---|:---:|:---:|---|
| 0 | LÝ TƯỞNG | 24 – 26 | 50 – 60 | Máy làm mát ECO mode |
| 1 | KHÔ NÓNG | 36 – 42 | 30 – 40 | Bật phun sương |
| 2 | OI BỨC | 32 – 36 | 72 – 85 | Bật chế độ DRY |
| 3 | LẠNH ẨM | 17 – 21 | 82 – 92 | Bật máy hút ẩm |

```cpp
// Khoảng cách Euclidean 2 chiều (T, RH)
double dist = sqrt(pow(T - Ti, 2) + pow(H - Hi, 2));
// Sắp xếp → lấy K=3 gần nhất → bỏ phiếu đa số → nhãn
```

### 6. Adaptive Sampling — Tần số quét thích ứng động

```
V_temp = |ΔT| / Δt   (°C/giây)
V_hum  = |ΔH| / Δt   (%/giây)

V_temp > 0.10 || V_hum > 0.60  →  interval = 2s   ⚡ TOÀN TỐC
V_temp > 0.02 || V_hum > 0.15  →  interval = 5s   📡 TIÊU CHUẨN
else                            →  interval = 15s  🌿 ECO (tiết kiệm pin)
```

Server trả `next_interval_sec` về cho ESP32 sau mỗi POST → ESP32 điều chỉnh `delay()` tương ứng.

### 7. Fire Detection — Rolling Window 15 giây

```cpp
// Lưu (timestamp, temp) vào cửa sổ trượt 15 giây
temp_rolling_window.push_back({now, current_temp});
// Loại bỏ điểm cũ hơn 15 giây
while (now - window.front().timestamp > 15) pop_front();
// Cảnh báo cháy sớm khi nhiệt tăng ≥ 1.5°C trong cửa sổ
if (current_temp - window.front().temperature >= 1.5)
    is_fire_risk_alert = true;
```

### 8. Offline Buffer — Không mất dữ liệu khi mất mạng

```
ESP32 POST → Server nhận → curl → ThingsBoard
                                      │
                            Thành công? ─YES─► Flush offline_queue.txt
                                      │
                                      NO──► Ghi vào offline_queue.txt
                                            Dashboard: "MẤT MẠNG (N gói tại biên)"
                                            Dữ liệu vẫn thu vào history_data.csv
                                            → Khi có mạng: auto flush toàn bộ
```

---

## 📊 Các chế độ vận hành

| Mode | Chế độ | Ngưỡng T | Ngưỡng RH | Cảnh báo khi vượt |
|:---:|---|:---:|:---:|---|
| 0 | Kho bảo quản thuốc | **30°C** | **70%** | Vaccine hỏng, dược phẩm biến chất |
| 1 | Kho thiết bị điện tử | **35°C** | **60%** | Oxy hóa chân IC, hư PCB |
| 2 | Kho nông sản | **25°C** | **85%** | Nấm mốc, lên men |
| 3 | Nông nghiệp / Nhà màng | **38°C** | **88%** | Sốc nhiệt rễ cây |

---

## ✅ Kết quả thực nghiệm

**Serial Monitor Arduino IDE (115200 baud):**
```
Da gui ve Server C++ -> Phản hồi từ Server: 200
Thông số mạch nhận -> Temp: 29 C | Hum: 72 %
Da gui ve Server C++ -> Phản hồi từ Server: 200
Thông số mạch nhận -> Temp: 29 C | Hum: 73 %
```

**ThingsBoard Cloud:** Thiết bị `ESP32` chuyển ❌ Inactive → ✅ Active. Key `Temp` và `Hum` cập nhật liên tục mỗi 3 giây. Không mất ký tự, không sai lệch chuyển đổi chuỗi.

---

## 🔮 Khả năng mở rộng

- **Relay control:** Tự động bật quạt / điều hòa khi T > 40°C qua GPIO STM32
- **Cảm biến thêm:** BMP280 (áp suất), MQ-135 (CO₂/chất lượng không khí)
- **OTA Update:** Nạp firmware ESP32 qua Wi-Fi không cần dây
- **Multi-node:** Nhiều cặp STM32+ESP32 gửi về một Crow Server trung tâm
- **ThingsBoard MQTT:** Nâng cấp ESP32 Gateway dùng thư viện `ThingsBoard.h` thay HTTPClient

---

## 🤝 Đóng góp

1. Fork repository
2. Tạo branch: `git checkout -b feature/ten-tinh-nang`
3. Commit: `git commit -m 'feat: mô tả thay đổi'`
4. Push & mở Pull Request

---

## 📄 License

Người dùng tự do sử dụng. Xem file [LICENSE](LICENSE).

---

<div align="center">

**Đại học Quốc gia TP.HCM · Trường ĐHKHTN · Khoa Vật lý · 2026**

*Developed entirely with ❤️ using Visual Studio Code*

⭐ **Nếu dự án hữu ích, hãy để lại một Star trên GitHub!** ⭐

</div>