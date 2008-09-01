/* at91sam7s.h -- AT91SAM7S hardware definitions
**
** Copyright 2006, Brian Swetland.  All rights reserved.
** See provided LICENSE file or http://frotz.net/LICENSE for details.
*/

#ifndef __PLATFORM_AT91SAM7_H__ 
#define __PLATFORM_AT91SAM7_H__

#if !defined(AT91_SAM7X) && !defined(AT91_SAM7S)
#error Unspecified Architecture - AT91SAM7S or AT91SAM7X must be defined
#endif

/* peripheral ids */
#define PID_AIC_FIQ    0
#define PID_SYSIRQ     1
#define PID_PIOA       2
#define PID_USART0     6
#define PID_USART1     7
#define PID_SSC        8
#define PID_TWI        9
#define PID_PWMC       10
#define PID_UDP        11
#define PID_TC0        12
#define PID_TC1        13
#define PID_TC2        14
#if AT91_SAM7X
#define PID_PIOB       3
#define PID_SPI0       4
#define PID_SPI1       5
#define PID_CAN        15
#define PID_EMAC       16
#define PID_ADC        17
#define PID_AIC_IRQ0   30
#define PID_AIC_IRQ1   31
#else
#define PID_ADC        4
#define PID_SPI0       5
#define PID_AIC_IRQ    30
#endif

#define BASE_FLASH     0x00100000
#define BASE_SRAM      0x00200000
#define BASE_TC        0xFFFA0000
#define BASE_UDP       0xFFFB0000
#define BASE_TWI       0xFFFB8000
#define BASE_USART0    0xFFFC0000
#define BASE_USART1    0xFFFC4000
#define BASE_PWMC      0xFFFCC000
#define BASE_SSC       0xFFFD4000
#define BASE_ADC       0xFFFD8000
#define BASE_SPI0      0xFFFE0000

#define BASE_AIC       0xFFFFF000
#define BASE_DBGU      0xFFFFF200
#define BASE_PIOA      0xFFFFF400
#define BASE_PMC       0xFFFFFC00
#define BASE_RSTC      0xFFFFFD00
#define BASE_RTT       0xFFFFFD20
#define BASE_PIT       0xFFFFFD30
#define BASE_WDT       0xFFFFFD40
#define BASE_VREG      0xFFFFFD60
#define BASE_MC        0xFFFFFF00

#if AT91_SAM7X
#define BASE_CAN       0xFFFD0000
#define BASE_EMAC      0xFFFDC000
#define BASE_SPI1      0xFFFE4000
#define BASE_PIOB      0xFFFFF600
#endif


typedef volatile unsigned int vu4;

typedef struct
{
    vu4 MR;
    vu4 SR;
    vu4 PIVR;
    vu4 PIIR;
} AT91PIT;

/* MR */
#define PIT_PITEN     (1 << 24)
#define PIT_PITIEN    (1 << 25)

/* SR */
#define PIT_PITS      (1)

/* PIxR */
#define PIT_PICNT(x)   (x >> 20)
#define PIT_CPIV(x)    (x & 0x000fffff)

#define AT91PIT_ADDR ((AT91PIT*) BASE_PIT)

typedef struct
{
    vu4 CR;
    vu4 MR;
    vu4 IER;
    vu4 IDR;
    vu4 IMR;
    vu4 SR;
    vu4 RHR;
    vu4 THR;
    vu4 BRGR;
    vu4 __0[7];
    vu4 CIDR;
    vu4 EXID;
    vu4 FNR;
} AT91DBGU;

/* CR bits */
#define DBGU_RSTRX       0x00000004
#define DBGU_RSTTX       0x00000008
#define DBGU_RXEN        0x00000010
#define DBGU_RXDIS       0x00000020
#define DBGU_TXEN        0x00000040
#define DBGU_TXDIS       0x00000080
#define DBGU_RSTSTA      0x00000100

