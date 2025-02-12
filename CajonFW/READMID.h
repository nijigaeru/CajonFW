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
  FMG_OPEN_ANS                      , // ファイルオープン完了通知
  FMG_READ_ANS                      , // データリード完了通知
  READMID_PAUSE                     , // 一時停止
  READMID_RESTART                   , // 再開
  // READMID_RETURN ,                 // 巻き戻し 
  READMID_END                         // 終了
  
};

// taskのステート
enum READMIDState {
  ST_IDLE = READMID_ST_BEGIN        , // 起動時, 待機時ステート
  ST_WAIT_OPEN                      , // ファイルオープン要求応答待ち
  ST_WAIT_READ                      , // 初回データリード要求応答待ち
  ST_READ_HEADER_HEADER             , // ヘッダチャンク読み出し先頭 4B
  ST_READ_HEADER_LENGTH             , // ヘッダチャンク長 4B格納
  ST_READ_HEADER_FORMAT             , // フォーマット 2B
  ST_READ_HEADER_TRACK              , // トラック数 2B
  ST_READ_HEADER_TIME               , // 時間分解能 2B
  ST_READ_HEADER_END                , // 分解能判定 & ヘッダチャンク終了まで待機
  ST_READ_TRACK_HEADER              , // トラックチャンク読み出し先頭4B
  ST_READ_TRACK_LENGTH              , // データ長 4B
  ST_READ_TRACK_DELTA               , // デルタタイム取得 1~4B
  ST_READ_TRACK_WAIT_DELTA          , // デルタタイム待機
  ST_READ_TRACK_EVENT               , // イベント 1B
  ST_READ_TRACK_EVENT_SYSEX         , // SysExイベント処理
  ST_READ_TRACK_EVENT_META          , // メタイベント処理
  ST_READ_TRACK_EVENT_MIDI_STATE_1B , // MIDIイベント先頭1B読み出し
  ST_READ_TRACK_EVENT_MIDI_STATE_2B , // MIDIイベント残り2B読み出し
  ST_READ_TRACK_EVENT_MIDI_NOTE     , // MIDIイベントノーツ処理
  ST_PAUSE_WAIT_READ                , // 一時停止要求による停止中
  ST_PAUSE_REQ                      , // データリード要求応答待ちによる停止中 
  ST_END                              // 終了処理
};

// 要求の構造体
typedef struct STagREADMIDStartParam {
  char ucFileName[32];
}TS_READMIDStartParam;

// タスク内データ構造体
typedef struct STagREADMIDTaskParam {
  uint32_t  ucNumBuf;         // 現在のバッファ位置
  uint8_t   ucState;          // 現在のステート
  uint8_t   ucStatePause;     // ポーズ時のステート保持. 再開時に遷移する. 
  uint32_t  ucBufHold;        // 残データ格納用変数
  uint8_t   ucCntBufHold;     // 格納された残データ数カウント
  uint32_t  ucCntStartTrack;  // トラックチャンク開始位置(巻き戻し時に使う)
  uint32_t  ucCntDataRead;    // 現在までの合計リードデータ数
  uint8_t   ucCntReadFMG;     // FMGへのリード要求数
  uint32_t  ucCheckBuf;       // リード結果データ(上限4B)
}TS_READMIDSTaskParam;

// 先頭データリード関数
uint8_t ReadDataProc (TS_READMIDSTaskParam* pstTaskParam, uint8_t ucByteNum);
void ResetStructProc ( TS_READMIDSTaskParam* pstTaskParam );
extern void READMIDTask(void* pvParameters);

#endif


// メモ
        // 処理部メモ

