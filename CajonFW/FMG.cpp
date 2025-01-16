#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <map>
#include <vector>
#include "pinasign.h"
#include "FMG.h"
/******** macro *****/
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3
// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(PIN_SD_D3, DEDICATED_SPI,  SD_SCK_MHZ(16))
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(PIN_SD_D3, SHARED_SPI)
#endif  // HAS_SDIO_CLASS
//------------------------------------------------------------------------------
// store error strings in flash to save RAM
#define error(s) SD.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
/******** global variable  ***** */
// SDカードのチップセレクトピン
const int chipSelect = PIN_SD_CMD;

// SdFatオブジェクトのインスタンス
SdFat SD;

// キューの定義
QueueHandle_t g_pstFMGQueue;
bool g_ulSDSetFlag = false;
FsFile g_file;
std::vector<String> htmlFiles;
std::vector<String> midFiles;

/******** function declaration ***** */
void FMGTask(void* pvParameters);   //ファイル管理タスク
void IRAM_ATTR SDDetInterrupt();    // SDカード挿抜時の割り込み
void listDir(SdFat &SD, const char *dirname, uint8_t levels, uint8_t indent);

/************************** */
// ファイル管理タスク
void FMGTask(void* pvParameters) {
  // SDカードの挿入検出ピンの設定（入力として、プルアップ抵抗）
  pinMode(PIN_SD_DET, INPUT);
  // 割り込み設定: SDカードの挿抜を監視
  attachInterrupt(digitalPinToInterrupt(PIN_SD_DET), SDDetInterrupt, CHANGE);

  SPI.begin(PIN_SD_CLK,PIN_SD_D0,PIN_SD_CMD,PIN_SD_D3);

  // キューの作成
  g_pstFMGQueue = xQueueCreate(REQ_QUE_NUM, REQ_QUE_SIZE);
  if (g_pstFMGQueue == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }
  if (digitalRead(PIN_SD_DET) == LOW) {
    // SDカードが挿入されている場合
    uint8_t ucSendReq[REQ_QUE_SIZE];
    TS_Req* pstSendReq = (TS_Req*)ucSendReq;
    g_ulSDSetFlag = true;
    // SDカードの挿入を通知する
    pstSendReq->unReqType = FMG_SD_SET;
    pstSendReq->ulSize = 0;
    xQueueSend(g_pstFMGQueue, pstSendReq, 100);
  } 

  while (true) {
    uint8_t ucRecvReq[REQ_QUE_SIZE];
    TS_Req* pstRecvReq = (TS_Req*)ucRecvReq;
    uint8_t ucSendReq[REQ_QUE_SIZE];
    TS_Req* pstSendReq = (TS_Req*)ucSendReq;
    if (xQueueReceive(g_pstFMGQueue, pstRecvReq, portMAX_DELAY) == pdPASS) {
      switch (pstRecvReq->unReqType) {
        case FMG_SD_SET:
          Serial.println("SD card inserted.");
          // SDカードの初期化
          if (SD.begin(SD_CONFIG)) {
            listDir(SD, "/", 0, 0);
            
            // .htmlファイルのリストを表示
            Serial.println("\n.html files:");
            for (const auto& file : htmlFiles) {
              Serial.println(file);
            }

            // .midファイルのリストを表示
            Serial.println("\n.mid files:");
            for (const auto& file : midFiles) {
              Serial.println(file);
            }
          } else {
            Serial.println("SD card initialization failed!");
          }
          break;
        case FMG_SD_CLR:
          Serial.println("SD card removed.");
          SD.end();
          htmlFiles.clear();
          midFiles.clear();
          break;
        case FMG_OPEN: 
        {
          TS_FMGOpenParam* pstOpen = (TS_FMGOpenParam*)pstRecvReq->ucParam;
          g_file = SD.open(pstOpen->ucFileName, O_READ);
          if (g_file) {
            pstSendReq->unReqType = FMG_OPEN_ANS;
            pstSendReq->unError = 0;
            xQueueSend(pstRecvReq->pstAnsQue, pstSendReq, 100);
          } else {
            Serial.println("Failed to open file for reading.");
            pstSendReq->unReqType = FMG_OPEN_ANS;
            pstSendReq->unError = (0xFFFF);
            xQueueSend(pstRecvReq->pstAnsQue, pstSendReq, 100);
          }
          break;
        }
        case FMG_READ:
        {
          TS_FMGReadParam* pstRead = (TS_FMGReadParam*)pstRecvReq->ucParam;
          if (g_file) {
            size_t bytesRead = g_file.read(pstRead->pucBuffer, pstRead->ulLength);
            Serial.print("Read ");
            Serial.print(bytesRead);
            Serial.println(" bytes from file.");
            pstSendReq->unReqType = FMG_READ_ANS;
            pstSendReq->unError = 0;
            TS_FMGReadAns* pstReadAns  = ((TS_FMGReadAns*)pstSendReq->ucParam);
            pstReadAns->ulLength = bytesRead;
            xQueueSend(pstRecvReq->pstAnsQue, pstSendReq, 100);
          } else {
            Serial.println("Failed to open file for reading.");
            pstSendReq->unReqType = FMG_READ_ANS;
            pstSendReq->unError = (0xFFFF);
            xQueueSend(pstRecvReq->pstAnsQue, pstSendReq, 100);
          }
          break;
        }
        case FMG_CLOSE:
        {
          if (g_file) {
            g_file.close();
            Serial.print("Close ");
          }
          pstSendReq->unReqType = FMG_CLOSE_ANS;
          pstSendReq->unError = (0);
          xQueueSend(pstRecvReq->pstAnsQue, pstSendReq, 100);
          break;
        }
        default:
          break;
      }
    }
  }
}