/* MR bits */
#define DBGU_PAR_EVEN    0x00000000
#define DBGU_PAR_ODD     0x00000200
#define DBGU_PAR_SPACE   0x00000400
#define DBGU_PAR_MARK    0x00000600
#define DBGU_PAR_NONE    0x00000800

#define DBGU_MODE_NORMAL 0x00000000
#define DBGU_MODE_ECHO   0x0000C000
#define DBGU_MODE_LLOOP  0x00008000
#define DBGU_MODE_RLOOP  0x00004000

/* IER, IDR, IMR, and SR bits */
#define DBGU_RXRDY       0x00000001
#define DBGU_TXRDY       0x00000002
#define DBGU_ENDRX       0x00000008
#define DBGU_ENDTX       0x00000010
#define DBGU_OVRE        0x00000020
#define DBGU_FRAME       0x00000040
#define DBGU_PARE        0x00000080
#define DBGU_TXEMPTY     0x00000200
#define DBGU_TXBUFE      0x00000800
#define DBGU_RXBUFF      0x00001000
#define DBGU_COMMTX      0x40000000
#define DBGU_COMMRX      0x80000000

#define AT91DBGU_ADDR ((AT91DBGU*) BASE_DBGU)
    
typedef struct 
{
    vu4 pio_enable;
    vu4 pio_disable;
    vu4 pio_status;
    vu4 __0;
    vu4 output_enable;
    vu4 output_disable;
    vu4 output_status;
    vu4 __1;
    vu4 filter_enable;
    vu4 filter_disable;
    vu4 filter_status;
    vu4 __2;
    vu4 data_set;
    vu4 data_clear;
    vu4 data_status;
    vu4 pin_status;
    vu4 irq_enable;
    vu4 irq_disable;
    vu4 irq_mask;
    vu4 irq_status;
    vu4 multidriver_enable;
    vu4 multidriver_disable;
    vu4 multidriver_status;
    vu4 __3;
    vu4 pullup_disable;
    vu4 pullup_enable;
    vu4 pullup_status;
    vu4 __4;
    vu4 select_a;
    vu4 select_b;
    vu4 select_status;
    vu4 __5[9];
    vu4 write_enable;
    vu4 write_disable;
    vu4 write_status;
} AT91PIO;    

#define AT91PIOA_ADDR ((AT91PIO*) BASE_PIOA)
#if AT91_SAM7X
#define AT91PIOB_ADDR ((AT91PIO*) BASE_PIOB)
#endif

typedef struct 
{
    vu4 SCER;
    vu4 SCDR;
    vu4 SCSR;
    vu4 __0;
    vu4 PCER;
    vu4 PCDR;
    vu4 PCSR;
    vu4 __1;
    vu4 MOR;
    vu4 MCFR;
    vu4 __2;
    vu4 PLLR;
    vu4 MCKR;
    vu4 __3[2];
    vu4 PCK0;
    vu4 PCK1;
    vu4 PCK2;
} AT91PMC;

#define AT91PMC_ADDR ((AT91PMC*) BASE_PMC)

/* PMC_SCER/SCDR */
#define PMC_PCK      0x00000001
#define PMC_UDP      0x00000080
#define PMC_PCK0     0x00000100
#define PMC_PCK1     0x00000200
#define PMC_PCK2     0x00000400

typedef struct 
{
    vu4 CR;
    vu4 MR;
    vu4 RDR;
    vu4 TDR;
    vu4 SR;
    vu4 IER;
    vu4 IDR;
    vu4 IMR;
    vu4 __0[4];
    vu4 CSR0;
    vu4 CSR1;
    vu4 CSR2;
    vu4 CSR3;
} AT91SPI;

#define AT91SPI0_ADDR ((AT91SPI*) BASE_SPI0)
#if AT91_SAM7X
#define AT91SPI1_ADDR ((AT91SPI*) BASE_SPI0)
#endif

/* CR bits */
#define SPI_SPIEN         0x00000001
#define SPI_SPIDIS        0x00000002
#define SPI_SWRST         0x00000080
#define SPI_LASTXFER      0x01000000

