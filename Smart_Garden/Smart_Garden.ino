#define BLYNK_TEMPLATE_ID "TMPL6ZqfkW1nX"
#define BLYNK_TEMPLATE_NAME "TEST"
#define BLYNK_AUTH_TOKEN "SGgFMWHdBcLsw2tKSNxfCYUoe2_DAxrs"
#define BLYNK_PRINT Serial

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <time.h>
#include <ESP8266HTTPClient.h>

// WiFi credentials
char ssid[] = "Viettel";
char pass[] = "tu1den8ok";

// Pin definitions
#define RELAY D3
#define DHTPIN 2
#define DHTTYPE DHT21
#define ONE_WIRE_BUS D2

// Global variables
bool relayState = false;
bool controlMode = false;
int doAmDat;
float lastDoam = -1;
float lastNhietdo = -1;

// DHT and OneWire setup
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Blynk Timer
SimpleTimer timer;

void sendSensor()
{
    float doam = dht.readHumidity();
    float nhietdo = dht.readTemperature();
    if (isnan(doam) || isnan(nhietdo))
    {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    Serial.println(nhietdo);
    Serial.println(doam);
    Blynk.virtualWrite(V5, doam);
    Blynk.virtualWrite(V6, nhietdo);

    // If there is a change in temperature or humidity
    if (doam != lastDoam || nhietdo != lastNhietdo)
    {
        lastDoam = doam;
        lastNhietdo = nhietdo;
        sendDataToServer(doam, nhietdo);
    }
}

void sendTemps()
{
    sensors.requestTemperatures();
    doAmDat = analogRead(A0);
    Serial.print("Soil Moisture Value: ");
    Serial.println(doAmDat);
    Blynk.virtualWrite(V2, doAmDat);
}

void sendDataToServer(float doam, float nhietdo)
{
    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
    char timeBuffer[30];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", p_tm);

    String postData = "nhietdo=" + String(nhietdo) + "&doam=" + String(doam) + "&datetime=" + String(timeBuffer);

    HTTPClient http;
    WiFiClient client;

    http.begin(client, "http://192.168.1.2:81/btl/postdemo.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = http.POST(postData);
    String payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);

    http.end();
}

void setup()
{
    Serial.begin(9600);
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, HIGH); // Tắt relay ban đầu
    dht.begin();
    sensors.begin();
    timer.setInterval(1000L, sendSensor);
}

BLYNK_WRITE(V3)
{
    controlMode = param.asInt();
    if (controlMode == 0)
    {
        digitalWrite(RELAY, HIGH); // Tắt relay khi chuyển sang chế độ thủ công
    }
}

BLYNK_WRITE(V4) // Widget "Switch" trên Blynk để điều khiển relay thủ công
{
    relayState = param.asInt();
    if (controlMode == 0) // Chỉ thay đổi trạng thái relay khi ở chế độ điều khiển bằng tay
    {
        digitalWrite(RELAY, relayState ? LOW : HIGH);
    }
}

void loop()
{
    Blynk.run();
    timer.run();
    sendTemps();

    // Chế độ tự động
    if (controlMode == 1)
    {
        if (doAmDat > 700)
        {
            digitalWrite(RELAY, LOW); // Bật bơm
        }
        else if (doAmDat < 680)
        {
            digitalWrite(RELAY, HIGH); // Tắt bơm
        }
    }
    delay(1000);
}
