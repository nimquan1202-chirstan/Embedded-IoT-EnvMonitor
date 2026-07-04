#ifndef MONITOR_SYSTEM_H
#define MONITOR_SYSTEM_H

#include <string>
#include <vector>

// Các chế độ hoạt động
enum class AppMode {
    WAREHOUSE_MEDICINE,
    WAREHOUSE_ELECTRONICS,
    WAREHOUSE_AGRICULTURE,
    AGRICULTURE_CROPS
};

// Lưu trữ ngưỡng cảnh báo
struct Thresholds {
    double max_temperature = 35.0; // Mặc định 35°C
    double max_humidity = 80.0;    // Mặc định 80%
};

class MonitorSystem {
private:
    double temperature;
    double humidity;
    Thresholds thresholds;
    AppMode current_mode;

public:
    MonitorSystem();
    
    // Cập nhật dữ liệu từ Thingsboard hoặc cảm biến
    void updateSensorData(double temp, double hum);
    
    // Cập nhật ngưỡng cài đặt từ người dùng
    void setThresholds(double max_t, double max_h);
    void setMode(AppMode mode);

    // Lấy trạng thái cảnh báo
    bool isAlert() const;
    
    // Khuyến nghị y tế
    std::string getMedicalRecommendation() const;
    
    // Khuyến nghị theo chế độ cụ thể
    std::string getModeRecommendation() const;

    // Getters
    double getTemperature() const;
    double getHumidity() const;
    Thresholds getThresholds() const;
    int getMode() const;
};

#endif // MONITOR_SYSTEM_H