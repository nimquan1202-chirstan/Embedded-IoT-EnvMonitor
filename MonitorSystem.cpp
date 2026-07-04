#include "MonitorSystem.h"

MonitorSystem::MonitorSystem() 
    : temperature(25.0), humidity(60.0), current_mode(AppMode::AGRICULTURE_CROPS) {}

void MonitorSystem::updateSensorData(double temp, double hum) {
    temperature = temp;
    humidity = hum;
}

void MonitorSystem::setThresholds(double max_t, double max_h) {
    thresholds.max_temperature = max_t;
    thresholds.max_humidity = max_h;
}

void MonitorSystem::setMode(AppMode mode) {
    current_mode = mode;
}

bool MonitorSystem::isAlert() const {
    return (temperature > thresholds.max_temperature || humidity > thresholds.max_humidity);
}

std::string MonitorSystem::getMedicalRecommendation() const {
    if (!isAlert()) return "Chỉ số an toàn. Không có rủi ro y tế.";
    
    std::string rec = "CẢNH BÁO Y TẾ: ";
    if (temperature > thresholds.max_temperature) {
        rec += "Nguy cơ sốc nhiệt cao. Hãy bổ sung nước, tránh vận động mạnh. ";
    }
    if (humidity > thresholds.max_humidity) {
        rec += "Độ ẩm cao gây khó bài tiết mồ hôi, nguy cơ sốc nhiệt tăng cường. Khuyến nghị bật điều hòa hoặc máy hút ẩm.";
    }
    return rec;
}

std::string MonitorSystem::getModeRecommendation() const {
    if (!isAlert()) return "Hệ thống hoạt động ổn định.";

    switch (current_mode) {
        case AppMode::WAREHOUSE_MEDICINE:
            return "KHO THUỐC: Nhiệt độ/Độ ẩm vượt ngưỡng có thể làm hỏng vaccine và biến chất dược phẩm. Bật hệ thống làm lạnh khẩn cấp.";
        case AppMode::WAREHOUSE_ELECTRONICS:
            return "KHO ĐIỆN TỬ: Nguy cơ oxy hóa các chân IC rời và board mạch (PCB). Kích hoạt ngay hệ thống hút ẩm công suất cao.";
        case AppMode::WAREHOUSE_AGRICULTURE:
            return "KHO NÔNG SẢN: Nguy cơ nấm mốc và lên men nông sản. Bật hệ thống thông gió và giảm nhiệt độ kho.";
        case AppMode::AGRICULTURE_CROPS:
            return "NÔNG NGHIỆP: Môi trường quá khắc nghiệt. Hãy bật hệ thống phun sương làm mát. Đối với các loại cây cảnh hoặc thực vật thủy sinh (như sen), cần theo dõi nhiệt độ nước để tránh rễ bị sốc nhiệt.";
        default:
            return "Cần kiểm tra lại hệ thống.";
    }
}

double MonitorSystem::getTemperature() const { return temperature; }
double MonitorSystem::getHumidity() const { return humidity; }
Thresholds MonitorSystem::getThresholds() const { return thresholds; }
int MonitorSystem::getMode() const { return static_cast<int>(current_mode); }