/* MR bits */
#define SPI_MSTR          0x00000001
#define SPI_PS            0x00000002
#define SPI_PCSDEC        0x00000004
#define SPI_MODFDIS       0x00000010
#define SPI_LLB           0x00000080
#define SPI_DLYBCS(n)     (((n) & 0xff) << 24)
#define SPI_PCS0          0x000e0000
#define SPI_PCS1          0x000d0000
#define SPI_PCS2          0x000b0000
#define SPI_PCS3          0x00070000

/* SR bits */
#define SPI_RDRF          0x00000001 /* recv data reg full */
#define SPI_TDRE          0x00000002 /* xmit data reg empty */
#define SPI_MODF          0x00000004 /* mode fault error */
#define SPI_OVRES         0x00000008 /* overrun error */
#define SPI_ENDRX         0x00000010 /* end of rx buffer */
#define SPI_ENDTX         0x00000020 /* end of tx buffer */
#define SPI_RXBUFF        0x00000040 /* rx buffer full */
#define SPI_TXBUFE        0x00000080 /* tx buffer empty */
#define SPI_NSSR          0x00000100 /* rising edge on NSS */
#define SPI_TXEMPTY       0x00000200 /* transmission regs empty */
#define SPI_SPIENS        0x00010000

typedef struct 
{
    vu4 FRM_NUM;
    vu4 GLB_STAT;
    vu4 FADDR;
    vu4 __0;
    vu4 IER;
    vu4 IDR;
    vu4 IMR;
    vu4 ISR;
    vu4 ICR;
    vu4 __1;
    vu4 RST_EP;
    vu4 __2;
    vu4 CSR0;
    vu4 CSR1;
    vu4 CSR2;
    vu4 CSR3;
    vu4 __3[4];
    vu4 FDR0;
    vu4 FDR1;
    vu4 FDR2;
    vu4 FDR3;
    vu4 __4[5];
    vu4 TXVC;
} AT91UDP;

#define AT91UDP_ADDR ((AT91UDP*) BASE_UDP)

// GLB_STAT bits
#define UDP_FADDEN    0x00000001
#define UDP_CONFG     0x00000002
#define UDP_ESR       0x00000004
#define UDP_RSMINPR   0x00000008
#define UDP_RMWUPE    0x00000010

// FADDR bits
#define UDP_FEN       0x00000100

// interrupt bits
#define UDP_EP0INT    0x00000001
#define UDP_EP1INT    0x00000002
#define UDP_EP2INT    0x00000004
#define UDP_EP3INT    0x00000008
#define UDP_RXSUSP    0x00000100
#define UDP_RXRSM     0x00000200
#define UDP_EXTRSM    0x00000400
#define UDP_SOFINT    0x00000800
#define UDP_ENDBUSRES 0x00001000
#define UDP_WAKEUP    0x00002000

// RST_EP bits
#define UDP_EP0       0x00000001
#define UDP_EP1       0x00000002
#define UDP_EP2       0x00000004
#define UDP_EP3       0x00000008

// CSR bits
#define UDP_TXCOMP         0x00000001
#define UDP_RX_DATA_BK0    0x00000002
#define UDP_RXSETUP        0x00000004
#define UDP_STALLSENT      0x00000008
#define UDP_ISOERROR       0x00000008
#define UDP_TXPKTRDY       0x00000010
#define UDP_FORCESTALL     0x00000020
#define UDP_RX_DATA_BK1    0x00000040
#define UDP_DIR            0x00000080

#define UDP_DTGL           0x00000800
#define UDP_EPEDS          0x00008000

#define UDP_TYPE_CONTROL     0x00000000
#define UDP_TYPE_ISOCH_OUT   0x00000100
#define UDP_TYPE_BULK_OUT    0x00000200
#define UDP_TYPE_INT_OUT     0x00000300
#define UDP_TYPE_ISOCH_IN    0x00000500
#define UDP_TYPE_BULK_IN     0x00000600
#define UDP_TYPE_INT_IN      0x00000700

