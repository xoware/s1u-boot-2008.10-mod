/*
 * (C) Copyright 2007
 * Michael Schwingen, <michael@schwingen.org>
 *
 * based in great part on jedec_probe.c from linux kernel:
 * (C) 2000 Red Hat. GPL'd.
 * Occasionally maintained by Thayne Harbaugh tharbaugh at lnxi dot com
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
 *
 */

/* The DEBUG define must be before common to enable debugging */
/*#define DEBUG*/

#include <common.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <environment.h>

#define P_ID_AMD_STD CFI_CMDSET_AMD_LEGACY

/* Manufacturers */
#define MANUFACTURER_AMD	0x0001
#define MANUFACTURER_FUJ	0x0004
#define MANUFACTURER_EON	0x007F
#define MANUFACTURER_INTEL	0x0089
#define MANUFACTURER_NMX	0x0089
#define MANUFACTURER_SST	0x00BF
#define MANUFACTURER_MX		0x00C2  

/* AMD */
#define AM29DL800BB	0x22CB
#define AM29DL800BT	0x224A

#define AM29F800BB	0x2258
#define AM29F800BT	0x22D6
#define AM29LV400BB	0x22BA
#define AM29LV400BT	0x22B9
#define AM29LV800BB	0x225B
#define AM29LV800BT	0x22DA
#define AM29LV160DT	0x22C4
#define AM29LV160DB	0x2249
#define AM29F017D	0x003D
#define AM29F016D	0x00AD
#define AM29F080	0x00D5
#define AM29F040	0x00A4
#define AM29LV040B	0x004F
#define AM29F032B	0x0041
#define AM29F002T	0x00B0

/* SST */
#define SST39LF800	0x2781
#define SST39LF160	0x2782
#define SST39VF1601	0x234b
#define SST39LF512	0x00D4
#define SST39LF010	0x00D5
#define SST39LF020	0x00D6
#define SST39LF040	0x00D7
#define SST39SF010A	0x00B5
#define SST39SF020A	0x00B6

/* MXIC */
#define MX29LV640DB	0x22CB
#define MX29LV640DT	0x22C9

/* EON */
#define EON29LV640B	0x22CB
#define EON29LV640T	0x22C9
#define EON29LV640H	0x227E

/* Numonyx */
#define M29EW      0x227E

/* Spansion */
#define S29GL      0x227E

/*
 * Unlock address sets for AMD command sets.
 * Intel command sets use the MTD_UADDR_UNNECESSARY.
 * Each identifier, except MTD_UADDR_UNNECESSARY, and
 * MTD_UADDR_NO_SUPPORT must be defined below in unlock_addrs[].
 * MTD_UADDR_NOT_SUPPORTED must be 0 so that structure
 * initialization need not require initializing all of the
 * unlock addresses for all bit widths.
 */
enum uaddr {
	MTD_UADDR_NOT_SUPPORTED = 0,	/* data width not supported */
	MTD_UADDR_0x0555_0x02AA,
	MTD_UADDR_0x0555_0x0AAA,
	MTD_UADDR_0x5555_0x2AAA,
	MTD_UADDR_0x0AAA_0x0555,
	MTD_UADDR_DONT_CARE,		/* Requires an arbitrary address */
	MTD_UADDR_UNNECESSARY,		/* Does not require any address */
};


struct unlock_addr {
	u32 addr1;
	u32 addr2;
};


/*
 * I don't like the fact that the first entry in unlock_addrs[]
 * exists, but is for MTD_UADDR_NOT_SUPPORTED - and, therefore,
 * should not be used.  The  problem is that structures with
 * initializers have extra fields initialized to 0.  It is _very_
 * desireable to have the unlock address entries for unsupported
 * data widths automatically initialized - that means that
 * MTD_UADDR_NOT_SUPPORTED must be 0 and the first entry here
 * must go unused.
 */
static const struct unlock_addr  unlock_addrs[] = {
	[MTD_UADDR_NOT_SUPPORTED] = {
		.addr1 = 0xffff,
		.addr2 = 0xffff
	},

	[MTD_UADDR_0x0555_0x02AA] = {
		.addr1 = 0x0555,
		.addr2 = 0x02aa
	},

	[MTD_UADDR_0x0555_0x0AAA] = {
		.addr1 = 0x0555,
		.addr2 = 0x0aaa
	},

	[MTD_UADDR_0x5555_0x2AAA] = {
		.addr1 = 0x5555,
		.addr2 = 0x2aaa
	},

	[MTD_UADDR_0x0AAA_0x0555] = {
		.addr1 = 0x0AAA,
		.addr2 = 0x0555
	},

	[MTD_UADDR_DONT_CARE] = {
		.addr1 = 0x0000,      /* Doesn't matter which address */
		.addr2 = 0x0000       /* is used - must be last entry */
	},

	[MTD_UADDR_UNNECESSARY] = {
		.addr1 = 0x0000,
		.addr2 = 0x0000
	}
};


