/****************************************************************************/
/*                                                                          */
/* ���ݴ������ӿƼ����޹�˾                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              NDK_TCP ���Գ���                                            */
/*                                                                          */
/*              2016��07��11��                                              */
/*                                                                          */
/****************************************************************************/
/*
 *   - Lewis
 *   - xgh@tronlong.com
 *   - DSP C6678 ��Ŀ��
 *
 */

// ȫ�ֶ���
#include "General.h"

extern Int srioInit();

// ƽ̨��Ϣ
platform_info  gPlatformInfo;

/****************************************************************************/
/*                                                                          */
/*              ƽ̨��ʼ��                                                  */
/*                                                                          */
/****************************************************************************/
void EVM_init()
{
	platform_init_flags   sFlags;
	platform_init_config  sConfig;
	int32_t pform_status;  // ƽ̨��ʼ��״̬

	memset( (void *) &sFlags,  0, sizeof(platform_init_flags));
	memset( (void *) &sConfig, 0, sizeof(platform_init_config));

	sFlags.pll  = 0; // ���໷
	sFlags.ddr  = 0; // DDR3
	sFlags.tcsl = 1; // ʱ���������
	sFlags.phy  = 1; // ��̫��
	sFlags.ecc  = 0; // DDR3 ECC

	sConfig.pllm = 0; // ʹ�ÿ�Ĭ��ʱ�ӷ�Ƶ

	pform_status = platform_init(&sFlags, &sConfig);

	if (pform_status != Platform_EOK) {
		// ƽ̨��ʼ��ʧ��
		printf("Platform failed to initialize. Error code %d \n", pform_status);
		printf("We will fall in an infinite loop... \n");

		while (1)
		{

		}
	}

	// ��ȡƽ̨��Ϣ
	memset( (void *) &gPlatformInfo, 0, sizeof(platform_info));
	(void) platform_get_info(&gPlatformInfo);

	(void) platform_uart_init();
	(void) platform_uart_set_baudrate(115200);
	(void) platform_write_configure(PLATFORM_WRITE_ALL);

}

/****************************************************************************/
/*                                                                          */
/*              ������                                                      */
/*                                                                          */
/****************************************************************************/
int main()
{
	srioInit();

	// ���� SYS/BIOS ϵͳ����
	BIOS_start ();
}