// 済            // リード開始(ヘッダチャンク)
// 済            // ヘッダチャンク読み出し 4B
// 済            if ( ((g_ucBuffer[ucNumBuf]<<24) | (g_ucBuffer[ucNumBuf+1]<<16) | (g_ucBuffer[ucNumBuf+1]<<8) | g_ucBuffer[ucNumBuf+1] ) != 0x4D546864 )
// 済            {
// 済              // エラー時処理
// 済            }
// 済            // データ長 4B
// 済            ucLengthHeader = ((g_ucBuffer[ucNumBuf+1]<<24) | (g_ucBuffer[ucNumBuf+1]<<16) | (g_ucBuffer[ucNumBuf+1]<<8) | g_ucBuffer[ucNumBuf+1] );
            // フォーマット 2B
            ucFileFormat = ((g_ucBuffer[ucNumBuf+1] << 8) | g_ucBuffer[ucNumBuf+1] );
            // トラック数 2B
            ucTrackNum = ((g_ucBuffer[ucNumBuf+1] << 8) | g_ucBuffer[ucNumBuf+1] );
            // 時間分解能 2B
            ucTimeScale =  ((g_ucBuffer[ucNumBuf+1] << 8) | g_ucBuffer[ucNumBuf+1] );
            // 分解能判定
            if (ucTimeScale>>15 == 1) {// 時間分解能判定(MSBがHなら何分何秒何フレーム/Lなら何小節何拍)
              ucScaleMode = TIMESCALE_MODE_FLAME;
            } else {
              ucScaleMode = TIMESCALE_MODE_HAKU;
            }
            // データ長に達するまで待機
            while ( ucLengthHeader >= ucNumBuf )
            {
              ucNumBuf++;
            }

            // リード開始(トラックチャンク)
            for ( ucCntTrack=0; ucCntTrack<=ucTrackNum; ucCntTrack++)
            {
              // チャンクタイプ読み出し 4B
              if ( ((g_ucBuffer[ucNumBuf+1]<<24) | (g_ucBuffer[ucNumBuf+1]<<16) | (g_ucBuffer[ucNumBuf+1]<<8) | g_ucBuffer[ucNumBuf+1] ) != /**/ )
              {
                // エラー時処理
              }
              // データ長 4B
              ucLengthTrack = ((g_ucBuffer[ucNumBuf+1]<<24) | (g_ucBuffer[ucNumBuf+1]<<16) | (g_ucBuffer[ucNumBuf+1]<<8) | g_ucBuffer[ucNumBuf+1] );
              while ( /* EOL or データ長終了 */ )
              {
                // カウンタリセット
                ucCntDeltaTime = 0;
                // デルタタイムカウント
                while (ucCntDeltaTime < 4)
                {
                    // 格納
                    ucDeltaTime = ((ucDeltaTime<<8) | g_ucBuffer[ucNumBuf+1]);
                    if (ucDeltaTime>>7 == 1)  // 判定
                    {
                      break;
                    }
                    // カウントアップ
                    ucCntDeltaTim++;
                }
                // デルタタイム待機
                switch (/* カウンタ */)
                {
                case constant expression:
                    /* code */
                    break;
                
                default:
                    break;
                }
                // デルタタイム待機
                // イベント判定
                // // 1B目取得
                if (/* 1B目がF0 or F7 */)  // SysExイベント(可変長)
                {
                    //一旦考慮しない
                }
                else if (/* 1B目がFF */) // メタイベント(固定長のトラック情報)
                {
                    // 必要なものだけ考慮する. END, テンポ, キー, 拍子だけ？
                }
                else // if(/*1B目が8nH, 9nH, BnH, それ以外*/)  // MIDIイベント 3Byte
                {
                    if (condition)  // MSBがLの場合はステータスバイト省略(前値と同じ)
                    {
                        // 前置を1B目に,今読んだ値を2B目にいれる. (3B目も読む) 
                    }
                    else
                    {
                        // 残りの2Bを読む
                    }
                    // 処理
                    if (/*1B目が8nH*/) // ノートオフ
                    {
                        // なんの音を(1B)
                        // どのくらいの速さで(1B)
                    }
                    else if (/*1B目が9nH*/) // ノートオン
                    {
                        // なんの音を(1B)
                        // どのくらいの速さで=強さで(1B)
                    }
                    else if (/*1B目がBnH*/) // コントロールチェンジ
                    {
                        // 
                        // 
                    }
                    else    // 用意していないステータスバイト
                    {

                    }
                }
              } 
            }