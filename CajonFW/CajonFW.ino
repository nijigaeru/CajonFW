#include <esp32-hal-gpio.h>
#include "hwinit.h"
#include "FMG.h"
#include "SLD.h"
#include "SW.h"
#include "READMID.h"
#include "HTTP_SERVER.h"

void setup() {
  Serial.begin(115200);
  Serial.println("setup started.");

  // HW初期化
  HwInit();

  // LED制御のためのキューを作成
  //ledQueue = xQueueCreate(10, sizeof(int));

  // HTTPサーバータスクをCore 1で実行
  xTaskCreatePinnedToCore(HTTPTask, "HTTPTask", 4096, NULL, 1, NULL, 1);
  // ファイル管理タスクの作成
  //! @note スタックはとりあえず少し多め。後で不要なのは減らしたほうが良いかも？
  xTaskCreatePinnedToCore(FMGTask, "FMGTask", 1024*4, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(READMIDTask, "READMIDTask", 1024*4, NULL, 2, NULL, 0);
  for (uint8_t ucFetCh = 1; ucFetCh <= 8; ucFetCh++) {
    // SLD制御タスクの作成
    xTaskCreatePinnedToCore(SLDTask, "SLDTask", 1024*4, NULL, 4, NULL, 0);
  }
  SWInit();

  Serial.println("setup finished.");
}

void loop() {
  // メインループは空のまま
  while(0)
  {
    
  }
}
