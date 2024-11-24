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
#define LONG_PUSH_TIME  10000   // 長押し判定時間(msec)
#define SW_INVALID_TIME 100     // 無効時間(msec)

/******** global variable  ***** */
TS_SWParam stSWParam[4] = 
{
  {SLD_TURN_ON, &g_pstSLDQueue[0], SLD_TURN_ON, &g_pstSLDQueue[4], 0},
  {SLD_TURN_ON, &g_pstSLDQueue[1], SLD_TURN_ON, &g_pstSLDQueue[5], 0},
  {SLD_TURN_ON, &g_pstSLDQueue[2], SLD_TURN_ON, &g_pstSLDQueue[6], 0},
  {SLD_TURN_ON, &g_pstSLDQueue[3], SLD_TURN_ON, &g_pstSLDQueue[7], 0},
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


// 割り込みサービスルーチン
void IRAM_ATTR SW1Interrupt() {
  TS_SWParam* pstSWParam = &stSWParam[0];
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;

  if (digitalRead(PIN_SW1) == LOW) {
    // ボタンが離れた
    TickType_t xTimeNow = xTaskGetTickCount(); 
    if ((xTimeNow - pstSWParam->xTimeNow) > LONG_PUSH_TIME) {
      // 長押し
      // 長押し用の要求を通知する
      uint16_t unReq = pstSWParam->unLongPushReq;
      xQueueSendFromISR(*(pstSWParam->pstLongQue), &unReq, &xHigherPriorityTaskWoken);
    } else if ((xTimeNow - pstSWParam->xTimeNow) < SW_INVALID_TIME){
      // 何もしない
    } else {
      // 短押し
      // 短押し用の要求を通知する
      uint16_t unReq = pstSWParam->unShortPushReq;
      xQueueSendFromISR(*(pstSWParam->pstShortQue), &unReq, &xHigherPriorityTaskWoken);
    }
  } else {
    if (pstSWParam->xTimeNow != 0)
    {
      // ボタンが押された
      pstSWParam->xTimeNow = xTaskGetTickCount(); 
    }
  }
}
// 割り込みサービスルーチン
void IRAM_ATTR SW2Interrupt() {
  TS_SWParam* pstSWParam = &stSWParam[1];
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;

  if (digitalRead(PIN_SW2) == HIGH) {
    // ボタンが離れた
    TickType_t xTimeNow = xTaskGetTickCount(); 
    if (xTimeNow - pstSWParam->xTimeNow > LONG_PUSH_TIME) {
      // 長押し
      // 長押し用の要求を通知する
      uint16_t unReq = pstSWParam->unLongPushReq;
      xQueueSendFromISR(*(pstSWParam->pstLongQue), &unReq, &xHigherPriorityTaskWoken);
    } else if (SW_INVALID_TIME){
      // 何もしない
    } else {
      // 短押し
      // 短押し用の要求を通知する
      uint16_t unReq = pstSWParam->unShortPushReq;
      xQueueSendFromISR(*(pstSWParam->pstShortQue), &unReq, &xHigherPriorityTaskWoken);
    }
  } else {
    if (pstSWParam->xTimeNow != 0)
    {
      // ボタンが押された
      pstSWParam->xTimeNow = xTaskGetTickCount(); 
    }
  }
}
// 割り込みサービスルーチン
void IRAM_ATTR SW3Interrupt() {
  TS_SWParam* pstSWParam = &stSWParam[2];
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;

  if (digitalRead(PIN_SW3) == LOW) {
    // ボタンが離れた
    TickType_t xTimeNow = xTaskGetTickCount(); 
    if (xTimeNow - pstSWParam->xTimeNow > LONG_PUSH_TIME) {
      // 長押し
      // 長押し用の要求を通知する
      uint16_t unReq = pstSWParam->unLongPushReq;
      xQueueSendFromISR(*(pstSWParam->pstLongQue), &unReq, &xHigherPriorityTaskWoken);
    } else if (SW_INVALID_TIME){
      // 何もしない
    } else {
      // 短押し
      // 短押し用の要求を通知する
      uint16_t unReq = pstSWParam->unShortPushReq;
      xQueueSendFromISR(*(pstSWParam->pstShortQue), &unReq, &xHigherPriorityTaskWoken);
    }
  } else {
    if (pstSWParam->xTimeNow != 0)
    {
      // ボタンが押された
      pstSWParam->xTimeNow = xTaskGetTickCount(); 
    }
  }
}
// 割り込みサービスルーチン
void IRAM_ATTR SW4Interrupt() {
  TS_SWParam* pstSWParam = &stSWParam[3];
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;

  if (digitalRead(PIN_SW4) == LOW) {
    // ボタンが離れた
    TickType_t xTimeNow = xTaskGetTickCount(); 
    if (xTimeNow - pstSWParam->xTimeNow > LONG_PUSH_TIME) {
      // 長押し
      // 長押し用の要求を通知する
      uint16_t unReq = pstSWParam->unLongPushReq;
      xQueueSendFromISR(*(pstSWParam->pstLongQue), &unReq, &xHigherPriorityTaskWoken);
    } else if (SW_INVALID_TIME){
      // 何もしない
    } else {
      // 短押し
      // 短押し用の要求を通知する
      uint16_t unReq = pstSWParam->unShortPushReq;
      xQueueSendFromISR(*(pstSWParam->pstShortQue), &unReq, &xHigherPriorityTaskWoken);
    }
  } else {
    if (pstSWParam->xTimeNow != 0)
    {
      // ボタンが押された
      pstSWParam->xTimeNow = xTaskGetTickCount(); 
    }
  }
}
