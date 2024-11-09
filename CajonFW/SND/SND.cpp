#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FreeRTOS.h>

// FETのピンアサイン
#define PIN_FET1    (46)
#define PIN_FET2    (43)
#define PIN_FET3    (42)
#define PIN_FET4    (44)
#define PIN_FET5    (41)
#define PIN_FET6    (40)
#define PIN_FET7    ( 0)
#define PIN_FET8    (39)

// ボタンのピンアサイン
#define PIN_SW1     ( 1)   // 再生要求
#define PIN_SW2     ( 2)   // 停止要求
#define PIN_SW3     ( 3)   // 次の曲要求
#define PIN_SW4     ( 4)   // 前の曲要求

// 拍の長さ（ミリ秒）
#define BEAT_LENGTH 500

// キューの定義
QueueHandle_t g_pstFMGQueue;
QueueHandle_t musicQueue;
QueueHandle_t g_pstFetQueue;

// 要求の種類
enum RequestType {
  TURN_ON_FET,
  READ_SD_FILE,
  PLAY_MUSIC,
  STOP_MUSIC,
  NEXT_MUSIC,
  PREVIOUS_MUSIC
};

// SDカード要求の構造体
struct TS_FMGRequest {
  RequestType type;
  char fileName[32];
};

// 音楽再生要求の構造体
struct MusicRequest {
  uint8_t fetPin;
  bool beatPattern[100]; // 仮の最大拍数、適宜調整
  uint16_t beatCount;
};

// FET要求の構造体
struct FETRequest {
  RequestType type;
  uint8_t fetPin;
  bool on;
  uint32_t duration;
};

// FET制御タスク
void FETControlTask(void* pvParameters) {
  uint8_t fetPin = *(uint8_t*)pvParameters;
  pinMode(fetPin, OUTPUT);
  digitalWrite(fetPin, LOW);

  while (true) {
    FETRequest request;
    if (xQueueReceive(g_pstFetQueue, &request, portMAX_DELAY) == pdPASS) {
      if (request.type == TURN_ON_FET && request.fetPin == fetPin) {
        if (request.on) {
          // FETをONにする
          digitalWrite(fetPin, HIGH);
          Serial.print("FET on pin ");
          Serial.print(fetPin);
          Serial.println(" turned ON.");
        } else {
          // FETをOFFにする
          digitalWrite(fetPin, LOW);
          Serial.print("FET on pin ");
          Serial.print(fetPin);
          Serial.println(" turned OFF.");
        }
        // 指定時間待つ
        vTaskDelay(pdMS_TO_TICKS(request.duration));
        // FETをOFFにする
        digitalWrite(fetPin, LOW);
        Serial.print("FET on pin ");
        Serial.print(fetPin);
        Serial.println(" turned OFF after duration.");
      }
    }
  }
}

