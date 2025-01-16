#ifndef _REQ_H_
#define _REQ_H_

#include <queue.h>

// キューの定義
#define REQ_QUE_SIZE (64+sizeof(TS_Req))
#define REQ_QUE_NUM  (2)

// REQのID定義
#define FMG_REQ_BEGIN       (0x0100)
#define FMG_REQ_END         (0x01FF)
#define SLD_REQ_BEGIN       (0x0200)
#define SLD_REQ_END         (0x02FF)
#define SND_REQ_BEGIN       (0x0300)
#define SND_REQ_END         (0x03FF)
#define READMID_REQ_BEGIN   (0x0400)
#define READMID_REQ_END     (0x04FF)

// 要求の構造体

typedef struct STagFMGRequest {
  uint16_t unReqType;
  uint16_t unError;
  uint32_t ulSize;
  QueueHandle_t pstAnsQue;
  uint8_t ucParam[];
}TS_Req;


#endif
