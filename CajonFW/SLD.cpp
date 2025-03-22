#include <Arduino.h>
#include <esp32-hal-gpio.h>
#include <queue.h>
#include "SLD.h"
#include "pinasign.h"

// 0 : 打面（中央）
// 1 : 打面（上）
// 2 : 打面（角）
// 3 : 木片
// 4 : タンバリン
// 5 : 円盤
// 6 : シンバル
// 7 : ー


// キューの定義
QueueHandle_t g_pstSLDQueue[SLD_NUM];
bool g_ulSLDInitFlg[SLD_NUM] = {false};
uint8_t fetPins[] = { PIN_FET1, PIN_FET2, PIN_FET3, PIN_FET4, PIN_FET5, PIN_FET6, PIN_FET7, PIN_FET8 };
uint32_t g_ulSldOnTime[] = { 10, 10, 10, 10, 10, 10, 10, 10}; // ソレノイド駆動時間（ミリ秒）
uint32_t g_ulBeginDelay[] = { 10, 10, 10, 15, 0, 15, 5, 5 };
uint8_t g_ucMinPower[] = { 80, 80, 80, 80, 70, 70, 70, 100 };
uint32_t g_ulFetCount = 1;
const double  PWM_Hz = 2000;   // PWM周波数
const uint8_t PWM_level = 8; // PWM分解能 16bit(1～256)

