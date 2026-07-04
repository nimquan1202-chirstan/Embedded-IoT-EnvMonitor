let chart;
const timeLabels = [];
const tempHistory = [];
const humHistory = [];

// Biến theo dõi Min/Max trong ngày (reset theo ngày)
let dailyStats = {
    date: '',          // ngày hiện tại dạng YYYY-MM-DD, dùng để reset khi sang ngày mới
    maxTemp: null, maxTempTime: '',
    minTemp: null, minTempTime: '',
    maxHum:  null, maxHumTime:  '',
    minHum:  null, minHumTime:  '',
    maxDew:  null, maxDewTime:  '',
    minDew:  null, minDewTime:  ''
};

// 1. Khởi tạo biểu đồ Chart.js độc lập 2 trục Y
function initChart() {
    const ctx = document.getElementById('telemetryChart').getContext('2d');
    chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: timeLabels,
            datasets: [
                {
                    label: 'Nhiệt độ (°C)',
                    borderColor: '#ef4444',
                    backgroundColor: 'rgba(239, 68, 68, 0.08)',
                    data: tempHistory,
                    yAxisID: 'y',
                    tension: 0.3,
                    pointRadius: 2,
                    borderWidth: 1.5
                },
                {
                    label: 'Độ ẩm (%)',
                    borderColor: '#3b82f6',
                    backgroundColor: 'rgba(59, 130, 246, 0.08)',
                    data: humHistory,
                    yAxisID: 'y1',
                    tension: 0.3,
                    pointRadius: 2,
                    borderWidth: 1.5
                }
            ]
        },
        options: {
            responsive: true,
            interaction: { mode: 'index', intersect: false },
            plugins: {
                legend: { display: false },
                tooltip: {
                    backgroundColor: '#0f172a',
                    borderColor: '#334155',
                    borderWidth: 1,
                    titleColor: '#94a3b8',
                    bodyColor: '#e2e8f0'
                }
            },
            scales: {
                x: {
                    grid: { color: '#1e293b' },
                    ticks: { color: '#475569', font: { size: 10 } }
                },
                y: {
                    type: 'linear', display: true, position: 'left',
                    grid: { color: '#1e293b' },
                    ticks: { color: '#94a3b8', font: { size: 10 } },
                    title: { display: true, text: 'Nhiệt độ', color: '#ef4444', font: { size: 10 } },
                    min: 0, max: 60
                },
                y1: {
                    type: 'linear', display: true, position: 'right',
                    grid: { drawOnChartArea: false },
                    ticks: { color: '#94a3b8', font: { size: 10 } },
                    title: { display: true, text: 'Độ ẩm', color: '#3b82f6', font: { size: 10 } },
                    min: 30, max: 100
                }
            }
        }
    });
}

