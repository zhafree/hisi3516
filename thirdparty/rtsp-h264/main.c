
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "rtspservice.h"
#include "rtputils.h"
#include "ringfifo.h"
#include "sample_comm.h"

extern int g_s32Quit ;

/**************************************************************************************************
**
**
**
**************************************************************************************************/

//#define DEBUG_HIVENC
#ifdef DEBUG_HIVENC

#define SV_CHANNEL_NUM      2

void *SAMPLE_VENC_PROC(void *p) {
    HI_S32          s32Ret;
    VENC_CHN        VencChn[VPSS_MAX_PHY_CHN_NUM];
    PIC_SIZE_E      enSize[SV_CHANNEL_NUM] = {PIC_720P, PIC_360P};
    VENC_STREAM_S   stStream;
    HI_U32          u32rameSize = 0;

    for (HI_S32 chn = 0; chn < SV_CHANNEL_NUM; ++chn) {
        VencChn[chn] = chn;
    }

    if (HI_SUCCESS != SAMPLE_VENC_Init(enSize, SV_CHANNEL_NUM)) {
        return (void *)HI_FAILURE;
    }

    /******************************************
     stream save process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, SV_CHANNEL_NUM);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("Start Venc failed!\n");
        SAMPLE_VENC_DeInit();
    }

    while(1) {
        if (HI_SUCCESS == SAMPLE_VENC_PeekStream(0, &stStream)) {
            /*******************************************************
             step 2.5 : save frame
            *******************************************************/
            u32rameSize = 0;
            for (int i = 0; i < stStream.u32PackCount; ++i) {
                printf("u32Offset = %d, u32Len = %d\n", 
                    stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len);
                u32rameSize += stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
            }
            printf("u32rameSize = %d\n", u32rameSize);
            HisiPutH264DataToBuffer(&stStream);
            SAMPLE_VENC_ReleaseStream(0, &stStream);
        }
        // usleep(10000);
    }

    return (void *)HI_SUCCESS;
}
#else
extern void *SAMPLE_VENC_H265_H264(void *p);
#endif

int main(void) {
    int s32MainFd,temp;
    struct timespec ts = { 2, 0 };
    pthread_t id;

    //rtsp
    ringmalloc(256*1024);
    printf("RTSP server START\n");
    PrefsInit();
    printf("listen for client connecting...\n");
    signal(SIGINT, IntHandl);
    s32MainFd = tcp_listen(SERVER_RTSP_PORT_DEFAULT);
    if (ScheduleInit() == ERR_FATAL) {
        fprintf(stderr,"Fatal: Can't start scheduler %s, %i \nServer is aborting.\n", __FILE__, __LINE__);
        return 0;
    }
    RTP_port_pool_init(RTP_DEFAULT_PORT);

#ifdef DEBUG_HIVENC
    pthread_create(&id, NULL, SAMPLE_VENC_PROC, NULL);
#else
    pthread_create(&id, NULL, SAMPLE_VENC_H265_H264, NULL);
#endif    
    pthread_detach(id);

    while (!g_s32Quit) {
        nanosleep(&ts, NULL);
        EventLoop(s32MainFd);
    }
    sleep(2);
    ringfree();
    printf("The Server quit!\n");

    return 0;
}

