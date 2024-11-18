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
#include <M5Unified.h>
#include "pinasign.h"
#include "FMG.h"

/******** global variable  ***** */
// SDカードのチップセレクトピン
const int chipSelect = PIN_SD_CMD;

// キューの定義
QueueHandle_t g_pstFMGQueue;
uint32_t g_ulSDSetFlag = False;

std::map<String, std::vector<String>> extensionMap;

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
    TS_FMGRequest stRequest;
    if (xQueueReceive(g_pstFMGQueue, &stRequest, portMAX_DELAY) == pdPASS) {
      switch (stRequest.unReqType) {
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
        case FMG_READ:
          if (file) {
            file = SD.open(request.fileName);
            size_t bytesRead = file.read(request.buffer, request.length);
            Serial.print("Read ");
            Serial.print(bytesRead);
            Serial.println(" bytes from file.");
            file.close();
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

void handleRequest(const String& request) {
  if (request == "listFiles") {
    Serial.println("Listing all files:");
    extensionMap.clear();
    listFiles(SD.open("/"), 0);
  } else {
    Serial.print("Requesting files with extension: ");
    Serial.println(request);
    requestFilesWithExtension(request);
  }
}

void listFiles(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFiles(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
      addToExtensionList(entry.name());
    }
    entry.close();
  }
}

void addToExtensionList(const char* filename) {
  String fileStr = String(filename);
  int dotIndex = fileStr.lastIndexOf('.');
  if (dotIndex != -1) {
    String ext = fileStr.substring(dotIndex + 1);
    Serial.print("File extension: ");
    Serial.println(ext);
    extensionMap[ext].push_back(fileStr);
  }
}

std::vector<String> getFilesWithExtension(const String& ext) {
  if (extensionMap.find(ext) != extensionMap.end()) {
    return extensionMap[ext];
  } else {
    return std::vector<String>();
  }
}

void requestFilesWithExtension(const String& ext) {
  std::vector<String> files = getFilesWithExtension(ext);
  Serial.print("Files with extension ");
  Serial.print(ext);
  Serial.println(":");
  for (const String& file : files) {
    Serial.println(file);
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
        }
        file = root.openNextFile();
    }
}
