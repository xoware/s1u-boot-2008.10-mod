/*
 * (C) Copyright 2003
 * Texas Instruments.
 * Kshitij Gupta <kshitij@ti.com>
 * Configuation settings for the TI OMAP Innovator board.
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 * Configuration for Versatile PB.
 *
 * (C) Copyright 2008
 * Cavium Networks Ltd.
 * Scott Shu <scott.shu@caviumnetworks.com>
 * Configuration for Cavium Networks CNS3000 Platform
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H
/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define PHYS_SDRAM_32BIT					/* undefined: 16 bits, defined: 32 bits */
#define MEM_1024MBIT_WIDTH_8

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT		(5 * CFG_HZ)		/* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT		(5 * CFG_HZ)		/* Timeout for Flash Write */

#define UBOOT_TYPE "CNS3420vb2x serial flash"
/*
 * SPI serial flash (dataflash) (Base Address: 0x60000000)
 */
#define CONFIG_SPI_FLASH_BOOT		1
//#define CONFIG_SKIP_LOWLEVEL_INIT
//#define CONFIG_SKIP_RELOCATE_UBOOT
#define CONFIG_SPI			1

/* PSE MAC/PHY Configuration */
#define CONFIG_VB_2

/* enable GPIO */
#define CNS3XXX_GPIO_SUPPORT

#include <configs/cavium_cns3xxx_common.h>

#ifdef CONFIG_BOOTDELAY
#undef CONFIG_BOOTDELAY
#endif
#ifdef CONFIG_ZERO_BOOTDELAY_CHECK
#undef CONFIG_ZERO_BOOTDELAY_CHECK
#endif
#ifdef CONFIG_BOOTCOMMAND
#undef CONFIG_BOOTCOMMAND
#endif
#ifdef CONFIG_ENV_OFFSET
#undef CONFIG_ENV_OFFSET
#endif
#ifdef CONFIG_ENV_ADDR 
#undef CONFIG_ENV_ADDR 
#endif
#ifdef CONFIG_KERNEL_OFFSET 
#undef CONFIG_KERNEL_OFFSET 
#endif
#ifdef CONFIG_BOOTARGS
#undef CONFIG_BOOTARGS
#endif

//backup bl
//ISSUE: fix flash partition layout.
//ISSUE: also add logic to load the env from mmc and test+save it.
#define CONFIG_BOOTDELAY 1
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_BOOTARGS ""
#define CONFIG_BOOTCOMMAND "cp.b 0x60060000 0x400000 0x40000; go 0x400000"
//#define CONFIG_BOOTCOMMAND "go 0x60060000"
#define CONFIG_ENV_OFFSET	0x50000		/* the offset of u-boot environment on dataflash */
#define GPIO_BKBL_SET_ISSUE 1   /* this is backup bootloader*/
#define BOARD_LATE_INIT  1
#define CONFIG_ALT_BA "console=ttyS0,38400 rw init=/linuxrc mem=512M rootwait root=/dev/mmcblk0p2"
#define CONFIG_ALT_BC "mmcinit; fatload mmc 0:1 0x4000000 uimage; bootm 0x4000000"
	#define CONFIG_ENV_ADDR         (CFG_DATAFLASH_LOGIC_ADDR_CS0 + CONFIG_ENV_OFFSET) /* the address of environment */
	#define CONFIG_KERNEL_OFFSET        0xB0000     /* the offset of bootpImage on dataflash */
#undef CONFIG_BOOTFILE
#undef CONFIG_DISPLAY_CPUINFO
#undef CONFIG_DISPLAY_BOARDINFO
#undef CONFIG_HARD_I2C
#undef CONFIG_SOFT_I2C
//-backup bl

#endif /* __CONFIG_H */
