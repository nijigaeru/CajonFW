#ifndef _READMID_H_
#define _READMID_H_

#include <queue.h>
#include <map>
#include <vector>
#include "REQ.h"

// キューの定義
extern QueueHandle_t g_pstREADMIDQueue;

// 要求の種類
enum READMIDRequestType {
  READMID_START  = READMID_REQ_BEGIN, // 動作開始要求
  READMID_PAUSE                     , // 一時停止
  // READMID_RETURN ,                 // 巻き戻し 
  READMID_SELF                      , // 動作継続要求
  READMID_END                       , // 終了
  READMID_PLAY_NOTS                 , // ノーツ単位の処理実行要求
};

// taskのステート
enum READMIDState {
  ST_IDLE = 0                               , // 起動時, 待機時ステート
  ST_WAIT_OPEN                              , // ファイルオープン要求応答待ち
  ST_WAIT_READ                              , // 初回データリード要求応答待ち
  ST_READ_HEADER_HEADER                     , // ヘッダチャンク読み出し先頭 4B
  ST_READ_HEADER_LENGTH                     , // ヘッダチャンク長 4B格納
  ST_READ_HEADER_FORMAT                     , // フォーマット 2B
  ST_READ_HEADER_TRACK                      , // トラック数 2B
  ST_READ_HEADER_TIME                       , // 時間分解能 2B
  ST_READ_TRACK_HEADER                      , // トラックチャンク読み出し先頭4B
  ST_READ_TRACK_LENGTH                      , // データ長 4B
  ST_READ_TRACK_DELTA                       , // デルタタイム取得 1~4B
  ST_READ_TRACK_WAIT_DELTA                  , // デルタタイム待機
  ST_READ_TRACK_EVENT                       , // イベント 1B
  ST_READ_TRACK_EVENT_SYSEX_F0              , // SysExイベント処理
  ST_READ_TRACK_EVENT_SYSEX_F7              , // SysExイベント処理
  ST_READ_TRACK_EVENT_SYSEX_LEN_F0          , // SysExイベント処理
  ST_READ_TRACK_EVENT_META                  , // メタイベント処理
  ST_READ_TRACK_EVENT_META_LENGTH           , // メタイベントデータ長読み出し
  ST_READ_TRACK_EVENT_META_TEMPO            , // テンポイベント テンポ
  ST_READ_TRACK_EVENT_META_TROUGH           , // テンポイベント 無視
  ST_READ_TRACK_EVENT_META_NEXT_TRACK       , // メタイベント処理
  ST_READ_TRACK_EVENT_MIDI_STATE_1B         , // MIDIイベント先頭1B読み出し
  ST_READ_TRACK_EVENT_MIDI_NOTE             , // MIDIイベントノーツ処理
  ST_PAUSE_WAIT_READ                        , // 一時停止要求による停止中
  ST_PAUSE_REQ                              , // データリード要求応答待ちによる停止中 
  ST_END                                      // 終了処理
};

// 要求の構造体
typedef struct STagREADMIDStartParam {
  char ucFileName[32];
}TS_READMIDStartParam;
typedef struct STagREADMIDNotsInfo {
  uint8_t ucScale;      // 音階
  uint8_t ucVelocity;   // 音量
}TS_READMIDPNotsInfo;

typedef struct STagREADMIDPlayNotsParam {
  uint16_t unNum;               // ノーツ数
  TS_READMIDPNotsInfo stInfo[8];   // ノーツ情報
}TS_READMIDPlayNotsParam;

// タスク内データ構造体
typedef struct STagREADMIDTaskParam {
  uint32_t  ulNumBuf;         // 現在のバッファ位置
  uint32_t  ulMaxBuf;         // 今あるデータの数
  uint8_t   ucState;          // 現在のステート
  uint8_t   ucStatePause;     // ポーズ時のステート保持. 再開時に遷移する. 
  uint8_t   ucStateTmp;       // 現在のステート
  uint32_t  ulCntStartTrack;  // トラックチャンク開始位置(巻き戻し時に使う)
  uint32_t  ulCntDataRead;    // 現在までの合計リードデータ数
  uint8_t   ucCntReadFMG;     // FMGへのリード要求数
  uint32_t  ulCheckBuf;       // リード結果データ(上限4B)
}TS_READMIDSTaskParam;

// 先頭データリード関数
uint32_t ReadDataProc (TS_READMIDSTaskParam* pstTaskParam, uint8_t ucByteNum);
void ResetStructProc ( TS_READMIDSTaskParam* pstTaskParam );
extern void READMIDTask(void* pvParameters );

#endif
