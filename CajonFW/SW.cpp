/* File Name: FMG.cpp */
/* File Description: File Management */
/* Created on: 2024/03/20 */
/* Created by: T.Ichikawa */


/******** include ***** */
#include <Arduino.h>
#include <esp32-hal-gpio.h>
#include "pinasign.h"
#include "SW.h"
#include "FMG.h"
#include "SLD.h"

/******** macro  ***** */
#define LONG_PUSH_TIME  1000   // 長押し判定時間(msec)
#define SW_INVALID_TIME 100     // 無効時間(msec)

/******** global variable  ***** */
TS_SWParam stSWParam[4] = 
{
  {SLD_TURN_ON, &g_pstSLDQueue[0], SLD_TURN_ON, &g_pstSLDQueue[4], 0, false},
  {SLD_TURN_ON, &g_pstSLDQueue[1], SLD_TURN_ON, &g_pstSLDQueue[5], 0, false},
  {SLD_TURN_ON, &g_pstSLDQueue[2], SLD_TURN_ON, &g_pstSLDQueue[6], 0, false},
  {SLD_TURN_ON, &g_pstSLDQueue[3], SLD_TURN_ON, &g_pstSLDQueue[7], 0, false},
}

;

/******** function declaration ***** */
void IRAM_ATTR SW1Interrupt();      // SW1の割り込み
void IRAM_ATTR SW2Interrupt();      // SW2の割り込み
void IRAM_ATTR SW3Interrupt();      // SW3の割り込み
void IRAM_ATTR SW4Interrupt();      // SW4の割り込み

/************************** */
// ファイル管理タスク
void SWInit(void) {

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW1), SW1Interrupt, CHANGE);

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW2), SW2Interrupt, CHANGE);

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW3), SW3Interrupt, CHANGE);

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW4), SW4Interrupt, CHANGE);

}

void SWInteruptProc(TS_SWParam* pstParam, uint32_t ulSWPin)
{
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;
  if (digitalRead(ulSWPin) == HIGH) {
    // ボタンが離れた
    if(pstParam->bSWFlag == true)
    {
      TickType_t xTimeNow = xTaskGetTickCountFromISR(); 
      if ((xTimeNow - pstParam->xTimeNow) > LONG_PUSH_TIME) {
        // 長押し
        Serial.print("longPush");
        Serial.print(ulSWPin);
        Serial.print(", ");
        Serial.print(pstParam->xTimeNow);
        Serial.print(", ");
        Serial.println(xTimeNow);
        // 長押し用の要求を通知する
        uint16_t unReq = pstParam->unLongPushReq;
        xQueueSendFromISR(*(pstParam->pstLongQue), &unReq, &xHigherPriorityTaskWoken);
        pstParam->xTimeNow = 0;
        pstParam->bSWFlag = false;
      } else if ((xTimeNow - pstParam->xTimeNow) < SW_INVALID_TIME){
        // 何もしない
      } else {
        // 短押し
        Serial.print("ShortPush");
        Serial.print(ulSWPin);
        Serial.print(", ");
        Serial.print(pstParam->xTimeNow);
        Serial.print(", ");
        Serial.println(xTimeNow);
        // 短押し用の要求を通知する
        uint16_t unReq = pstParam->unShortPushReq;
        xQueueSendFromISR(*(pstParam->pstShortQue), &unReq, &xHigherPriorityTaskWoken);
        pstParam->xTimeNow = 0;
        pstParam->bSWFlag = false;
      }
    }
  } else {
    if (pstParam->bSWFlag == false)
    {
      // ボタンが押された
      pstParam->xTimeNow = xTaskGetTickCountFromISR(); 
      pstParam->bSWFlag = true;
    }
  }
}


// 割り込みサービスルーチン
void IRAM_ATTR SW1Interrupt() {
  TS_SWParam* pstParam = &stSWParam[0];
  SWInteruptProc(pstParam, PIN_SW1);
}
// 割り込みサービスルーチン
void IRAM_ATTR SW2Interrupt() {
  TS_SWParam* pstParam = &stSWParam[1];
  SWInteruptProc(pstParam, PIN_SW2);
}
// 割り込みサービスルーチン
void IRAM_ATTR SW3Interrupt() {
  TS_SWParam* pstParam = &stSWParam[2];
  SWInteruptProc(pstParam, PIN_SW3);
}
// 割り込みサービスルーチン
void IRAM_ATTR SW4Interrupt() {
  TS_SWParam* pstParam = &stSWParam[3];
  SWInteruptProc(pstParam, PIN_SW4);
}
