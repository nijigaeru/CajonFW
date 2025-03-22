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
    Serial.println("Failed to create queue.");
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
  uint8_t   ucMetaLength;       // テンポデータ長 1B
  uint32_t  ulTempBPM;          // テンポデータusオーダー 可変長
  uint32_t  ulWaitTimeMsec;     // 待機時間 4B
  uint32_t  ulTotalWaitTimeMsec;     // 待機時間 4B
  uint32_t  ulTotalEventNum;    // イベント数 4B

  // 制御用
  uint8_t  ucExtraSkip = 0;
  uint32_t ulSysExLen = 0;
  uint8_t  ucHasReadData = 0;
  uint32_t ulReadData = 0;
  uint8_t  ucSLDOn[SLD_NUM] = {0};
   
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
    TS_FMGReadAns* pstReadAns = (TS_FMGReadAns *)pstRecvReq->ucParam;

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
          Serial.println("READMID_START request");
          switch (stTaskParam.ucState)
          {
            case ST_IDLE:
              // リセット
              ResetStructProc ( &stTaskParam );

              ulTotalWaitTimeMsec = 0;
              ulTotalEventNum = 0;

              // ファイルオープン要求
              pstSendReq->unReqType = FMG_OPEN;
              pstSendReq->pstAnsQue = g_pstREADMIDQueue;
              pstSendReq->ulSize = sizeof(TS_FMGOpenParam);
              memcpy(pstOpen->ucFileName, pstStart->ucFileName, sizeof(pstStart->ucFileName));
              xQueueSend(g_pstFMGQueue, pstSendReq, 100);

              stTaskParam.ucState = ST_WAIT_OPEN;
              break;
            case ST_PAUSE_REQ:
              stTaskParam.ucState          = stTaskParam.ucStatePause;  // ステート再開
              // 動作継続要求を送る
              pstSendReq->unReqType = READMID_SELF;
              pstSendReq->pstAnsQue = NULL;
              pstSendReq->ulSize = 0;
              xQueueSend(g_pstREADMIDQueue, pstSendReq, 100);
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
          else
          {
            // リセット
            ResetStructProc ( &stTaskParam );
          }
          break;
        case FMG_READ_ANS   :/* リード完了要求 */
          if(pstRecvReq->unError == 0)
          {
            switch (stTaskParam.ucState)
            {
            case ST_WAIT_READ:
              // データ追加
              stTaskParam.ulMaxBuf += pstReadAns->ulLength;
              // リード開始
              stTaskParam.ucState  = ST_READ_HEADER_HEADER;
              break;

            case ST_PAUSE_WAIT_READ:
              // データ追加
              stTaskParam.ulMaxBuf += pstReadAns->ulLength;
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
          else
          {
            // リセット
            ResetStructProc ( &stTaskParam );
            // ファイルクローズ要求
            pstSendReq->unReqType = FMG_CLOSE;
            pstSendReq->pstAnsQue = g_pstREADMIDQueue;
            pstSendReq->ulSize = sizeof(TS_FMGReadParam);
            pstRead->pucBuffer = g_ucBuffer;
            pstRead->ulLength = BUFSIZE;
            xQueueSend(g_pstFMGQueue, pstSendReq, 100);
          }
          break;

        case READMID_PAUSE  :/* 一時停止要求 */
          stTaskParam.ucStatePause     = stTaskParam.ucState; // ステート保持
          stTaskParam.ucState          = ST_PAUSE_REQ;
          break;
        case READMID_END    :/* 終了要求 */
          // リセット
          ResetStructProc ( &stTaskParam );
          // ファイルクローズ要求
          pstSendReq->unReqType = FMG_CLOSE;
          pstSendReq->pstAnsQue = g_pstREADMIDQueue;
          pstSendReq->ulSize = sizeof(TS_FMGReadParam);
          pstRead->pucBuffer = g_ucBuffer;
          pstRead->ulLength = BUFSIZE;
          xQueueSend(g_pstFMGQueue, pstSendReq, 100);
          break;
        case READMID_PLAY_NOTS:
          switch (stTaskParam.ucState)
          {
          case ST_IDLE:
            {
              TS_READMIDPlayNotsParam* pstNots = (TS_READMIDPlayNotsParam*)pstRecvReq->ucParam;
              uint8_t vel[SLD_NUM] = {0};
              for (uint32_t ulI=0;ulI < pstNots->unNum; ulI++)
              {
                uint8_t targetSld = process_drum_hit(pstNots->stInfo[ulI].ucScale);
                if ((targetSld < SLD_NUM) && (pstNots->stInfo[ulI].ucVelocity != 0))
                {
                  vel[targetSld] = pstNots->stInfo[ulI].ucVelocity;
                }
              }
              for (uint8_t i = 0; i < SLD_NUM; i++)
              {
                if (vel[i] != 0)
                {
                  pstSendReq->unReqType = SLD_TURN_ON;
                  pstSLDParam->ucPower = vel[i];
                  xQueueSend(g_pstSLDQueue[i], pstSendReq, 100);
                }
              }
            }
            break;
          default:
            // 何もしない
            break;
          }
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
                    // Serial.println("MIDI head chank detected.");
                    stTaskParam.ucState = ST_READ_HEADER_LENGTH;
                  }
                  else
                  {
                    Serial.println("MIDI head chank not detected.");
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
                  // Serial.print("MIDI file head chank length:");
                  // Serial.println(stTaskParam.ulCheckBuf);

                  ulLengthHeader = stTaskParam.ulCheckBuf;
                  stTaskParam.ucState = ST_READ_HEADER_FORMAT;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_HEADER_FORMAT            : // フォーマット 2B
                // vTaskDelay(pdMS_TO_TICKS(100)); // for serial debug
                if ( ReadDataProc ( &stTaskParam,2) == RET_OK )
                {
                  // Serial.print("MIDI file format:");
                  // Serial.println(stTaskParam.ulCheckBuf);

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
                  Serial.print("MIDI file track number:");
                  Serial.println(stTaskParam.ulCheckBuf);

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
                  Serial.print("MIDI file time scale:");
                  Serial.println(stTaskParam.ulCheckBuf);

                  unTimeScale = stTaskParam.ulCheckBuf;
                  if (( unTimeScale >> 15 & 0x0001 ) == 1) // 時間分解能判定(MSBがHなら何分何秒何フレーム/Lなら何小節何拍)
                  {
                    ucScaleMode = TIMESCALE_MODE_FLAME;
                    // Serial.println("MIDI time scale mode:flame");
                  } 
                  else 
                  {
                    ucScaleMode = TIMESCALE_MODE_BEATS;
                    // Serial.println("MIDI time scale mode:beats");
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

              case ST_READ_TRACK_HEADER             : // トラックチャンク読み出し先頭 4B
                if ( ReadDataProc ( &stTaskParam,4) == RET_OK )
                {
                  if ( stTaskParam.ulCheckBuf == 0x4D54726B )
                  { 
                    // Serial.print("MIDI file track chank detected:");
                    // Serial.println(ucCntTrack);

                    ucCntTrack++; // トラック数加算
                    ucMidiEvent = 0; // イベント用バッファクリア
                    // stTaskParam.ulCntStartTrack = stTaskParam.ucCntReadFMG*BUFSIZE + ulNumBuf; // トラックチャンク開始位置を記録(巻き戻し時に実装する)
                    stTaskParam.ucState = ST_READ_TRACK_LENGTH;
                  }
                  else
                  {
                    Serial.println("MIDI track chank not detected.");

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
                  // Serial.print("MIDI file track chank length:");
                  // Serial.println(stTaskParam.ulCheckBuf);

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
                  ulDeltaTime = (( ulDeltaTime << 7) & 0xFFFFFF80) | ( stTaskParam.ulCheckBuf & 0x0000007F );  // 下位7bitに格納しつつ上位にビットシフト 
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
                if (ulDeltaTime != 0)
                {
                  // ここでまとめて音を鳴らす。
                  for (size_t i = 0; i < SLD_NUM; ++i)
                  {
                    if (ucSLDOn[i] != 0)
                    {
                      pstSendReq->unReqType = SLD_TURN_ON;
                      pstSLDParam->ucPower = ucSLDOn[i];
                      xQueueSend(g_pstSLDQueue[i], pstSendReq, 100);
                      ucSLDOn[i] = 0;
                    }
                  }

                  if ( ucScaleMode == TIMESCALE_MODE_FLAME )   // 何分何秒何フレーム
                  {
                    // こちらのモードはメジャーではないため一旦同じ実装とする. 
                    ulWaitTimeMsec = ulTempBPM * ulDeltaTime / unTimeScale / 1000; // msオーダー換算
                  } 
                  else if ( ucScaleMode == TIMESCALE_MODE_BEATS ) // 何小節何拍
                  {
                    ulWaitTimeMsec = ulTempBPM * ulDeltaTime / unTimeScale / 1000; // msオーダー換算
                  }
                  ulTotalWaitTimeMsec += ulWaitTimeMsec;
                  vTaskDelay(pdMS_TO_TICKS(ulWaitTimeMsec));  // 待機
                }
                stTaskParam.ucState = ST_READ_TRACK_EVENT;
                ulDeltaTime = 0;  // デルタタイムクリア
                break;

              case ST_READ_TRACK_EVENT              : // イベント判定 1B
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK )
                {
                  ulTotalEventNum += 1;
                  // vTaskDelay(pdMS_TO_TICKS(100));  // ログ出すために 
                  if (( stTaskParam.ulCheckBuf & 0x000000FF ) == 0xF0 ) // SysExイベント
                  {
                    // Serial.println("MIDI SysEx event F0.");
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_SYSEX_F0;
                  }
                  else if (( stTaskParam.ulCheckBuf & 0x000000FF ) == 0xF7 ) // SysExイベント
                  {
                    // Serial.println("MIDI SysEx event F7.");
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_SYSEX_F7; // 次のイベントを読む
                  }
                  else if ( stTaskParam.ulCheckBuf == 0xFF ) // メタイベント処理
                  {
                    // Serial.println("MIDI meta event.");
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META;
                  }
                  else if (
                    (( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0x80 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0x90 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0xA0 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0xB0 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0xC0 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0xD0 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000F0 ) == 0xE0 )
                  ||(( stTaskParam.ulCheckBuf & 0x000000FF )  < 0x80 )) // MIDIイベント処理、ランニングステータス
                  {
                    // Serial.println("MIDI MIDI event.");
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_MIDI_STATE_1B;
                  }
                  else  // 想定していないイベントは無視する.(遷移しない)
                  {
                    Serial.print("MIDI file unknown event:");
                    Serial.println(stTaskParam.ulCheckBuf);
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_SYSEX_F0        : // SysExイベント処理
                stTaskParam.ucState = ST_READ_TRACK_EVENT_SYSEX_LEN_F0;
                ulSysExLen = 0;
                break;

              case ST_READ_TRACK_EVENT_SYSEX_F7        : // SysExイベント処理
                stTaskParam.ucState = ST_READ_TRACK_EVENT_META_LENGTH;
                stTaskParam.ucStateTmp = ST_READ_TRACK_EVENT_META_TROUGH;
                ucExtraSkip = 0;
                break;
              
              case ST_READ_TRACK_EVENT_SYSEX_LEN_F0              : // デルタタイム取得 1~4B
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK )
                {
                  ulSysExLen = (( ulSysExLen << 7) & 0xFFFFFF80) | ( stTaskParam.ulCheckBuf & 0x0000007F );  // 下位7bitに格納しつつ上位にビットシフト 
                  if ((( stTaskParam.ulCheckBuf >> 7) & 0x00000001 ) == 0 )  // MSBが0なら次のステートに, 1なら引き続きデルタタイム取得. 
                  { 
                    // stTaskParam.ucState = ST_READ_TRACK_WAIT_DELTA;
                    ucMetaLength = ulSysExLen;
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META_TROUGH;
                    // ★さらにF7が最後にあればスキップする必要があるのかも？
                    // ucExtraSkip = 1;
                    ucExtraSkip = 0;
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

              case ST_READ_TRACK_EVENT_META         : // メタイベント処理
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK)
                {
                  if (( stTaskParam.ulCheckBuf & 0x000000FF ) == 0x2F ) // 終了イベント
                  {
                    Serial.println("MIDI meta track end.");

                    stTaskParam.ulCntDataRead = 0;  // データ数リセット
                    if ( ucCntTrack==unTrackNum ) // 規定トラック数読み出し完了
                    {
                      stTaskParam.ucState = ST_END;
                    }
                    else
                    {
                      stTaskParam.ucState = ST_READ_TRACK_EVENT_META_NEXT_TRACK; // 次のトラック読み出し開始
                    }
                  }
                  else if (( stTaskParam.ulCheckBuf & 0x000000FF ) == 0x51 )  // テンポイベント
                  {
                    // Serial.println("MIDI meta tempo.");
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META_LENGTH;
                    stTaskParam.ucStateTmp = ST_READ_TRACK_EVENT_META_TEMPO;
                  }
                  else
                  {
                    // Serial.println("MIDI meta other.");
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META_LENGTH;
                    stTaskParam.ucStateTmp = ST_READ_TRACK_EVENT_META_TROUGH; // スルー
                    ucExtraSkip = 0;
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_META_LENGTH:
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK)
                {
                  // ★可変長！！
                  ucMetaLength = stTaskParam.ulCheckBuf; // 次のメタデータ格納
                  stTaskParam.ucState = stTaskParam.ucStateTmp;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_META_TEMPO:
                if ( ReadDataProc ( &stTaskParam,ucMetaLength) == RET_OK)
                {
                  Serial.print("MIDI file tempo:");
                  Serial.println(stTaskParam.ulCheckBuf);
                  
                  ulTempBPM = stTaskParam.ulCheckBuf; // テンポ格納
                  stTaskParam.ucState = ST_READ_TRACK_DELTA; // 次のイベントを読む
                  ulDeltaTime = 0;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_META_TROUGH:
                if ( ReadDataProc ( &stTaskParam,ucMetaLength +ucExtraSkip) == RET_OK)
                {
                  // よんだデータ使わない。
                  // Serial.print("MIDI file meta trough:");
                  // Serial.println(ucMetaLength +ucExtraSkip);
                  // Serial.print("Last Byte:");
                  // Serial.println(stTaskParam.ulCheckBuf & 0x000000FF);

                  stTaskParam.ucState = ST_READ_TRACK_DELTA; // 次のイベントを読む
                  ulDeltaTime = 0;
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_META_NEXT_TRACK:
                if ( ReadDataProc ( &stTaskParam,1) == RET_OK)
                {
                    stTaskParam.ulCntDataRead = 0;    // データ数リセット
                    stTaskParam.ucState = ST_READ_TRACK_HEADER; // 次のトラックを読む
                }
                else
                {
                  // 何もしない
                }
                break;
                

              case ST_READ_TRACK_EVENT_MIDI_STATE_1B: // MIDIイベント先頭1B読み出し
                if ((( stTaskParam.ulCheckBuf >> 7 ) & 0x000000FF ) == 1 )  // MSBがLの場合, 前のイベントを引き継ぎ
                {
                  ucMidiEvent = ( stTaskParam.ulCheckBuf & 0x000000FF );  // イベント変更
                  ucHasReadData = 0;
                  ulReadData = 0;
                }
                else
                {
                  ucMidiEvent = ucMidiEvent;  // 前のイベントを引き継ぎ
                  ucHasReadData = 1;
                  ulReadData = stTaskParam.ulCheckBuf; // 今読んだデータは次で使う。
                }

                // イベントの種類によって割り振る
                
                // ノートオン
                if (( ucMidiEvent & 0xF0 ) == 0x90 ) // ノートオン
                {
                  // Serial.println("MIDI note on.");
                  stTaskParam.ucState = ST_READ_TRACK_EVENT_MIDI_NOTE;
                }
                // 他の2byteイベント
                else if (
                  (( ucMidiEvent & 0xF0 ) == 0x80 ) ||
                  (( ucMidiEvent & 0xF0 ) == 0xA0 ) ||
                  (( ucMidiEvent & 0xF0 ) == 0xB0 ) ||
                  (( ucMidiEvent & 0xF0 ) == 0xE0 ) )
                {
                  // if (( ucMidiEvent & 0xF0 ) == 0x80 ) {
                  //   Serial.println("MIDI note off.");
                  // } else {
                  //   Serial.println("MIDI other event 2.");
                  // }
                  ucMetaLength = (ucHasReadData ? 1 : 2);
                  ucExtraSkip = 0;
                  stTaskParam.ucState = ST_READ_TRACK_EVENT_META_TROUGH;
                }
                // 他の1byteイベント
                else if (
                  (( ucMidiEvent & 0xF0 ) == 0xC0 ) ||
                  (( ucMidiEvent & 0xF0 ) == 0xD0 ) )
                {
                  // Serial.println("MIDI other event 1.");
                  if (ucHasReadData)
                  {
                    // よみ飛ばす必要なし
                    stTaskParam.ucState = ST_READ_TRACK_DELTA; // 次のイベントを読む
                    ulDeltaTime = 0;
                  }
                  else
                  {
                    ucMetaLength = 1;
                    ucExtraSkip = 0;
                    stTaskParam.ucState = ST_READ_TRACK_EVENT_META_TROUGH;
                  }
                }
                else
                {
                  // 何もしない
                }
                break;

              case ST_READ_TRACK_EVENT_MIDI_NOTE    : // MIDIイベントノーツ処理
                if ( ReadDataProc (&stTaskParam,(ucHasReadData ? 1 : 2)) == RET_OK )
                {
                  uint32_t ulThisData = 0;
                  if (ucHasReadData)
                  {
                    ulThisData = (ulReadData << 8) | stTaskParam.ulCheckBuf;
                  }
                  else
                  {
                    ulThisData = stTaskParam.ulCheckBuf;
                  }
                  ucMidiScale          = (( ulThisData >> 8 ) & 0x000000FF ); // MIDI音階 1B
                  ucMidiVelocity       = (ulThisData & 0x000000FF);             // MIDIベロシティ 1B
                  
                  // ドラムのみ再生
                  if ((ucMidiEvent & 0x0F) == 0x09)
                  {
                    // Serial.print("MIDI file note:");
                    // Serial.println(ucMidiScale);
                    uint8_t targetSld = process_drum_hit(ucMidiScale);
                    if (targetSld < SLD_NUM)
                    {
                      ucSLDOn[targetSld] = ucMidiVelocity;
                    }
                  }
                  stTaskParam.ucState = ST_READ_TRACK_DELTA; // 次のイベントを読む
                  ulDeltaTime = 0;
                }
                else
                {
                  // 何もしない
                }
                break;

                case ST_END                           : // 終了処理
                  // Serial.print("Total wait time:");
                  // Serial.println(ulTotalWaitTimeMsec);
                  Serial.print("Total event number:");
                  Serial.println(ulTotalEventNum);

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

  // バッファデータチェック&出力
  if (( stTaskParam->ulNumBuf + ucOutByteNum ) < stTaskParam->ulMaxBuf )   // データが足りている場合
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
  else  // データがぴったりか不足している場合 
  {
    // 余ってるデータを前に詰める。
    for (size_t i = 0; i < stTaskParam->ulMaxBuf - stTaskParam->ulNumBuf; ++i)
    {
      g_ucBuffer[i] = g_ucBuffer[i + stTaskParam->ulNumBuf];
    }
    stTaskParam->ulMaxBuf = stTaskParam->ulMaxBuf - stTaskParam->ulNumBuf;
    stTaskParam->ulNumBuf = 0;

    // ファイルデータ取得要求
    TS_FMGReadParam* pstRead = (TS_FMGReadParam*)pstSendReq->ucParam;
    pstSendReq->unReqType = FMG_READ;
    pstSendReq->pstAnsQue = g_pstREADMIDQueue;
    pstSendReq->ulSize = sizeof(TS_FMGReadParam);
    pstRead->pucBuffer = g_ucBuffer + stTaskParam->ulMaxBuf;
    pstRead->ulLength = BUFSIZE- stTaskParam->ulMaxBuf;
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
  stTaskParam->ulMaxBuf         = 0;       
  stTaskParam->ucState          = ST_IDLE;        
  stTaskParam->ucStatePause     = ST_IDLE; 
  stTaskParam->ucStateTmp       = ST_IDLE;  
  stTaskParam->ulCntStartTrack  = 0;
  stTaskParam->ulCntDataRead    = 0;
  stTaskParam->ulCheckBuf       = 0;
  stTaskParam->ucCntReadFMG     = 0;
}