// ソレノイド駆動タスク
void SLDTask(void* pvParameters) {
  //uint8_t ucFetCh = *(uint8_t*)pvParameters;
  uint8_t ucFetCh = g_ulFetCount ;
  g_ulFetCount ++;
  uint8_t ucSLDPin = fetPins[ucFetCh-1];

  if (g_ulSLDInitFlg[ucFetCh-1])
  {
    Serial.print("SLD already initialized.");
    Serial.println(ucFetCh);
    return;
  }

  // ピンの初期化
  pinMode(ucSLDPin, OUTPUT);
  // チャンネルと周波数の分解能を設定
  ledcSetup(ucFetCh, PWM_Hz, PWM_level);
  // ピンとチャンネルの設定
  ledcAttachPin(ucSLDPin, ucFetCh);
  ledcWrite(ucFetCh,0);

  // キューの作成
  g_pstSLDQueue[ucFetCh-1] = xQueueCreate(REQ_QUE_NUM, REQ_QUE_SIZE);
  if (g_pstSLDQueue[ucFetCh-1] == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  // 初期化完了フラグ
  g_ulSLDInitFlg[ucFetCh-1] = true;
  Serial.print("SLD initialized.");
  Serial.println(ucFetCh);
  while (true) {
    uint8_t ucRecvReq[REQ_QUE_SIZE];
    TS_Req* pstRecvReq = (TS_Req*)ucRecvReq;
    if (xQueueReceive(g_pstSLDQueue[ucFetCh-1], pstRecvReq, portMAX_DELAY) == pdPASS) {
      if (pstRecvReq->unReqType == SLD_TURN_ON) {
        // ちょっと待つ
        vTaskDelay(pdMS_TO_TICKS(g_ulBeginDelay[ucFetCh-1]));
        // SLDをONにする
        TS_SLDOnParam* pstSLDOnParam = (TS_SLDOnParam*)pstRecvReq->ucParam;
        ledcWrite(ucFetCh, g_ucMinPower[ucFetCh-1] + (uint32_t)(255 - g_ucMinPower[ucFetCh-1]) * pstSLDOnParam->ucPower / 127);
        // Serial.print("SLD(");
        // Serial.print(ucFetCh);
        // Serial.print("),power(");
        // Serial.print(pstSLDOnParam->ucPower);
        // Serial.println(") turned ON.");
        // 一定時間待つ
        vTaskDelay(pdMS_TO_TICKS(g_ulSldOnTime[ucFetCh-1]));
        // SLDをOFFにする
        ledcWrite(ucFetCh,0);
        // Serial.print("SLD(");
        // Serial.print(ucFetCh);
        // Serial.println(") turned OFF.");
      }
    }
  }
}

// MIDIノート番号 → 割り当てる楽器（255 は無視）
const uint8_t drum_mapping[128] = {
  255, // 0   (未使用)
  255, // 1   (未使用)
  255, // 2   (未使用)
  255, // 3   (未使用)
  255, // 4   (未使用)
  255, // 5   (未使用)
  255, // 6   (未使用)
  255, // 7   (未使用)
  255, // 8   (未使用)
  255, // 9   (未使用)
  255, // 10  (未使用)
  255, // 11  (未使用)
  255, // 12  (未使用)
  255, // 13  (未使用)
  255, // 14  (未使用)
  255, // 15  (未使用)
  255, // 16  (未使用)
  255, // 17  (未使用)
  255, // 18  (未使用)
  255, // 19  (未使用)
  255, // 20  (未使用)
  255, // 21  (未使用)
  255, // 22  (未使用)
  255, // 23  (未使用)
  255, // 24  (未使用)
  255, // 25  (未使用)
  255, // 26  (未使用)
  255, // 27  (未使用)
  255, // 28  (未使用)
  255, // 29  (未使用)
  255, // 30  (未使用)
  255, // 31  (未使用)
  3,   // 32  Sticks → 木片
  255, // 33  (未使用)
  3,   // 34  Bass Drum 2? (要確認) → 木片
  0,   // 35  Bass Drum 2 → 打面（中央）
  0,   // 36  Bass Drum 1 → 打面（中央）
  3,   // 37  Side Stick → 木片
  255, // 38  (未使用)
  3,   // 39  Hand Clap → 木片
  2,   // 40  Electric Snare → 打面（角）
  1,   // 41  Low Floor Tom → 打面（上）
  5,   // 42  Closed Hi-Hat → 円盤
  1,   // 43  High Floor Tom → 打面（上）
  5,   // 44  Pedal Hi-Hat → 円盤
  1,   // 45  Low Tom → 打面（上）
  4,   // 46  Open Hi-Hat → タンバリン
  1,   // 47  Mid Tom → 打面（上）
  1,   // 48  High Tom → 打面（上）
  6,   // 49  Crash Cymbal 1 → シンバル
  1,   // 50  High Tom 1 → 打面（上）
  5,   // 51  Ride Cymbal 1 → 円盤
  5,   // 52  Chinese Cymbal → 円盤
  5,   // 53  Ride Bell → 円盤
  4,   // 54  Tambourine → タンバリン
  5,   // 55  Splash Cymbal → 円盤
  255, // 56  (未使用)
  6,   // 57  Crash Cymbal 2 → シンバル
  255, // 58  (未使用)
  5,   // 59  Ride Cymbal 2 → 円盤
  255, // 60  (未使用)
  255, // 61  (未使用)
  255, // 62  (未使用)
  255, // 63  (未使用)
  255, // 64  (未使用)
  255, // 65  (未使用)
  255, // 66  (未使用)
  255, // 67  (未使用)
  255, // 68  (未使用)
  255, // 69  (未使用)
  4,   // 70  Maracas → タンバリン
  255, // 71  (未使用)
  255, // 72  (未使用)
  255, // 73  (未使用)
  255, // 74  (未使用)
  3,   // 75  Claves → 木片
  3,   // 76  Hi Wood Block → 木片
  3,   // 77  Low Wood Block → 木片
  255, // 78  (未使用)
  255, // 79  (未使用)
  255, // 80  (未使用)
  255, // 81  (未使用)
  255, // 82  (未使用)
  255, // 83  (未使用)
  255, // 84  (未使用)
  255, // 85  (未使用)
  255, // 86  (未使用)
  255, // 87  (未使用)
  255, // 88  (未使用)
  255, // 89  (未使用)
  255, // 90  (未使用)
  255, // 91  (未使用)
  255, // 92  (未使用)
  255, // 93  (未使用)
  255, // 94  (未使用)
  255, // 95  (未使用)
  255, // 96  (未使用)
  255, // 97  (未使用)
  255, // 98  (未使用)
  255, // 99  (未使用)
  255, // 100 (未使用)
  255, // 101 (未使用)
  255, // 102 (未使用)
  255, // 103 (未使用)
  255, // 104 (未使用)
  255, // 105 (未使用)
  255, // 106 (未使用)
  255, // 107 (未使用)
  255, // 108 (未使用)
  255, // 109 (未使用)
  255, // 110 (未使用)
  255, // 111 (未使用)
  255, // 112 (未使用)
  255, // 113 (未使用)
  255, // 114 (未使用)
  255, // 115 (未使用)
  255, // 116 (未使用)
  255, // 117 (未使用)
  255, // 118 (未使用)
  255, // 119 (未使用)
  255, // 120 (未使用)
  255, // 121 (未使用)
  255, // 122 (未使用)
  255, // 123 (未使用)
  255, // 124 (未使用)
  255, // 125 (未使用)
  255, // 126 (未使用)
  255, // 127 (未使用)
};

// MIDIノート番号を対応する打面に変換
uint8_t process_drum_hit(uint8_t note) {
  if (note >= 128) return 255; // 無効なノート番号
  return drum_mapping[note]; // 直接配列参照
}

