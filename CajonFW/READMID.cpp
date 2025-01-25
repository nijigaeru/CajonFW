#include <Arduino.h>
#include "REQ.h"
#include "READMID.h"
#include "FMG.h"
/******** macro *****/
#define BUFSIZE 1024
/******** global variable  ***** */
uint8_t g_ucBuffer[BUFSIZE];

// キューの定義
QueueHandle_t g_pstREADMIDQueue;

/******** function declaration ***** */
void READMIDTask(void* pvParameters);   //ファイル管理タスク

/************************** */
// タスク
void READMIDTask(void* pvParameters) {

  // キューの作成
  g_pstREADMIDQueue = xQueueCreate(REQ_QUE_NUM, REQ_QUE_SIZE);
  if (g_pstREADMIDQueue == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  while (true) {
    uint8_t ucRecvReq[REQ_QUE_SIZE];
    TS_Req* pstRecvReq = (TS_Req*)ucRecvReq;
    uint8_t ucSendReq[REQ_QUE_SIZE];
    TS_Req* pstSendReq = (TS_Req*)ucSendReq;
    if (xQueueReceive(g_pstREADMIDQueue, pstRecvReq, portMAX_DELAY) == pdPASS) {
      switch (pstRecvReq->unReqType)
      {
        case READMID_START:/* 再生開始 */
        {
          TS_READMIDStartParam *pstStart = (TS_READMIDStartParam *)pstRecvReq->ucParam;
          TS_FMGOpenParam* pstOpen = (TS_FMGOpenParam*)pstSendReq->ucParam;
          pstSendReq->unReqType = FMG_OPEN;
          pstSendReq->pstAnsQue = g_pstREADMIDQueue;
          pstSendReq->ulSize = sizeof(TS_FMGOpenParam);
          memcpy(pstOpen->ucFileName, pstStart->ucFileName, sizeof(pstStart->ucFileName));
          xQueueSend(g_pstFMGQueue, pstSendReq, 100);
          break;
        }
        case FMG_OPEN_ANS:/* オープン完了 */
        {
          if(pstRecvReq->unError == 0)
          {
            TS_FMGReadParam* pstRead = (TS_FMGReadParam*)pstSendReq->ucParam;
            pstSendReq->unReqType = FMG_READ;
            pstSendReq->pstAnsQue = g_pstREADMIDQueue;
            pstSendReq->ulSize = sizeof(TS_FMGReadParam);
            pstRead->pucBuffer = g_ucBuffer;
            pstRead->ulLength = BUFSIZE;
            xQueueSend(g_pstFMGQueue, pstSendReq, 100);
          }
          break;
        }
        default:
        {
          break;
        }
      }
    }
  }
}
