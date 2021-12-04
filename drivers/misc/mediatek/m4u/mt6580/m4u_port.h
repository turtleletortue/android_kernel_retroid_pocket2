#ifndef __M4U_PORT_H__
#define __M4U_PORT_H__

/* ==================================== */
/* about portid */
/* ==================================== */

enum {
	M4U_PORT_DISP_OVL0            ,
	M4U_PORT_DISP_RDMA0           ,
	M4U_PORT_DISP_WDMA0           ,
	M4U_PORT_MDP_RDMA             ,
	M4U_PORT_MDP_WDMA             ,
	M4U_PORT_MDP_WROT             ,

	M4U_PORT_IMGO                 ,
	M4U_PORT_IMG2O                ,
	M4U_PORT_LSCI                 ,
	M4U_PORT_VENC_BSDMA_VDEC_POST0,
	M4U_PORT_IMGI                 ,
	M4U_PORT_ESFKO                ,
	M4U_PORT_AAO                  ,
	M4U_PORT_VENC_MVQP            ,
	M4U_PORT_VENC_MC              ,
	M4U_PORT_VENC_CDMA_VDEC_CDMA  ,
	M4U_PORT_VENC_REC_VDEC_WDMA   ,

	M4U_PORT_UNKNOWN             ,
};

#define M4U_PORT_NR M4U_PORT_UNKNOWN

#endif

