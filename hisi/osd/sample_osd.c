/******************************************************************************
  A simple program of Hisilicon Hi35xx video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include<sys/time.h>

#include "sample_comm.h"
#include "sample_comm_ive.h"

#define MAX_FRM_CNT     256
#define MAX_FRM_WIDTH   4096

static HI_S32 s_s32MemDev = -1;

#define MEM_DEV_OPEN() \
    do {\
        if (s_s32MemDev <= 0)\
        {\
            s_s32MemDev = open("/dev/mem", O_CREAT|O_RDWR|O_SYNC);\
            if (s_s32MemDev < 0)\
            {\
                perror("Open dev/mem error");\
                return -1;\
            }\
        }\
    }while(0)

#define MEM_DEV_CLOSE() \
    do {\
        HI_S32 s32Ret;\
        if (s_s32MemDev > 0)\
        {\
            s32Ret = close(s_s32MemDev);\
            if(HI_SUCCESS != s32Ret)\
            {\
                perror("Close mem/dev Fail");\
                return s32Ret;\
            }\
            s_s32MemDev = -1;\
        }\
    }while(0)

HI_VOID* COMM_SYS_Mmap(HI_U32 u32PhyAddr, HI_U32 u32Size)
{
    HI_U32 u32Diff;
    HI_U32 u32PagePhy;
    HI_U32 u32PageSize;
    HI_U8* pPageAddr;

    /* The mmap address should align with page */
    u32PagePhy = u32PhyAddr & 0xfffff000;
    u32Diff    = u32PhyAddr - u32PagePhy;

    /* The mmap size shuld be mutliples of 1024 */
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;
    pPageAddr   = mmap ((void*)0, u32PageSize, PROT_READ | PROT_WRITE,
                        MAP_SHARED, s_s32MemDev, u32PagePhy);
    if (MAP_FAILED == pPageAddr )
    {
        perror("mmap error");
        return NULL;
    }
    return (HI_VOID*) (pPageAddr + u32Diff);
}

HI_S32 COMM_SYS_Munmap(HI_VOID* pVirAddr, HI_U32 u32Size)
{
    HI_U32 u32PageAddr;
    HI_U32 u32PageSize;
    HI_U32 u32Diff;

    u32PageAddr = (((HI_U32)pVirAddr) & 0xfffff000);
    u32Diff     = (HI_U32)pVirAddr - u32PageAddr;
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;

    return munmap((HI_VOID*)u32PageAddr, u32PageSize);
}

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;

void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_S32 SAMPLE_VENC_1080P_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[3] = {PT_H264, PT_H264, PT_H264};
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_HD1080, PIC_D1};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;

    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 8;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 8;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt = 8;


    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;

    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble        = HI_FALSE;
    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width       = stSize.u32Width;
    stVpssChnMode.u32Height      = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    VpssChn = 1;
    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble         = HI_FALSE;
    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width        = stSize.u32Width;
    stVpssChnMode.u32Height       = stSize.u32Height;
    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    VpssGrp = 0;
    VpssChn = 1;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
                                    gs_enNorm, enSize[1], enRcMode, u32Profile);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }
	
	VI_CHN viChn = 0;
	VIDEO_FRAME_INFO_S stFrameInfo;
    HI_S32 s32GetFrameMilliSec = 20000;
	s32Ret = HI_MPI_VI_GetFrame(viChn, &stFrameInfo, s32GetFrameMilliSec);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VI_GetFrame fail,ViChn(%d),Error(%#x)\n", viChn, s32Ret);
	}

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    
    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return;
}