struct amd_flash_info {
	const __u16 mfr_id;
	const __u16 dev_id;
	const char *name;
	const int DevSize;
	const int NumEraseRegions;
	const int CmdSet;
	const __u8 uaddr[4];		/* unlock addrs for 8, 16, 32, 64 */
	const ulong regions[6];
};

#define ERASEINFO(size,blocks) (size<<8)|(blocks-1)

#define SIZE_64KiB  16
#define SIZE_128KiB 17
#define SIZE_256KiB 18
#define SIZE_512KiB 19
#define SIZE_1MiB   20
#define SIZE_2MiB   21
#define SIZE_4MiB   22
#define SIZE_8MiB   23
#define SIZE_16MiB   24
#define SIZE_32MiB   25
#define SIZE_64MiB   26
#define SIZE_128MiB   27

static const struct amd_flash_info jedec_table[] = {
#ifdef CFG_FLASH_LEGACY_256Kx8
	{
		.mfr_id		= MANUFACTURER_SST,
		.dev_id		= SST39LF020,
		.name		= "SST 39LF020",
		.uaddr		= {
			[0] = MTD_UADDR_0x5555_0x2AAA /* x8 */
		},
		.DevSize	= SIZE_256KiB,
		.CmdSet		= P_ID_AMD_STD,
		.NumEraseRegions= 1,
		.regions	= {
			ERASEINFO(0x01000,64),
		}
	},
#endif
#ifdef CFG_FLASH_LEGACY_512Kx8
	{
		.mfr_id		= MANUFACTURER_AMD,
		.dev_id		= AM29LV040B,
		.name		= "AMD AM29LV040B",
		.uaddr		= {
			[0] = MTD_UADDR_0x0555_0x02AA /* x8 */
		},
		.DevSize	= SIZE_512KiB,
		.CmdSet		= P_ID_AMD_STD,
		.NumEraseRegions= 1,
		.regions	= {
			ERASEINFO(0x10000,8),
		}
	},
	{
		.mfr_id		= MANUFACTURER_SST,
		.dev_id		= SST39LF040,
		.name		= "SST 39LF040",
		.uaddr		= {
			[0] = MTD_UADDR_0x5555_0x2AAA /* x8 */
		},
		.DevSize	= SIZE_512KiB,
		.CmdSet		= P_ID_AMD_STD,
		.NumEraseRegions= 1,
		.regions	= {
			ERASEINFO(0x01000,128),
		}
	},
#endif
#ifdef CFG_FLASH_LEGACY_512Kx16
	{
		.mfr_id		= MANUFACTURER_AMD,
		.dev_id		= AM29LV400BB,
		.name		= "AMD AM29LV400BB",
		.uaddr		= {
			[1] = MTD_UADDR_0x0555_0x02AA /* x16 */
		},
		.DevSize	= SIZE_512KiB,
		.CmdSet		= CFI_CMDSET_AMD_LEGACY,
		.NumEraseRegions= 4,
		.regions	= {
			ERASEINFO(0x04000,1),
			ERASEINFO(0x02000,2),
			ERASEINFO(0x08000,1),
			ERASEINFO(0x10000,7),
		}
	},
	{
		.mfr_id		= MANUFACTURER_AMD,
		.dev_id		= AM29LV800BB,
		.name		= "AMD AM29LV800BB",
		.uaddr		= {
			[1] = MTD_UADDR_0x0555_0x02AA /* x16 */
		},
		.DevSize	= SIZE_1MiB,
		.CmdSet		= CFI_CMDSET_AMD_LEGACY,
		.NumEraseRegions= 4,
		.regions	= {
			ERASEINFO(0x04000, 1),
			ERASEINFO(0x02000, 2),
			ERASEINFO(0x08000, 1),
			ERASEINFO(0x10000, 15),
		}
	},
#endif
#ifdef CFG_FLASH_LEGACY_8MiBx8
	{
		.mfr_id		= MANUFACTURER_MX,
		.dev_id		= MX29LV640DT,
		.name		= "MXIC 29LV640DT",
		.uaddr		= {
			[0] = MTD_UADDR_0x0AAA_0x0555 /* x8 */
		},
		.DevSize	= SIZE_8MiB,
		.CmdSet		= P_ID_AMD_STD,
		.NumEraseRegions= 2,
		.regions	= {
			ERASEINFO(0x10000, 127),
			ERASEINFO(0x02000, 8),
		}
	},
#endif
#ifdef CFG_FLASH_LEGACY_64MiBx16
	{
		.mfr_id		= MANUFACTURER_AMD,
		.dev_id		= S29GL,
		.name		= "Spansion 512MB",
		.uaddr		= {
			[1] = MTD_UADDR_0x0555_0x02AA /* x16 */
		},
		.DevSize	= SIZE_64MiB,
		.CmdSet		= CFI_CMDSET_AMD_LEGACY,
		.NumEraseRegions= 1,
		.regions	= {
			ERASEINFO(0x20000, 512),
		}
	},
#endif
#ifdef CFG_FLASH_LEGACY_128MiBx16
	{
		.mfr_id		= MANUFACTURER_NMX,
		.dev_id		= M29EW,
		.name		= "Numonyx M29EW",
		.uaddr		= {
			[1] = MTD_UADDR_0x0555_0x02AA /* x16 */
		},
		.DevSize	= SIZE_128MiB,
		.CmdSet		= CFI_CMDSET_AMD_LEGACY,
		.NumEraseRegions= 1,
		.regions	= {
			ERASEINFO(0x20000, 1024),
		}
	},
	{
		.mfr_id		= MANUFACTURER_AMD,
		.dev_id		= S29GL,
		.name		= "Spansion S29GL",
		.uaddr		= {
			[1] = MTD_UADDR_0x0555_0x02AA /* x16 */
		},
		.DevSize	= SIZE_128MiB,
		.CmdSet		= CFI_CMDSET_AMD_LEGACY,
		.NumEraseRegions= 1,
		.regions	= {
			ERASEINFO(0x20000, 1024),
		}
	},
#endif
};

