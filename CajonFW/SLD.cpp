#include <Arduino.h>
#include <M5Unified.h>
#include <queue.h>
#include "SLD.h"
#include "pinasign.h"

// ソレノイド駆動時間（ミリ秒）
#define SLD_ON_TIME 1000
#define SLD_NUM 8
#define PIN_FET(ch) (PIN_FET##ch) 

// キューの定義
QueueHandle_t g_pstSLDQueue[SLD_NUM];
bool g_ulSLDInitFlg[SLD_NUM] = flase;


// ソレノイド駆動タスク
void SLDTask(void* pvParameters) {
  uint8_t ucFetCh = *(uint8_t*)pvParameters;
  uint8_t ucSLDPin = PIN_FET(ucFetCh);

  if (g_ulSLDInitFlg[ucFetCh-1])
  {
    Serial.println("SLD already initialized.");
    return;
  }

  // ピンの初期化
  pinMode(ucSLDPin, OUTPUT);
  digitalWrite(ucSLDPin, LOW);

  // キューの作成
  g_pstSLDQueue[ucFetCh-1] = xQueueCreate(10, sizeof(SLDRequest));
  if (g_pstSLDQueue[ucFetCh-1] == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  // 初期化完了フラグ
  g_ulSLDInitFlg[ucFetCh-1] = true:
  Serial.println("SLD(%d) initialized.", ucFetCh);

  while (true) {
    SLDRequest request;
    if (xQueueReceive(g_pstSLDQueue[ucFetCh-1], &request, portMAX_DELAY) == pdPASS) {
      if (request.type == SLD_TURN_ONs) {
        // SLDをONにする
        digitalWrite(ucSLDPin, HIGH);
        Serial.print("SLD on pin ");
        Serial.print(ucSLDPin);
        Serial.println(" turned ON.");
        // 一定時間待つ
        vTaskDelay(pdMS_TO_TICKS(SLD_ON_TIME));
        // SLDをOFFにする
        digitalWrite(ucSLDPin, LOW);
        Serial.print("SLD on pin ");
        Serial.print(ucSLDPin);
        Serial.println(" turned OFF.");
      }
    }
  }
}
