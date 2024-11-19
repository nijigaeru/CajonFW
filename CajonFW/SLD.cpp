#include <Arduino.h>
#include <M5Unified.h>
#include <queue.h>
#include "SLD.h"
#include "pinasign.h"

// ソレノイド駆動時間（ミリ秒）
#define SLD_ON_TIME 1000
#define SLD_NUM 8

// キューの定義
QueueHandle_t g_pstSLDQueue[SLD_NUM];
bool g_ulSLDInitFlg[SLD_NUM] = false;
uint8_t fetPins[] = { PIN_FET1, PIN_FET2, PIN_FET3, PIN_FET4, PIN_FET5, PIN_FET6, PIN_FET7, PIN_FET8 };

// ソレノイド駆動タスク
void SLDTask(void* pvParameters) {
  uint8_t ucFetCh = *(uint8_t*)pvParameters;
  uint8_t ucSLDPin = fetPins[ucFetCh];

  if (g_ulSLDInitFlg[ucFetCh-1])
  {
    Serial.println("SLD already initialized.");
    return;
  }

  // ピンの初期化
  pinMode(ucSLDPin, OUTPUT);
  digitalWrite(ucSLDPin, LOW);

  // キューの作成
  g_pstSLDQueue[ucFetCh-1] = xQueueCreate(10, sizeof(TS_SLDRequest));
  if (g_pstSLDQueue[ucFetCh-1] == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  // 初期化完了フラグ
  g_ulSLDInitFlg[ucFetCh-1] = true;
  Serial.println("SLD(%d) initialized.", ucFetCh);

  while (true) {
    TS_SLDRequest request;
    if (xQueueReceive(g_pstSLDQueue[ucFetCh-1], &request, portMAX_DELAY) == pdPASS) {
      if (request.type == SLD_TURN_ON) {
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