typedef struct 
{
    vu4 SMR[32];
    vu4 SVR[32];
    vu4 IVR;
    vu4 FVR;
    vu4 ISR;
    vu4 IPR;
    vu4 IMR;
    vu4 CISR;
    vu4 __0[2];
    vu4 IECR;
    vu4 IDCR;
    vu4 ICCR;
    vu4 ISCR;
    vu4 EOICR;
    vu4 SPU;
    vu4 DCR;
    vu4 __1;
    vu4 FFER;
    vu4 FFDR;
    vu4 FFSR;
} AT91AIC;

#define AT91AIC_ADDR ((AT91AIC*) BASE_AIC)


typedef struct 
{
    vu4 CR;
    vu4 MR;
    vu4 IER;
    vu4 IDR;
    vu4 IMD;
    vu4 CSR;
    vu4 RHR;
    vu4 THR;
    vu4 BRGR;
    vu4 RTOR;
    vu4 TTGR;
    vu4 __0[5];
    vu4 FIDI;
    vu4 NER;
    vu4 __1;
    vu4 IF;
    vu4 MAN;
} AT91USART;

#define AT91USART0_ADDR ((AT91USART*) 0xFFFC0000)
#define AT91USART1_ADDR ((AT91USART*) 0xFFFC4000)

/* CR */
#define USART_RSTRX            0x00000004
#define USART_RSTTX            0x00000008
#define USART_RXEN             0x00000010
#define USART_RXDIS            0x00000020
#define USART_TXEN             0x00000040
#define USART_TXDIS            0x00000080
#define USART_RSTSTA           0x00000100
#define USART_STTBRK           0x00000200
#define USART_STPBRK           0x00000400
#define USART_STTTO            0x00000800
#define USART_SENDA            0x00001000
#define USART_RSTIT            0x00002000
#define USART_RSTNACK          0x00004000
#define USART_RETTO            0x00008000
#define USART_DTREN            0x00010000
#define USART_DTRDIS           0x00020000
#define USART_RTSEN            0x00040000
#define USART_RTSDIS           0x00080000

/* MR */
#define USART_MODE_NORMAL      0x00000000
#define USART_MODE_RS485       0x00000001
#define USART_MODE_HWHS        0x00000002
#define USART_MODE_MODEM       0x00000003
#define USART_MODE_ISO7816T0   0x00000004
#define USART_MODE_ISO7816T1   0x00000006
#define USART_MODE_IRDA        0x00000008

#define USART_CLK_MCK          0x00000000
#define USART_CLK_MCK_DIV      0x00000010
#define USART_CLK_SCK          0x00000030

#define USART_CHRL_5BITS       0x00000000
#define USART_CHRL_6BITS       0x00000040
#define USART_CHRL_7BITS       0x00000080
#define USART_CHRL_8BITS       0x000000C0

#define USART_SYNCHRONOUS      0x00000100

#define USART_PARITY_EVEN      0x00000000
#define USART_PARITY_ODD       0x00000200
#define USART_PARITY_SPACE     0x00000400
#define USART_PARITY_MARK      0x00000600
#define USART_PARITY_NONE      0x00000800
#define USART_PARITY_MULTIDROP 0x00000C00

#define USART_1STOP            0x00000000
#define USART_1X5STOP          0x00001000
#define USART_2STOP            0x00002000

#define USART_CHMODE_NORMAL    0x00000000
#define USART_CHMODE_ECHO      0x00004000
#define USART_CHMODE_LLOOP     0x00008000
#define USART_CHMODE_RLOOP     0x0000C000

#define USART_MSBF             0x00010000
#define USART_MODE9            0x00020000
#define USART_CLKO             0x00040000
#define USART_OVER             0x00080000
#define USART_INACK            0x00100000
#define USART_DSNACK           0x00200000
#define USART_VAR_SYNC         0x00400000

#define USART_FILTER           0x10000000
#define USART_MAN              0x20000000
#define USART_ONEBIT           0x80000000

