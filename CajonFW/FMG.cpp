/* File Name: FMG.cpp */
/* File Description: File Management */
/* Created on: 2024/03/20 */
/* Created by: T.Ichikawa */


/******** include ***** */
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <map>
#include <vector>
#include "pinasign.h"
#include "FMG.h"

/******** global variable  ***** */
// SDカードのチップセレクトピン
const int chipSelect = PIN_SD_CMD;

// キューの定義
QueueHandle_t g_pstFMGQueue;
bool g_ulSDSetFlag = false;

std::vector<String> htmlFiles;
std::vector<String> csvFiles;

/******** function declaration ***** */
void FMGTask(void* pvParameters);   //ファイル管理タスク
void IRAM_ATTR SDDetInterrupt();      // SDカード挿抜時の割り込み
void listDir(fs::FS &fs, const char * dirname, uint8_t levels, uint8_t indent);

/************************** */
// ファイル管理タスク
void FMGTask(void* pvParameters) {
  File file;

  // SDカードの挿入検出ピンの設定（入力として、プルアップ抵抗）
  pinMode(PIN_SD_DET, INPUT);
  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SD_DET), SDDetInterrupt, CHANGE);

  // キューの作成
  g_pstFMGQueue = xQueueCreate(10, sizeof(TS_FMGRequest));
  if (g_pstFMGQueue == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  while (true) {
    TS_FMGRequest stRequest;
    if (xQueueReceive(g_pstFMGQueue, &stRequest, portMAX_DELAY) == pdPASS) {
      switch (stRequest.unReqType) {
        case FMG_SD_SET:
          Serial.println("SD card inserted.");
          // SDカードの初期化
          if (!SD.begin(chipSelect)) {
            listDir(SD, "/", 0, 0);
            
            // .htmlファイルのリストを表示
            Serial.println("\n.html files:");
            for (const auto& file : htmlFiles) {
                Serial.println(file);
            }

            // .csvファイルのリストを表示
            Serial.println("\n.csv files:");
            for (const auto& file : csvFiles) {
                Serial.println(file);
            }
          }
          else
          {
            Serial.println("SD card initialization failed!");
          }
          break;
        case FMG_SD_CLR:
          Serial.println("SD card removed.");
          break;
        case FMG_READ:
          file = SD.open(stRequest.ucFileName);
          size_t bytesRead = file.read(stRequest.pucBuffer, stRequest.ulLength);
          Serial.print("Read ");
          Serial.print(bytesRead);
          Serial.println(" bytes from file.");
          file.close();
          break;
      }
    }
  }
}


// 割り込みサービスルーチン
void IRAM_ATTR SDDetInterrupt() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  TS_FMGRequest stRequest;
  if (digitalRead(PIN_SD_DET) == LOW) {
    // SDカードが挿入された
    if (g_ulSDSetFlag == false) {
      g_ulSDSetFlag = true;
      // SDカードの挿入を通知する
      stRequest = { FMG_SD_SET,0,NULL,0 };
      xQueueSendFromISR(g_pstFMGQueue, &stRequest, &xHigherPriorityTaskWoken);
    }
  } else {
    // SDカードが抜かれた
    g_ulSDSetFlag = false;
    // SDカードの抜かれたことを通知する
    stRequest = { FMG_SD_CLR,0,NULL,0 };
    xQueueSendFromISR(g_pstFMGQueue, &stRequest, &xHigherPriorityTaskWoken);
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels, uint8_t indent) {
    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        for (uint8_t i = 0; i < indent; i++) {
            Serial.print("  ");
        }
        if (file.isDirectory()) {
            Serial.print("|-- ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1, indent + 1);
            }
        } else {
            Serial.print("|-- ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());

            // 拡張子をチェックしてリストに追加
            String fileName = file.name();
            if (fileName.endsWith(".html")) {
                htmlFiles.push_back(fileName);
            } else if (fileName.endsWith(".csv")) {
                csvFiles.push_back(fileName);
            }
        }
        file = root.openNextFile();
    }
}
