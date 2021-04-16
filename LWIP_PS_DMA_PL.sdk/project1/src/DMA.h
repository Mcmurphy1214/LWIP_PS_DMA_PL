#ifndef DMA_H__
#define DMA_H__

/***************************** Include Files *********************************/
#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xscugic.h"

/************************** Constant Definitions *****************************/

#define DMA_DEV_ID          XPAR_AXIDMA_0_DEVICE_ID
#define TX_INTR_ID          XPAR_FABRIC_AXIDMA_0_MM2S_INTROUT_VEC_ID
#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define DDR_BASE_ADDR       XPAR_PS7_DDR_0_S_AXI_BASEADDR   //0x00100000
#define MEM_BASE_ADDR       (DDR_BASE_ADDR + 0x1000000)     //0x01100000
#define TX_BUFFER_BASE      (MEM_BASE_ADDR + 0x00100000)    //0x01200000
#define RESET_TIMEOUT_COUNTER   10000    //复位时间
#define MAX_PKT_LEN             0x100    //发送包长度

/************************** Function Prototypes ******************************/

void tx_intr_handler(void *callback);
int setup_intr_system(XScuGic * int_ins_ptr, XAxiDma * axidma_ptr,
        u16 tx_intr_id, u16 rx_intr_id);
void disable_intr_system(XScuGic * int_ins_ptr, u16 tx_intr_id,
        u16 rx_intr_id);
int DMA_Init(XAxiDma *DMAPtr,u32 DeviceId);

/************************** Variable Definitions *****************************/
XAxiDma axidma;     //XAxiDma实例
XScuGic intc;       //中断控制器的实例
volatile int tx_done;      //发送完成标志
volatile int error;        //传输出错标志


#endif