// 2. Hàm Fetch status chu kỳ đồng bộ dữ liệu thời gian thực
async function fetchStatus() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();

        const datePicker = document.getElementById('history-date-picker');
        const todayStr = new Date().toISOString().slice(0, 10);
        const isTodaySelected = datePicker ? (datePicker.value === todayStr) : true;

        // Đẩy dữ liệu vào mảng biểu đồ nếu thiết bị đang trực tuyến
        if (isTodaySelected && data.is_connected) {
            const now    = new Date();
            const nowLbl = now.toLocaleTimeString();

            // Ghi vào buffer toàn phiên (dùng cho switchChartRange)
            _origPush(nowLbl, data.temperature, data.humidity);

            // Chỉ đẩy vào biểu đồ nếu đang ở chế độ 1h (realtime rolling 20 điểm)
            if (currentChartRange === '1h') {
                timeLabels.push(nowLbl);
                tempHistory.push(data.temperature);
                humHistory.push(data.humidity);

                if (timeLabels.length > 20) {
                    timeLabels.shift();
                    tempHistory.shift();
                    humHistory.shift();
                }
                chart.update();
            }
        }

        // Đổ dữ liệu lên các Smart Card số lớn thời gian thực
        document.getElementById('card-temp').innerText = data.temperature == 0 ? "0" : data.temperature;
        document.getElementById('card-hum').innerText  = data.humidity    == 0 ? "0" : data.humidity;
        document.getElementById('card-dew').innerText  = data.dew_point   == 0 ? "0.0" : data.dew_point;

        // Cập nhật thống kê Min/Max trong ngày (reset tự động khi sang ngày mới)
        if (data.is_connected) {
            const nowDate   = new Date().toISOString().slice(0, 10);
            const nowTime   = new Date().toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });
            const t = data.temperature;
            const h = data.humidity;
            const d = data.dew_point;

            // Reset nếu sang ngày mới
            if (dailyStats.date !== nowDate) {
                dailyStats = { date: nowDate,
                    maxTemp: t, maxTempTime: nowTime,
                    minTemp: t, minTempTime: nowTime,
                    maxHum:  h, maxHumTime:  nowTime,
                    minHum:  h, minHumTime:  nowTime,
                    maxDew:  d, maxDewTime:  nowTime,
                    minDew:  d, minDewTime:  nowTime
                };
            } else {
                if (t > dailyStats.maxTemp) { dailyStats.maxTemp = t; dailyStats.maxTempTime = nowTime; }
                if (t < dailyStats.minTemp) { dailyStats.minTemp = t; dailyStats.minTempTime = nowTime; }
                if (h > dailyStats.maxHum)  { dailyStats.maxHum  = h; dailyStats.maxHumTime  = nowTime; }
                if (h < dailyStats.minHum)  { dailyStats.minHum  = h; dailyStats.minHumTime  = nowTime; }
                if (d > dailyStats.maxDew)  { dailyStats.maxDew  = d; dailyStats.maxDewTime  = nowTime; }
                if (d < dailyStats.minDew)  { dailyStats.minDew  = d; dailyStats.minDewTime  = nowTime; }
            }

            // Đổ dữ liệu Min/Max lên DOM
            document.getElementById('stat-max-temp').innerText = dailyStats.maxTemp;
            document.getElementById('stat-min-temp').innerText = dailyStats.minTemp;
            document.getElementById('stat-max-hum').innerText  = dailyStats.maxHum;
            document.getElementById('stat-min-hum').innerText  = dailyStats.minHum;
            document.getElementById('stat-max-dew').innerText  = dailyStats.maxDew;
            document.getElementById('stat-min-dew').innerText  = dailyStats.minDew;

            // Cập nhật thanh progress bar tỉ lệ hiện tại giữa min và max
            const calcPct = (cur, mn, mx) => (mx === mn) ? 50 : Math.round(((cur - mn) / (mx - mn)) * 100);
            document.getElementById('bar-temp').style.width = calcPct(t, dailyStats.minTemp, dailyStats.maxTemp) + '%';
            document.getElementById('bar-hum').style.width  = calcPct(h, dailyStats.minHum,  dailyStats.maxHum)  + '%';
            document.getElementById('bar-dew').style.width  = calcPct(d, dailyStats.minDew,  dailyStats.maxDew)  + '%';
        }

        // Lấy các thành phần DOM điều khiển chỉ thị trạng thái mạng
        const banner = document.getElementById('status-banner');
        const dot    = document.getElementById('conn-dot');
        const badge  = document.getElementById('conn-badge');

        const current_time   = new Date();
        const formatted_time = current_time.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        const formatted_date = current_time.toLocaleDateString('vi-VN',  { day: '2-digit', month: '2-digit', year: 'numeric' });
        const timestamp_str  = `<span class="ts">🕒 ${formatted_time} — ${formatted_date}</span>`;

        // Chuỗi thông tin Cloud ThingsBoard chạy ngầm
        let cloudInfo = ` | ☁️ ThingsBoard Cloud: ${data.is_cloud_online ? "Đã đồng bộ" : `MẤT MẠNG (Đã nén sao lưu ${data.offline_count} gói tin tại biên)`}`;

        // Phân tầng trạng thái Banner thông minh dựa trên logic điều khiển Backend
        if (!data.is_connected) {
            dot.style.background   = '#ef4444';
            badge.className        = 'badge badge-err';
            badge.innerHTML        = '<i class="ti ti-wifi-off"></i> Mất tín hiệu';
            banner.style.cssText   = 'background:#1c0a0a;color:#f87171;border-bottom:1px solid #450a0a;padding:10px 28px;display:flex;align-items:center;gap:8px;font-size:12px;font-weight:500';
            banner.innerHTML       = `<i class="ti ti-wifi-off" style="font-size:14px"></i> MẤT TÍN HIỆU THIẾT BỊ: Không nhận được dữ liệu từ Gateway ESP32! (Biểu đồ tạm dừng) ${timestamp_str}`;
        } else if (data.is_fire_risk) {
            dot.style.background   = '#f97316';
            badge.className        = 'badge badge-warn';
            badge.innerHTML        = '<i class="ti ti-flame"></i> Nguy cơ cháy';
            banner.style.cssText   = 'background:#431407;color:#fb923c;border-bottom:2px dashed #7c2d12;padding:10px 28px;display:flex;align-items:center;gap:8px;font-size:12px;font-weight:500';
            banner.innerHTML       = `<i class="ti ti-alert-triangle" style="font-size:14px"></i> 🚨 CẢNH BÁO NGUY CƠ CHÁY SỚM: Nhiệt độ trong kho đang tăng vọt đột ngột bất thường! ${cloudInfo} ${timestamp_str}`;
        } else if (data.is_condensation_risk) {
            dot.style.background   = '#a78bfa';
            badge.className        = 'badge badge-info';
            badge.innerHTML        = '<i class="ti ti-droplets"></i> Đọng sương';
            banner.style.cssText   = 'background:#2e1065;color:#c4b5fd;border-bottom:2px solid #6d28d9;padding:10px 28px;display:flex;align-items:center;gap:8px;font-size:12px;font-weight:500';
            banner.innerHTML       = `<i class="ti ti-droplets" style="font-size:14px"></i> ⚠️ CẢNH BÁO TIÊU CHUẨN JEDEC: Nguy cơ đọng sương hủy hoại linh kiện! ${cloudInfo} ${timestamp_str}`;
        } else if (data.is_alert) {
            dot.style.background   = '#f97316';
            badge.className        = 'badge badge-warn';
            badge.innerHTML        = '<i class="ti ti-alert-triangle"></i> Vượt ngưỡng';
            banner.style.cssText   = 'background:#431407;color:#fb923c;border-bottom:1px solid #7c2d12;padding:10px 28px;display:flex;align-items:center;gap:8px;font-size:12px;font-weight:500';
            banner.innerHTML       = `<i class="ti ti-alert-triangle" style="font-size:14px"></i> CẢNH BÁO TIÊU CHUẨN TĨNH: Vượt ngưỡng an toàn cài đặt! (Nhiệt độ: ${data.temperature}°C, Độ ẩm: ${data.humidity}%) ${cloudInfo} ${timestamp_str}`;
        } else {
            dot.style.background   = '#22c55e';
            badge.className        = 'badge badge-ok';
            badge.innerHTML        = '<i class="ti ti-wifi"></i> Trực tuyến';
            banner.style.cssText   = 'background:#052e16;color:#4ade80;border-bottom:1px solid #166534;padding:10px 28px;display:flex;align-items:center;gap:8px;font-size:12px;font-weight:500';
            banner.innerHTML       = `<i class="ti ti-circle-check" style="font-size:14px"></i> Hệ thống ổn định (Nhiệt độ: ${data.temperature}°C, Độ ẩm: ${data.humidity}%) ${cloudInfo} ${timestamp_str}`;
        }

        // Đổ văn bản phân tích sức khỏe và kết quả thuật toán AI KNN biên xuống vùng Panel Khuyến nghị
        document.getElementById('med-rec').innerText  = data.medical_rec;
        document.getElementById('mode-rec').innerText = data.mode_rec;

        // Cập nhật giá trị hiển thị của thanh cấu hình nếu người dùng không thao tác nhập liệu
        if (document.activeElement.tagName !== "INPUT") {
            document.getElementById('max-temp').value = data.threshold_temp;
            document.getElementById('max-hum').value  = data.threshold_hum;
        }
    } catch (error) {
        console.error("Lỗi khi fetch dữ liệu trạng thái:", error);
    }
}

