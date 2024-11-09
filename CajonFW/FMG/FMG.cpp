#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FreeRTOS.h>
#include "HwInit/pinasign.h"
#include "FMG.h"

// SDカードのチップセレクトピン
const int chipSelect = PIN_SD_CMD;

// キューの定義
QueueHandle_t g_pstFMGQueue;
uint32_t g_ulSDSetFlag = False;

// ファイル管理タスク
void FMGTask(void* pvParameters) {
  File file;

  // SDカードの初期化
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

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
