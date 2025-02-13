#include <WiFi.h>
#include <WebServer.h>
#include <esp32-hal-gpio.h>
#include "hwinit.h"
#include "FMG.h"
#include "SLD.h"
#include "SW.h"


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
  xTaskCreatePinnedToCore(HTTPTask, "HTTPTask", 4096, NULL, 1, NULL, 1);
  // ファイル管理タスクの作成
  xTaskCreatePinnedToCore(FMGTask, "FMGTask", 2048, NULL, 1, NULL, 0);
  for (uint8_t ucFetCh = 1; ucFetCh <= 8; ucFetCh++) {
    // SLD制御タスクの作成
    xTaskCreatePinnedToCore(SLDTask, "SLDTask", 1024, NULL, 1, NULL, 0);
  }
  SWInit();

}

void loop() {
  // メインループは空のまま
  while(0)
  {
    
  }
}
