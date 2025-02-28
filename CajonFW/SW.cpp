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
#include "READMID.h"
#include "REQ.h"

/******** macro  ***** */
#define LONG_PUSH_TIME  1000   // 長押し判定時間(msec)
#define SW_INVALID_TIME 100     // 無効時間(msec)
#define DEBOUNCE_DELAY 500  // デバウンス時間 (ミリ秒) 　ひとまずかなり長め
// #define SW_DEBUG

/******** global variable  ***** */
TS_SWParam stSWParam[4] = 
{
  {SLD_TURN_ON, &g_pstSLDQueue[0], SLD_TURN_ON, &g_pstSLDQueue[4], 0, false},
  {SLD_TURN_ON, &g_pstSLDQueue[1], SLD_TURN_ON, &g_pstSLDQueue[5], 0, false},
  {SLD_TURN_ON, &g_pstSLDQueue[2], SLD_TURN_ON, &g_pstSLDQueue[6], 0, false},
  {SLD_TURN_ON, &g_pstSLDQueue[3], SLD_TURN_ON, &g_pstSLDQueue[7], 0, false},
};

// char Filename[] = "senbonzakura.mid";
char Filename[] = "senbon_only6drams_001.mid";

volatile unsigned long lastInterruptTime = 0; // チャタリング対策

/******** function declaration ***** */
void IRAM_ATTR SW1Interrupt();      // SW1の割り込み
void IRAM_ATTR SW2Interrupt();      // SW2の割り込み
void IRAM_ATTR SW3Interrupt();      // SW3の割り込み
void IRAM_ATTR SW4Interrupt();      // SW4の割り込み

/************************** */
// ファイル管理タスク
void SWInit(void) {

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW1), SW1Interrupt, FALLING);

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW2), SW2Interrupt, FALLING);

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW3), SW3Interrupt, FALLING);

  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SW4), SW4Interrupt, FALLING);

}

void SWInteruptProc(TS_SWParam* pstParam, uint32_t ulSWPin, uint32_t ulCh)
{
  // いったん、一曲再生を優先するため、長押し判定はコメントアウト。最低限の処理に絞る。(Hirose)
  #if 0
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;
  uint8_t ucSendReq[REQ_QUE_SIZE] = { 0 };
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  if (digitalRead(ulSWPin) == HIGH) {
    // ボタンが離れた
    if(pstParam->bSWFlag == true)
    {
      TickType_t xTimeNow = xTaskGetTickCountFromISR(); 
      if ((xTimeNow - pstParam->xTimeNow) > LONG_PUSH_TIME) {
        // 長押し
        #ifdef SW_DEBUG
        Serial.print("longPush");
        Serial.print(ulSWPin);
        Serial.print(", ");
        Serial.print(pstParam->xTimeNow);
        Serial.print(", ");
        Serial.println(xTimeNow);
        #endif
        // 長押し用の要求を通知する
        pstSendReq->unReqType = pstParam->unLongPushReq;
        TS_SLDOnParam* pstSLDParam = (TS_SLDOnParam*)pstSendReq->ucParam;
        pstSLDParam->ucPower = 255;
        xQueueSendFromISR(*(pstParam->pstLongQue), pstSendReq, &xHigherPriorityTaskWoken);
        // pstSendReq->unReqType = ;
        //TS_SLDOnParam* pstSLDParam = (TS_SLDOnParam*)pstSendReq->ucParam;
        //pstSLDParam->ucPower = 255;
        //xQueueSendFromISR(*(pstParam->pstLongQue), pstSendReq, &xHigherPriorityTaskWoken);
        pstParam->xTimeNow = 0;
        pstParam->bSWFlag = false;
      } else if ((xTimeNow - pstParam->xTimeNow) < SW_INVALID_TIME){
        // 何もしない
      } else {
        // 短押し
        #ifdef SW_DEBUG
        Serial.print("ShortPush");
        Serial.print(ulSWPin);
        Serial.print(", ");
        Serial.print(pstParam->xTimeNow);
        Serial.print(", ");
        Serial.println(xTimeNow);
        #endif
        // 短押し用の要求を通知する
        // pstSendReq->unReqType = pstParam->unShortPushReq;
        // TS_SLDOnParam* pstSLDParam = (TS_SLDOnParam*)pstSendReq->ucParam;
        // pstSLDParam->ucPower = 255;
        // xQueueSendFromISR(*(pstParam->pstShortQue), pstSendReq, &xHigherPriorityTaskWoken);
        pstSendReq->unReqType = READMID_START;
        TS_READMIDStartParam* pstREADParam = (TS_READMIDStartParam*)pstSendReq->ucParam;
        memcpy(pstREADParam->ucFileName,Filename, sizeof(Filename));
        xQueueSendFromISR(g_pstREADMIDQueue, pstSendReq, &xHigherPriorityTaskWoken);
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

  #endif

  uint8_t ucSendReq[REQ_QUE_SIZE] = { 0 };
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  portBASE_TYPE xHigherPriorityTaskWoken; 
  xHigherPriorityTaskWoken = pdFALSE;

  pstSendReq->unReqType = READMID_START;
  TS_READMIDStartParam* pstREADParam = (TS_READMIDStartParam*)pstSendReq->ucParam;
  memcpy(pstREADParam->ucFileName,Filename, sizeof(Filename));
  xQueueSendFromISR(g_pstREADMIDQueue, pstSendReq, &xHigherPriorityTaskWoken);
}


// 割り込みサービスルーチン
void IRAM_ATTR SW1Interrupt() {
  unsigned long interruptTime = millis();
  // デバウンス処理
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
    TS_SWParam* pstParam = &stSWParam[0];
    SWInteruptProc(pstParam, PIN_SW1, 1);
    lastInterruptTime = interruptTime;
  }
}

// 割り込みサービスルーチン
void IRAM_ATTR SW2Interrupt() {
  unsigned long interruptTime = millis();
  // デバウンス処理
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
    TS_SWParam* pstParam = &stSWParam[1];
    SWInteruptProc(pstParam, PIN_SW2, 2);
    lastInterruptTime = interruptTime;
  }
}

// 割り込みサービスルーチン
void IRAM_ATTR SW3Interrupt() {
  unsigned long interruptTime = millis();
  // デバウンス処理
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
    TS_SWParam* pstParam = &stSWParam[2];
    SWInteruptProc(pstParam, PIN_SW3, 3);
    lastInterruptTime = interruptTime;
  }
}

// 割り込みサービスルーチン
void IRAM_ATTR SW4Interrupt() {
  unsigned long interruptTime = millis();
  // デバウンス処理
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
    TS_SWParam* pstParam = &stSWParam[3];
    SWInteruptProc(pstParam, PIN_SW4, 4);
    lastInterruptTime = interruptTime;
  }
}