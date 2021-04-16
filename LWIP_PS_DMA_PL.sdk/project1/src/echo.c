/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "xil_printf.h"
#include "echo.h"
#include "DMA.h"

#include "xaxidma.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

u8 start_update_flag = 0;
u8 rxbuffer[MAX_FLASH_LEN];		//是DMA发送数据到fifo的地址：TX_BUFFER_BASE
u32 *rxbuff;
u32 total_bytes = 0;
struct tcp_pcb *c_pcb;
int transfer_data() {
	if(start_update_flag){
		xil_printf("\r\nStart Waveform Updata\r\n");
		xil_printf("waveform size is %lu Bytes\r\n",total_bytes);	
		// if(DMA_updata(total_bytes,rxbuffer) != XST_SUCCESS)				//DMA_updata()鍑芥暟杩樻病娣诲姞
		// 	xil_printf("Update DMA Error!\r\n");	
		// else 
		// 	total_bytes = 0;

	}
	start_update_flag = 0;
	return 0;
}

void print_app_header()
{
	xil_printf("\n\r\n\r-----lwIP TCP wave transfer ------\n\r");
	xil_printf("TCP packets sent to port 7 generate wave through ZYNQ\n\r");
}


err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	struct pbuf *q;
	rxbuff = rxbuffer[0];
	xil_printf("rxbuffer address:%d\n\r",*rxbuff);
	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}
	q = p;

	if (q->tot_len == 6 && !(memcmp("update", p->payload, 6))) {
        start_update_flag = 1;
    } else if (q->tot_len == 5 && !(memcmp("clear", p->payload, 5))) {
        start_update_flag = 0;
        total_bytes = 0;
        xil_printf("Clear received data\r\n");
    } else {
        while (q->tot_len != q->len) {
            memcpy(&rxbuffer[total_bytes], q->payload, q->len);
            q = q->next;
        }
        memcpy(&rxbuffer[total_bytes], q->payload, q->len);
        total_bytes += q->len;
    }
	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);

	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	xil_printf("tcp_server: Connection Accepted\r\n");
	c_pcb = newpcb;
	/* set the receive callback for this connection */
	tcp_recv(c_pcb, recv_callback);

	tcp_arg(c_pcb, NULL);

	return ERR_OK;
}


int start_application()
{
    int status;
	struct tcp_pcb *pcb;
	err_t err;
	u8 *tx_buffer_ptr;
    XAxiDma_Config *config;

	tx_buffer_ptr = (u8 *) rxbuffer;		//DMA发送指针指向波形缓冲区

	//DMA初始化
	DMA_Init(&AxiDma,0);



	/* create new TCP PCB structure */
	pcb = tcp_new();
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ADDR_ANY, portsever);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", portsever, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);

	xil_printf("TCP echo server started @ port %d\n\r", portsever);

	return 0;
}
