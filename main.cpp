#include "crow_all.h" 
#include "MonitorSystem.h"
#include <fstream>
#include <sstream>
#include <cstdlib> 
#include <vector>    
#include <ctime>     
#include <algorithm> 
#include <cmath>     
#include <map>

struct TempReadingTime {
    std::time_t timestamp;
    double temperature;
};

struct KNNPoint {
    double temperature; 
    double humidity;    
    int label;          
};

const std::vector<KNNPoint> training_dataset = {
    {24.0, 50.0, 0}, {25.0, 55.0, 0}, {26.0, 52.0, 0}, {25.5, 60.0, 0}, {24.5, 58.0, 0},
    {36.0, 35.0, 1}, {38.0, 40.0, 1}, {40.0, 32.0, 1}, {42.0, 30.0, 1}, {37.5, 38.0, 1},
    {32.0, 80.0, 2}, {34.0, 75.0, 2}, {35.0, 78.0, 2}, {33.5, 85.0, 2}, {36.0, 72.0, 2},
    {18.0, 85.0, 3}, {19.0, 90.0, 3}, {20.0, 82.0, 3}, {21.0, 88.0, 3}, {17.5, 92.0, 3}
};

int predictEnvironmentKNN(double current_temp, double current_hum, const std::vector<KNNPoint>& dataset, int k) {
    std::vector<std::pair<double, int>> distances;
    for (const auto& point : dataset) {
        double dist = std::sqrt(std::pow(current_temp - point.temperature, 2) + std::pow(current_hum - point.humidity, 2));
        distances.push_back({dist, point.label});
    }
    std::sort(distances.begin(), distances.end(), [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
        return a.first < b.first;
    });
    std::map<int, int> vote_counter;
    for (int i = 0; i < k && i < (int)distances.size(); i++) { vote_counter[distances[i].second]++; }
    int best_label = 0; int max_votes = -1;
    for (const auto& vote : vote_counter) {
        if (vote.second > max_votes) { max_votes = vote.second; best_label = vote.first; }
    }
    return best_label;
}

// =========================================================================
// BIẾN TOÀN CỤC PHỤC VỤ THUẬT TOÁN TẦN SỐ LẤY MẪU THÍCH ỨNG ĐỘNG (DIRECTIONS 1)
// =========================================================================
double last_adaptive_temp = 0.0;
double last_adaptive_hum = 0.0;
std::time_t last_adaptive_time = 0;
int current_sampling_interval_sec = 3; // Tần số quét mặc định ban đầu là 3 giây
// =========================================================================

std::vector<TempReadingTime> temp_rolling_window;
bool is_fire_risk_alert = false; 
std::time_t last_telemetry_time = 0; 
bool is_thingsboard_online = true; 

