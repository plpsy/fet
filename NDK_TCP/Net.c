/****************************************************************************/
/*                                                                          */
/* 广州创龙电子科技有限公司                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              NDK_TCP 通信                                                */
/*                                                                          */
/*              2016年07月11日                                              */
/*                                                                          */
/****************************************************************************/

// 全局定义
#include "General.h"
#include "fet/feterr.h"
#include "fet/fetmsg.h"

/****************************************************************************/
/*                                                                          */
/*              全局变量                                                    */
/*                                                                          */
/****************************************************************************/
// 配置
char *HostName    = "Tronlong-DSP_C6678";
char *LocalIPAddr = "10.10.10.5";          // DHCP 模式下设置为 "0.0.0.0"
char *LocalIPMask = "255.255.255.0";    // 使用 DHCP 时，不用
char *GatewayIP   = "10.10.10.111";    // 使用 DHCP 时，不用
char *DomainName  = "x.51dsp.net";         // 使用 DHCP 时，不用
char *DNSServer   = "0.0.0.0";          // 当不是0时，会用到

int g_ipAddressAlloced = 0;
// 句柄
static HANDLE hHello=0;
static HANDLE hTcpDaemon=0;
static HANDLE hUdpDaemon=0;


/****************************************************************************/
/*                                                                          */
/*              函数声明                                                    */
/*                                                                          */
/****************************************************************************/
extern int TcpTest();
extern int fetTcpDaemon();
extern int fetUdpDaemon();

// 网络控制回调函数
static void   NetworkOpen();
static void   NetworkClose();
static void   NetworkIPAddr( IPN IPAddr, uint IfIdx, uint fAdd );

// 服务报告函数
static void   ServiceReport( uint Item, uint Status, uint Report, HANDLE hCfgEntry );

/****************************************************************************/
/*                                                                          */
/*              NDK 打开                                                    */
/*                                                                          */
/****************************************************************************/
static void NetworkOpen()
{

#if 0
	// 创建一个服务器 端口 1025
    hHello = DaemonNew( SOCK_STREAMNC, 0, 1025, TcpTest,
                       OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3 );
#else
	// 创建一个服务器 端口 1025
    hTcpDaemon = DaemonNew( SOCK_STREAMNC, 0, FET_MSGPORT, fetTcpDaemon,
                       OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3 );

    hUdpDaemon = DaemonNew( SOCK_DGRAM, 0, FET_BROADCASTPORT, fetUdpDaemon,
                           OS_TASKPRINORM, OS_TASKSTKNORM, 0, 1 );


#endif

}

/****************************************************************************/
/*                                                                          */
/*              NDK 关闭                                                    */
/*                                                                          */
/****************************************************************************/
static void NetworkClose()
{
    DaemonFree( hTcpDaemon );
    DaemonFree( hUdpDaemon );
}

/****************************************************************************/
/*                                                                          */
/*              IP 地址                                                     */
/*                                                                          */
/****************************************************************************/
static void NetworkIPAddr( IPN IPAddr, uint IfIdx, uint fAdd )
{
    IPN IPTmp;

    if( fAdd )
    {
    	g_ipAddressAlloced = 1;
    	platform_write("Network Added: ");
    }
    else
    	platform_write("Network Removed: ");

    IPTmp = ntohl( IPAddr );
    platform_write("If-%d:%d.%d.%d.%d\n", IfIdx,
            (UINT8)(IPTmp>>24)&0xFF, (UINT8)(IPTmp>>16)&0xFF,
            (UINT8)(IPTmp>>8)&0xFF, (UINT8)IPTmp&0xFF );
}

/****************************************************************************/
/*                                                                          */
/*              服务状态                                                    */
/*                                                                          */
/****************************************************************************/
static char *TaskName[]  = { "Telnet","HTTP","NAT","DHCPS","DHCPC","DNS" };
static char *ReportStr[] = { "","Running","Updated","Complete","Fault" };
static char *StatusStr[] = { "Disabled","Waiting","IPTerm","Failed","Enabled" };

static void ServiceReport( uint Item, uint Status, uint Report, HANDLE h )
{
	platform_write("Service Status: %-9s: %-9s: %-9s: %03d\n",
            TaskName[Item-1], StatusStr[Status],
            ReportStr[Report/256], Report&0xFF);

    // 配置 DHCP
    if( Item == CFGITEM_SERVICE_DHCPCLIENT &&
        Status == CIS_SRV_STATUS_ENABLED &&
        (Report == (NETTOOLS_STAT_RUNNING|DHCPCODE_IPADD) ||
         Report == (NETTOOLS_STAT_RUNNING|DHCPCODE_IPRENEW)) )
    {
        IPN IPTmp;

        // 配置 DNS
        IPTmp = inet_addr(DNSServer);
        if( IPTmp )
            CfgAddEntry( 0, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER,
                         0, sizeof(IPTmp), (UINT8 *)&IPTmp, 0 );
    }
}

