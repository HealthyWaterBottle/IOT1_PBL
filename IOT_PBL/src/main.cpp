#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// Pin definitions
#define RedLed 25
#define BlueLed 26
#define GreenLed 27
#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_DC 16
#define OLED_CS 5
#define OLED_RST 4
#define I2C_SDA 21
#define I2C_SCL 22

// website configurations
const char *ssid = "VAF PBL";
const char *password = "password";
WebServer server(80);

//OLED screen config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);

// BME280 I2C address
#define BME280_ADDRESS 0x76
Adafruit_BME280 bme;

// THRESHOLDS (Editable in Webpage)

float tempLow = 20;
float tempHigh = 25;
float humidLow = 20;
float humidHigh = 40;
float pressLow = 950;
float pressHigh = 1050;

// DATA LOGGING
struct LogEntry
{
  float temp;
  float humid;
  float press;
  unsigned long timestamp;
};

LogEntry logs[20];
int logIndex = 0;

void addLog(float t, float h, float p)
{
  logs[logIndex].temp = t;
  logs[logIndex].humid = h;
  logs[logIndex].press = p;
  logs[logIndex].timestamp = millis() / 1000; // seconds since boot

  logIndex++;
  if (logIndex >= 20)
    logIndex = 0;
}

//website formatting and coding
void handleroot()
{
  float temp = bme.readTemperature();
  float humid = bme.readHumidity();
  float press = bme.readPressure() / 100.0F;

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
 
  <head>
    <title>ESP32 ROOM SENSOR</title>
    <meta http-equiv="refresh" content="4">
    <style>
      table {
        margin-left: auto;
        margin-right: auto;
        width: 40%;
        border-collapse: collapse;

      }
      td {
        border: 1px solid black;
        padding: 10px;
      }
      h2 {
        font-family: Arial;
        text-align: center;
      }
        .center-container {
        text-align: center;
        }
    </style>
  </head>
  <body>
     
     
    <h2>Current Sensor Values</h2>
    <table>
      <tr><td>Temperature</td><td>)rawliteral";

  html += String(temp) + " &deg;C";

  html += R"rawliteral(</td></tr>
      <tr><td>Humidity</td><td>)rawliteral";

  html += String(humid) + " %";

  html += R"rawliteral(</td></tr>
      <tr><td>Pressure</td><td>)rawliteral";

  html += String(press) + " hPa";

  html += R"rawliteral(</td></tr>
    </table>

    <h2>Set Thresholds</h2>
    <form action="/setThresholds" method="GET">
<div class ="center-container">
Temperature Low: <input type="number" step="0.1" name="tLow" value=")rawliteral";
  html += String(tempLow);
  html += R"rawliteral("><br><br>

Temperature High: <input type="number" step="0.1" name="tHigh" value=")rawliteral";
  html += String(tempHigh);
  html += R"rawliteral("><br><br>

Humidity Low: <input type="number" step="0.1" name="hLow" value=")rawliteral";
  html += String(humidLow);
  html += R"rawliteral("><br><br>

Humidity High: <input type="number" step="0.1" name="hHigh" value=")rawliteral";
  html += String(humidHigh);
  html += R"rawliteral("><br><br>

Pressure Low: <input type="number" step="0.1" name="pLow" value=")rawliteral";
  html += String(pressLow);
  html += R"rawliteral("><br><br>

Pressure High: <input type="number" step="0.1" name="pHigh" value=")rawliteral";
  html += String(pressHigh);
  html += R"rawliteral("><br><br>

<input type="submit" value="Update Thresholds">
</div>
</form>

    <h2>Data Log (Last 20 Records)</h2>
    <table>
      <tr>
        <td>Time (s)</td><td>Temp</td><td>Humidity</td><td>Pressure</td>
      </tr>
)rawliteral";

  // Print logs
  for (int i = 0; i < 20; i++)
  {
    html += "<tr><td>";
    html += String(logs[i].timestamp);
    html += "</td><td>";
    html += String(logs[i].temp);
    html += "</td><td>";
    html += String(logs[i].humid);
    html += "</td><td>";
    html += String(logs[i].press);
    html += "</td></tr>";
  }

  html += "</table></body></html>";

  server.send(200, "text/html", html);
}

void setup()
{
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  Serial.println("AP IP:");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleroot);

  // handle threshold updates
  server.on("/setThresholds", []()
            {
    if (server.hasArg("tLow")) tempLow = server.arg("tLow").toFloat();
    if (server.hasArg("tHigh")) tempHigh = server.arg("tHigh").toFloat();
    if (server.hasArg("hLow")) humidLow = server.arg("hLow").toFloat();
    if (server.hasArg("hHigh")) humidHigh = server.arg("hHigh").toFloat();
    if (server.hasArg("pLow")) pressLow = server.arg("pLow").toFloat();
    if (server.hasArg("pHigh")) pressHigh = server.arg("pHigh").toFloat();

    server.sendHeader("Location", "/");
    server.send(302, "text/plain", ""); });

  server.begin();
  //pin configuration
  pinMode(GreenLed, OUTPUT);
  pinMode(RedLed, OUTPUT);

  Wire.begin(I2C_SDA, I2C_SCL);

  // troubleshooting help
  if (!bme.begin(BME280_ADDRESS))
  {
    Serial.println("Could not find BME280!");
    while (1)
      ;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println("OLED fail");
    while (1)
      ;
  }
}

void loop()
{
  //Internal Variables
  float temp = bme.readTemperature();
  float humid = bme.readHumidity();
  float press = bme.readPressure() / 100.0F;

  // Log data
  addLog(temp, humid, press);

  // OLED Display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print("T:");
  display.print(temp);
  display.println(" C");

  display.print("H:");
  display.print(humid);
  display.println(" %");

  display.print("P:");
  display.print(press);
  display.println("hPa");
  display.display();

  // Correct LED Threshold Logic
  bool tempOK = (temp >= tempLow && temp <= tempHigh);
  bool humidOK = (humid >= humidLow && humid <= humidHigh);
  bool pressOK = (press >= pressLow && press <= pressHigh);

  if (tempOK && humidOK && pressOK)
  {
    //If all conditions are acceptible it will turn green
    digitalWrite(GreenLed, HIGH);
    digitalWrite(RedLed, LOW);
  }
  else
  {
    //otherwise it warns the user by turning red
    digitalWrite(GreenLed, LOW);
    digitalWrite(RedLed, HIGH);
  }

  server.handleClient();
  delay(2000);
}