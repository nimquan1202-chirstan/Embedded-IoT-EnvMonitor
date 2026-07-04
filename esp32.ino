#include <WiFi.h>                  
#include <HTTPClient.h> // THAY ĐỔI: Dùng thư viện HTTPClient để bắn dữ liệu về Server C++

// Điền chính xác thông tin WiFi 2.4GHz
#define SSID "Redmi Note 9S" 
#define PASSWORD "quandeptrai"

// ====================================================================================
// THAY ĐỔI QUAN TRỌNG: Bạn hãy thay "192.168.1.X" bằng địa chỉ IP thật của máy tính bạn nhé!
// (Mẹo lấy IP: Mở CMD trên Windows gõ "ipconfig", tìm dòng IPv4 Address)
// ====================================================================================
// SỬA DÒNG NÀY THÀNH IP THẬT VỪA TÌM ĐƯỢC:
#define SERVER_URL "http://10.11.17.204:8080/api/telemetry"
String rxString = ""; 

void setup() {
  Serial.begin(115200);
  delay(1000); 
  
  Serial.println("\n--- Mach bat dau khoi dong ---");

  // Kết nối UART với STM32
  Serial2.begin(115200, SERIAL_8N1, 16, 17); 

  Serial.print("Bat dau ket noi wifi den mang: ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PASSWORD);
}

void loop() {
  // Nếu mất WiFi thì dừng lại đợi kết nối
  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    return;
  }

  // Đọc dữ liệu từ STM32 truyền sang qua UART
  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    if (inChar == '\n') { 
      int commaIndex = rxString.indexOf(',');
      if (commaIndex > 0) {
        String t_str = rxString.substring(0, commaIndex);
        String h_str = rxString.substring(commaIndex + 1);
        
        int temperature = t_str.toInt();
        int humidity = h_str.toInt();

        // --- LỚP KHIÊN CHỐNG SPAM 3 GIÂY ---
        static unsigned long lastSendTime = 0;
        if (millis() - lastSendTime > 3000) {
            
            // --- TIẾN HÀNH BẮN HTTP POST VỀ SERVER C++ ---
            if (WiFi.status() == WL_CONNECTED) {
              HTTPClient http;
              
              // Thiết lập đường dẫn đến Server C++ của bạn
              http.begin(SERVER_URL);
              http.addHeader("Content-Type", "application/json");
              
              // Tự động đóng gói dữ liệu thành chuỗi JSON đúng chuẩn: {"temperature": X, "humidity": Y}
              String jsonPayload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";
              
              // Thực hiện gửi lệnh POST
              int httpResponseCode = http.POST(jsonPayload);
              
              // In trạng thái phản hồi để debug trên Serial Monitor
              if (httpResponseCode > 0) {
                Serial.print("Da gui ve Server C++ -> Phản hồi từ Server: ");
                Serial.println(httpResponseCode); // Nếu ra số 200 là thành công rực rỡ
              } else {
                Serial.print("Loi gui dữ liệu: ");
                Serial.println(http.errorToString(httpResponseCode).c_str());
              }
              
              // Giải phóng bộ nhớ sau khi gửi xong
              http.end();
            }

            Serial.print("Thông số mạch nhận -> Temp: ");
            Serial.print(temperature);
            Serial.print(" C | Hum: ");
            Serial.print(humidity);
            Serial.println(" %");

            lastSendTime = millis(); // Cập nhật lại mốc thời gian
        }
      }
      rxString = ""; 
    } else {
      rxString += inChar; 
    }
  }
}