#include <WiFi.h>
#include <WebServer.h>
#include <esp32-hal-gpio.h>
#include "hwinit.h"
#include "FMG.h"
#include "SLD.h"
#include "SW.h"

const char* ssid = "M5StampAP"; // アクセスポイントのSSID
const char* password = "your_PASSWORD"; // アクセスポイントのパスワード

WebServer server(80);

const int ledPin = 2; // LEDが接続されているピン番号
uint8_t g_ucLedState = 0;
uint8_t g_ucDeviceConnected = 0;
int display_rotation = 1;                         // 画面向きの初期化
// LED制御のためのキュー
QueueHandle_t ledQueue;


void ledControlTask(void *pvParameters) {
  int command;
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  while (true) {
    if (xQueueReceive(ledQueue, &command, portMAX_DELAY) == pdPASS) {
      g_ucLedState = command;
      if (command == 1) {
        digitalWrite(ledPin, HIGH);
      } else {
        digitalWrite(ledPin, LOW);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // HW初期化
  HwInit();

  // LED制御のためのキューを作成
  //ledQueue = xQueueCreate(10, sizeof(int));

  // HTTPサーバータスクをCore 1で実行
  // xTaskCreatePinnedToCore(httpServerTask, "HTTP Server Task", 4096, NULL, 1, NULL, 1);
  // ファイル管理タスクの作成
  xTaskCreatePinnedToCore(FMGTask, "FMGTask", 2048, NULL, 1, NULL, 0);
  for (uint8_t ucFetCh = 1; ucFetCh <= 8; ucFetCh++) {
    // SLD制御タスクの作成
    xTaskCreatePinnedToCore(SLDTask, "SLDTask", 2048, NULL, 1, NULL, 0);
  }
  SWInit();

}

void loop() {
  // メインループは空のまま
  while(0)
  {
    
  }
}