/* CSR */
#define USART_RXRDY            0x00000001
#define USART_TXRDY            0x00000002
#define USART_RXBRK            0x00000004
#define USART_ENDRX            0x00000008
#define USART_ENDTX            0x00000010
#define USART_OVRE             0x00000020
#define USART_FRAME            0x00000040
#define USART_PARE             0x00000080
#define USART_TIMEOUT          0x00000100
#define USART_TXEMPTY          0x00000200
#define USART_ITERATION        0x00000400
#define USART_TXBUFE           0x00000800
#define USART_RXBUFF           0x00001000
#define USART_NACK             0x00002000


typedef struct 
{
    vu4 CR;
    vu4 SR;
    vu4 MR;
} AT91RSTC;

#define RSTC_KEY               0xA5000000

/* cr */
#define RSTC_PROCRST           0x00000001
#define RSTC_PERRST            0x00000004
#define RSTC_EXTRST            0x00000008

/* sr */
#define RSTC_URSTS             0x00000001
#define RSTC_BODSTS            0x00000002
#define RSTC_RSTTYP_MASK       0x00000070
#define RSTC_RSTTYP_COLD       0x00000000
#define RSTC_RSTTYP_WATCHDOG   0x00000020
#define RSTC_RSTTYP_SOFTWARE   0x00000030
#define RSTC_RSTTYP_NRST_PIN   0x00000040
#define RSTC_RSTTYP_BROWNOUT   0x00000060
#define RSTC_NRSTL             0x00010000
#define RSTC_SRCMP             0x00020000

/* mr */
#define RSTC_URSTEN            0x00000001
#define RSTC_URSTIEN           0x00000010
#define RSTC_ERSTL(n)          (((n) & 0xf) << 8)
#define RSTC_BODIEN            0x00010000

#define AT91RSTC_ADDR ((AT91RSTC*) BASE_RSTC)
    
#if AT91_SAM7X

typedef struct
{
    vu4 NCR;
    vu4 NCFG;
    vu4 NSR;
    vu4 __0;
    
    vu4 __1;
    vu4 TSR;
    vu4 RBQP;
    vu4 TBQP;
    
    vu4 RSR;
    vu4 ISR;
    vu4 IER;
    vu4 IDR;
    
    vu4 IMR;
    vu4 MAN;
    vu4 PTR;
    vu4 PFR;
    
    vu4 FTO;
    vu4 SCF;
    vu4 MCF;
    vu4 FRO;
    
    vu4 FCSE;
    vu4 ALE;
    vu4 DTF;
    vu4 LCOL;
    
    vu4 ECOL;
    vu4 TUND;
    vu4 CSE;
    vu4 RRE;
    
    vu4 ROV;
    vu4 RSE;
    vu4 ELE;
    vu4 RJA;
    
    vu4 USF;
    vu4 STE;
    vu4 RLE;
    vu4 __2;
    
    vu4 HRB;
    vu4 HRT;
    vu4 SA1B;
    vu4 SA1T;
    
    vu4 SA2B;
    vu4 SA2T;
    vu4 SA3B;
    vu4 SA3T;
    
    vu4 SA4B;
    vu4 SA5T;
    vu4 TID;
    vu4 __3;
    
    vu4 USRIO;
} AT91EMAC;


#define NCR_LB        0x00000001
#define NCR_LLB       0x00000002
#define NCR_RE        0x00000004
#define NCR_TE        0x00000008
#define NCR_MPE       0x00000010
#define NCR_CLRSTAT   0x00000020
#define NCR_INCSTAT   0x00000040
#define NCR_WESTAT    0x00000080
#define NCR_BP        0x00000100
#define NCR_TSTART    0x00000200
#define NCR_THALT     0x00000400

