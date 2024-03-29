/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*
 * The RealView Emulation BaseBoard provides timers and soft reset
 * - the cpu code does not need to provide these.
 */
#include <common.h>
#include "cns3xxx_symbol.h"
#include "misc.h"
#include "pm.h"

DECLARE_GLOBAL_DATA_PTR;

static ulong timestamp;
static ulong lastdec;

#define READ_TIMER (*(volatile ulong *)(CFG_TIMERBASE))

static void flash__init (void);
static void ether__init (void);
static void timer_init(void);

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

#define COMP_MODE_ENABLE ((unsigned int)0x0000EAEF)

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */

int board_init (void)
{
	gd->bd->bi_arch_number = MACH_TYPE_VEGA;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x00000100;

	gd->flags = 0;
	gd->flags |= GD_FLG_RELOC;

	icache_enable ();
	flash__init ();
	timer_init();

	return 0;
}

int cns3xxx_gpio_init (void)
{
#ifdef CNS3XXX_GPIO_SUPPORT
	// initialize GPIO
        cns3xxx_pwr_clk_en(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);
        cns3xxx_pwr_power_up(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);
        cns3xxx_pwr_soft_rst(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);

	// set GPIOA pins 1,3,8,9,12,13 to enable PHYs, eth switch, USB
	// 1,3 -> low
	// 8,9,12,13 -> high

	// block with local var avoids "unused var" warnings when ifdef is false
	{ 
	unsigned int output_low_mask = 1<<3 | 1<<1;
	unsigned int output_high_mask = 1<<13 | 1<<12 | 1<<9 | 1<<8;
	unsigned int output_pin_mask = output_low_mask | output_high_mask;
	unsigned int gpiob_disable_mask = 1<<21 | 1<<20; // MDIO, MDC

	/* configure GPIO A pins that are outputs */
	GPIOA_REG_VALUE(8) = output_pin_mask;     /* direction = out */
	GPIOA_REG_VALUE(0x10) = output_high_mask; /* value = high */
	GPIOA_REG_VALUE(0x14) = output_low_mask;  /* value = low */

	/* configure GPIO B pins for alternate functions */
        GPIOB_PIN_EN_REG |= gpiob_disable_mask;   /* select non-GPIO function */
	}
#endif
}

int misc_init_r (void)
{
	setenv("verify", "n");
	return (0);
}

/******************************
 Routine:
 Description:
******************************/
static void flash__init (void)
{

}
/*************************************************************
 Routine: cns3xxx_ether_init
 Description: set up an ethernet interface
 Prerequisites: serial, clkout, gpio should already be init             
                note this means it can't run from board_init()
*************************************************************/
void cns3xxx_ether_init(void)
{
	bd_t *bd = gd->bd;

	/* run ethernet init now, instead of waiting for a net cmd */
	eth_init(bd);
}


void cns3xxx_pwr_clk_en(unsigned int block)
{
        PM_CLK_GATE_REG |= (block&PM_CLK_GATE_REG_MASK);
}

void cns3xxx_pwr_power_up(unsigned int dev_num)
{
        PM_PLL_HM_PD_CTRL_REG &= ~(dev_num & CNS3XXX_PWR_PLL_ALL);

        /* TODO: wait for 300us for the PLL output clock locked */
};

void cns3xxx_pwr_soft_rst_force(unsigned int block)
{
        /* bit 0, 28, 29 => program low to reset, 
         * the other else program low and then high
         */
        if (block & 0x30000001) {
                PM_SOFT_RST_REG &= ~(block&PM_SOFT_RST_REG_MASK);
        } else {
                PM_SOFT_RST_REG &= ~(block&PM_SOFT_RST_REG_MASK);
                PM_SOFT_RST_REG |= (block&PM_SOFT_RST_REG_MASK);
        }
}

