/****************************************************************************/
/*                                                                          */
/* 广州创龙电子科技有限公司                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              NDK TCP 通信                                                */
/*                                                                          */
/*              2014年09月22日                                              */
/*                                                                          */
/****************************************************************************/

#include <ti/ndk/inc/netmain.h>

/****************************************************************************/
/*                                                                          */
/*              回调函数 TCP Server Daemon                                  */
/*                                                                          */
/****************************************************************************/
int TcpTest(SOCKET s, UINT32 unused)
{
    struct timeval to;
    int i;
    char *pBuf;
    char Title[] = "Tronlong Tcp Server Application ......";

	HANDLE hBuffer;

    // 配置超时时间 5s
    to.tv_sec  = 5;
    to.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

    // 发送一个固定字符串 长度 38
	send(s, &Title, 38, 0);
    for(;;)
    {
    	i = (int)recvnc(s, (void **)&pBuf, 0, &hBuffer);

    	// 回传接收到的数据
        if(i > 0)
        {
            if(send(s, pBuf, i, 0 ) < 0)
                break;

            recvncfree(hBuffer);
        }
        else
        {
            break;
        }
    }
    fdClose(s);

    return(0);
}