// 3. Hàm kích hoạt đổi chế độ lưu cài đặt xuống C++
async function updateSettings() {
    const mode = parseInt(document.getElementById('mode-select').value);
    const payload = { mode: mode };

    await fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
    });
    alert("Hệ thống đã tự động cấu hình ngưỡng an toàn theo quy chuẩn quốc tế!");
    fetchStatus();
}

// 4. Chuyển đổi Menu điều khiển tab
function switchTab(tabName) {
    const dashboardView = document.getElementById('dashboard-view');
    const historyView   = document.getElementById('history-view');
    const btnDash       = document.getElementById('btn-menu-dash');
    const btnHist       = document.getElementById('btn-menu-hist');

    if (tabName === 'dashboard') {
        dashboardView.style.display = 'block';
        historyView.style.display   = 'none';
        btnDash.className = 'menu-btn active';
        btnHist.className = 'menu-btn';
    } else if (tabName === 'history') {
        dashboardView.style.display = 'none';
        historyView.style.display   = 'block';
        btnDash.className = 'menu-btn';
        btnHist.className = 'menu-btn active';
        renderHistoryTable();
    }
}

// 5. Kết xuất bảng nhật ký
function renderHistoryTable() {
    const tbody = document.getElementById('history-table-body');
    tbody.innerHTML = '';
    if (!timeLabels.length) {
        tbody.innerHTML = '<tr><td colspan="3" class="empty-state">Chưa có dữ liệu trong phiên này</td></tr>';
        return;
    }
    for (let i = timeLabels.length - 1; i >= 0; i--) {
        const row = `<tr>
            <td>${timeLabels[i]}</td>
            <td class="col-temp">${tempHistory[i]} °C</td>
            <td class="col-hum">${humHistory[i]} %</td>
        </tr>`;
        tbody.innerHTML += row;
    }
}

