/****************************************************************************/
/*                                                                          */
/* ���ݴ������ӿƼ����޹�˾                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              NDK TCP ͨ��                                                */
/*                                                                          */
/*              2014��09��22��                                              */
/*                                                                          */
/****************************************************************************/

#include <ti/ndk/inc/netmain.h>

/****************************************************************************/
/*                                                                          */
/*              �ص����� TCP Server Daemon                                  */
/*                                                                          */
/****************************************************************************/
int TcpTest(SOCKET s, UINT32 unused)
{
    struct timeval to;
    int i;
    char *pBuf;
    char Title[] = "Tronlong Tcp Server Application ......";

	HANDLE hBuffer;

    // ���ó�ʱʱ�� 5s
    to.tv_sec  = 5;
    to.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

    // ����һ���̶��ַ��� ���� 38
	send(s, &Title, 38, 0);
    for(;;)
    {
    	i = (int)recvnc(s, (void **)&pBuf, 0, &hBuffer);

    	// �ش����յ�������
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


