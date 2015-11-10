/*
 * Copyright (C) 2015, CompuLab ltd.
 * Author: Andrey Gelman <andrey.gelman@compulab.co.il>
 * License: GPL-2
 */

#ifndef _REGISTERS_H
#define _REGISTERS_H

/*
 * AirTop Front Panel registers
 */
#define ATFP_REG_SIG0			0x00
#define ATFP_REG_SIG1			0x01
#define ATFP_REG_SIG2			0x02
#define ATFP_REG_SIG3			0x03
#define ATFP_REG_LAYOUT_VER		0x04
#define ATFP_REG_MAJOR_LSB		0x05
#define ATFP_REG_MAJOR_MSB		0x06
#define ATFP_REG_MINOR_LSB		0x07
#define ATFP_REG_MINOR_MSB		0x08

#define ATFP_REG_POST_CODE_LSB		0x10
#define ATFP_REG_POST_CODE_MSB		0x11
#define ATFP_REG_POWER_STATE		0x12

#define ATFP_REG_CPU0T			0x20
#define ATFP_REG_CPU1T			0x21
#define ATFP_REG_CPU2T			0x22
#define ATFP_REG_CPU3T			0x23
#define ATFP_REG_CPU4T			0x24
#define ATFP_REG_CPU5T			0x25
#define ATFP_REG_CPU6T			0x26
#define ATFP_REG_CPU7T			0x27
#define ATFP_REG_GPUT			0x28
#define ATFP_REG_AMBT			0x29

#define ATFP_REG_HDD0T			0x30
#define ATFP_REG_HDD1T			0x31
#define ATFP_REG_HDD2T			0x32
#define ATFP_REG_HDD3T			0x33
#define ATFP_REG_HDD4T			0x34
#define ATFP_REG_HDD5T			0x35
#define ATFP_REG_HDD6T			0x36
#define ATFP_REG_HDD7T			0x37
#define ATFP_REG_CPUTS			0x38
#define ATFP_REG_SENSORT		0x39
#define ATFP_REG_HDDTS			0x3a

#define ATFP_REG_ADC_LSB		0x40
#define ATFP_REG_ADC_MSB		0x41

#define ATFP_REG_MEM_LSB		0x50
#define ATFP_REG_MEM_MSB		0x51

#define ATFP_REG_HDD0_SZ_LSB		0x60
#define ATFP_REG_HDD0_SZ_MSB		0x61
#define ATFP_REG_HDD1_SZ_LSB		0x62
#define ATFP_REG_HDD1_SZ_MSB		0x63
#define ATFP_REG_HDD2_SZ_LSB		0x64
#define ATFP_REG_HDD2_SZ_MSB		0x65
#define ATFP_REG_HDD3_SZ_LSB		0x66
#define ATFP_REG_HDD3_SZ_MSB		0x67
#define ATFP_REG_HDD4_SZ_LSB		0x68
#define ATFP_REG_HDD4_SZ_MSB		0x69
#define ATFP_REG_HDD5_SZ_LSB		0x6a
#define ATFP_REG_HDD5_SZ_MSB		0x6b
#define ATFP_REG_HDD6_SZ_LSB		0x6c
#define ATFP_REG_HDD6_SZ_MSB		0x6d
#define ATFP_REG_HDD7_SZ_LSB		0x6e
#define ATFP_REG_HDD7_SZ_MSB		0x6f

#define ATFP_REG_CPU0F_LSB		0x70
#define ATFP_REG_CPU0F_MSB		0x71
#define ATFP_REG_CPU1F_LSB		0x72
#define ATFP_REG_CPU1F_MSB		0x73
#define ATFP_REG_CPU2F_LSB		0x74
#define ATFP_REG_CPU2F_MSB		0x75
#define ATFP_REG_CPU3F_LSB		0x76
#define ATFP_REG_CPU3F_MSB		0x77
#define ATFP_REG_CPU4F_LSB		0x78
#define ATFP_REG_CPU4F_MSB		0x79
#define ATFP_REG_CPU5F_LSB		0x7a
#define ATFP_REG_CPU5F_MSB		0x7b
#define ATFP_REG_CPU6F_LSB		0x7c
#define ATFP_REG_CPU6F_MSB		0x7d
#define ATFP_REG_CPU7F_LSB		0x7e
#define ATFP_REG_CPU7F_MSB		0x7f

#define ATFP_REG_FPCTRL			0x80
#define ATFP_REG_REQ			0x81
#define ATFP_REG_PENDR0			0x82

#define ATFP_REG_DMIN			0x90
#define ATFP_REG_DMIV			0x91

#define ATFP_REG_RTCT			0x98
#define ATFP_REG_RTCD			0x99

/* 
 * Bit flag offsets within the registers
 */
#define ATFP_OFFS_PENDR0_HDDTR		0
#define ATFP_OFFS_PENDR0_CPUFR		1
#define ATFP_OFFS_PENDR0_CPUTR		2
#define ATFP_OFFS_PENDR0_GPUTR		3

/* 
 * Bit flag masks within the registers
 */
#define ATFP_MASK_PENDR0_HDDTR		(1 << ATFP_OFFS_PENDR0_HDDTR)
#define ATFP_MASK_PENDR0_CPUFR		(1 << ATFP_OFFS_PENDR0_CPUFR)
#define ATFP_MASK_PENDR0_CPUTR		(1 << ATFP_OFFS_PENDR0_CPUTR)
#define ATFP_MASK_PENDR0_GPUTR		(1 << ATFP_OFFS_PENDR0_GPUTR)

#endif	/* _REGISTERS_H */

