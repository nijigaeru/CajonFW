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
  connectToWiFi();

  server.on("/", handleInitial);
  server.on("/playlist.html", handlePlaylist);
  server.on("/prevTrack", handlePrevTrack);
  server.on("/startTrack", handleStartTrack);
  server.on("/nextTrack", handleNextTrack);
  server.on("/stopTrack", handleStopTrack);
  server.on("/playTrack", handlePlayTrack);
  server.on("/seek", handleSeek);

  server.begin();
  Serial.println("HTTP server started");
  
  while(1)
  {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Reconnecting...");
      server.stop();
      connectToWiFi();
      server.begin();
      Serial.println("HTTP server restarted");
    }
    server.handleClient();
  }
}

void handleInitial() {
  server.send_P(200, "text/html", initialHtml);
}

void handlePlaylist() {
  server.send_P(200, "text/html", playlistHtml);
}

void handlePrevTrack() {
  // コールバック処理
  Serial.println("Previous track requested");
  server.send(200, "text/plain", "Previous track");
}

void handleStartTrack() {
  // コールバック処理
  Serial.println("Start track requested");
  server.send(200, "text/plain", "Start track");
}

void handleNextTrack() {
  // コールバック処理
  Serial.println("Next track requested");
  server.send(200, "text/plain", "Next track");
}

void handleStopTrack() {
  // コールバック処理
  Serial.println("Stop track requested");
  server.send(200, "text/plain", "Stop track");
}

void handlePlayTrack() {
  String trackName = server.arg("name");
  // コールバック処理
  Serial.println("Play track requested: " + trackName);
  server.send(200, "text/plain", "Play track: " + trackName);
}

void handleSeek() {
  String value = server.arg("value");
  // コールバック処理
  Serial.println("Seek requested: " + value);
  server.send(200, "text/plain", "Seek: " + value);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

