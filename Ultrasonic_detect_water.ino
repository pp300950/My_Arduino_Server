#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define ECHO_PIN D6
#define TRIG_PIN D5
#define LED_PIN  D4

const char* ssid = "Pone";
const char* password = "12345678";

ESP8266WebServer server(80);

const int SAMPLE_RATE = 10;
const int INTERVAL = 5000;

float previousDistance = 0;
float currentDistance = 0;
float futurePrediction1 = 0;
float futurePrediction2 = 0;
String trend = "ค่าคงที่";

float maxDistance = 20.0;
float minDistance = 1.5;
float bottleRadius = 4.0;

float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = (duration * 0.0343) / 2;
    return constrain(distance, minDistance, maxDistance);
}

float calculateVolume(float height) {
    return 3.1416 * bottleRadius * bottleRadius * height;
}

void updateCalculations() {
    float delta = currentDistance - previousDistance;
    
    // ป้องกันค่าทำนายไม่ให้เกินช่วง min-max
    futurePrediction1 = constrain(currentDistance + delta, minDistance, maxDistance);
    futurePrediction2 = constrain(futurePrediction1 + delta, minDistance, maxDistance);

    // ปรับเงื่อนไขการตรวจจับแนวโน้ม
    if (abs(delta) <= 0.8) {
        trend = "ค่าคงที่";
    } else {
        trend = (delta > 0) ? "เพิ่มขึ้น" : "ลดลง";
    }

    previousDistance = currentDistance;
}

void handleRoot() {
    String html = "<!DOCTYPE html>";
    html += "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial; text-align: center; background: #f4f4f4; }";
    html += "#dataTable { width: 90%; max-width: 400px; margin: auto; }";
    html += "</style>";
    
    html += "<script>";
    html += "function fetchData() {";
    html += "  fetch('/data')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('currentDistance').innerText = data.current + ' cm';";
    html += "      document.getElementById('trend').innerText = data.trend;";
    html += "      document.getElementById('future1').innerText = data.future1 + ' cm';";
    html += "      document.getElementById('future2').innerText = data.future2 + ' cm';";
    html += "      document.getElementById('volume').innerText = data.volume + ' cm³';";
    html += "    });";
    html += "}";
    html += "setInterval(fetchData, 7500);";
    html += "</script>";
    
    html += "</head><body onload='fetchData()'>";
    html += "<h2>ระดับน้ำ & ปริมาตร</h2>";
    html += "<table id='dataTable'>";
    html += "<tr><td>ระดับปัจจุบัน</td><td id='currentDistance'>...</td></tr>";
    html += "<tr><td>แนวโน้ม</td><td id='trend'>...</td></tr>";
    html += "<tr><td>ทำนายครั้งที่ 1</td><td id='future1'>...</td></tr>";
    html += "<tr><td>ทำนายครั้งที่ 2</td><td id='future2'>...</td></tr>";
    html += "<tr><td>ปริมาตรน้ำ</td><td id='volume'>...</td></tr>";
    html += "</table>";
    html += "</body></html>";
    
    server.send(200, "text/html; charset=UTF-8", html);
}

void handleData() {
    String json = "{";
    json += "\"current\":" + String(currentDistance) + ",";
    json += "\"trend\":\"" + trend + "\",";
    json += "\"future1\":" + String(futurePrediction1) + ",";
    json += "\"future2\":" + String(futurePrediction2) + ",";
    json += "\"volume\":" + String(calculateVolume(currentDistance));
    json += "}";
    server.send(200, "application/json", json);
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(50);

    float totalDistance = 0;
    for (int i = 0; i < SAMPLE_RATE; i++) {
        totalDistance += measureDistance();
        delay(100);
    }
    currentDistance = totalDistance / SAMPLE_RATE;
    digitalWrite(LED_PIN, LOW);
 
    Serial.print("Current Distance: ");
    Serial.print(currentDistance);
    Serial.println(" cm");

    updateCalculations();
    server.handleClient();
    delay(INTERVAL - 1000);
}