void cns3xxx_pwr_soft_rst(unsigned int block)
{
        static unsigned int soft_reset = 0;

        if(soft_reset & block) {
                //Because SPI/I2C/GPIO use the same block, just only reset once...
                return;
        }
        else {
                soft_reset |= block;
        }
        cns3xxx_pwr_soft_rst_force(block);
}





int inline get_spi_boot_status(){
	/* CHIP_CFG(0x7600_0004)
		Reset_Latch_Config bit2 Boot Flash Select
			0:SMI parallelflash booting
			1:SPI serial flash booting
	 */
	return *((volatile unsigned int*)0x76000004) & (0x1<<2) ;
}

/*************************************************************
 Routine:checkboard
 Description: Check Board Identity
*************************************************************/
int checkboard(void)
{
	char *s = getenv("serial#");
	// show CNS3XXX EVB infomation
	printf("Boot from %s flash\n",get_spi_boot_status()?"serial":"parallel");

	
	#if 0
	puts("Board: FPGA LX330 - Cavium Networks Reference Platform");
	#endif

	if (s != NULL) {
		puts(", serial# ");
		puts(s);
	}
	putc('\n');

	return (0);
}

/******************************
 Routine:
 Description:
******************************/
int dram_init (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
        gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

/*
 * Start the timer
 * U-Boot expects a 32 bit timer, running at CFG_HZ == 1000
 */
#include "cns3000.h"

#define PMU_REG_VALUE(addr) (*((volatile unsigned int *)(CNS3000_VEGA_PMU_BASE+addr)))
#define CLK_GATE_REG PMU_REG_VALUE(0) 
#define SOFT_RST_REG PMU_REG_VALUE(4) 
#define HS_REG PMU_REG_VALUE(8) 
static int timer_div = 75000;
static void timer_init(void)
{
	CLK_GATE_REG |= (1 << 14);
	SOFT_RST_REG &= (~(1 << 14));
	SOFT_RST_REG |= (1 << 14);
	//HS_REG |= (1 << 14);
	/*
	 * Now setup timer1
	 */	
	*(volatile ulong *)(CFG_TIMERBASE + 0x00) = CFG_TIMER_RELOAD;
	*(volatile ulong *)(CFG_TIMERBASE + 0x04) = CFG_TIMER_RELOAD;
	*(volatile ulong *)(CFG_TIMERBASE + 0x30) |= 0x0201;	/* Enabled,
								 * down counter,
								 * no interrupt,
								 * 32-bit,
								 */
	/* We use PCLK for timer 1 clock source, pclk = pll_cpu_clk_div/8 */
	timer_div = ((cns3xxx_pll_cpu_clock() >> 3) * CFG_HZ);
	
	reset_timer_masked();
}

int interrupt_init (void){
	return 0;
}

/*
 * Write the system control status register to cause reset
 */
/* */
/* see reset_cpu in cpu/arm11mpcore/start.S 
void reset_cpu(ulong addr)
{

}
*/

/* delay x useconds AND perserve advance timstamp value */
/* ASSUMES timer is ticking at 1 msec			*/
void udelay (unsigned long usec)
{
	/* scott.patch */
#if 0
	delay(usec);
	return;
#endif

	ulong tmo, tmp;
	tmo = usec/100;

    if (1000 <= usec) {       /* if "big" number, spread normalization to seconds */
        tmo = usec / 1000;  /* start to normalize for usec to ticks per sec */
        tmo *= CFG_HZ;      /* find number of "ticks" to wait to achieve target */
        tmo /= 1000;        /* finish normalize. */
    } else {              /* else small number, don't kill it prior to HZ multiply */
        tmo = usec * CFG_HZ;
        tmo /= (1000*1000);
    }

	tmp = get_timer (0);		/* get current timestamp */

	if( (tmo + tmp + 1) < tmp )	/* if setting this forward will roll time stamp */
		reset_timer_masked ();	/* reset "advancing" timestamp to 0, set lastdec value */
	else
		tmo += tmp;		/* else, set advancing stamp wake up time */
	while (get_timer_masked () < tmo)/* loop till event */
		/*NOP*/;
}

ulong get_timer (ulong base)
{
	return get_timer_masked () - base;
}

void reset_timer_masked (void)
{
	/* reset time */
	//lastdec = READ_TIMER/1000;  /* capure current decrementer value time */
	lastdec = READ_TIMER/timer_div;  /* capure current decrementer value time */
	timestamp = 0;	       	    /* start "advancing" time stamp from 0 */
}

/* ASSUMES 1MHz timer */
ulong get_timer_masked (void)
{
	//ulong now = READ_TIMER/1000;	/* current tick value @ 1 tick per msec */
	ulong now = READ_TIMER/timer_div;	/* current tick value @ 1 tick per msec */

	//printf("now: %x\n", now);
	if (lastdec >= now) {		/* normal mode (non roll) */
		/* normal mode */
		timestamp += lastdec - now; /* move stamp forward with absolute diff ticks */
	} else {			/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += lastdec + TIMER_LOAD_VAL - now;
	}
	lastdec = now;

	return timestamp;
}

/*
 *  u32 get_board_rev() for ARM supplied development boards
 */
ARM_SUPPLIED_GET_BOARD_REV

#if defined(CONFIG_CNS3000) && defined(CONFIG_FLASH_CFI_LEGACY)
ulong board_flash_get_legacy (ulong base, int banknum, flash_info_t *info)
{
	if (banknum == 0) {     /* non-CFI boot flash */
#if 0
		info->portwidth = FLASH_CFI_8BIT;
		info->chipwidth = FLASH_CFI_BY8;
		info->interface = FLASH_CFI_X8X16;
#endif
		info->portwidth = FLASH_CFI_16BIT;
		info->chipwidth = FLASH_CFI_BY16;
		info->interface = FLASH_CFI_X8X16;
		return 1;
	} else
		return 0;
}
#endif

/*
 *  * Miscelaneous platform dependent initialisations
 *   */

int
/**********************************************************/
board_late_init (void)
/**********************************************************/
{

#define MY_GPIO 0xb
#define MY_PIN 0x1
#define MY_PIN_MASK (0x00000000 | MY_PIN)
#define GPIOB_MEM_MAP_VALUE(reg_offset)       (*((u32 volatile *)(0x74800000 + reg_offset)))
#define GPIOB_PIN_DIR_REG   GPIOB_MEM_MAP_VALUE(0x08)
#define PIN_PULLUP (0x2 << (MY_PIN - 1))
#define PULL_MASK (0x00000000 | (0x3 << (MY_PIN - 1)))
#define GPIOBI_REGISTER GPIOB_REG_VALUE(0x4)
 
    if( MY_GPIO == 0xb) {
	MISC_GPIOB_PIN_ENABLE_REG &=  ~MY_PIN_MASK; 
//by default the gpio function is enabled. i.e, the value should be 0. Also dir is in by default.
	GPIOB_PIN_DIR_REG &=  ~MY_PIN_MASK; 
	MISC_GPIOB_15_0_PULL_CTRL_REG = (MISC_GPIOB_15_0_PULL_CTRL_REG & ~PULL_MASK) | PIN_PULLUP;
    }
printf(" MISC_GPIOB_15_0_PULL_CTRL_REG = 0x%08x \n", MISC_GPIOB_15_0_PULL_CTRL_REG );
printf(" Jumper situation is = 0x%08x \n", GPIOBI_REGISTER & MY_PIN_MASK );
    if(!(GPIOBI_REGISTER & MY_PIN_MASK)) {
    	printf("Boot reached stage : board_late_init and found the jumper\n");
	cns3xxx_ether_init();
	setenv("bootargs", CONFIG_ALT_BA);
	setenv("bootcmd", CONFIG_ALT_BC);
        return (0);
    }
}