#define NCFG_SPD      0x00000001
#define NCFG_FD       0x00000002
#define NCFG_JFRAME   0x00000008
#define NCFG_CAF      0x00000010
#define NCFG_NBC      0x00000020
#define NCFG_MTI      0x00000040
#define NCFG_UNI      0x00000080
#define NCFG_BIG      0x00000100
#define NCFG_CLK_d8   0x00000000
#define NCFG_CLK_d16  0x00000400
#define NCFG_CLK_d32  0x00000800
#define NCFG_CLK_d64  0x00000C00
#define NCFG_RTY      0x00001000
#define NCFG_PAE      0x00002000
#define NCFG_RBOF_0   0x00000000
#define NCFG_RBOF_1   0x00004000
#define NCFG_RBOF_2   0x00008000
#define NCFG_RBOF_3   0x0000C000
#define NCFG_RLCE     0x00010000
#define NCFG_DRFCS    0x00020000
#define NCFG_EFRHD    0x00040000
#define NCFG_IRXFCS   0x00080000

#define NSR_MDIO      0x00000002
#define NSR_IDLE      0x00000004

#define TSR_UBR       0x00000001
#define TSR_COL       0x00000002
#define TSR_RLE       0x00000004
#define TSR_TGO       0x00000008
#define TSR_BEX       0x00000010
#define TSR_COMP      0x00000020
#define TSR_UND       0x00000040

#define RSR_BNA       0x00000001
#define RSR_REC       0x00000002
#define RSR_OVR       0x00000004

#define ISR_MFD       0x00000001
#define ISR_RCOMP     0x00000002
#define ISR_RXUBR     0x00000004
#define ISR_TXUBR     0x00000008
#define ISR_TUND      0x00000010
#define ISR_RLE       0x00000020
#define ISR_TXERR     0x00000040
#define ISR_TCOMP     0x00000080
#define ISR_ROVR      0x00000400
#define ISR_HRESP     0x00000800
#define ISR_PFR       0x00001000
#define ISR_PTZ       0x00002000

#define USRIO_RMII    0x00000001
#define USRIO_CLKEN   0x00000002

#define AT91EMAC_ADDR ((AT91EMAC*) BASE_EMAC)


typedef struct 
{
    vu4 addr;
    vu4 info;
} emac_xmit_entry;

#define XMIT_USED           0x80000000
#define XMIT_WRAP           0x40000000
#define XMIT_ERR_RETRY      0x20000000
#define XMIT_ERR_UNDERRUN   0x10000000
#define XMIT_ERR_EXHAUSTED  0x08000000
#define XMIT_NO_CRC         0x00010000
#define XMIT_LAST           0x00008000
#define XMIT_LENGTH(n)      ((n) & 0x3FF)


/* CAN Registers */


typedef struct
{
    vu4 MMR;		/* Mailbox Mode Register */
    vu4 MAM;		/* Mailbox Acceptance Mask Register */
    vu4 MID;		/* Mailbox ID Register */
    vu4 MFID;		/* Mailbox Family ID Register */
    vu4 MSR;		/* Mailbox Status Register */
    vu4 MDL;		/* Mailbox Data Low Register */
    vu4 MDH;		/* Mailbox Data High Register */
    vu4 MCR;		/* Mailbox Control Register */
}  AT91CAN_MAILBOX;

typedef struct
{
    vu4 MR;		/* Mode Register */
    vu4 IER;		/* Interrupt Enable Register */
    vu4 IDR;		/* Interrupt Disable Register */
    vu4 IMR;		/* Interrupt Mask Register */
    vu4 SR;		/* Status Register */
    vu4 BR;		/* Baudrate Register */
    vu4 TIM;		/* Timer Register */
    vu4 TIMESTP;	/* Timestamp Register */
    vu4 ECR;		/* Error Counter Register */
    vu4 TCR;		/* Transfer Command Register */
    vu4 ACR;		/* Abort Command Register */
    	
    vu4 __0[53];	/* 0x002c - 0x0100 is undefined */
    vu4 __1[63];	/* 0x0200 - 0x01fc is reserved */
    AT91CAN_MAILBOX Mailbox[8];
} AT91CAN;

