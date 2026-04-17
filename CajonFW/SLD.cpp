#include <Arduino.h>
#include <esp32-hal-gpio.h>
#include <queue.h>
#include "SLD.h"
#include "pinasign.h"

// 0 : 打面（中央）
// 1 : 打面（上）
// 2 : 打面（角）
// 3 : マラカス
// 4 : タンバリン
// 5 : 円盤
// 6 : 8インチシンバル
// 7 : 10インチシンバル


// キューの定義
QueueHandle_t g_pstSLDQueue[SLD_NUM];
bool g_ulSLDInitFlg[SLD_NUM] = {false};
uint8_t fetPins[] = { PIN_FET1, PIN_FET2, PIN_FET3, PIN_FET4, PIN_FET5, PIN_FET6, PIN_FET7, PIN_FET8 };
uint32_t g_ulSldOnTime[] = { 10, 10, 10, 10, 10, 10, 10, 10}; // ソレノイド駆動時間（ミリ秒）
uint32_t g_ulBeginDelay[] = { 10, 10, 10, 15, 0, 15, 5, 5 };
uint8_t g_ucMinPower[] = { 80, 80, 80, 120, 70, 70, 90, 90 };
uint8_t g_ucMaxPower[] = { 255, 255, 255, 255, 255, 255, 255, 255 };
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
  ledcSetup(ucFetCh-1, PWM_Hz, PWM_level);
  // ピンとチャンネルの設定
  ledcAttachPin(ucSLDPin, ucFetCh-1);
  ledcWrite(ucFetCh-1,0);

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
        ledcWrite(ucFetCh-1, g_ucMinPower[ucFetCh-1] + (uint32_t)(g_ucMaxPower[ucFetCh-1] - g_ucMinPower[ucFetCh-1]) * pstSLDOnParam->ucPower / 127);
        // Serial.print("SLD(");
        // Serial.print(ucFetCh);
        // Serial.print("),power(");
        // Serial.print(pstSLDOnParam->ucPower);
        // Serial.println(") turned ON.");
        // 一定時間待つ
        vTaskDelay(pdMS_TO_TICKS(g_ulSldOnTime[ucFetCh-1]));
        // SLDをOFFにする
        ledcWrite(ucFetCh-1,0);
        // Serial.print("SLD(");
        // Serial.print(ucFetCh);
        // Serial.println(") turned OFF.");
      }
    }
  }
}

