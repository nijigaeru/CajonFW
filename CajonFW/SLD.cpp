#include <Arduino.h>
#include <esp32-hal-gpio.h>
#include <queue.h>
#include "SLD.h"
#include "pinasign.h"

// ソレノイド駆動時間（ミリ秒）
#define SLD_ON_TIME 100

// キューの定義
QueueHandle_t g_pstSLDQueue[SLD_NUM];
bool g_ulSLDInitFlg[SLD_NUM] = {false};
uint8_t fetPins[] = { PIN_FET1, PIN_FET2, PIN_FET3, PIN_FET4, PIN_FET5, PIN_FET6, PIN_FET7, PIN_FET8 };
uint32_t g_ulFetCount = 1;

// ソレノイド駆動タスク
void SLDTask(void* pvParameters) {
  //uint8_t ucFetCh = *(uint8_t*)pvParameters;
  uint8_t ucFetCh = g_ulFetCount ;
  g_ulFetCount ++;
  uint8_t ucSLDPin = fetPins[ucFetCh-1];

  if (g_ulSLDInitFlg[ucFetCh-1])
  {
    Serial.print("SLD already initialized.");
    Serial.println(ucFetCh);
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
  Serial.print("SLD initialized.");
  Serial.println(ucFetCh);

  while (true) {
    TS_SLDRequest request;
    if (xQueueReceive(g_pstSLDQueue[ucFetCh-1], &request, portMAX_DELAY) == pdPASS) {
      if (request.unReqType == SLD_TURN_ON) {
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