#if 0
void QueryOrCfgIPAddr1()
{
	HANDLE hCfg = NULL;
    CI_IPNET NA;
    CI_IPNET NAGet;
    CI_ROUTE RT;
    IPN      IPTmp;
    int i = 0;

	if(NULL == hCfg)
	{
	    // 创建新的系统配置
	    hCfg = CfgNew();
	    if( !hCfg )
	    {
	        printf("Unable to create configuration1\n");
	        return;
	    }
	}

	/* check if dhcp already get ip address */
	for(i = 0; i < 10; i++)
	{
		CfgGetEntry( hCfg, CFGTAG_IPNET, 1, 0, (HANDLE *)&NAGet);
		if(NAGet.IPAddr != 0xbebebebe)
		{
			printf("get NA=0x%08x\n", NAGet.IPAddr);
			return;
		}
		else
		{
			Task_sleep(1000);
		}
	}
	/* dhcp get ip address fail, cfg static ip address */
	printf("cfg static ip address.\n");
	// 设置IP 地址
	bzero( &NA, sizeof(NA) );
	NA.IPAddr  = inet_addr(LocalIPAddr);
	NA.IPMask  = inet_addr(LocalIPMask);
	strcpy( NA.Domain, DomainName );
	NA.NetType = 0;

	// 添加地址到接口
	CfgAddEntry( hCfg, CFGTAG_IPNET, 1, 0,
                           sizeof(CI_IPNET), (UINT8 *)&NA, 0 );
	// 添加默认网关
	bzero( &RT, sizeof(RT) );
	RT.IPDestAddr = 0;
	RT.IPDestMask = 0;
	RT.IPGateAddr = inet_addr(GatewayIP);

	// 添加网关路由
	CfgAddEntry( hCfg, CFGTAG_ROUTE, 0, 0,
                           sizeof(CI_ROUTE), (UINT8 *)&RT, 0 );

	// 如果指定 DNS,则手动添加 DNS 服务
	IPTmp = inet_addr(DNSServer);
	if( IPTmp )
		CfgAddEntry( hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER,
                         0, sizeof(IPTmp), (UINT8 *)&IPTmp, 0 );
}
#endif



HANDLE g_hdhcpClient = NULL;
void AddIPAddressCfg(HANDLE hCfg, UINT32 isStatic)
{
    // 如果指定 IP 地址，手动配置 IP 和 网关
    if (1 == isStatic)
    {
        CI_IPNET NA;
        CI_ROUTE RT;
        IPN      IPTmp;

        printf("cfg static ipaddress\n");
        if(NULL == g_hdhcpClient)
        {
        	CfgRemoveEntry(hCfg, g_hdhcpClient);
        	g_hdhcpClient = NULL;
        }
        // 设置IP 地址
        bzero( &NA, sizeof(NA) );
        NA.IPAddr  = inet_addr(LocalIPAddr);
        NA.IPMask  = inet_addr(LocalIPMask);
        strcpy( NA.Domain, DomainName );
        NA.NetType = 0;

        // 添加地址到接口
        CfgAddEntry( hCfg, CFGTAG_IPNET, 1, 0,
                           sizeof(CI_IPNET), (UINT8 *)&NA, 0 );
        // 添加默认网关
        bzero( &RT, sizeof(RT) );
        RT.IPDestAddr = 0;
        RT.IPDestMask = 0;
        RT.IPGateAddr = inet_addr(GatewayIP);

        // 添加网关路由
        CfgAddEntry( hCfg, CFGTAG_ROUTE, 0, 0,
                           sizeof(CI_ROUTE), (UINT8 *)&RT, 0 );

        // 如果指定 DNS,则手动添加 DNS 服务
        IPTmp = inet_addr(DNSServer);
        if( IPTmp )
            CfgAddEntry( hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER,
                         0, sizeof(IPTmp), (UINT8 *)&IPTmp, 0 );
    }
    // 通过 DHCP 自动分配 IP 和 网关
    else
    {
        CI_SERVICE_DHCPC dhcpc;

        printf("cfg dhcp ipaddress\n");
        // 开启 DHCP 服务
        bzero( &dhcpc, sizeof(dhcpc) );
        dhcpc.cisargs.Mode   = CIS_FLG_IFIDXVALID;
        dhcpc.cisargs.IfIdx  = 1;
        dhcpc.cisargs.pCbSrv = &ServiceReport;
        CfgAddEntry( hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 0,
                     sizeof(dhcpc), (UINT8 *)&dhcpc, &g_hdhcpClient);
    }
}