// MIDIノート番号 → 割り当てる楽器（255 は無視）
// 2つの楽器を割り当てるため2次元配列に変更
const uint8_t drum_mapping[128][2] = {
  {255, 255}, // 0   (未使用)
  {255, 255}, // 1   (未使用)
  {255, 255}, // 2   (未使用)
  {255, 255}, // 3   (未使用)
  {255, 255}, // 4   (未使用)
  {255, 255}, // 5   (未使用)
  {255, 255}, // 6   (未使用)
  {255, 255}, // 7   (未使用)
  {255, 255}, // 8   (未使用)
  {255, 255}, // 9   (未使用)
  {255, 255}, // 10  (未使用)
  {255, 255}, // 11  (未使用)
  {255, 255}, // 12  (未使用)
  {255, 255}, // 13  (未使用)
  {255, 255}, // 14  (未使用)
  {255, 255}, // 15  (未使用)
  {255, 255}, // 16  (未使用)
  {255, 255}, // 17  (未使用)
  {255, 255}, // 18  (未使用)
  {255, 255}, // 19  (未使用)
  {255, 255}, // 20  (未使用)
  {255, 255}, // 21  (未使用)
  {255, 255}, // 22  (未使用)
  {255, 255}, // 23  (未使用)
  {255, 255}, // 24  (未使用)
  {255, 255}, // 25  (未使用)
  {255, 255}, // 26  (未使用)
  {255, 255}, // 27  (未使用)
  {255, 255}, // 28  (未使用)
  {255, 255}, // 29  (未使用)
  {255, 255}, // 30  (未使用)
  {255, 255}, // 31  (未使用)
  {3,   255}, // 32  Sticks → マラカス
  {255, 255}, // 33  (未使用)
  {1,   255}, // 34  Bass Drum 2? (要確認) → 打面（上）
  {0,   255}, // 35  Bass Drum 2 → 打面（中央）
  {0,   255}, // 36  Bass Drum 1 → 打面（中央）
  {3,   255}, // 37  Side Stick → マラカス
  {2,   3  }, // 38  Acoustic Snare → 打面（角）, マラカス
  {3,   255}, // 39  Hand Clap → マラカス
  {2,   3  }, // 40  Electric Snare → 打面（角）, マラカス
  {1,   255}, // 41  Low Floor Tom → 打面（上）
  {5,   255}, // 42  Closed Hi-Hat → 円盤
  {1,   255}, // 43  High Floor Tom → 打面（上）
  {5,   255}, // 44  Pedal Hi-Hat → 円盤
  {1,   255}, // 45  Low Tom → 打面（上）
  {4,   5  }, // 46  Open Hi-Hat → タンバリン, 円盤
  {1,   255}, // 47  Mid Tom → 打面（上）
  {1,   255}, // 48  High Tom → 打面（上）
  {7,   255}, // 49  Crash Cymbal 1 → 10インチシンバル
  {1,   255}, // 50  High Tom 1 → 打面（上）
  {5,   255}, // 51  Ride Cymbal 1 → 円盤
  {7,   255}, // 52  Chinese Cymbal → 10インチシンバル
  {5,   255}, // 53  Ride Bell → 円盤
  {4,   255}, // 54  Tambourine → タンバリン
  {6,   255}, // 55  Splash Cymbal → 8インチシンバル
  {3,   255}, // 56  Cowbell -> マラカス
  {6,   255}, // 57  Crash Cymbal 2 → 8インチシンバル
  {3,   255}, // 58  Vibraslap -> マラカス
  {5,   255}, // 59  Ride Cymbal 2 → 円盤
  {3,   255}, // 60  Hight Bongo → マラカス
  {255, 255}, // 61  (未使用)
  {3,   255}, // 62  Mute Hi Conga → マラカス
  {3,   255}, // 63  Open Hi Conga → マラカス
  {3,   255}, // 64  Low Conga → マラカス
  {3,   255}, // 65  High Tambale → マラカス
  {3,   255}, // 66  Low Tambale → マラカス
  {255, 255}, // 67  (未使用)
  {5,   255}, // 68  Cabasa -> 円盤
  {255, 255}, // 69  (未使用)
  {4,   255}, // 70  Maracas → タンバリン
  {255, 255}, // 71  (未使用)
  {255, 255}, // 72  (未使用)
  {255, 255}, // 73  (未使用)
  {255, 255}, // 74  (未使用)
  {3,   255}, // 75  Claves → マラカス
  {3,   255}, // 76  Hi Wood Block → マラカス
  {3,   255}, // 77  Low Wood Block → マラカス
  {255, 255}, // 78  (未使用)
  {255, 255}, // 79  (未使用)
  {255, 255}, // 80  (未使用)
  {255, 255}, // 81  (未使用)
  {255, 255}, // 82  (未使用)
  {255, 255}, // 83  (未使用)
  {255, 255}, // 84  (未使用)
  {255, 255}, // 85  (未使用)
  {255, 255}, // 86  (未使用)
  {255, 255}, // 87  (未使用)
  {255, 255}, // 88  (未使用)
  {255, 255}, // 89  (未使用)
  {255, 255}, // 90  (未使用)
  {255, 255}, // 91  (未使用)
  {255, 255}, // 92  (未使用)
  {255, 255}, // 93  (未使用)
  {255, 255}, // 94  (未使用)
  {255, 255}, // 95  (未使用)
  {255, 255}, // 96  (未使用)
  {255, 255}, // 97  (未使用)
  {255, 255}, // 98  (未使用)
  {255, 255}, // 99  (未使用)
  {255, 255}, // 100 (未使用)
  {255, 255}, // 101 (未使用)
  {255, 255}, // 102 (未使用)
  {255, 255}, // 103 (未使用)
  {255, 255}, // 104 (未使用)
  {255, 255}, // 105 (未使用)
  {255, 255}, // 106 (未使用)
  {255, 255}, // 107 (未使用)
  {255, 255}, // 108 (未使用)
  {255, 255}, // 109 (未使用)
  {255, 255}, // 110 (未使用)
  {255, 255}, // 111 (未使用)
  {255, 255}, // 112 (未使用)
  {255, 255}, // 113 (未使用)
  {255, 255}, // 114 (未使用)
  {255, 255}, // 115 (未使用)
  {255, 255}, // 116 (未使用)
  {255, 255}, // 117 (未使用)
  {255, 255}, // 118 (未使用)
  {255, 255}, // 119 (未使用)
  {255, 255}, // 120 (未使用)
  {255, 255}, // 121 (未使用)
  {255, 255}, // 122 (未使用)
  {255, 255}, // 123 (未使用)
  {255, 255}, // 124 (未使用)
  {255, 255}, // 125 (未使用)
  {255, 255}, // 126 (未使用)
  {255, 255}, // 127 (未使用)
};

// MIDIノート番号を対応する打面に変換（1つ目）
uint8_t process_drum_hit(uint8_t note) {
  if (note >= 128) return 255; // 無効なノート番号
  return drum_mapping[note][0];
}

// MIDIノート番号を対応する打面に変換（2つ目）
uint8_t process_drum_hit_2(uint8_t note) {
  if (note >= 128) return 255; // 無効なノート番号
  return drum_mapping[note][1];
}

