#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h> // DNSサーバー用ライブラリ
#include <WiFiUdp.h>
#include "HTTP_SERVER.h"
#include "READMID.h"
/******** macro *****/


/******** global variable  ***** */
const char* ssid = "tobukaeru_Cajon"; // アクセスポイントのSSID
const char* password = "tobukaeru_Cajon"; // アクセスポイントのパスワード
char Filename[32];
char g_cUdpBuffer[256];          // UDP用バッファ
uint32_t g_ulUdpBufferCount;     // UDP用バッファカウント

// キューの定義
QueueHandle_t g_pstHTTPQueue;
WebServer server(80);
DNSServer dnsServer; // DNSサーバーインスタンス
WiFiUDP udp; // UDPインスタンス

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);
unsigned int localUdpPort = 23; // UDPポート番号
const byte DNS_PORT = 53; // DNSサーバーポート
char incomingPacket[256]; // 受信パケット用バッファ
char replyPacket[] = "Hi there! Got the message"; // 応答メッセージ

/******** function declaration ***** */
void HTTPTask(void* pvParameters);   //HTTPタスク
bool g_bWifiConncet = false;
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
void connectToWiFi();

uint32_t UdpCmdProc(TS_READMIDPlayNotsParam* pstParam, char* cBuffer, uint32_t ulSize);

/************************** */
// HTTPタスク
void HTTPTask(void* pvParameters){
  // アクセスポイントとして動作するように設定
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // DNSサーバーの設定: すべてのドメインをローカルIPにリダイレクト
  dnsServer.start(DNS_PORT, "*", local_IP);

  // キャプティブポータル検出用のリクエストに対応
  server.on("/generate_204", handleInitial); // Android用
  server.on("/hotspot-detect.html", handleInitial); // iOS用
  server.on("/connecttest.txt", handleWindowsDetect); // Windows用

  // HTTPサーバーの設定
  server.on("/", handleInitial);
  server.on("/playlist.html", handlePlaylist);
  server.on("/prevTrack", handlePrevTrack);
  server.on("/startTrack", handleStartTrack);
  server.on("/pauseTrack", handlePauseTrack);
  server.on("/nextTrack", handleNextTrack);
  server.on("/stopTrack", handleStopTrack);
  server.on("/playTrack", handlePlayTrack);
  server.on("/seek", handleSeek);

  // HTTPサーバーの開始
  server.begin();
  Serial.println("HTTP server started");
  
  // UDPの開始
  udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", IP.toString().c_str(), localUdpPort);

  while(1)
  {
    dnsServer.processNextRequest(); // DNSリクエストを処理
    server.handleClient();          // HTTPリクエストを処理
    
    int packetSize = udp.parsePacket();
    if (packetSize) {
      // 受信パケットの読み込み
      int len = udp.read(incomingPacket, 255);
      if (len > 0) {
        incomingPacket[len] = 0;
      }
      // Serial.printf("Received packet of size %d from %s:%d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
      // Serial.printf("Packet contents: %s\n", incomingPacket);
      
      // 受信データを処理し、先頭の'D'を見つけるまでデータを破棄
      for (int i = 0; i < len; i++) {
        char bChar = incomingPacket[i];
        if (bChar == 'D')
        {
          /* バッファのクリア */
          g_ulUdpBufferCount =0;
          // バッファに詰め込む
          g_cUdpBuffer[g_ulUdpBufferCount] = bChar;
          g_ulUdpBufferCount++;
        }
        else if (bChar == '\r')
        {
          // バッファにスペース詰め込む(処理の共通化のため)
          g_cUdpBuffer[g_ulUdpBufferCount] = ' ';
          g_ulUdpBufferCount++;
          // ノーツ実行要求を送信する。
          uint8_t ucSendReq[REQ_QUE_SIZE];
          TS_Req* pstSendReq = (TS_Req*)ucSendReq;
          pstSendReq->unReqType = READMID_PLAY_NOTS;
          TS_READMIDPlayNotsParam* pstNotsParam = (TS_READMIDPlayNotsParam*)pstSendReq->ucParam;
          uint32_t ulErr = UdpCmdProc(pstNotsParam, g_cUdpBuffer, g_ulUdpBufferCount);
          if (ulErr == 0)
          {
            xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
          }
        }
        else
        {
          // バッファに詰め込む
          g_cUdpBuffer[g_ulUdpBufferCount] = bChar;
          g_ulUdpBufferCount++;
        }
      }
    }
  }
}

void handleInitial() {
  server.send_P(200, "text/html", initialHtml);
}

// Windows用のキャプティブポータル検出リクエストのハンドラ
void handleWindowsDetect() {
  server.send(200, "text/plain", "Microsoft Connect Test");
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

void handlePauseTrack() {
  // コールバック処理
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_PAUSE;
  xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
  Serial.println("Pause track requested");
  server.send(200, "text/plain", "Stop track");
}

void handleStopTrack() {
  // コールバック処理
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  pstSendReq->unReqType = READMID_END;
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

uint32_t UdpCmdProc(TS_READMIDPlayNotsParam* pstParam, char* cBuffer, uint32_t ulSize)
{
  if (pstParam == NULL || cBuffer == NULL || ulSize == 0) {
    return 1; // エラー: 無効な引数
  }

  // コマンドが 'D' で始まるか確認
  if (cBuffer[0] != 'D') {
    return 2; // エラー: 無効なコマンド
  }

  // コマンド文字列をトークンに分割
  char* token = strtok(cBuffer, " ");
  if (token == NULL) {
    return 3; // エラー: トークン分割失敗
  }

  // ノート数を取得
  token = strtok(NULL, " ");
  if (token == NULL) {
    return 4; // エラー: ノート数が見つからない
  }

  int noteCount = atoi(token);
  if (noteCount < 1 || noteCount > 8) {
    return 5; // エラー: ノート数が範囲外
  }

  pstParam->unNum = noteCount;

  // ノート番号とベロシティを取得
  for (int i = 0; i < noteCount; i++) {
    token = strtok(NULL, " ");
    if (token == NULL) {
      return 6; // エラー: ノート番号が見つからない
    }
    pstParam->stInfo[i].ucScale = (uint8_t)atoi(token);

    token = strtok(NULL, " ");
    if (token == NULL) {
      return 7; // エラー: ベロシティが見つからない
    }
    pstParam->stInfo[i].ucVelocity = (uint8_t)atoi(token);
  }

  return 0; // 成功
}