#define CAN_CANEN      0x00000001      /* CAN Controller Enable */
#define CAN_LPM        0x00000002      /* Enable Low Power Mode */
#define CAN_ABM        0x00000004      /* Enable Autoband/Listen Mode */
#define CAN_OVL        0x00000008      /* Enable Overload Frame */
#define CAN_TEOF       0x00000010      /* Timestamp Messages at each Frame */
#define CAN_TTM        0x00000020      /* Enable Time Trigger Mode */
#define CAN_TIMFRZ     0x00000040      /* Enable Timer Freeze */
#define CAN_DRPT       0x00000080      /* Disable Repeat */

#define CAN_MB(x)    (0x00000001 << x) /* Enable Interrupt Enable */
#define CAN_ERRA      0x00010000      /* Enable Error Active Mode Interrupt */
#define CAN_WARN      0x00020000      /* Enable Warning Limit Interrupt */
#define CAN_ERRP      0x00040000      /* Enable Passive mode interrupt */
#define CAN_BOFF      0x00080000      /* Enable Bus-off mode interrupt */
#define CAN_SLEEP     0x00100000      /* Enable Sleep Interrupt */
#define CAN_WAKEUP    0x00200000      /* Enable Wakeup Interrupt */
#define CAN_TOVF      0x00400000      /* Enable Timer Overflow Interrupt */
#define CAN_TSTP      0x00800000      /* Enable TimeStamp Interrupt */
#define CAN_CERR      0x01000000      /* Enable CRC Error Interrupt */
#define CAN_SERR      0x02000000      /* Enable Stuffing Error Interrupt */
#define CAN_AERR      0x04000000      /* Enable Acknowledgement Error Int */
#define CAN_FERR      0x08000000      /* Enable Form Error Interrupt */
#define CAN_BERR      0x10000000      /* Enable Bit Error Interrupt */

#define CAN_RBSY      0x20000000      /* Receiver Busy */
#define CAN_TBSY      0x40000000      /* Transmitter Busy */
#define CAN_OVLSY     0x80000000      /* Overload Busy */

/* Can Baudrate Regiister */

#define CAN_PHASE2(x)	(x)
#define CAN_PHASE2_MASK	0x07
#define CAN_PHASE1(x)	(x<<4)
#define CAN_PHASE1_MASK	(0x07 << 4)
#define CAN_PROPAG(x)	(x<<8)
#define CAN_PROPAG_MASK	(0x07 << 8)
#define CAN_SJW(x)	(x<<12)
#define CAN_SJW_MASK(x)	(0x03 << 12)
#define CAN_BRP(x)	(x<<16)
#define CAN_BRP_MASK	(0x7f << 16)
#define CAN_SMP	     0x01000000	     /* Sampling Mode */

/* CAN Transfer Command Register */

#define TCR_TIMRST   0x80000000	     /* Timer Reset */

/* CAN Message Mode Register */

#define CAN_MTIMEMARK(x)   (0x0000001 << x)
#define CAN_PRIOR(x)	(x << 16)
#define CAN_MOT(x)	(x << 24)

#define CAN_MIDVB(x)	(x)
#define CAN_MIDVA(x)	(x << 18)
#define CAN_MIDE	0x20000000


/* CAN MSRx */

/* These are receive, so pass in the value of the register... */

#define CAN_MTIMESTAMP(x)  (x & 0x0000ffff)
#define CAN_MDLC(x)	   ( (x >> 16) & 0x0f)  /* Mailbox code length */
#define CAN_MRTR	0x00100000              /* Mailbox Remote Trx Request*/
#define CAN_MABT	0x00400000              /* Mailbox Message Abort  */
#define CAN_MRDY	0x00800000              /* Mailbox Ready */
#define CAN_MMI 	0x01000000              /* Mailbox Message Ignored */

/* Message Control Register */

//#define CAN_MDLC(x)	(x<<16)	        /* Mailbox Data Length Code */
#define CAN_MACR	(0x01 << 22)	/* Abort Request */
#define CAN_MTCR	(0x01 << 23)	/* Mailbox Transfer Command */


#define AT91CAN_ADDR ((AT91CAN*) BASE_CAN)



#endif

#endif