// 音楽再生タスク
void MusicTask(void* pvParameters) {
  while (true) {
    MusicRequest request;
    if (xQueueReceive(musicQueue, &request, portMAX_DELAY) == pdPASS) {
      for (uint16_t i = 0; i < request.beatCount; i++) {
        if (request.beatPattern[i]) {
          FETRequest fetRequest = { TURN_ON_FET, request.fetPin, true, BEAT_LENGTH };
          xQueueSend(g_pstFetQueue, &fetRequest, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(BEAT_LENGTH));
      }
    }
  }
}

// SDカード管理タスク
void SDTask(void* pvParameters) {
  while (true) {
    TS_FMGRequest request;
    if (xQueueReceive(g_pstFMGQueue, &request, portMAX_DELAY) == pdPASS) {
      if (request.type == READ_SD_FILE) {
        File file = SD.open(request.fileName);
        if (file) {
          Serial.println("File opened successfully.");
          
          // ヘッダ部の読み込み
          String line = file.readStringUntil('\n');
          Serial.println("Header: " + line);
          
          // コード部の処理
          while (file.available()) {
            line = file.readStringUntil('\n');
            Serial.println("Code: " + line);
            
            // パース処理
            if (line.startsWith('(')) {
              int fetPin = line.substring(1, line.indexOf(')')).toInt();
              String code = line.substring(line.indexOf('{') + 1, line.indexOf('}'));

              MusicRequest musicRequest;
              musicRequest.fetPin = fetPin;
              musicRequest.beatCount = 0;
              
              // 拍ごとの処理
              for (int i = 0; i < code.length(); i++) {
                if (code[i] == '1') {
                  musicRequest.beatPattern[musicRequest.beatCount] = true;
                } else {
                  musicRequest.beatPattern[musicRequest.beatCount] = false;
                }
                musicRequest.beatCount++;
              }
              xQueueSend(musicQueue, &musicRequest, portMAX_DELAY);
            }
          }
          file.close();
        } else {
          Serial.println("Failed to open file.");
        }
      }
    }
  }
}

// ボタン制御タスク
void ButtonTask(void* pvParameters) {
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP);
  pinMode(PIN_SW4, INPUT_PULLUP);
  uint8_t previousSW1State = HIGH;
  uint8_t previousSW2State = HIGH;
  uint8_t previousSW3State = HIGH;
  uint8_t previousSW4State = HIGH;

  while (true) {
    uint8_t currentSW1State = digitalRead(PIN_SW1);
    uint8_t currentSW2State = digitalRead(PIN_SW2);
    uint8_t currentSW3State = digitalRead(PIN_SW3);
    uint8_t currentSW4State = digitalRead(PIN_SW4);

    if (currentSW1State == LOW && previousSW1State == HIGH) {
      MusicRequest request = {0};  // Play music request
      xQueueSend(musicQueue, &request, portMAX_DELAY);
      Serial.println("Play request sent.");
    }
    if (currentSW2State == LOW && previousSW2State == HIGH) {
      MusicRequest request = {0};  // Stop music request
      xQueueSend(musicQueue, &request, portMAX_DELAY);
      Serial.println("Stop request sent.");
    }
    if (currentSW3State == LOW && previousSW3State == HIGH) {
      MusicRequest request = {0};  // Next music request
      xQueueSend(musicQueue, &request, portMAX_DELAY);
      Serial.println("Next music request sent.");
    }
    if (currentSW4State == LOW && previousSW4State == HIGH) {
      MusicRequest request = {0};  // Previous music request
      xQueueSend(musicQueue, &request, portMAX_DELAY);
      Serial.println("Previous music request sent.");
    }

    previousSW1State = currentSW1State;
    previousSW2State = currentSW2State;
    previousSW3State = currentSW3State;
    previousSW4State = currentSW4State;

    vTaskDelay(pdMS_TO_TICKS(10)); // デバウンス用
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // シリアルポートが接続されるのを待つ
  }

  // FETピンの初期化
  static uint8_t fetPins[] = { PIN_FET1, PIN_FET2, PIN_FET3, PIN_FET4, PIN_FET5, PIN_FET6, PIN_FET7, PIN_FET8 };
  for (int i = 0; i < 8; i++) {
    pinMode(fetPins[i], OUTPUT);
    digitalWrite(fetPins[i], LOW);
  }

  // SDカードの初期化
  if (!SD.begin(13)) { // Use appropriate CS pin for your setup
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  // キューの作成
  g_pstFMGQueue = xQueueCreate(10, sizeof(TS_FMGRequest));
  musicQueue = xQueueCreate(10, sizeof(MusicRequest));
  g_pstFetQueue = xQueueCreate(20, sizeof(FETRequest));
  if (g_pstFMGQueue == NULL || musicQueue == NULL || g_pstFetQueue == NULL) {
    Serial.println("Failed to create queue.");
    return;
  }

  // FET制御タスクの作成
  for (int i = 0; i < 8; i++) {
    xTaskCreate(FETControlTask, "FETTask", 2048, &fetPins[i], 1, NULL);
  }

  // 音楽再生タスクの作成
  xTaskCreate(MusicTask, "MusicTask", 4096, NULL, 1, NULL);

  // SDカード管理タスクの作成
  xTaskCreate(SDTask, "SDTask", 4096, NULL, 1, NULL);

  // ボタンタスクの作成
  xTaskCreate(ButtonTask, "ButtonTask", 2048, NULL, 1, NULL);

  // SDカード読み込み要求を送信
  TS_FMGRequest sdRequest = { READ_SD_FILE, "test.txt" };
  xQueueSend(g_pstFMGQueue, &sdRequest, portMAX_DELAY);
}

void loop() {
  // メインループは空
}