// 6. Hàm xuất báo cáo Excel trích xuất trực tiếp từ Cơ sở dữ liệu CSV của Backend
async function exportToExcel() {
    try {
        const datePicker   = document.getElementById('history-date-picker');
        const selectedDate = datePicker ? datePicker.value : new Date().toISOString().slice(0, 10);

        const response = await fetch(`/api/history?date=${selectedDate}`);
        const fullData = await response.json();

        if (!fullData || fullData.length === 0) {
            alert("Không tìm thấy dữ liệu lịch sử của ngày này trên hệ thống để xuất file!");
            return;
        }

        let csvContent = "\uFEFF" + "Thời gian,Nhiệt độ (°C),Độ ẩm (%)\n";
        fullData.forEach(item => {
            csvContent += `${item.time},${item.temperature},${item.humidity}\n`;
        });

        const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
        const url  = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.setAttribute("href", url);
        link.setAttribute("download", `Bao_cao_IoT_Toan_Bo_${selectedDate}.csv`);
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    } catch (error) {
        console.error("Lỗi khi xuất file Excel:", error);
        alert("Không thể trích xuất dữ liệu từ Server C++!");
    }
}

// 7. Tải dữ liệu lịch sử lưu trữ từ ổ cứng (ĐÃ SỬA LỖI CHAT.UPDATE)
async function loadSavedHistory(targetDate = '') {
    try {
        if (!targetDate) {
            targetDate = new Date().toISOString().slice(0, 10);
            const datePicker = document.getElementById('history-date-picker');
            if (datePicker) datePicker.value = targetDate;
        }
        const response = await fetch(`/api/history?date=${targetDate}`);
        const data = await response.json();
        
        timeLabels.length = 0; 
        tempHistory.length = 0; 
        humHistory.length = 0;
        
        data.forEach(item => {
            timeLabels.push(item.time);
            tempHistory.push(item.temperature);
            humHistory.push(item.humidity);
        });
        
        // Cập nhật biểu đồ và bảng sau khi nạp xong mảng dữ liệu lịch sử
        chart.update();
        renderHistoryTable();
    } catch (error) {
        console.error("Lỗi khi tải lịch sử:", error);
    }
}

function filterHistoryByDate() {
    const selectedDate = document.getElementById('history-date-picker').value;
    if (!selectedDate) { alert("Vui lòng chọn ngày!"); return; }
    loadSavedHistory(selectedDate);
}

// 8. Đồng bộ trạm khí tượng hàng không TP.HCM từ vệ tinh Internet
async function fetchHCMCWeather() {
    try {
        const response = await fetch('https://wttr.in/Ho_Chi_Minh_City?format=j1');
        const data = await response.json();
        const realTemp = data.current_condition[0].temp_C;
        const realHum  = data.current_condition[0].humidity;
        document.getElementById('weather-info').innerText = `TP.HCM: ${realTemp}°C  |  💧 ${realHum}%`;
    } catch (error) {
        console.error("Lỗi kết nối trạm khí tượng mạng:", error);
        document.getElementById('weather-info').innerText = "❌ Không kết nối được thời tiết.";
    }
}

