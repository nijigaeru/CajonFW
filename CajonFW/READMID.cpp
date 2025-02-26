#include <Arduino.h>
#include "REQ.h"
#include "READMID.h"
#include "FMG.h"
#include "SLD.h"
/******** macro ***** */
#define BUFSIZE              1024
#define TIMESCALE_MODE_FLAME 1
#define TIMESCALE_MODE_BEATS 0
#define RET_OK               1
#define RET_NG               0

/******** global variable  ***** */
uint8_t g_ucBuffer[BUFSIZE];

// キューの定義
QueueHandle_t g_pstREADMIDQueue;

/******** function declaration ***** */
void READMIDTask(void* pvParameters);   //MIDファイル処理タスク

/************************** */
// タスク
void READMIDTask(void* pvParameters) {

  // キューの作成
  g_pstREADMIDQueue = xQueueCreate(REQ_QUE_NUM, REQ_QUE_SIZE);
  if (g_pstREADMIDQueue == NULL) {
    USBSerial.println("Failed to create queue.");
    return;
  }

  // 内部パラメータ
  TS_READMIDSTaskParam stTaskParam;

  // 要求
  static uint8_t ucRecvReq[REQ_QUE_SIZE]; // スタックを消費したくないのでstatic
  TS_Req* pstRecvReq = (TS_Req*)ucRecvReq;
  static uint8_t ucSendReq[REQ_QUE_SIZE]; // スタックを消費したくないのでstatic
  TS_Req* pstSendReq = (TS_Req*)ucSendReq;

  // 内部変数
  uint32_t  ulLengthHeader;     // ヘッダチャンク長 4B
  uint16_t  unFileFormat;       // ファイルフォーマット 2B
  uint16_t  unTrackNum;         // トラック数 2B
  uint16_t  unTimeScale;        // 時間分解能 2B
  uint8_t   ucScaleMode;        // 時間分解能モード
  uint8_t   ucCntTrack;         // トラック数カウント
  uint32_t  ulLengthTrack;      // トラックチャンク長 4B
  uint32_t  ulCntDeltaTime;     // デルタタイムカウント数 
  uint32_t  ulDeltaTime;        // デルタタイム 1~4B
  uint32_t  ulCntWaitDeltaTime; // 仮のデルタタイム待機カウンタ 4B
  uint8_t   ucMidiEvent;        // MIDIイベント 1B
  uint8_t   ucMidiScale;        // MIDI音階 1B
  uint8_t   ucMidiVelocity;     // MIDIベロシティ 1B
  uint8_t   ucTempLength;       // テンポデータ長 1B
  uint32_t  ulTempBPM;          // テンポデータusオーダー 可変長
  uint32_t  ulWaitTimeMsec;     // 待機時間 4B
   
  // 内部構造帯リセット
  ResetStructProc ( &stTaskParam );

  // 初期値セット
  unTimeScale = 480;    // 480tick
  ulTempBPM   = 500000; // 120BPM(500,000us)

  while (true) {

    // ファイルオープン要求準備
    TS_READMIDStartParam *pstStart = (TS_READMIDStartParam *)pstRecvReq->ucParam;
    TS_FMGOpenParam* pstOpen = (TS_FMGOpenParam*)pstSendReq->ucParam;

    // ファイルデータ取得要求準備
    TS_FMGReadParam* pstRead = (TS_FMGReadParam*)pstSendReq->ucParam;

    // SLD要求準備
    pstSendReq->unReqType = SLD_TURN_ON;
    TS_SLDOnParam* pstSLDParam = (TS_SLDOnParam*)pstSendReq->ucParam;
    // pstSLDParam->ucPower = 255;
    // xQueueSend( g_pstSLDQueue[/*0~7*/], pstSendReq );

    // ReadMidi要求準備

    // 要求処理
    if (xQueueReceive(g_pstREADMIDQueue, pstRecvReq, portMAX_DELAY) == pdPASS) {
      switch (pstRecvReq->unReqType)
      {
        case READMID_START:/* 再生開始 */
          USBSerial.println("READMID_START request");
          switch (stTaskParam.ucState)
          {
            case ST_IDLE:
              // リセット
              ResetStructProc ( &stTaskParam );

              // ファイルオープン要求
              pstSendReq->unReqType = FMG_OPEN;
              pstSendReq->pstAnsQue = g_pstREADMIDQueue;
              pstSendReq->ulSize = sizeof(TS_FMGOpenParam);
              memcpy(pstOpen->ucFileName, pstStart->ucFileName, sizeof(pstStart->ucFileName));
              xQueueSend(g_pstFMGQueue, pstSendReq, 100);

              stTaskParam.ucState = ST_WAIT_OPEN;
              break;
            
            default:
              // 何もしない
              break;
          } // endcase
          break;

        case FMG_OPEN_ANS:/* オープン完了 */
          if(pstRecvReq->unError == 0)
          {
            switch (stTaskParam.ucState)
            {
              case ST_WAIT_OPEN:
                // ファイルデータ取得要求
                pstSendReq->unReqType = FMG_READ;
                pstSendReq->pstAnsQue = g_pstREADMIDQueue;
                pstSendReq->ulSize = sizeof(TS_FMGReadParam);
                pstRead->pucBuffer = g_ucBuffer;
                pstRead->ulLength = BUFSIZE;
                xQueueSend(g_pstFMGQueue, pstSendReq, 100);
                
                stTaskParam.ucState = ST_WAIT_READ;
                break;
              default:
                // 何もしない
                break;
            } // endcase
          }
          break;

        case FMG_READ_ANS   :/* リード完了要求 */
          if(pstRecvReq->unError == 0)
          {
            switch (stTaskParam.ucState)
            {
            case ST_WAIT_READ:
              // カウンタリセット
              stTaskParam.ulNumBuf = 0;
              // リード開始
              stTaskParam.ucState  = ST_READ_HEADER_HEADER;
              break;

            case ST_PAUSE_WAIT_READ:
              // カウンタリセット
              stTaskParam.ulNumBuf = 0;
              // リード再開
              stTaskParam.ucState  = stTaskParam.ucStatePause;
              break;

            default:
              // 何もしない
              break;
            } // endcase

            // 動作継続要求を送る
            pstSendReq->unReqType = READMID_SELF;
            pstSendReq->pstAnsQue = NULL;
            pstSendReq->ulSize = 0;
            xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
          }
          break;

        case READMID_PAUSE  :/* 一時停止要求 */
          if(pstRecvReq->unError == 0)
          {
            stTaskParam.ucStatePause     = stTaskParam.ucState; // ステート保持
            stTaskParam.ucState          = ST_PAUSE_REQ;
          }
          break;

        case READMID_RESTART:/* 再生再開要求 */
          if(pstRecvReq->unError == 0)
          {
            stTaskParam.ucState          = stTaskParam.ucStatePause;  // ステート再開
            // 動作継続要求を送る
            pstSendReq->unReqType = READMID_SELF;
            pstSendReq->pstAnsQue = NULL;
            pstSendReq->ulSize = 0;
            xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
          }
          break;

        case READMID_END    :/* 終了要求 */
          if(pstRecvReq->unError == 0)
          {
            // リセット
            ResetStructProc ( &stTaskParam );
          }
          // ファイルクローズ要求
          pstSendReq->unReqType = FMG_CLOSE;
          pstSendReq->pstAnsQue = g_pstREADMIDQueue;
          pstSendReq->ulSize = sizeof(TS_FMGReadParam);
          pstRead->pucBuffer = g_ucBuffer;
          pstRead->ulLength = BUFSIZE;
          xQueueSend(g_pstFMGQueue, pstSendReq, 100);
          break;

        case READMID_SELF   :/* 動作継続要求 */
            // 内部処理部(MIDIリード)
            switch (stTaskParam.ucState)
            {
              case ST_READ_HEADER_HEADER            : // ヘッダチャンク読み出し先頭 4B
                if ( ReadDataProc ( &stTaskParam,4) == RET_OK )
                {
                  if ( stTaskParam.ulCheckBuf == 0x4D546864 )
                  {
                    // USBSerial.println("MIDI file head chank detected.");
                    stTaskParam.ucState = ST_READ_HEADER_LENGTH;
                  }
                  else
                  {
                    USBSerial.println("MIDI file head chank not detected.");
                    // エラー処理
                    stTaskParam.ucState = ST_END;
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_HEADER_LENGTH            : // ヘッダチャンク長 4B格納
                if ( ReadDataProc ( &stTaskParam,4) == RET_OK )
                {
                  USBSerial.print("MIDI file head chank length:");
                  USBSerial.println(stTaskParam.ulCheckBuf);

                  ulLengthHeader = stTaskParam.ulCheckBuf;
                  stTaskParam.ucState = ST_READ_HEADER_FORMAT;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_HEADER_FORMAT            : // フォーマット 2B
                vTaskDelay(pdMS_TO_TICKS(100)); // for serial debug
                if ( ReadDataProc ( &stTaskParam,2) == RET_OK )
                {
                  USBSerial.print("MIDI file format:");
                  USBSerial.println(stTaskParam.ulCheckBuf);

                  unFileFormat = stTaskParam.ulCheckBuf;
                  stTaskParam.ucState = ST_READ_HEADER_TRACK ;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_HEADER_TRACK             : // トラック数 2B
                if ( ReadDataProc ( &stTaskParam,2) == RET_OK )
                {
                  USBSerial.print("MIDI file track number:");
                  USBSerial.println(stTaskParam.ulCheckBuf);

                  unTrackNum = stTaskParam.ulCheckBuf;
                  stTaskParam.ucState = ST_READ_HEADER_TIME;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_HEADER_TIME              : // 時間分解能 2B
                if ( ReadDataProc ( &stTaskParam,2) == RET_OK )
                {
                  USBSerial.print("MIDI file time scale:");
                  USBSerial.println(stTaskParam.ulCheckBuf);

                  unTimeScale = stTaskParam.ulCheckBuf;
                  if (( unTimeScale >> 15 & 0x0001 ) == 1) // 時間分解能判定(MSBがHなら何分何秒何フレーム/Lなら何小節何拍)
                  {
                    ucScaleMode = TIMESCALE_MODE_FLAME;
                  } 
                  else 
                  {
                    ucScaleMode = TIMESCALE_MODE_BEATS;
                  }
                  unTimeScale = ( unTimeScale & 0x7FFF ); // MSBはフラグのためカット
                  // stTaskParam.ucState = ST_READ_HEADER_END;
                  stTaskParam.ucState = ST_READ_TRACK_HEADER; // ここでヘッダは終わり。トラックを読み始める。
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_HEADER_END               : // ヘッダチャンク終了まで待機
                //! @bug ヘッダ長は6byte固定っぽいので、ここで1byte読んでしまうと変になると思われる（hirose）
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK )
                {
                  if (stTaskParam.ulCntDataRead == ulLengthHeader ) // 規定数の読み出し完了
                  {
                    USBSerial.println("MIDI file head chank end.");

                    ucCntTrack = 0;                   // トラック数リセット
                    stTaskParam.ulCntDataRead = 0;    // データ数リセット
                    stTaskParam.ucState = ST_READ_TRACK_HEADER;
                  }
                  else
                  {
                    // 何もしない
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_HEADER             : // トラックチャンク読み出し先頭 4B
                if ( ReadDataProc ( &stTaskParam,4) == RET_OK )
                {
                  if ( stTaskParam.ulCheckBuf == 0x4D54726B )
                  { 
                    USBSerial.print("MIDI file track chank detected:");
                    USBSerial.println(ucCntTrack);

                    ucCntTrack++; // トラック数加算
                    ucMidiEvent = 0; // イベント用バッファクリア
                    // stTaskParam.ulCntStartTrack = stTaskParam.ucCntReadFMG*BUFSIZE + ulNumBuf; // トラックチャンク開始位置を記録(巻き戻し時に実装する)
                    stTaskParam.ucState = ST_READ_TRACK_LENGTH;
                  }
                  else
                  {
                    USBSerial.println("MIDI file track chank not detected.");

                    // エラー処理
                    stTaskParam.ucState = ST_END;
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_LENGTH             : // データ長 4B
                if ( ReadDataProc ( &stTaskParam,4) == RET_OK )
                {
                  USBSerial.print("MIDI file track chank length:");
                  USBSerial.println(stTaskParam.ulCheckBuf);

                  ulLengthTrack = stTaskParam.ulCheckBuf;
                  ulDeltaTime = 0;  // デルタタイムクリア
                  stTaskParam.ucState = ST_READ_TRACK_DELTA;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_DELTA              : // デルタタイム取得 1~4B
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK )
                {
                  ulDeltaTime = (( ulDeltaTime << 25) & 0xFFFFFF80) | ( stTaskParam.ulCheckBuf & 0x0000007F );  // 下位7bitに格納しつつ上位にビットシフト 
                  if ((( stTaskParam.ulCheckBuf >> 7) & 0x00000001 ) == 0 )  // MSBが0なら次のステートに, 1なら引き続きデルタタイム取得. 
                  { 
                    stTaskParam.ucState = ST_READ_TRACK_WAIT_DELTA;
                  }
                  else
                  {
                    // 何もしない
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_WAIT_DELTA         : // デルタタイム待機
                if ( ucScaleMode == TIMESCALE_MODE_FLAME )   // 何分何秒何フレーム
                {
                  // こちらのモードはメジャーではないため一旦同じ実装とする. 
                  ulWaitTimeMsec = ulTempBPM * ( ulDeltaTime / unTimeScale ) / 1000; // msオーダー換算
                } 
                else if ( ucScaleMode == TIMESCALE_MODE_BEATS ) // 何小節何拍
                {
                  ulWaitTimeMsec = ulTempBPM * ( ulDeltaTime / unTimeScale ) / 1000; // msオーダー換算
                }
                vTaskDelay(pdMS_TO_TICKS(ulWaitTimeMsec));  // 待機
                stTaskParam.ucState = ST_READ_TRACK_EVENT;
                break;

              case ST_READ_TRACK_EVENT              : // イベント判定 1B
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK )
                {
                  if ((( stTaskParam.ulCheckBuf & 0x000000FF ) == 0xF0 )||(( stTaskParam.ulCheckBuf & 0x000000FF ) == 0xF7 )) // SysExイベント
                  {
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_SYSEX;
                  }
                  else if ( stTaskParam.ulCheckBuf == 0xFF ) // メタイベント処理
                  {
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META;
                  }
                  else if ((( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0x80 )||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0x90 ))  // 0x8n or 0x9n // MIDIイベント
                  {
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_MIDI_STATE_1B;
                  }
                  else  // 想定していないイベントは無視する.(遷移しない)
                  {
                    // 何もしない
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_SYSEX        : // SysExイベント処理
                stTaskParam.ucState = ST_READ_TRACK_EVENT; // 一旦考慮しない(無視する)
                break;

              case ST_READ_TRACK_EVENT_META         : // メタイベント処理
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK)
                {
                  if (( stTaskParam.ulCheckBuf & 0x000000FF ) == 0xF2 ) // 終了イベント
                  {
                    stTaskParam.ulCntDataRead = 0;  // データ数リセット
                    if ( ucCntTrack==unTrackNum ) // 規定トラック数読み出し完了
                    {
                      ucCntTrack = 0; // トラック数リセット
                      stTaskParam.ucState = ST_END;
                    }
                    else
                    {
                      stTaskParam.ucState = ST_READ_TRACK_HEADER; // 次のトラック読み出し開始
                    }
                  }
                  if (( stTaskParam.ulCheckBuf & 0x000000FF ) == 0x51 )  // テンポイベント
                  {
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META_TEMPO_LENGTH;
                  }
                  else
                  {
                    stTaskParam.ucState = ST_READ_TRACK_EVENT; // 一旦考慮しない(別のメタイベント)
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_META_TEMPO_LENGTH:
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK)
                {
                  ucTempLength = stTaskParam.ulCheckBuf; // テンポデータ長格納
                  stTaskParam.ucState = ST_READ_TRACK_EVENT_META_TEMPO;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_META_TEMPO:
                if ( ReadDataProc ( &stTaskParam,ucTempLength) == RET_OK)
                {
                  ulTempBPM = stTaskParam.ulCheckBuf; // テンポ格納
                  stTaskParam.ucState = ST_READ_TRACK_EVENT; // 次のイベントを読む
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_MIDI_STATE_1B: // MIDIイベント先頭1B読み出し
                if ((( stTaskParam.ulCheckBuf >> 7 ) & 0x000000FF ) == 1 )  // MSBがHの場合, 前のイベントを引き継ぎ
                {
                  ucMidiEvent = ( stTaskParam.ulCheckBuf & 0x000000FF );  // イベント変更
                }
                else
                {
                  ucMidiEvent = ucMidiEvent;  // 前のイベントを引き継ぎ
                }
                stTaskParam.ucState = ST_READ_TRACK_EVENT_MIDI_STATE_2B;
                break;

              case ST_READ_TRACK_EVENT_MIDI_STATE_2B: // MIDIイベント残り2B読み出し
                if ( ReadDataProc (&stTaskParam,2) == RET_OK )
                {
                  ucMidiScale          = (( stTaskParam.ulCheckBuf >> 8 ) & 0x000000FF ); // MIDI音階 1B
                  ucMidiVelocity       = stTaskParam.ulCheckBuf & 0x000000FF;             // MIDIベロシティ 1B
                  pstSLDParam->ucPower = ucMidiVelocity;                                  // ベロシティ(0~127)
                  stTaskParam.ucState  = ST_READ_TRACK_EVENT_MIDI_NOTE;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_MIDI_NOTE    : // MIDIイベントノーツ処理
                if (( ucMidiEvent & 0x0F ) == 0x09 ) // ノートオン
                {
                  if ( ucMidiScale == 0x28 )
                  {
                    xQueueSend( g_pstSLDQueue[0], pstSendReq, 100 );
                  }
                  else if ( ucMidiScale == 0x24 )
                  {
                    xQueueSend( g_pstSLDQueue[1], pstSendReq, 100 );
                  }
                  else if ( ucMidiScale == 0x46 )
                  {
                    xQueueSend( g_pstSLDQueue[4], pstSendReq, 100 );
                  }
                  else if ( ucMidiScale == 0x31 )
                  {
                    xQueueSend( g_pstSLDQueue[5], pstSendReq, 100 );
                  }
                  else if ( ucMidiScale == 0x2E )
                  {
                    xQueueSend( g_pstSLDQueue[6], pstSendReq, 100 );
                  }
                }
                else if (( ucMidiEvent & 0x0F ) == 0x8 ) // ノートオフ
                {
                  // ドラムなのでノートオフは無し
                }
                else if (( ucMidiEvent & 0x0F ) == 0xB ) // コントロールチェンジ
                {
                  // 何もしない(一旦考慮しない)
                }
                else 
                {
                  // 何もしない(想定していないMIDIイベント)
                }
                stTaskParam.ucState = ST_READ_TRACK_EVENT; // 次のイベントを読む
                break;

                case ST_END                           : // 終了処理
                  // リセット
                  ResetStructProc ( &stTaskParam );
                  // 待機状態に遷移(次の開始要求まで何もしない)
                  stTaskParam.ucState = ST_IDLE;
                  break;

                default : // ST_IDLE, ST_WAIT_OPEN, ST_WAIT_READ, ST_PAUSE_REQ, ST_PAUSE_WAIT_READ
                  // 何もしない
                  break;
            } // endcase
            break;
        default:
          // 何もしない
          break;
      } // endcase

      // 継続のための再帰要求を送る. 
      switch (stTaskParam.ucState)
      {
        case ST_IDLE:
        case ST_PAUSE_REQ:
        case ST_PAUSE_WAIT_READ:
        case ST_WAIT_OPEN:
        case ST_WAIT_READ:
        case ST_END:
          // 何もしない
          break;
        default:
          // 動作継続要求を送る
          pstSendReq->unReqType = READMID_SELF;
          pstSendReq->pstAnsQue = NULL;
          pstSendReq->ulSize = 0;
          xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
          break;
      } // endcase
    }
  }
}

// データ格納関数(先頭データは下位バイト) // 残データ出力後にデータバッファ不足のパターンはないものとする
uint32_t ReadDataProc ( TS_READMIDSTaskParam* stTaskParam, uint8_t ucByteNum ) {
  uint8_t ucOutByteNum = ucByteNum; // 残り必要出力データ数
    // 要求
    uint8_t ucSendReq[REQ_QUE_SIZE];
    TS_Req* pstSendReq = (TS_Req*)ucSendReq;
  // 残データチェック&出力
  if ( stTaskParam->ucCntBufHold != 0 )
  {
    if ( stTaskParam->ucCntBufHold == ucByteNum )  // 残データをちょうど使い切る場合
    {
      stTaskParam->ulCheckBuf = stTaskParam->ulBufHold; // 残データ格納
      stTaskParam->ucCntBufHold = 0;  // 残データ数クリア
      stTaskParam->ulBufHold = 0;  // 残データバッファクリア
      return RET_OK;
    }
    else if ( stTaskParam->ucCntBufHold > ucByteNum ) // 残データが足りている場合
    {
      stTaskParam->ulCheckBuf = stTaskParam->ulBufHold; // 残データ格納(ビッグエンディアンになっているためそのまま格納)
      stTaskParam->ucCntBufHold = stTaskParam->ucCntBufHold - ucByteNum;  // 残データ数減算
      stTaskParam->ulBufHold = ( stTaskParam->ulBufHold >> ucByteNum*8 );  // 残データバッファシフト
      return RET_OK;
    }
    else  // 残データはあるが不足している場合
    {
      stTaskParam->ulCheckBuf = stTaskParam->ulBufHold; // 残データ格納(ビッグエンディアンになっているためそのまま格納)
      stTaskParam->ucCntBufHold = 0;  // 残データ数クリア
      stTaskParam->ulBufHold = 0;  // 残データバッファクリア
      ucOutByteNum = ucOutByteNum - stTaskParam->ucCntBufHold; // 出力データ分の減算
    }
  }
  else
  {
    // 何もしない
  }
  // バッファデータチェック&出力
  if (( stTaskParam->ulNumBuf + ucOutByteNum ) < BUFSIZE )   // データが足りている場合
  {
    // 格納
    stTaskParam->ulCheckBuf = 0;
    for (size_t i = 0; i < ucOutByteNum; i++)
    {
      stTaskParam->ulCheckBuf = ( stTaskParam->ulCheckBuf << 8 ) | g_ucBuffer[stTaskParam->ulNumBuf] ;  // ビッグエンディアンとして格納. 
      stTaskParam->ulNumBuf++;
      stTaskParam->ulCntDataRead++;
    }
    return RET_OK;
  }
  else if (( stTaskParam->ulNumBuf + ucOutByteNum ) == BUFSIZE )  // データをちょうど使い切る場合
  {
    // 格納
    stTaskParam->ulCheckBuf = 0;
    for (int i = 0; i < ucOutByteNum; i++)
    {
      stTaskParam->ulCheckBuf = ( stTaskParam->ulCheckBuf << 8 ) | g_ucBuffer[stTaskParam->ulNumBuf] ;  // ビッグエンディアンとして格納. 
      stTaskParam->ulNumBuf++;
      stTaskParam->ulCntDataRead++;
    }
    // ファイルデータ取得要求
    TS_FMGReadParam* pstRead = (TS_FMGReadParam*)pstSendReq->ucParam;
    pstSendReq->unReqType = FMG_READ;
    pstSendReq->pstAnsQue = g_pstREADMIDQueue;
    pstSendReq->ulSize = sizeof(TS_FMGReadParam);
    pstRead->pucBuffer = g_ucBuffer;
    pstRead->ulLength = BUFSIZE;
    xQueueSend(g_pstFMGQueue, pstSendReq, 100);
    stTaskParam->ucCntReadFMG++;
    // 現在のステートを保持. 
    stTaskParam->ucStatePause = stTaskParam->ucState;
    // ステートをポーズに変更. 
    stTaskParam->ucState = ST_PAUSE_WAIT_READ;
    return RET_OK;
  }
  else  // データが不足している場合 
  {
    ucOutByteNum = BUFSIZE - stTaskParam->ulNumBuf;  // 残データ数計算
    // 残データバッファに格納する. 
    stTaskParam->ulBufHold = 0;
    for (int i = 0; i < ucOutByteNum; i++)
    {
      stTaskParam->ulBufHold = ( stTaskParam->ulBufHold << 8 ) | g_ucBuffer[stTaskParam->ulNumBuf] ;  // ビッグエンディアンとして格納. 
      stTaskParam->ulNumBuf++;
      stTaskParam->ulCntDataRead++;
    }
    // ファイルデータ取得要求
    TS_FMGReadParam* pstRead = (TS_FMGReadParam*)pstSendReq->ucParam;
    pstSendReq->unReqType = FMG_READ;
    pstSendReq->pstAnsQue = g_pstREADMIDQueue;
    pstSendReq->ulSize = sizeof(TS_FMGReadParam);
    pstRead->pucBuffer = g_ucBuffer;
    pstRead->ulLength = BUFSIZE;
    xQueueSend(g_pstFMGQueue, pstSendReq, 100);
    stTaskParam->ucCntReadFMG++;
    // 現在のステートを保持. 
    stTaskParam->ucStatePause = stTaskParam->ucState;
    // ステートをポーズに変更. 
    stTaskParam->ucState = ST_PAUSE_WAIT_READ;
    return RET_NG;
  }
}

// 内部構造体リセット関数
void ResetStructProc ( TS_READMIDSTaskParam* stTaskParam ) {
  stTaskParam->ulNumBuf         = 0;       
  stTaskParam->ucState          = ST_IDLE;        
  stTaskParam->ucStatePause     = ST_IDLE;   
  stTaskParam->ulBufHold        = 0;      
  stTaskParam->ucCntBufHold     = 0;   
  stTaskParam->ulCntStartTrack  = 0;
  stTaskParam->ulCntDataRead    = 0;
  stTaskParam->ulCheckBuf       = 0;
  stTaskParam->ucCntReadFMG     = 0;
}