/* sp420 -> p420 ; sp422 -> p422  */
HI_S32 vi_dump_save_one_frame(VIDEO_FRAME_S* pVBuf, char* pfd)
{
    unsigned int w, h;
    char* pVBufVirt_Y;
    char* pVBufVirt_C;
    char* pMemContent;
    unsigned char TmpBuff[MAX_FRM_WIDTH];
    HI_U32 phy_addr, size;
    HI_CHAR* pUserPageAddr[2];
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/* UV height for planar format */

    if (pVBuf->u32Width > MAX_FRM_WIDTH)
    {
        printf("Over max frame width: %d, can't support.\n", MAX_FRM_WIDTH);
        return HI_FAILURE;
    }

    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 3 / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else
    {
        size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 2;
        u32UvHeight = pVBuf->u32Height;
    }

    phy_addr = pVBuf->u32PhyAddr[0];

    pUserPageAddr[0] = (HI_CHAR*) COMM_SYS_Mmap(phy_addr, size);
    if (NULL == pUserPageAddr[0])
    {
        return HI_FAILURE;
    }
    //printf("stride: %d,%d\n", pVBuf->u32Stride[0], pVBuf->u32Stride[1] );

    pVBufVirt_Y = pUserPageAddr[0];
    pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0]) * (pVBuf->u32Height);

    // /* save Y ----------------------------------------------------------------*/
    int npos = 0;
    for (h = 0; h < pVBuf->u32Height; h+=4)
    {
        pMemContent = pVBufVirt_Y + h * pVBuf->u32Stride[0];
        int i = 0;
        for (i = 0; i < pVBuf->u32Width; i+=4)
        {
            pfd[npos++] = pMemContent[i];
        }
    }

    // /* save U ----------------------------------------------------------------*/
    for (h = 0; h < u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

        pMemContent += 1;

        for (w = 0; w < pVBuf->u32Width / 2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width / 2, 1, pfd);
    }

    // /* save V ----------------------------------------------------------------*/
    // fprintf(stderr, "V......");
    // fflush(stderr);
    // for (h = 0; h < u32UvHeight; h++)
    // {
    //     pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

    //     for (w = 0; w < pVBuf->u32Width / 2; w++)
    //     {
    //         TmpBuff[w] = *pMemContent;
    //         pMemContent += 2;
    //     }
    //     fwrite(TmpBuff, pVBuf->u32Width / 2, 1, pfd);
    // }
    // fflush(pfd);

    // fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    // fflush(stderr);

    COMM_SYS_Munmap(pUserPageAddr[0], size);

    return HI_SUCCESS;
}