void QueryOrCfgIPAddr()
{
    int i = 0;
    HANDLE hCfg = NULL;

    hCfg = CfgGetDefault();
    if (!hCfg)
    {
        System_printf("QueryOrCfgIPAddr couldn't get default config\n");
    }
	/* check if dhcp already get ip address */
	for(i = 0; i < 10; i++)
	{
		if(g_ipAddressAlloced)
		{
			return;
		}
		Task_sleep(1000);
	}

    AddIPAddressCfg(hCfg, 1);
	/* restart stack */
//	NC_NetStop(1);

}

/****************************************************************************/
/*                                                                          */
/*              任务（Task）线程                                            */
/*                                                                          */
/****************************************************************************/
Void NDKTask(UArg a0, UArg a1)
{
    int             rc;
    HANDLE          hCfg;
    QMSS_CFG_T      qmss_cfg;
    CPPI_CFG_T      cppi_cfg;

    /* 初始化必要的组件：
     *  (1) QMSS
     *  (2) CPPI
     *  (3) Packet Accelerator
     */
    // 初始化 QMSS
    if (platform_get_coreid() == 0)
    {
        qmss_cfg.master_core        = 1;
    }
    else
    {
        qmss_cfg.master_core        = 0;
    }
    qmss_cfg.max_num_desc       = MAX_NUM_DESC;
    qmss_cfg.desc_size          = MAX_DESC_SIZE;
    qmss_cfg.mem_region         = Qmss_MemRegion_MEMORY_REGION0;
    if (res_mgr_init_qmss (&qmss_cfg) != 0)
    {
        platform_write ("Failed to initialize the QMSS subsystem \n");
        goto main_exit;
    }
    else
    {
     platform_write ("QMSS successfully initialized \n");
    }

    // 初始化 CPPI
    if (platform_get_coreid() == 0)
    {
        cppi_cfg.master_core        = 1;
    }
    else
    {
        cppi_cfg.master_core        = 0;
    }
    cppi_cfg.dma_num            = Cppi_CpDma_PASS_CPDMA;
    cppi_cfg.num_tx_queues      = NUM_PA_TX_QUEUES;
    cppi_cfg.num_rx_channels    = NUM_PA_RX_CHANNELS;
    if (res_mgr_init_cppi (&cppi_cfg) != 0)
    {
        platform_write ("Failed to initialize CPPI subsystem \n");
        goto main_exit;
    }
    else
    {
     platform_write ("CPPI successfully initialized \n");
    }

    // 初始化 PA
    if (res_mgr_init_pass()!= 0) {
        platform_write ("Failed to initialize the Packet Accelerator \n");
        goto main_exit;
    }
    else
    {
     platform_write ("PA successfully initialized \n");
    }

    // 使用 NDK 协议栈的必须首先调用此函数
    rc = NC_SystemOpen( NC_PRIORITY_LOW, NC_OPMODE_INTERRUPT );
    if( rc )
    {
        platform_write("NC_SystemOpen Failed (%d)\n",rc);
        for(;;);
    }

    // 创建新的系统配置
    hCfg = CfgNew();
    if( !hCfg )
    {
        platform_write("Unable to create configuration\n");
        goto main_exit;
    }

    // 验证支持的命名长度
    if( strlen( DomainName ) >= CFG_DOMAIN_MAX ||
        strlen( HostName ) >= CFG_HOSTNAME_MAX )
    {
    	platform_write("Names too long\n");
        goto main_exit;
    }

    // 添加全局主机名到 hCfg
    CfgAddEntry( hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_HOSTNAME, 0,
                 strlen(HostName), (UINT8 *)HostName, 0 );

    AddIPAddressCfg(hCfg, 0);

    // 不查看低于警告级别的调试信息
    rc = DBG_WARN;
    CfgAddEntry( hCfg, CFGTAG_OS, CFGITEM_OS_DBGPRINTLEVEL,
                 CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

    // TCP 发生 buffer 大小
	rc = 8192;
	CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPTXBUF,
				 CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

	// TCP 接受 buffer 大小（复制模式）
	rc = 8192;
	CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPRXBUF,
				 CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

	// TCP 接受限制（非复制模式）
	rc = 8192;
	CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPRXLIMIT,
				 CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

    // UDP 接受限制
    rc = 8192;
    CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_SOCKUDPRXLIMIT,
                 CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

    // 使用这些配置启动系统
    do
    {
        rc = NC_NetStart( hCfg, NetworkOpen, NetworkClose, NetworkIPAddr );

    } while( rc > 0 );

    // 释放配置句柄
    CfgFree( hCfg );

main_exit:
    NC_SystemClose();
}

