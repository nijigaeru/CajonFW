/* File Name: FMG.cpp */
/* File Description: File Management */
/* Created on: 2024/03/20 */
/* Created by: T.Ichikawa */


/******** include ***** */
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FreeRTOS.h>
#include "HwInit/pinasign.h"
#include "FMG.h"

/******** global variable  ***** */
// SDカードのチップセレクトピン
const int chipSelect = PIN_SD_CMD;

// キューの定義
QueueHandle_t g_pstFMGQueue;
uint32_t g_ulSDSetFlag = False;

/******** function declaration ***** */
void FMGTask(void* pvParameters);   //ファイル管理タスク
void IRAM_ATTR SDDetInterrupt();      // SDカード挿抜時の割り込み

/************************** */
// ファイル管理タスク
void FMGTask(void* pvParameters) {
  File file;

  // SDカードの挿入検出ピンの設定（入力として、プルアップ抵抗）
  pinMode(PIN_SD_DET, INPUT_PULLUP);
  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SD_DET), SDDetInterrupt, CHANGE);

  // キューの作成
  g_pstFMGQueue = xQueueCreate(10, sizeof(TS_FMGRequest));
  if (g_pstFMGQueue == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  while (true) {
    TS_FMGRequest request;
    if (xQueueReceive(g_pstFMGQueue, &request, portMAX_DELAY) == pdPASS) {
      switch (request.type) {
        case FMG_SD_SET:
          Serial.println("SD card inserted.");
          // SDカードの初期化
          if (!SD.begin(chipSelect)) {
            Serial.println("SD card initialization failed!");
            return;
          }
          Serial.println("SD card initialized.");
          break;
        case FMG_SD_CLR:
          Serial.println("SD card removed.");
          break;
        case OPEN:
          file = SD.open(request.fileName);
          if (file) {
            Serial.println("File opened successfully.");
          } else {
            Serial.println("Failed to open file.");
          }
          break;
        case CLOSE:
          if (file) {
            file.close();
            Serial.println("File closed successfully.");
          }
          break;
        case READ:
          if (file) {
            size_t bytesRead = file.read(request.buffer, request.length);
            Serial.print("Read ");
            Serial.print(bytesRead);
            Serial.println(" bytes from file.");
          }
          break;
      }
    }
  }
}


// 割り込みサービスルーチン
void IRAM_ATTR SDDetInterrupt() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (digitalRead(PIN_SD_DET) == LOW) {
    // SDカードが挿入された
    if (g_ulSDSetFlag == False) {
      g_ulSDSetFlag = True;
      // SDカードの挿入を通知する
      TS_FMGRequest sdRequest = { FMG_SD_SET,0,NULL,0 };
      xQueueSendFromISR(g_pstFMGQueue, &sdRequest, &xHigherPriorityTaskWoken);
    }
  } else {
    // SDカードが抜かれた
    g_ulSDSetFlag = False;
    // SDカードの抜かれたことを通知する
    TS_FMGRequest sdRequest = { FMG_SD_CLR,0,NULL,0 };
    xQueueSendFromISR(g_pstFMGQueue, &sdRequest, &xHigherPriorityTaskWoken);
  }
}