int countOfflinePackets() {
    std::ifstream file("offline_queue.txt");
    if (!file.is_open()) return 0;
    std::string line; int count = 0;
    while (std::getline(file, line)) { if (!line.empty()) count++; }
    file.close();
    return count;
}

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return "Loi: Khong the mo file " + filepath;
    std::stringstream buffer; buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    crow::SimpleApp app;
    MonitorSystem monitor;
    bool has_device_connected = false; 

    CROW_ROUTE(app, "/")([]() {
        auto page = crow::response(readFile("../frontend/index.html"));
        page.set_header("Content-Type", "text/html");
        return page;
    });

    CROW_ROUTE(app, "/app.js")([]() {
        auto page = crow::response(readFile("../frontend/app.js"));
        page.set_header("Content-Type", "application/javascript");
        return page;
    });

    // API Real-time Status (Bổ sung trạng thái Tần số quét thích ứng)
    CROW_ROUTE(app, "/api/status").methods("GET"_method)([&monitor, &has_device_connected]() {
        crow::json::wvalue x;
        std::time_t now = std::time(nullptr);

        if (has_device_connected && (now - last_telemetry_time > 20)) {
            has_device_connected = false; 
        }

        double temp = has_device_connected ? monitor.getTemperature() : 0.0;
        double hum = has_device_connected ? monitor.getHumidity() : 0.0;

        double dew_point = 0.0;
        if (hum > 0) {
            double alpha = ((17.27 * temp) / (237.7 + temp)) + log(hum / 100.0);
            dew_point = (237.7 * alpha) / (17.27 - alpha);
            dew_point = std::round(dew_point * 10.0) / 10.0;
        }

        double humidex = temp;
        std::string medical_advice = "Hệ thống ngoại tuyến - Vui lòng kết nối thiết bị.";
        std::string ai_knn_advice = "Thiết bị ngắt kết nối.";

        if (has_device_connected && hum > 0) {
            double e = 6.11 * exp((17.27 * dew_point) / (237.7 + dew_point));
            humidex = temp + (0.5555 * (e - 10.0));
            int h_index = static_cast<int>(std::round(humidex));

            if (h_index < 30) medical_advice = "🟢 An toàn (Humidex: " + std::to_string(h_index) + "). Môi trường lý tưởng.";
            else if (h_index >= 30 && h_index < 40) medical_advice = "🟡 Cảnh báo oi bức (Humidex: " + std::to_string(h_index) + "). Gây cảm giác khó chịu nhẹ.";
            else if (h_index >= 40 && h_index < 46) medical_advice = "🟠 NGUY HIỂM SỨC KHỎE (Humidex: " + std::to_string(h_index) + "). Cơ thể mất nước nhanh.";
            else medical_advice = "🔴 CỰC KỲ NGUY HIỂM (Humidex: " + std::to_string(h_index) + "). Nguy cơ cao đột quỵ!";

            int ai_predicted_label = predictEnvironmentKNN(temp, hum, training_dataset, 3);
            if (ai_predicted_label == 0) ai_knn_advice = "🤖 EDGE AI (KNN): [LÝ TƯỞNG] -> Môi trường hoàn hảo. Máy làm mát chạy Eco.";
            else if (ai_predicted_label == 1) ai_knn_advice = "🤖 EDGE AI (KNN): [KHÔ NÓNG] -> Độ ẩm thấp kèm nhiệt cao. Khuyến nghị: Bật phun sương.";
            else if (ai_predicted_label == 2) ai_knn_advice = "🤖 EDGE AI (KNN): [OI BỨC] -> Độ ẩm bão hòa ngột ngạt. Khuyến nghị: Bật chế độ DRY.";
            else if (ai_predicted_label == 3) ai_knn_advice = "🤖 EDGE AI (KNN): [LẠNH ẨM] -> Nguy cơ nấm mốc rất cao. Khuyến nghị: Bật máy hút ẩm.";
        }

        bool is_condensation_risk = false;
        if (has_device_connected && temp > 0 && dew_point > 0) {
            if ((temp - dew_point) <= 2.5) is_condensation_risk = true;
        }

        x["temperature"] = temp;
        x["humidity"] = hum;
        x["dew_point"] = dew_point;
        x["is_connected"] = has_device_connected;
        x["is_fire_risk"] = has_device_connected ? is_fire_risk_alert : false; 
        x["is_condensation_risk"] = is_condensation_risk; 
        x["is_cloud_online"] = is_thingsboard_online;
        x["offline_count"] = countOfflinePackets();
        x["threshold_temp"] = monitor.getThresholds().max_temperature;
        x["threshold_hum"] = monitor.getThresholds().max_humidity;
        x["is_alert"] = has_device_connected ? monitor.isAlert() : false;
        x["medical_rec"] = medical_advice;
        x["mode_rec"] = ai_knn_advice;
        
        // Gửi tần số quét hiện tại lên frontend hiển thị
        x["sampling_interval"] = current_sampling_interval_sec; 
        return x;
    });

    // API Settings
    CROW_ROUTE(app, "/api/settings").methods("POST"_method)([&monitor](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "Invalid JSON");
        if (x.has("mode")) {
            int mode = x["mode"].i();
            double target_max_temp = 40.0; double target_max_hum = 90.0;
            if (mode == 0) { target_max_temp = 30.0; target_max_hum = 70.0; } 
            else if (mode == 1) { target_max_temp = 35.0; target_max_hum = 60.0; } 
            else if (mode == 2) { target_max_temp = 25.0; target_max_hum = 85.0; } 
            else if (mode == 3) { target_max_temp = 38.0; target_max_hum = 88.0; }
            monitor.setThresholds(target_max_temp, target_max_hum);
            monitor.setMode(static_cast<AppMode>(mode));
        }
        return crow::response(200, "Settings adapted successfully");
    });

    // API Telemetry: Nơi thực hiện phép tính Đạo hàm toán học và phản hồi điều khiển ngược ESP32
    CROW_ROUTE(app, "/api/telemetry").methods("POST"_method)([&monitor, &has_device_connected](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "Invalid JSON");

        crow::json::wvalue response_to_esp;
        response_to_esp["status"] = "acknowledged";

        if (x.has("temperature") && x.has("humidity")) {
            has_device_connected = true;
            last_telemetry_time = std::time(nullptr); 

            double current_temp = x["temperature"].d();
            double current_hum = x["humidity"].d();
            monitor.updateSensorData(current_temp, current_hum);

            std::time_t now = std::time(nullptr);

            // -----------------------------------------------------------------
            // THUẬT TOÁN ĐẠO HÀM BIẾN THIÊN THÍCH ỨNG TẦN SỐ QUÉT ĐỘNG
            // -----------------------------------------------------------------
            if (last_adaptive_time != 0) {
                double delta_t = std::difftime(now, last_adaptive_time);
                if (delta_t > 0) {
                    // Tính độ biến thiên tuyệt đối dT và dH
                    double dT = std::abs(current_temp - last_adaptive_temp);
                    double dH = std::abs(current_hum - last_adaptive_hum);
                    
                    // Tính Đạo hàm vận tốc: V_temp = dT/dt (°C/giây), V_hum = dH/dt (%/giây)
                    double v_temp = dT / delta_t;
                    double v_hum = dH / delta_t;

                    // Phân tầng động dựa trên tốc độ thay đổi môi trường
                    if (v_temp > 0.10 || v_hum > 0.6) {
                        current_sampling_interval_sec = 2; // CHẾ ĐỘ TOÀN TỐC (Môi trường biến thiên cực nhanh / Nguy cơ cháy)
                    } else if (v_temp > 0.02 || v_hum > 0.15) {
                        current_sampling_interval_sec = 5; // CHẾ ĐỘ TIÊU CHUẨN (Môi trường có dịch chuyển nhẹ)
                    } else {
                        current_sampling_interval_sec = 15; // CHẾ ĐỘ ECO (Môi trường đứng yên, kéo dài chu kỳ để ESP32 tiết kiệm PIN)
                    }
                }
            }
            last_adaptive_temp = current_temp;
            last_adaptive_hum = current_hum;
            last_adaptive_time = now;
            // -----------------------------------------------------------------

            temp_rolling_window.push_back({now, current_temp});
            while (!temp_rolling_window.empty() && (now - temp_rolling_window.front().timestamp > 15)) {
                temp_rolling_window.erase(temp_rolling_window.begin());
            }
            is_fire_risk_alert = false;
            if (temp_rolling_window.size() >= 2) {
                if (current_temp - temp_rolling_window.front().temperature >= 1.5) is_fire_risk_alert = true;
            }

            std::ofstream logFile("history_data.csv", std::ios::app);
            if (logFile.is_open()) {
                char dateStr[100]; std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", std::localtime(&now));
                char timeStr[100]; std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&now));
                logFile << dateStr << "," << timeStr << "," << current_temp << "," << current_hum << "\n";
                logFile.close();
            }

            std::string tb_token = "jG4GYTz2IKmIGXVgm60E"; 
            std::string tb_url = "http://demo.thingsboard.io/api/v1/" + tb_token + "/telemetry";
            std::string command = "curl --connect-timeout 3 -X POST " + tb_url + " --data-raw \"" + req.body + "\" -H \"Content-Type: application/json\"";
            
            int cloud_status = std::system(command.c_str());
            if (cloud_status != 0) {
                is_thingsboard_online = false;
                std::ofstream offlineFile("offline_queue.txt", std::ios::app);
                if (offlineFile.is_open()) { offlineFile << req.body << "\n"; offlineFile.close(); }
            } else {
                is_thingsboard_online = true;
                std::ifstream offlineFileRead("offline_queue.txt");
                std::vector<std::string> queued_payloads; std::string line_payload;
                if (offlineFileRead.is_open()) {
                    while (std::getline(offlineFileRead, line_payload)) { if (!line_payload.empty()) queued_payloads.push_back(line_payload); }
                    offlineFileRead.close();
                }
                std::ofstream offlineFileClear("offline_queue.txt", std::ios::trunc); offlineFileClear.close();
                for (const auto& old_payload : queued_payloads) {
                    std::string flush_cmd = "curl --connect-timeout 3 -X POST " + tb_url + " --data-raw \"" + old_payload + "\" -H \"Content-Type: application/json\"";
                    int flush_status = std::system(flush_cmd.c_str());
                    if (flush_status != 0) {
                        std::ofstream offlineFileReappend("offline_queue.txt", std::ios::app);
                        if (offlineFileReappend.is_open()) { offlineFileReappend << old_payload << "\n"; offlineFileReappend.close(); }
                        is_thingsboard_online = false;
                    }
                }
            }
        }
        
        // Đóng gói cấu hình động trả về cho phần cứng ESP32 nhận lệnh
        response_to_esp["next_interval_sec"] = current_sampling_interval_sec; 
        return crow::response(response_to_esp);
    });

    // API History
    // API History nâng cấp: Tự động bóc tách thời gian để lọc 1h, 1d, 7d
    CROW_ROUTE(app, "/api/history").methods("GET"_method)([](const crow::request& req) {
        crow::json::wvalue::list history_list;
        std::time_t now = std::time(nullptr);
        
        // Kiểm tra xem người dùng muốn lọc theo khoảng (range) hay theo ngày cố định (date)
        if (req.url_params.get("date")) {
            std::string target_date = req.url_params.get("date");
            std::ifstream logFile("history_data.csv"); std::string line; std::vector<std::string> filtered_lines;
            if (logFile.is_open()) {
                while (std::getline(logFile, line)) { if (!line.empty() && line.rfind(target_date, 0) == 0) filtered_lines.push_back(line); }
                logFile.close();
            }
            int start = std::max(0, (int)filtered_lines.size() - 50);
            for (size_t i = start; i < filtered_lines.size(); i++) {
                std::stringstream ss(filtered_lines[i]); std::string dateStr, timeStr, tempStr, humStr;
                if (std::getline(ss, dateStr, ',') && std::getline(ss, timeStr, ',') && std::getline(ss, tempStr, ',') && std::getline(ss, humStr, ',')) {
                    crow::json::wvalue item; item["time"] = timeStr; item["temperature"] = std::stod(tempStr); item["humidity"] = std::stod(humStr);
                    history_list.push_back(item);
                }
            }
            return crow::json::wvalue(history_list);
        }

        // Xử lý lọc động theo Khoảng thời gian: 1h, 1d, 7d
        std::string range = "1d"; 
        if (req.url_params.get("range")) range = req.url_params.get("range");

        double max_seconds = 86400; // Mặc định 1 ngày = 86400 giây
        if (range == "1h") max_seconds = 3600;
        else if (range == "1d") max_seconds = 86400;
        else if (range == "7d") max_seconds = 7 * 86400;

        std::ifstream logFile("history_data.csv");
        std::string line;
        if (logFile.is_open()) {
            while (std::getline(logFile, line)) {
                if (line.empty()) continue;
                std::stringstream ss(line); std::string dateStr, timeStr, tempStr, humStr;
                if (std::getline(ss, dateStr, ',') && std::getline(ss, timeStr, ',') && std::getline(ss, tempStr, ',') && std::getline(ss, humStr, ',')) {
                    
                    int y, m, d, hr, min, sec;
                    if (sscanf(dateStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3 && sscanf(timeStr.c_str(), "%d:%d:%d", &hr, &min, &sec) == 3) {
                        std::tm t = {};
                        t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d;
                        t.tm_hour = hr; t.tm_min = min; t.tm_sec = sec; t.tm_isdst = -1;
                        
                        std::time_t log_time = std::mktime(&t);
                        // Nếu khoảng cách thời gian nhỏ hơn mốc lựa chọn thì lấy dữ liệu
                        if (std::difftime(now, log_time) <= max_seconds) {
                            crow::json::wvalue item;
                            // Nếu xem khoảng 7 ngày thì hiện cả Ngày + Giờ, nếu ngắn thì chỉ hiện Giờ cho gọn
                            item["time"] = (range == "7d") ? (dateStr.substr(5) + " " + timeStr.substr(0, 5)) : timeStr.substr(0, 5);
                            item["temperature"] = std::stod(tempStr);
                            item["humidity"] = std::stod(humStr);
                            history_list.push_back(item);
                        }
                    }
                }
            }
            logFile.close();
        }

        // Khống chế tối đa 300 mẫu phân bổ đều để tránh làm lag biểu đồ frontend nếu file CSV quá lớn
        if (history_list.size() > 300) {
            crow::json::wvalue::list limited_list;
            size_t step = history_list.size() / 300;
            for (size_t i = 0; i < history_list.size(); i += step) {
                limited_list.push_back(history_list[i]);
                if (limited_list.size() >= 300) break;
            }
            return crow::json::wvalue(limited_list);
        }
        return crow::json::wvalue(history_list);
    });

    app.port(8080).multithreaded().run();
}