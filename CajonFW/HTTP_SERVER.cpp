#include <WiFi.h>
#include <WebServer.h>
#include "HTTP_SERVER.h"
#include "READMID.h"
/******** macro *****/


/******** global variable  ***** */
const char* ssid = "tobukaeru_Cajon"; // アクセスポイントのSSID
const char* password = "tobukaeru_Cajon"; // アクセスポイントのパスワード
char Filename[32];

// キューの定義
QueueHandle_t g_pstHTTPQueue;
WebServer server(80);

/******** function declaration ***** */
void HTTPTask(void* pvParameters);   //HTTPタスク
bool g_bWifiConncet = false;
void handleInitial();
void handlePlaylist();
void handlePrevTrack();
void handleStartTrack();
void handleNextTrack();
void handleStopTrack();
void handlePlayTrack();
void handleSeek();
void connectToWiFi();

/************************** */
// HTTPタスク
void HTTPTask(void* pvParameters){
  //connectToWiFi();
  // アクセスポイントとして動作するように設定
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
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
    #if 0
    if ((WiFi.status() != WL_CONNECTED)&&(g_bWifiConncet)) {
      Serial.println("WiFi disconnected. Reconnecting...");
      server.stop();
      // connectToWiFi();
      server.begin();
      Serial.println("HTTP server restarted");
      g_bWifiConncet = false;
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
      g_bWifiConncet = true;
    }
    #endif
    server.handleClient();
    delay(10);
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
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_START;
  TS_READMIDStartParam* pstREADParam = (TS_READMIDStartParam*)pstSendReq->ucParam;
  memcpy(pstREADParam->ucFileName,Filename, 32);
  xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
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
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_PAUSE;
  xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
  Serial.println("Stop track requested");
  server.send(200, "text/plain", "Stop track");
}

void handlePlayTrack() {
  String tracName = server.arg("name");
  tracName.toCharArray(Filename, sizeof(Filename));
  // コールバック処理
  Serial.println("Play track requested: " + tracName);
  server.send(200, "text/plain", "Play track: " + tracName);
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