// 割り込みサービスルーチン
void IRAM_ATTR SDDetInterrupt() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint8_t ucSendReq[REQ_QUE_SIZE];
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  if (digitalRead(PIN_SD_DET) == LOW) {
    // SDカードが挿入された
    if (g_ulSDSetFlag == false) {
      g_ulSDSetFlag = true;
      // SDカードの挿入を通知する
      pstSendReq->unReqType = FMG_SD_SET;
      pstSendReq->ulSize = 0;
      xQueueSendFromISR(g_pstFMGQueue, pstSendReq, &xHigherPriorityTaskWoken);
    }
  } else {
    // SDカードが抜かれた
    g_ulSDSetFlag = false;
    // SDカードの抜かれたことを通知する
    pstSendReq->unReqType = FMG_SD_CLR;
    pstSendReq->ulSize = 0;
    xQueueSendFromISR(g_pstFMGQueue, pstSendReq, &xHigherPriorityTaskWoken);
  }
}

void listDir(SdFat &sd, const char *dirname, uint8_t levels, uint8_t indent) {
  FsFile root = sd.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  FsFile file = root.openNextFile();
  char nameBuffer[128]; // ファイル名を格納するバッファ
  while (file) {
    for (uint8_t i = 0; i < indent; i++) {
      Serial.print("  ");
    }
    if (file.isDirectory()) {
      file.getName(nameBuffer, sizeof(nameBuffer));
      Serial.print("|-- ");
      Serial.println(nameBuffer);
      if (levels) {
        listDir(sd, nameBuffer, levels - 1, indent + 1);
      }
    } else {
      file.getName(nameBuffer, sizeof(nameBuffer));
      Serial.print("|-- ");
      Serial.print(nameBuffer);
      Serial.print("\tSIZE: ");
      Serial.println(file.fileSize());

      // 拡張子をチェックしてリストに追加
      String fileName = String(nameBuffer); // char配列をStringに変換
      if (fileName.endsWith(".html")) {
        htmlFiles.push_back(fileName);
      } else if (fileName.endsWith(".mid")) {
        midFiles.push_back(fileName);
      }
    }
    file = root.openNextFile();
  }
}
