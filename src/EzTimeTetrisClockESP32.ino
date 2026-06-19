/*******************************************************************
    Tetris clock with Web Configuration, OpenWeather & Quran API
    Hardware: ESP32 + 64x32 HUB75 Matrix
 *******************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <TetrisMatrixDraw.h>
#include <ezTime.h>

// --- Configuration Constants ---
const char* ssid = "wifi";
const char* password = "8111874110";

// --- Global Settings (Configurable via Web UI) ---
String timeZoneStr = "Asia/Kolkata";
bool twelveHourFormat = true;
int matrixBrightness = 90;
int scrollSpeedMs = 40;

String apiKey = "0196388992da70b59107c8ddaa24a7e4";
String cityId = "1263280"; 
String globalBottomText = "Hello World!";

// --- Quran API Variables ---
bool quranMode = false;
unsigned long lastQuranUpdate = 0;
bool forceQuranUpdate = false;

// --- Matrix & Display Variables ---
MatrixPanel_I2S_DMA *display = nullptr;
TetrisMatrixDraw *tetris = nullptr;
TetrisMatrixDraw *tetris2 = nullptr;
TetrisMatrixDraw *tetris3 = nullptr;

AsyncWebServer server(80);
TaskHandle_t animationTaskHandle = NULL;
Timezone myTZ;

// --- State Variables ---
float weatherTemp = 0;
int weatherHumidity = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long weatherInterval = 600000; // 10 mins
bool forceWeatherUpdate = false;

int topCycleMode = 0; 
unsigned long lastTopCycle = 0;
String globalTopText = "Loading...";

int scrollX = 64;
unsigned long lastScrollMove = 0;
bool showColon = true;
volatile bool finishedAnimating = false;
bool displayIntro = true;
bool forceRefresh = true;

String lastDisplayedTime = "";
String lastDisplayedAmPm = "";
unsigned long oneSecondLoopDue = 0;

// ==========================================
//             WEB SERVER HTML
// ==========================================
// Removed PROGMEM to avoid ESPAsyncWebServer truncation bugs.
const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Tetris Clock Setup</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: #121212; color: #e0e0e0; }
    .container { background: #1e1e1e; padding: 30px; border-radius: 12px; max-width: 500px; margin: 20px auto; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
    h2 { text-align: center; color: #4CAF50; margin-bottom: 25px; }
    fieldset { border: 1px solid #333; border-radius: 8px; margin-bottom: 20px; padding: 15px; }
    legend { color: #4CAF50; font-weight: bold; padding: 0 10px; }
    label { font-weight: bold; font-size: 0.9em; color: #aaa; display: block; margin-bottom: 5px; }
    input[type=text], input[type=number], select { width: 100%; padding: 12px; margin-bottom: 15px; border: 1px solid #333; border-radius: 6px; background: #2a2a2a; color: #fff; box-sizing: border-box; font-size: 1em; font-family: inherit; }
    input[type=range] { width: 100%; margin-bottom: 15px; }
    input[type=text]:focus, input[type=number]:focus, select:focus { outline: none; border-color: #4CAF50; }
    input[type=submit] { background: #4CAF50; color: white; padding: 14px 20px; border: none; border-radius: 6px; cursor: pointer; width: 100%; font-size: 1.1em; font-weight: bold; transition: background 0.3s; margin-top: 10px; }
    input[type=submit]:hover { background: #45a049; }
    .val-display { display: inline-block; float: right; color: #4CAF50; }
  </style>
</head>
<body>
  <div class="container">
    <h2>Clock Control Panel</h2>
    <form action="/update" method="POST">
      
      <fieldset>
        <legend>Display Settings</legend>
        <label>Brightness: <span class="val-display" id="briVal">%BRIGHTNESS%</span></label>
        <input type="range" name="brightness" min="0" max="255" value="%BRIGHTNESS%" oninput="document.getElementById('briVal').innerText = this.value">
        
        <label>Scroll Speed (ms/pixel): <span class="val-display" id="spdVal">%SCROLL_SPEED%</span></label>
        <input type="range" name="scrollSpeed" min="10" max="150" value="%SCROLL_SPEED%" oninput="document.getElementById('spdVal').innerText = this.value">
      </fieldset>

      <fieldset>
        <legend>Time & Location</legend>
        <label>Timezone (IANA format):</label>
        <input type="text" name="timezone" value="%TIMEZONE%">
        
        <label>Time Format:</label>
        <select name="timeFormat">
          <option value="12" %FMT_12%>12-Hour (AM/PM)</option>
          <option value="24" %FMT_24%>24-Hour</option>
        </select>
      </fieldset>

      <fieldset>
        <legend>Bottom Scroller Content</legend>
        <label>Scroll Text Mode:</label>
        <select name="scrollMode">
          <option value="custom" %CUSTOM_SEL%>Custom Text</option>
          <option value="quran" %QURAN_SEL%>Random Quran Verse (API)</option>
        </select>

        <label>Custom Message:</label>
        <input type="text" name="scrollMsg" value="%SCROLL_MSG%">
      </fieldset>

      <fieldset>
        <legend>Weather Config (OpenWeather)</legend>
        <label>API Key:</label>
        <input type="text" name="apiKey" value="%API_KEY%">

        <label>City ID:</label>
        <input type="text" name="cityId" value="%CITY_ID%">
      </fieldset>

      <input type="submit" value="Update & Apply Settings">
    </form>
  </div>
</body>
</html>
)rawliteral";

// Bulletproof string building bypasses the buggy template processor entirely
String buildHTML() {
  String html = String(index_html);
  html.replace("%SCROLL_MSG%", globalBottomText);
  html.replace("%API_KEY%", apiKey);
  html.replace("%CITY_ID%", cityId);
  html.replace("%CUSTOM_SEL%", quranMode ? "" : "selected");
  html.replace("%QURAN_SEL%", quranMode ? "selected" : "");
  html.replace("%TIMEZONE%", timeZoneStr);
  html.replace("%FMT_12%", twelveHourFormat ? "selected" : "");
  html.replace("%FMT_24%", twelveHourFormat ? "" : "selected");
  html.replace("%BRIGHTNESS%", String(matrixBrightness));
  html.replace("%SCROLL_SPEED%", String(scrollSpeedMs));
  return html;
}

// ==========================================
//               FUNCTIONS
// ==========================================

void setupWebServer() {
  if (!MDNS.begin("clock")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started. Go to http://clock.local");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // Send the fully constructed string instead of relying on the template callback
    request->send(200, "text/html", buildHTML());
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("scrollMode", true)) {
      String mode = request->getParam("scrollMode", true)->value();
      if (mode == "quran") {
        quranMode = true;
        forceQuranUpdate = true;
      } else {
        quranMode = false;
      }
    }
    if (request->hasParam("scrollMsg", true) && !quranMode) {
      globalBottomText = request->getParam("scrollMsg", true)->value();
      scrollX = 64; 
    }
    if (request->hasParam("apiKey", true)) apiKey = request->getParam("apiKey", true)->value();
    if (request->hasParam("cityId", true)) {
      cityId = request->getParam("cityId", true)->value();
      forceWeatherUpdate = true;
    }
    if (request->hasParam("timezone", true)) {
      timeZoneStr = request->getParam("timezone", true)->value();
      myTZ.setLocation(timeZoneStr);
      lastDisplayedTime = ""; 
      finishedAnimating = false;
    }
    if (request->hasParam("timeFormat", true)) {
      twelveHourFormat = (request->getParam("timeFormat", true)->value() == "12");
      lastDisplayedTime = ""; 
      finishedAnimating = false;
    }
    if (request->hasParam("brightness", true)) {
      matrixBrightness = request->getParam("brightness", true)->value().toInt();
      if(display) display->setBrightness8(matrixBrightness);
    }
    if (request->hasParam("scrollSpeed", true)) {
      scrollSpeedMs = request->getParam("scrollSpeed", true)->value().toInt();
    }
    request->send(200, "text/html", "<body style='background:#121212; color:white; text-align:center; font-family:sans-serif; margin-top:50px;'><h2>Settings Applied Successfully!</h2><br><a href='/' style='color:#4CAF50; font-size: 1.2em;'>Go Back</a></body>");
  });

  server.begin();
}

void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?id=" + cityId + "&appid=" + apiKey + "&units=metric";
    
    if (http.begin(client, url)) {
      if (http.GET() == HTTP_CODE_OK) {
        StaticJsonDocument<1024> doc;
        if (!deserializeJson(doc, http.getString())) {
          weatherTemp = doc["main"]["temp"];
          weatherHumidity = doc["main"]["humidity"];
        }
      }
      http.end();
    }
  }
}

void updateQuranVerse() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = "http://api.alquran.cloud/v1/ayah/random/en.sahih";
    
    if (http.begin(client, url)) {
      if (http.GET() == HTTP_CODE_OK) {
        DynamicJsonDocument doc(3072); 
        if (!deserializeJson(doc, http.getString())) {
          String text = doc["data"]["text"].as<String>();
          String surah = doc["data"]["surah"]["englishName"].as<String>();
          int number = doc["data"]["numberInSurah"].as<int>();
          
          text.replace("\n", " ");
          text.replace("\r", "");
          
          globalBottomText = surah + " " + String(number) + ": " + text;
          scrollX = 64; 
        }
      }
      http.end();
    }
  }
}

uint16_t getWheelColor(uint8_t wheelPos) {
  if (wheelPos < 85) return display->color565(wheelPos * 3, 255 - wheelPos * 3, 0);
  if (wheelPos < 170) { wheelPos -= 85; return display->color565(255 - wheelPos * 3, 0, wheelPos * 3); }
  wheelPos -= 170; return display->color565(0, wheelPos * 3, 255 - wheelPos * 3);
}

void drawTopInfo() {
  display->fillRect(0, 0, 64, 7, 0); 
  display->setTextSize(1);
  display->setTextWrap(false);
  
  display->setTextColor(getWheelColor((millis() / 20) % 256));

  int16_t x1, y1; uint16_t w, h;
  display->getTextBounds(globalTopText, 0, 0, &x1, &y1, &w, &h);
  int xPos = max(0, (64 - w) / 2);

  display->setCursor(xPos, 0); 
  display->print(globalTopText);
}

void drawBottomScroll() {
  display->fillRect(0, 25, 64, 7, 0); 
  display->setTextSize(1);
  display->setTextWrap(false);
  
  display->setTextColor(getWheelColor(((millis() / 20) + 128) % 256));
  display->setCursor(scrollX, 25); 
  display->print(globalBottomText);

  if (millis() - lastScrollMove > scrollSpeedMs) {
    scrollX--;
    if (scrollX < -(int)(globalBottomText.length() * 6)) {
      scrollX = 64;
      if (quranMode && (millis() - lastQuranUpdate > 10000)) forceQuranUpdate = true;
    }
    lastScrollMove = millis();
  }
}

void animationHandler() {
  if (!finishedAnimating) {
    display->fillRect(0, 7, 64, 20, 0); 

    if (displayIntro) {
      finishedAnimating = tetris->drawText(1, 21);
    } else {
      if (twelveHourFormat) {
        bool t1 = tetris->drawNumbers(-6, 26, showColon);
        bool t2 = tetris2->drawText(56, 25);
        bool t3 = t2 ? tetris3->drawText(56, 15) : false;
        finishedAnimating = t1 && t2 && t3;
      } else {
        finishedAnimating = tetris->drawNumbers(2, 26, showColon);
      }
    }
  }
  
  drawTopInfo();
  drawBottomScroll();
}

void animationTask(void *parameter) {
  for (;;) {
    animationHandler();
    vTaskDelay(pdMS_TO_TICKS(30)); 
  }
}

void setMatrixTime() {
  if (timeStatus() != timeSet) return; 

  String timeString = twelveHourFormat ? myTZ.dateTime("g:i") : myTZ.dateTime("H:i");
  if (twelveHourFormat && timeString.length() == 4) timeString = " " + timeString;

  if (twelveHourFormat) {
    String AmPmString = myTZ.dateTime("A");
    if (lastDisplayedAmPm != AmPmString) {
      lastDisplayedAmPm = AmPmString;
      tetris2->setText("M", forceRefresh);
      tetris3->setText(AmPmString.substring(0, 1), forceRefresh);
    }
  }

  if (lastDisplayedTime != timeString) {
    lastDisplayedTime = timeString;
    tetris->setTime(timeString, forceRefresh);
    finishedAnimating = false;
  }
}

void drawIntroWord(const char* word, int x, int y) {
  uint16_t colors[] = {tetris->tetrisCYAN, tetris->tetrisMAGENTA, tetris->tetrisYELLOW, tetris->tetrisGREEN, tetris->tetrisBLUE, tetris->tetrisRED, tetris->tetrisWHITE};
  for(int i=0; i<strlen(word); i++) {
    String c = String(word[i]);
    tetris->drawChar(c, x + (i*5), y, colors[i % 7]);
  }
}

// ==========================================
//               SETUP & LOOP
// ==========================================

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print(".");
  }
  
  // --- PRINT IP ADDRESS FOR EASY ACCESS ---
  Serial.println("\nWiFi Connected!");
  Serial.print("Open your browser and go to: http://");
  Serial.println(WiFi.localIP());
  Serial.println("Or try: http://clock.local");
  Serial.println("----------------------------------------");

  HUB75_I2S_CFG mxconfig(64, 32, 1);
  mxconfig.gpio.a = 23; mxconfig.gpio.b = 22; mxconfig.gpio.c = 5; 
  mxconfig.gpio.d = 17; 
  mxconfig.gpio.e = -1; // Disabled unused E pin to prevent floating noise
  mxconfig.gpio.lat = 4; 
  mxconfig.gpio.oe = 15; mxconfig.gpio.clk = 16;
  
  mxconfig.clkphase = false;          
  mxconfig.latch_blanking = 8;        
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M; 
  
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(matrixBrightness);

  tetris = new TetrisMatrixDraw(*display);
  tetris2 = new TetrisMatrixDraw(*display);
  tetris3 = new TetrisMatrixDraw(*display);

  display->fillScreen(0);
  drawIntroWord("Connecting", 5, 10);
  delay(1000);

  setupWebServer();

  setDebug(INFO);
  waitForSync(30);
  myTZ.setLocation(timeZoneStr);

  display->fillScreen(0);
  drawIntroWord("Ready", 20, 12);
  delay(2000);

  updateWeather();

  tetris->setText("PRO26", forceRefresh);
  
  xTaskCreatePinnedToCore(animationTask, "animTask", 8192, NULL, 2, &animationTaskHandle, 1);

  while (!finishedAnimating) { delay(10); }
  delay(2000);
  finishedAnimating = false;
  displayIntro = false;
  tetris->scale = 2;
}

void loop() {
  events(); 
  unsigned long now = millis();

  if (forceWeatherUpdate || now - lastWeatherUpdate > weatherInterval || (weatherTemp == 0 && now - lastWeatherUpdate > 30000)) {
    updateWeather();
    lastWeatherUpdate = now;
    forceWeatherUpdate = false;
  }

  if (quranMode && forceQuranUpdate) {
    updateQuranVerse();
    lastQuranUpdate = now;
    forceQuranUpdate = false;
  }

  if (now - lastTopCycle > 5000) {
    topCycleMode = (topCycleMode + 1) % 3;
    lastTopCycle = now;
  }

  if (topCycleMode == 0) {
    if (weatherTemp == 0 && weatherHumidity == 0) globalTopText = "Loading...";
    else { char buf[16]; sprintf(buf, "%.0fC %d%%", weatherTemp, weatherHumidity); globalTopText = String(buf); }
  } 
  else if (topCycleMode == 1) globalTopText = myTZ.dateTime("d M yy");
  else globalTopText = myTZ.dateTime("l");

  if (now > oneSecondLoopDue) {
    setMatrixTime();
    showColon = !showColon;
    if (finishedAnimating) {
      uint16_t colour = showColon ? tetris->tetrisWHITE : tetris->tetrisBLACK;
      int x = twelveHourFormat ? -6 : 2;
      int y = 26 - (TETRIS_Y_DROP_DEFAULT * tetris->scale);
      tetris->drawColon(x, y, colour);
    }
    oneSecondLoopDue = now + 1000;
  }
}