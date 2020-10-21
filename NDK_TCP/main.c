/****************************************************************************/
/*                                                                          */
/* 广州创龙电子科技有限公司                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              NDK_TCP 测试程序                                            */
/*                                                                          */
/*              2016年07月11日                                              */
/*                                                                          */
/****************************************************************************/
/*
 *   - Lewis
 *   - xgh@tronlong.com
 *   - DSP C6678 项目组
 *
 */

// 全局定义
#include "General.h"

extern Int srioInit();

// 平台信息
platform_info  gPlatformInfo;

/****************************************************************************/
/*                                                                          */
/*              平台初始化                                                  */
/*                                                                          */
/****************************************************************************/
void EVM_init()
{
	platform_init_flags   sFlags;
	platform_init_config  sConfig;
	int32_t pform_status;  // 平台初始化状态

	memset( (void *) &sFlags,  0, sizeof(platform_init_flags));
	memset( (void *) &sConfig, 0, sizeof(platform_init_config));

	sFlags.pll  = 0; // 锁相环
	sFlags.ddr  = 0; // DDR3
	sFlags.tcsl = 1; // 时间戳计数器
	sFlags.phy  = 1; // 以太网
	sFlags.ecc  = 0; // DDR3 ECC

	sConfig.pllm = 0; // 使用库默认时钟分频

	pform_status = platform_init(&sFlags, &sConfig);

	if (pform_status != Platform_EOK) {
		// 平台初始化失败
		printf("Platform failed to initialize. Error code %d \n", pform_status);
		printf("We will fall in an infinite loop... \n");

		while (1)
		{

		}
	}

	// 获取平台信息
	memset( (void *) &gPlatformInfo, 0, sizeof(platform_info));
	(void) platform_get_info(&gPlatformInfo);

	(void) platform_uart_init();
	(void) platform_uart_set_baudrate(115200);
	(void) platform_write_configure(PLATFORM_WRITE_ALL);

}

/****************************************************************************/
/*                                                                          */
/*              主函数                                                      */
/*                                                                          */
/****************************************************************************/
int main()
{
	srioInit();

	// 启动 SYS/BIOS 系统调度
	BIOS_start ();
}