static inline void fill_info(flash_info_t *info, const struct amd_flash_info *jedec_entry, ulong base)
{
	int i,j;
	int sect_cnt;
	int size_ratio;
	int total_size;
	enum uaddr uaddr_idx;

	size_ratio = info->portwidth / info->chipwidth;

	debug("Found JEDEC Flash: %s\n", jedec_entry->name);
	info->vendor = jedec_entry->CmdSet;
	/* Todo: do we need device-specific timeouts? */
#if defined(CONFIG_CNS3000)
	info->erase_blk_tout = CFG_FLASH_ERASE_TOUT;
#if defined(CFG_FLASH_USE_BUFFER_WRITE)
	#if 1
	info->buffer_size=256;
	#else
	info->buffer_size=64;
	#endif
#endif
#else
	info->erase_blk_tout = 30000;
#endif
	info->buffer_write_tout = 1000;
	info->write_tout = 100;
	info->name = jedec_entry->name;

	/* copy unlock addresses from device table to CFI info struct. This
	   is just here because the addresses are in the table anyway - if
	   the flash is not detected due to wrong unlock addresses,
	   flash_detect_legacy would have to try all of them before we even
	   get here. */
	switch(info->chipwidth) {
	case FLASH_CFI_8BIT:
		uaddr_idx = jedec_entry->uaddr[0];
		break;
	case FLASH_CFI_16BIT:
		uaddr_idx = jedec_entry->uaddr[1];
		break;
	case FLASH_CFI_32BIT:
		uaddr_idx = jedec_entry->uaddr[2];
		break;
	default:
		uaddr_idx = MTD_UADDR_NOT_SUPPORTED;
		break;
	}

	debug("unlock address index %d\n", uaddr_idx);
	info->addr_unlock1 = unlock_addrs[uaddr_idx].addr1;
	info->addr_unlock2 = unlock_addrs[uaddr_idx].addr2;
	debug("unlock addresses are 0x%x/0x%x\n", info->addr_unlock1, info->addr_unlock2);

	sect_cnt = 0;
	total_size = 0;
	for (i = 0; i < jedec_entry->NumEraseRegions; i++) {
		// scott.patch
		#if 0
		ulong erase_region_size = jedec_entry->regions[i] >> 8;
		ulong erase_region_count = (jedec_entry->regions[i] & 0xff) + 1;
		#else
		ulong erase_region_size = 0x20000;
		ulong erase_region_count = CFG_MAX_FLASH_SECT;
		#endif

		total_size += erase_region_size * erase_region_count;
		debug ("erase_region_count = %d erase_region_size = %d\n",
		       erase_region_count, erase_region_size);
		for (j = 0; j < erase_region_count; j++) {
			if (sect_cnt >= CFG_MAX_FLASH_SECT) {
				printf("ERROR: too many flash sectors\n");
				break;
			}
			info->start[sect_cnt] = base;
			base += (erase_region_size * size_ratio);
			sect_cnt++;
		}
	}
	info->sector_count = sect_cnt;
	info->size = total_size * size_ratio;
}

/*-----------------------------------------------------------------------
 * match jedec ids against table. If a match is found, fill flash_info entry
 */
int jedec_flash_match(flash_info_t *info, ulong base)
{
	int ret = 0;
	int i;
	ulong mask = 0xFFFF;
	if (info->chipwidth == 1)
		mask = 0xFF;

	for (i = 0; i < ARRAY_SIZE(jedec_table); i++) {
		if ((jedec_table[i].mfr_id & mask) == (info->manufacturer_id & mask) &&
		    (jedec_table[i].dev_id & mask) == (info->device_id & mask)) {
			fill_info(info, &jedec_table[i], base);
			ret = 1;
			break;
		}
	}
	return ret;
}
