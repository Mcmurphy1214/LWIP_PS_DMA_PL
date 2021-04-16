/***************************** Include Files *********************************/
#include "DMA.h"


//DMA TX中断处理函数
void tx_intr_handler(void *callback)
{
    int timeout;
    u32 irq_status;
    XAxiDma *axidma_inst = (XAxiDma *) callback;

    //读取待处理的中断
    irq_status = XAxiDma_IntrGetIrq(axidma_inst, XAXIDMA_DMA_TO_DEVICE);
    //确认待处理的中断
    XAxiDma_IntrAckIrq(axidma_inst, irq_status, XAXIDMA_DMA_TO_DEVICE);

    //Tx出错
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK)) {
        error = 1;
        XAxiDma_Reset(axidma_inst);
        timeout = RESET_TIMEOUT_COUNTER;
        while (timeout) {
            if (XAxiDma_ResetIsDone(axidma_inst))
                break;
            timeout -= 1;
        }
        return;
    }

    //Tx完成
    if ((irq_status & XAXIDMA_IRQ_IOC_MASK))
        tx_done = 1;
}

//建立DMA中断系统
//  @param   int_ins_ptr是指向XScuGic实例的指针
//  @param   AxiDmaPtr是指向DMA引擎实例的指针
//  @param   tx_intr_id是TX通道中断ID
//  @param   rx_intr_id是RX通道中断ID
//  @return：成功返回XST_SUCCESS，否则返回XST_FAILURE
int setup_intr_system(XScuGic * int_ins_ptr, XAxiDma * axidma_ptr,
        u16 tx_intr_id, u16 rx_intr_id)
{
    int status;
    XScuGic_Config *intc_config;

    //初始化中断控制器驱动
    intc_config = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (NULL == intc_config) {
        return XST_FAILURE;
    }
    status = XScuGic_CfgInitialize(int_ins_ptr, intc_config,
            intc_config->CpuBaseAddress);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    //设置优先级和触发类型
    XScuGic_SetPriorityTriggerType(int_ins_ptr, tx_intr_id, 0xA0, 0x3);
    XScuGic_SetPriorityTriggerType(int_ins_ptr, rx_intr_id, 0xA0, 0x3);

    //为中断设置中断处理函数
    status = XScuGic_Connect(int_ins_ptr, tx_intr_id,
            (Xil_InterruptHandler) tx_intr_handler, axidma_ptr);
    if (status != XST_SUCCESS) {
        return status;
    }

    XScuGic_Enable(int_ins_ptr, tx_intr_id);

    //启用来自硬件的中断
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
            (Xil_ExceptionHandler) XScuGic_InterruptHandler,
            (void *) int_ins_ptr);
    Xil_ExceptionEnable();

    //使能DMA中断
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

    return XST_SUCCESS;
}

//此函数禁用DMA引擎的中断
void disable_intr_system(XScuGic * int_ins_ptr, u16 tx_intr_id,
        u16 rx_intr_id)
{
    XScuGic_Disconnect(int_ins_ptr, tx_intr_id);
    XScuGic_Disconnect(int_ins_ptr, rx_intr_id);
}

//DMA初始化函数
int DMA_Init(XAxiDma *DMAPtr,u32 DeviceId)
{
    int status;
    XAxiDma_Config *config=NULL;

    config = XAxiDma_LookupConfig(DMA_DEV_ID);
    if (!config) {
        xil_printf("No config found for %d\r\n", DMA_DEV_ID);
        return XST_FAILURE;
    }

    //初始化DMA引擎
    status = XAxiDma_CfgInitialize(&axidma, config);
    if (status != XST_SUCCESS) {
        xil_printf("Initialization failed %d\r\n", status);
        return XST_FAILURE;
    }

    if (XAxiDma_HasSg(&axidma)) {
        xil_printf("Device configured as SG mode \r\n");
        return XST_FAILURE;
    }

    //建立中断系统
    status = setup_intr_system(&intc, &axidma, TX_INTR_ID, RX_INTR_ID);
    if (status != XST_SUCCESS) {
        xil_printf("Failed intr setup\r\n");
        return XST_FAILURE;
    }
}