#include <WiFi.h>
#include <WebServer.h>
#include <M5Unified.h>

const char* ssid = "M5StampAP"; // アクセスポイントのSSID
const char* password = "your_PASSWORD"; // アクセスポイントのパスワード

WebServer server(80);

const int ledPin = 2; // LEDが接続されているピン番号
uint8_t g_ucLedState = 0;
uint8_t g_ucDeviceConnected = 0;
int display_rotation = 1;                         // 画面向きの初期化
// LED制御のためのキュー
QueueHandle_t ledQueue;

void display_init();
void display_update();

void handleRoot() {
  String html = "<html><body><h1>LED Control</h1><button onclick=\"location.href='/led/on'\">Turn On</button><button onclick=\"location.href='/led/off'\">Turn Off</button></body></html>";
  server.send(200, "text/html", html);
}

void handleLedOn() {
  int command = 1; // LED ONコマンド
  xQueueSend(ledQueue, &command, portMAX_DELAY);
  server.send(200, "text/html", "<html><body><h1>LED is ON</h1><a href='/'>Go Back</a></body></html>");
}

void handleLedOff() {
  int command = 0; // LED OFFコマンド
  xQueueSend(ledQueue, &command, portMAX_DELAY);
  server.send(200, "text/html", "<html><body><h1>LED is OFF</h1><a href='/'>Go Back</a></body></html>");
}

void httpServerTask(void *pvParameters) {
  // アクセスポイントとして動作するように設定
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);

  server.begin();
  Serial.println("HTTP server started");

  while (true) {
    server.handleClient();
    delay(10);
  }
}

void ledControlTask(void *pvParameters) {
  int command;
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  while (true) {
    if (xQueueReceive(ledQueue, &command, portMAX_DELAY) == pdPASS) {
      g_ucLedState = command;
      if (command == 1) {
        digitalWrite(ledPin, HIGH);
      } else {
        digitalWrite(ledPin, LOW);
      }
      display_update();
    }
  }
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  display_init();
  
  // LED制御のためのキューを作成
  ledQueue = xQueueCreate(10, sizeof(int));

  // HTTPサーバータスクをCore 1で実行
  xTaskCreatePinnedToCore(httpServerTask, "HTTP Server Task", 4096, NULL, 1, NULL, 1);

  // LED制御タスクをCore 0で実行
  xTaskCreatePinnedToCore(ledControlTask, "LED Control Task", 1024, NULL, 1, NULL, 0);
}

void loop() {
  // メインループは空のまま
}

void display_init()
{
  switch (M5.getBoard())
  {
    case m5gfx::board_M5StackCore2:
    case m5gfx::board_M5Stack:
    case m5gfx::board_M5StickCPlus2:
      M5.Lcd.setRotation(display_rotation);           // 画面の向きを変更

      M5.Lcd.setTextSize(1);                          // テキストサイズの指定
      M5.Lcd.fillScreen(BLACK);                       // 画面の塗りつぶし　Screen fill.
      M5.Lcd.setTextColor(WHITE, BLACK);              // テキストカラーの設定
      M5.Lcd.setCursor(5, 5, 4);                      // カーソル位置とフォントの設定
      M5.Lcd.print("OFF");                 // ディスプレイに表示（プログラム名）
      
      M5.update();
      break;
    default:
      break;
  }
}

void display_update()
{
  switch (M5.getBoard())
  {
    case m5gfx::board_M5StackCore2:
    case m5gfx::board_M5Stack:
    case m5gfx::board_M5StickCPlus2:
      M5.Lcd.fillRect(5, 5, 90, 50, BLACK);       // デューティ比表示域を塗りつぶし
      M5.Lcd.setTextColor(WHITE, BLACK);              // テキストカラーの設定
      M5.Lcd.setCursor(5, 5, 4);                      // カーソル位置とフォントの設定
      switch (g_ucLedState)
      {
      case 1:
        M5.Lcd.print("ON");                 // ディスプレイに表示（プログラム名）
        break;
      case 0:
        M5.Lcd.print("OFF");                 // ディスプレイに表示（プログラム名）
        break;
      default:
        break;
      }
      M5.Lcd.fillRect(200, 5, 90, 30, BLACK);       // デューティ比表示域を塗りつぶし
      M5.Lcd.setTextColor(WHITE, BLACK);              // テキストカラーの設定
      M5.Lcd.setCursor(200, 5);                      // カーソル位置とフォントの設定
      if (g_ucDeviceConnected)
      {
        M5.Lcd.print("Connect");                 // ディスプレイに表示（デューティ
      }
      else
      {
        M5.Lcd.print("Discnnect");                 // ディスプレイに表示（デューティ
      }
      break;
    default:
      break;
  }
  M5.update();
}