void SAMPLE_VENC_1080()
{
	PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize = PIC_HD1080;
	HI_S32 s32Ret = HI_SUCCESS;
	
	VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    VENC_CHN VencChn = 0;
	
	SAMPLE_PRT("Sensor [%d]\n", enSize);
    SAMPLE_COMM_VI_GetSizeBySensor(&enSize);
	SAMPLE_PRT("Sensor [%d]\n", enSize);

	VB_CONF_S stVbConf;
	memset(&stVbConf, 0, sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 128;
    HI_U32 u32BlkSize;
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 8;

	// step 2: mpp system init.
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }
	
	// step 3: start vi dev & chn to capture
	SAMPLE_VI_CONFIG_S stViConfig = {0};
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    s32Ret = HI_MPI_VI_SetFrameDepth(0, 4);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetFrameDepth fail,ViChn(%d),Error(%#x)\n", 0, s32Ret);
    }

    // VENC
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, PT_H264, gs_enNorm, enSize, SAMPLE_RC_CBR, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_Start fail,VencChn(%d),Error(%#x)\n", VencChn, s32Ret);
        goto END_VENC_1080P_CLASSIC_1;
    }
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_StartGetStream fail,Error(%#x)\n", s32Ret);
        goto END_720P_6;
    }

    SAMPLE_RECT_ARRAY_S pstRect;
    pstRect.u16Num = 1;
    // pstRect.astRect[0].astPoint[0].s32X = 200;
    // pstRect.astRect[0].astPoint[0].s32Y = 200;
    // pstRect.astRect[0].astPoint[1].s32X = 200;
    // pstRect.astRect[0].astPoint[1].s32Y = 600;
    // pstRect.astRect[0].astPoint[2].s32X = 600;
    // pstRect.astRect[0].astPoint[2].s32Y = 200;
    // pstRect.astRect[0].astPoint[3].s32X = 600;
    // pstRect.astRect[0].astPoint[3].s32Y = 600;

    VI_CHN viChn = 0;
    VIDEO_FRAME_INFO_S stFrameInfo;
    HI_S32 s32GetFrameMilliSec = 20000;
    HI_S32 s32SetFrameMilliSec = 20000;
    int nn = 100;
    int xx = 1000;
    FILE* pfd = fopen("zzz.yuv", "w+b");
    while(nn--)
    {
        pstRect.astRect[0].astPoint[0].s32X = 100;
        pstRect.astRect[0].astPoint[0].s32Y = 100;
        pstRect.astRect[0].astPoint[1].s32X = 100;
        pstRect.astRect[0].astPoint[1].s32Y = xx;
        pstRect.astRect[0].astPoint[2].s32X = xx;
        pstRect.astRect[0].astPoint[2].s32Y = 100;
        pstRect.astRect[0].astPoint[3].s32X = xx;
        pstRect.astRect[0].astPoint[3].s32Y = xx;
        xx = xx - 2; if(xx < 200) xx = 1000;
        
        s32Ret = HI_MPI_VI_GetFrame(viChn, &stFrameInfo, s32GetFrameMilliSec);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_GetFrame fail,ViChn(%d),Error(%#x)\n", viChn, s32Ret);
            continue;
        }
        // SAMPLE_PRT("===%d\n", stFrameInfo.stVFrame.u32Width);
        // SAMPLE_PRT("===%d\n", stFrameInfo.stVFrame.u32Height);
        // SAMPLE_PRT("===%d\n", stFrameInfo.stVFrame.u32Stride[0]);
        // SAMPLE_PRT("===%d\n", stFrameInfo.stVFrame.u32Stride[1]);
        // SAMPLE_PRT("===%d\n", stFrameInfo.stVFrame.u32Stride[2]);
    
        if(nn == 10)
        {
            struct  timeval st, et;       
            gettimeofday(&st, NULL);
            char buff[480*270];
            s32Ret = vi_dump_save_one_frame(&stFrameInfo.stVFrame, buff);
            fwrite(buff, 480*270, 1, pfd);
            gettimeofday(&et, NULL);
            SAMPLE_PRT("========== %d ms\n", (et.tv_sec - st.tv_sec)*1000 + (et.tv_usec - st.tv_usec)/1000);
        }

        //Draw rect
        s32Ret = SAMPLE_COMM_VGS_FillRect(&stFrameInfo, &pstRect, 0x00FF0000);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_VGS_FillRect fail,Error(%#x)\n", s32Ret);
            (HI_VOID)HI_MPI_VI_ReleaseFrame(viChn, &stFrameInfo);
            continue;
        }

        s32Ret = HI_MPI_VENC_SendFrame(VencChn, &stFrameInfo, s32SetFrameMilliSec);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VENC_SendFrame fail,Error(%#x)\n", s32Ret);
        }

        //Release frame
        s32Ret = HI_MPI_VI_ReleaseFrame(viChn, &stFrameInfo);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_ReleaseFrame fail,ViChn(%d),Error(%#x)\n", viChn, s32Ret);
        }
    }
    fclose(pfd);

    SAMPLE_COMM_VENC_StopGetStream();


END_720P_6:
    SAMPLE_COMM_VENC_Stop(VencChn);

END_VENC_1080P_CLASSIC_1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:
    SAMPLE_COMM_SYS_Exit();
}

int main()
{
	signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);
	
	//SAMPLE_VENC_1080P_CLASSIC();
    MEM_DEV_OPEN();
	SAMPLE_VENC_1080();
    MEM_DEV_CLOSE();
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
