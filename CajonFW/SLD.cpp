#include <Arduino.h>
#include <esp32-hal-gpio.h>
#include <queue.h>
#include "SLD.h"
#include "pinasign.h"

// 0 : 打面（中央）
// 1 : 打面（上）
// 2 : 打面（角）
// 3 : ー
// 4 : タンバリン
// 5 : 円盤
// 6 : シンバル
// 7 : ー


// キューの定義
QueueHandle_t g_pstSLDQueue[SLD_NUM];
bool g_ulSLDInitFlg[SLD_NUM] = {false};
uint8_t fetPins[] = { PIN_FET1, PIN_FET2, PIN_FET3, PIN_FET4, PIN_FET5, PIN_FET6, PIN_FET7, PIN_FET8 };
uint32_t g_ulSldOnTime[] = { 10, 10, 10, 10, 10, 10, 20, 10}; // ソレノイド駆動時間（ミリ秒）
uint32_t g_ulBeginDelay[] = { 15, 15, 15, 15, 0, 15, 5, 0 };
uint8_t g_ucMinPower[] = { 80, 80, 80, 100, 150, 200, 230, 200 };
uint32_t g_ulFetCount = 1;
const double  PWM_Hz = 2000;   // PWM周波数
const uint8_t PWM_level = 8; // PWM分解能 16bit(1～256)

// ソレノイド駆動タスク
void SLDTask(void* pvParameters) {
  //uint8_t ucFetCh = *(uint8_t*)pvParameters;
  uint8_t ucFetCh = g_ulFetCount ;
  g_ulFetCount ++;
  uint8_t ucSLDPin = fetPins[ucFetCh-1];

  if (g_ulSLDInitFlg[ucFetCh-1])
  {
    USBSerial.print("SLD already initialized.");
    USBSerial.println(ucFetCh);
    return;
  }

  // ピンの初期化
  pinMode(ucSLDPin, OUTPUT);
  // チャンネルと周波数の分解能を設定
  ledcSetup(ucFetCh, PWM_Hz, PWM_level);
  // ピンとチャンネルの設定
  ledcAttachPin(ucSLDPin, ucFetCh);
  ledcWrite(ucFetCh,0);

  // キューの作成
  g_pstSLDQueue[ucFetCh-1] = xQueueCreate(REQ_QUE_NUM, REQ_QUE_SIZE);
  if (g_pstSLDQueue[ucFetCh-1] == NULL) {
    USBSerial.println("Failed to create queue.");
    return;
  }

  // 初期化完了フラグ
  g_ulSLDInitFlg[ucFetCh-1] = true;
  USBSerial.print("SLD initialized.");
  USBSerial.println(ucFetCh);
  while (true) {
    uint8_t ucRecvReq[REQ_QUE_SIZE];
    TS_Req* pstRecvReq = (TS_Req*)ucRecvReq;
    if (xQueueReceive(g_pstSLDQueue[ucFetCh-1], pstRecvReq, portMAX_DELAY) == pdPASS) {
      if (pstRecvReq->unReqType == SLD_TURN_ON) {
        // ちょっと待つ
        vTaskDelay(pdMS_TO_TICKS(g_ulBeginDelay[ucFetCh-1]));
        // SLDをONにする
        TS_SLDOnParam* pstSLDOnParam = (TS_SLDOnParam*)pstRecvReq->ucParam;
        ledcWrite(ucFetCh, g_ucMinPower[ucFetCh-1] + (uint32_t)(255 - g_ucMinPower[ucFetCh-1]) * pstSLDOnParam->ucPower / 127);
        // USBSerial.print("SLD(");
        // USBSerial.print(ucFetCh);
        // USBSerial.print("),power(");
        // USBSerial.print(pstSLDOnParam->ucPower);
        // USBSerial.println(") turned ON.");
        // 一定時間待つ
        vTaskDelay(pdMS_TO_TICKS(g_ulSldOnTime[ucFetCh-1]));
        // SLDをOFFにする
        ledcWrite(ucFetCh,0);
        // USBSerial.print("SLD(");
        // USBSerial.print(ucFetCh);
        // USBSerial.println(") turned OFF.");
      }
    }
  }
}
