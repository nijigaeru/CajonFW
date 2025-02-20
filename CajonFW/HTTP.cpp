#include <WiFi.h>
#include <WebServer.h>
#include "HTTP.h"
/******** macro *****/


/******** global variable  ***** */
const char* ssid = "tobukaeru_Cajon"; // アクセスポイントのSSID
const char* password = "tobukaeru_Cajon"; // アクセスポイントのパスワード

// キューの定義
QueueHandle_t g_pstHTTPQueue;
WebServer server(80);

/******** function declaration ***** */
void HTTPTask(void* pvParameters);   //HTTPタスク
bool bWifiConncet = false;

/************************** */
// HTTPタスク

void HTTTPTask(void* pvParameters){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });

  server.on("/prevTrack", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Previous Track");
    request->send(200, "text/plain", "Previous Track");
  });

  server.on("/startTrack", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Start Track");
    request->send(200, "text/plain", "Start Track");
  });

  server.on("/nextTrack", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Next Track");
    request->send(200, "text/plain", "Next Track");
  });

  server.on("/stopTrack", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Stop Track");
    request->send(200, "text/plain", "Stop Track");
  });

  server.on("/playTrack", HTTP_GET, [](AsyncWebServerRequest *request){
    String trackName;
    if (request->hasParam("name")) {
      trackName = request->getParam("name")->value();
      Serial.println("Play " + trackName);
    }
    request->send(200, "text/plain", "Play " + trackName);
  });

  server.on("/seek", HTTP_GET, [](AsyncWebServerRequest *request){
    String value;
    if (request->hasParam("value")) {
      value = request->getParam("value")->value();
      Serial.println("Seek to: " + value);
    }
    request->send(200, "text/plain", "Seek to: " + value);
  });

  server.begin();
  
  while(1)
  {
    if ((WiFi.status() == WL_CONNECTED) && ( !bWifiConncet ))
    {
        
    }
    else if ((WiFi.status() != WL_CONNECTED) && ( bWifiConncet ))
    {
        
    }
  }
}
