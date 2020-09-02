/*
 * dram_sun8i_r40.c
 *
 *  Created on: Apr 23, 2020
 *      Author: user003
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/dram.h>
#include <asm/arch/prcm.h>
#include <axp_pmic.h>
#include <linux/delay.h>
#include "dram_sun8i_r40/mctl_hal.h"

//#define OKA40I_C_PARA
#define BPI_M2ULTRA_PARA

//extern signed int init_DRAM(int type, __dram_para_t *para);

void __usdelay(unsigned int n)
{
	udelay(n);
}

int set_ddr_voltage(int set_vol)
{
	int ret;
#if defined CONFIG_AXP221_POWER || defined CONFIG_AXP809_POWER || \
	defined CONFIG_AXP818_POWER
	/* ddr3 vdd range:
	 * Operating maximum = 1575mV
	 * Absolute maximum  = 1975mV
	 * ddr2 vdd range:
	 * Operating maximum = 1900mV */
	if (set_vol > 1900) {
		dram_dbg_error("ddr voltage %dmV too high . Limited to 1900\n", set_vol);
		set_vol = 1900;
	}
	ret = axp_set_dcdc5(set_vol);
	return ret;
#else
	retirn -1;
#endif
}

static void set_ce_gating(void)
{
	//note: this function for ddr read ss id
	struct sunxi_ccm_reg * const ccm =
			(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
#ifndef CCMU_BUS_SOFT_RST_REG0
#define CCMU_BUS_SOFT_RST_REG0 (SUNXI_CCM_BASE+0x2c0)
#endif
	//reset
	writel( readl(CCMU_BUS_SOFT_RST_REG0)  | (1<<5), CCMU_BUS_SOFT_RST_REG0);
	//open ce gating
	writel( readl(&ccm->ahb_gate0)  | (1<<5), &ccm->ahb_gate0);
}


unsigned long sunxi_dram_init(void)
{
	signed int size;
	__dram_para_t para;

#if defined(OKA40I_C_PARA)
	/* OKA40i-C V1.2 dev board stock firmware
	 * (FETA40i-C V1.2 processor module) */
	para.dram_clk	=0x00000240;
	para.dram_type	=0x00000003;
	para.dram_zq	=0x003b3bfb;
	para.dram_odt_en=0x00000031;
	para.dram_para1	=0x10e410e4;
	para.dram_para2	=0x04001000;
	para.dram_mr0	=0x00001c70;
	para.dram_mr1	=0x00000040;
	para.dram_mr2	=0x00000018;
	para.dram_mr3	=0x00000000;
	para.dram_tpr0	=0x00479950;
	para.dram_tpr1	=0x01b1a94b;
	para.dram_tpr2	=0x00065046;
	para.dram_tpr3	=0xb47d7d96;
	para.dram_tpr4	=0x00000000;
	para.dram_tpr5	=0x00000198;
	para.dram_tpr6	=0x21000000;
	para.dram_tpr7	=0x2406c1e0;
	para.dram_tpr8	=0x00000000;
	para.dram_tpr9	=0x00000000;
	para.dram_tpr10	=0x00000003;
	para.dram_tpr11	=0x33330000;
	para.dram_tpr12	=0x00007777;
	para.dram_tpr13	=0xc4096953;
#elif defined(BPI_M2ULTRA_PARA)
#ifdef CONFIG_DRAM_SUN8I_R40_AWLIB
	/* autodetect dram type, configuration and timing */
	para.dram_clk	=0x00000288;
	para.dram_type	=0x00000007;	// 2:DDR2,3:DDR3,6:LPDDR2,7:LPDDR3
	para.dram_zq	=0x003b3bfb;
	para.dram_odt_en=0x00000031;
	para.dram_para1	=0x10e410e4;
	para.dram_para2	=0x00001000;
	para.dram_mr0	=0x00001840;
	para.dram_mr1	=0x00000040;
	para.dram_mr2	=0x00000018;
	para.dram_mr3	=0x00000002;
	para.dram_tpr0	=0x0048a192;
	para.dram_tpr1	=0x01b1a94b;
	para.dram_tpr2	=0x00061043;
	para.dram_tpr3	=0xb47d7d96;	// dram voltage
	para.dram_tpr4	=0x00000000;
	para.dram_tpr5	=0x00000198;
	para.dram_tpr6	=0x21000000;
	para.dram_tpr7	=0x2406c1e0;
	para.dram_tpr8	=0x00000000;
	para.dram_tpr9	=0x00000000;
	para.dram_tpr10	=0x00000008;
	para.dram_tpr11	=0x44450000;
	para.dram_tpr12	=0x00009777;
	para.dram_tpr13	=0xc4090950;
	/* dram_tpr13
	 * .1  =1 user defined timing parameter, =0 auto set timing
	 * .13 =0 auto scan dram type
	 * .14 =0 rank & width detect
	 * .27 =0 set master priority
	 * .28 =1 run simple memory test
	 * 	 */

#else
	/* user-defined dram type
	 * autodetect configuration and timing  */
	para.dram_clk	=CONFIG_DRAM_CLK;	//0x00000240
	para.dram_type	=0x00000003;	// 2:DDR2,3:DDR3,6:LPDDR2,7:LPDDR3
	para.dram_zq	=0x003b3bfb;
	para.dram_odt_en=0x00000031;
	para.dram_para1	=0x10e410e4;
	para.dram_para2	=0x04001000;
	para.dram_mr0	=0x00001c70;
	para.dram_mr1	=0x00000040;
	para.dram_mr2	=0x00000018;
	para.dram_mr3	=0x00000000;
	para.dram_tpr0	=0x00479950;
	para.dram_tpr1	=0x01b1a94b;
	para.dram_tpr2	=0x00065046;
	para.dram_tpr3	=0xb47d7d96;
	para.dram_tpr4	=0x00000000;
	para.dram_tpr5	=0x00000198;
	para.dram_tpr6	=0x21000000;
	para.dram_tpr7	=0x2406c1e0;
	para.dram_tpr8	=0x00000000;
	para.dram_tpr9	=0x00000000;
	para.dram_tpr10	=0x00000003;
	para.dram_tpr11	=0x33330000;
	para.dram_tpr12	=0x00007777;
	para.dram_tpr13	=0xc4096950;
#ifdef DRAM_TYPE_SCAN
	para.dram_tpr13 &= ~(1<<13); /* auto dram type */
#endif
#ifdef DRAM_RANK_SCAN
	para.dram_tpr13 &= ~(1<<14); /* auto rank & width */
#endif
#endif
#endif

	/* common settings */
	para.dram_tpr13 |= (1<<28);  /* run simple memory test */
	para.dram_tpr13 &= ~(1<<27); /* set master priority */

	set_ce_gating();	/* turn on crypto engine ! */
	size = init_DRAM(0, &para);

	if (size <0)
		size =0;
	return size<<20;
}



