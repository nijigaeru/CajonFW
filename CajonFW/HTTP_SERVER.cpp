#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h> // DNSサーバー用ライブラリ
#include "HTTP_SERVER.h"
#include "READMID.h"

/******** global variable  ***** */
const char* ssid = "tobukaeru_Cajon"; // AP SSID
const char* password = "tobukaeru_Cajon"; // APパスワード
char Filename[32];
char g_cSerialBuffer[256];      // シリアル受信用バッファ
uint32_t g_ulSerialBufferCount; // シリアル受信バッファカウント

QueueHandle_t g_pstHTTPQueue;
WebServer server(80);
DNSServer dnsServer;

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);
const byte DNS_PORT = 53;

/******** function declaration ***** */
void HTTPTask(void* pvParameters);
uint32_t SerialCmdProc(TS_READMIDPlayNotsParam* pstParam, char* cBuffer, uint32_t ulSize);
void handleInitial();
void handleWindowsDetect();
void handlePlaylist();
void handlePrevTrack();
void handleStartTrack();
void handlePauseTrack();
void handleNextTrack();
void handleStopTrack();
void handlePlayTrack();
void handleSeek();

/************************** */
// HTTPタスク
void HTTPTask(void* pvParameters) {
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  dnsServer.start(DNS_PORT, "*", local_IP);

  server.on("/generate_204", handleInitial); 
  server.on("/hotspot-detect.html", handleInitial);
  server.on("/connecttest.txt", handleWindowsDetect);

  server.on("/", handleInitial);
  server.on("/playlist.html", handlePlaylist);
  server.on("/prevTrack", handlePrevTrack);
  server.on("/startTrack", handleStartTrack);
  server.on("/pauseTrack", handlePauseTrack);
  server.on("/nextTrack", handleNextTrack);
  server.on("/stopTrack", handleStopTrack);
  server.on("/playTrack", handlePlayTrack);
  server.on("/seek", handleSeek);

  server.begin();
  Serial.println("HTTP server started");

  while(1)
  {
    dnsServer.processNextRequest();
    server.handleClient();

    while (Serial.available()) {
      char bChar = Serial.read();

      if (bChar == 'D') {
        g_ulSerialBufferCount = 0;
        g_cSerialBuffer[g_ulSerialBufferCount++] = bChar;
      } 
      else if (bChar == '\r') {
        g_cSerialBuffer[g_ulSerialBufferCount++] = ' ';

        uint8_t ucSendReq[REQ_QUE_SIZE];
        TS_Req* pstSendReq = (TS_Req*)ucSendReq;
        pstSendReq->unReqType = READMID_PLAY_NOTS;
        TS_READMIDPlayNotsParam* pstNotsParam = (TS_READMIDPlayNotsParam*)pstSendReq->ucParam;

        uint32_t ulErr = SerialCmdProc(pstNotsParam, g_cSerialBuffer, g_ulSerialBufferCount);
        if (ulErr == 0) {
          xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
        }

        g_ulSerialBufferCount = 0;
      } 
      else {
        if (g_ulSerialBufferCount < sizeof(g_cSerialBuffer) - 1) {
          g_cSerialBuffer[g_ulSerialBufferCount++] = bChar;
        }
      }
    }

    vTaskDelay(1);
  }
}

void handleInitial() {
  server.send_P(200, "text/html", initialHtml);
}

void handleWindowsDetect() {
  server.send(200, "text/plain", "Microsoft Connect Test");
}

void handlePlaylist() {
  server.send_P(200, "text/html", playlistHtml);
}

void handlePrevTrack() {
  Serial.println("Previous track requested");
  server.send(200, "text/plain", "Previous track");
}

void handleStartTrack() {
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_START;
  TS_READMIDStartParam* pstREADParam = (TS_READMIDStartParam*)pstSendReq->ucParam;
  memcpy(pstREADParam->ucFileName, Filename, 32);
  xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
  Serial.println("Start track requested");
  server.send(200, "text/plain", "Start track");
}

void handleNextTrack() {
  Serial.println("Next track requested");
  server.send(200, "text/plain", "Next track");
}

void handlePauseTrack() {
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_PAUSE;
  xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
  Serial.println("Pause track requested");
  server.send(200, "text/plain", "Pause track");
}

void handleStopTrack() {
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_END;
  xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
  Serial.println("Stop track requested");
  server.send(200, "text/plain", "Stop track");
}

void handlePlayTrack() {
  String trackName = server.arg("name");
  trackName.toCharArray(Filename, sizeof(Filename));
  Serial.println("Play track requested: " + trackName);
  server.send(200, "text/plain", "Play track: " + trackName);
}

void handleSeek() {
  String value = server.arg("value");
  Serial.println("Seek requested: " + value);
  server.send(200, "text/plain", "Seek: " + value);
}

uint32_t SerialCmdProc(TS_READMIDPlayNotsParam* pstParam, char* cBuffer, uint32_t ulSize) {
  if (pstParam == NULL || cBuffer == NULL || ulSize == 0) return 1;
  if (cBuffer[0] != 'D') return 2;

  char* token = strtok(cBuffer, " ");
  if (!token) return 3;

  token = strtok(NULL, " ");
  if (!token) return 4;

  int noteCount = atoi(token);
  if (noteCount < 1 || noteCount > 8) return 5;

  pstParam->unNum = noteCount;

  for (int i = 0; i < noteCount; i++) {
    token = strtok(NULL, " ");
    if (!token) return 6;
    pstParam->stInfo[i].ucScale = (uint8_t)atoi(token);

    token = strtok(NULL, " ");
    if (!token) return 7;
    pstParam->stInfo[i].ucVelocity = (uint8_t)atoi(token);
  }

  return 0;
}