// 9. Chuyển đổi khoảng thời gian biểu đồ (1h / 24h / 7d)
// Buffer toàn phiên — lưu mọi điểm nhận được kể từ khi mở trang
const fullSessionLabels = [];
const fullSessionTemp   = [];
const fullSessionHum    = [];
let   currentChartRange = '1h'; // mặc định hiển thị 1h gần nhất

function switchChartRange(range) {
    currentChartRange = range;

    // Cập nhật trạng thái nút active
    ['1h', '24h', '7d'].forEach(r => {
        const btn = document.getElementById('range-btn-' + r);
        if (btn) btn.className = 'range-btn' + (r === range ? ' active' : '');
    });

    // Tính mốc thời gian cắt
    const now = Date.now();
    const cutoffMs = { '1h': 60 * 60 * 1000, '24h': 24 * 60 * 60 * 1000, '7d': 7 * 24 * 60 * 60 * 1000 }[range];
    const subtitleMap = { '1h': '1 giờ gần nhất', '24h': '24 giờ gần nhất', '7d': '7 ngày gần nhất' };

    const subtitle = document.getElementById('chart-subtitle');
    if (subtitle) subtitle.innerText = 'Nhiệt độ & độ ẩm — ' + subtitleMap[range];

    // Lọc điểm trong khoảng range từ buffer toàn phiên
    const filtered = fullSessionLabels
        .map((lbl, i) => ({ lbl, t: fullSessionTemp[i], h: fullSessionHum[i], ts: fullSessionTimestamps[i] }))
        .filter(pt => (now - pt.ts) <= cutoffMs);

    // Nếu buffer phiên không đủ (vd mới mở trang mà chọn 7d) thì dùng /api/history
    if (range === '7d' || range === '24h') {
        _loadChartFromHistory(range);
        return;
    }

    // Với 1h: dùng buffer RAM
    timeLabels.length = 0; tempHistory.length = 0; humHistory.length = 0;
    filtered.forEach(pt => { timeLabels.push(pt.lbl); tempHistory.push(pt.t); humHistory.push(pt.h); });
    chart.update();
}

// Mảng timestamp song song với fullSession (ms epoch) — dùng để lọc theo range
const fullSessionTimestamps = [];

// Override hàm đẩy dữ liệu: mỗi điểm realtime cũng ghi vào fullSession buffer
const _origPush = function(lbl, t, h) {
    fullSessionLabels.push(lbl);
    fullSessionTemp.push(t);
    fullSessionHum.push(h);
    fullSessionTimestamps.push(Date.now());
};

// Hàm tải lịch sử nhiều ngày từ /api/history để vẽ biểu đồ 24h / 7d
async function _loadChartFromHistory(range) {
    try {
        const days     = range === '7d' ? 7 : 1;
        const combined = { labels: [], temp: [], hum: [] };

        for (let d = days - 1; d >= 0; d--) {
            const dt = new Date(Date.now() - d * 86400000);
            const dateStr = dt.toISOString().slice(0, 10);
            const res  = await fetch('/api/history?date=' + dateStr);
            const data = await res.json();
            data.forEach(item => {
                combined.labels.push((d > 0 ? dateStr.slice(5) + ' ' : '') + item.time);
                combined.temp.push(item.temperature);
                combined.hum.push(item.humidity);
            });
        }

        timeLabels.length = 0; tempHistory.length = 0; humHistory.length = 0;
        combined.labels.forEach((l, i) => {
            timeLabels.push(l); tempHistory.push(combined.temp[i]); humHistory.push(combined.hum[i]);
        });
        chart.update();
    } catch(e) { console.error('Lỗi tải lịch sử biểu đồ:', e); }
}

// KHỞI ĐỘNG CHU KỲ KIỂM SOÁT HỆ THỐNG AN TOÀN CHỐNG TRÙNG LẶP
try { initChart(); } catch (error) { console.log("Biểu đồ đã được dựng sẵn."); }

const todayStr = new Date().toISOString().slice(0, 10);
const dp = document.getElementById('history-date-picker');
if (dp) dp.value = todayStr;

loadSavedHistory();
fetchHCMCWeather();
fetchStatus();
setInterval(fetchStatus, 3000);