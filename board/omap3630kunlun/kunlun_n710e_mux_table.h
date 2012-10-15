#define MUX_DEFAULT_ES2()\
	/*SDRC*/\
	MUX_VAL(CP(SDRC_D0),        (IEN  | PTD | DIS | M0)) /*SDRC_D0*/\
	MUX_VAL(CP(SDRC_D1),        (IEN  | PTD | DIS | M0)) /*SDRC_D1*/\
	MUX_VAL(CP(SDRC_D2),        (IEN  | PTD | DIS | M0)) /*SDRC_D2*/\
	MUX_VAL(CP(SDRC_D3),        (IEN  | PTD | DIS | M0)) /*SDRC_D3*/\
	MUX_VAL(CP(SDRC_D4),        (IEN  | PTD | DIS | M0)) /*SDRC_D4*/\
	MUX_VAL(CP(SDRC_D5),        (IEN  | PTD | DIS | M0)) /*SDRC_D5*/\
	MUX_VAL(CP(SDRC_D6),        (IEN  | PTD | DIS | M0)) /*SDRC_D6*/\
	MUX_VAL(CP(SDRC_D7),        (IEN  | PTD | DIS | M0)) /*SDRC_D7*/\
	MUX_VAL(CP(SDRC_D8),        (IEN  | PTD | DIS | M0)) /*SDRC_D8*/\
	MUX_VAL(CP(SDRC_D9),        (IEN  | PTD | DIS | M0)) /*SDRC_D9*/\
	MUX_VAL(CP(SDRC_D10),       (IEN  | PTD | DIS | M0)) /*SDRC_D10*/\
	MUX_VAL(CP(SDRC_D11),       (IEN  | PTD | DIS | M0)) /*SDRC_D11*/\
	MUX_VAL(CP(SDRC_D12),       (IEN  | PTD | DIS | M0)) /*SDRC_D12*/\
	MUX_VAL(CP(SDRC_D13),       (IEN  | PTD | DIS | M0)) /*SDRC_D13*/\
	MUX_VAL(CP(SDRC_D14),       (IEN  | PTD | DIS | M0)) /*SDRC_D14*/\
	MUX_VAL(CP(SDRC_D15),       (IEN  | PTD | DIS | M0)) /*SDRC_D15*/\
	MUX_VAL(CP(SDRC_D16),       (IEN  | PTD | DIS | M0)) /*SDRC_D16*/\
	MUX_VAL(CP(SDRC_D17),       (IEN  | PTD | DIS | M0)) /*SDRC_D17*/\
	MUX_VAL(CP(SDRC_D18),       (IEN  | PTD | DIS | M0)) /*SDRC_D18*/\
	MUX_VAL(CP(SDRC_D19),       (IEN  | PTD | DIS | M0)) /*SDRC_D19*/\
	MUX_VAL(CP(SDRC_D20),       (IEN  | PTD | DIS | M0)) /*SDRC_D20*/\
	MUX_VAL(CP(SDRC_D21),       (IEN  | PTD | DIS | M0)) /*SDRC_D21*/\
	MUX_VAL(CP(SDRC_D22),       (IEN  | PTD | DIS | M0)) /*SDRC_D22*/\
	MUX_VAL(CP(SDRC_D23),       (IEN  | PTD | DIS | M0)) /*SDRC_D23*/\
	MUX_VAL(CP(SDRC_D24),       (IEN  | PTD | DIS | M0)) /*SDRC_D24*/\
	MUX_VAL(CP(SDRC_D25),       (IEN  | PTD | DIS | M0)) /*SDRC_D25*/\
	MUX_VAL(CP(SDRC_D26),       (IEN  | PTD | DIS | M0)) /*SDRC_D26*/\
	MUX_VAL(CP(SDRC_D27),       (IEN  | PTD | DIS | M0)) /*SDRC_D27*/\
	MUX_VAL(CP(SDRC_D28),       (IEN  | PTD | DIS | M0)) /*SDRC_D28*/\
	MUX_VAL(CP(SDRC_D29),       (IEN  | PTD | DIS | M0)) /*SDRC_D29*/\
	MUX_VAL(CP(SDRC_D30),       (IEN  | PTD | DIS | M0)) /*SDRC_D30*/\
	MUX_VAL(CP(SDRC_D31),       (IEN  | PTD | DIS | M0)) /*SDRC_D31*/\
	MUX_VAL(CP(SDRC_CLK),       (IEN  | PTD | DIS | M0)) /*SDRC_CLK*/\
	MUX_VAL(CP(SDRC_DQS0),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS0*/\
	MUX_VAL(CP(SDRC_DQS1),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS1*/\
	MUX_VAL(CP(SDRC_DQS2),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS2*/\
	MUX_VAL(CP(SDRC_DQS3),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS3*/\
	/*GPMC*/\
	MUX_VAL(CP(GPMC_A1),        (IDIS | PTD | DIS | M0)) /*GPMC_A1*/\
	MUX_VAL(CP(GPMC_A2),        (IDIS | PTD | DIS | M0)) /*GPMC_A2*/\
	MUX_VAL(CP(GPMC_A3),        (IDIS | PTD | DIS | M0)) /*GPMC_A3*/\
	MUX_VAL(CP(GPMC_A4),        (IDIS | PTD | DIS | M0)) /*GPMC_A4*/\
	MUX_VAL(CP(GPMC_A5),        (IDIS | PTD | DIS | M0)) /*GPMC_A5*/\
	MUX_VAL(CP(GPMC_A6),        (IDIS | PTD | DIS | M0)) /*GPMC_A6*/\
	MUX_VAL(CP(GPMC_A7),        (IDIS | PTD | DIS | M0)) /*GPMC_A7*/\
	MUX_VAL(CP(GPMC_A8),        (IDIS | PTD | DIS | M0)) /*GPMC_A8*/\
	MUX_VAL(CP(GPMC_A9),        (IDIS | PTD | DIS | M0)) /*GPMC_A9*/\
	MUX_VAL(CP(GPMC_A10),       (IDIS | PTD | DIS | M0)) /*GPMC_A10*/\
	MUX_VAL(CP(GPMC_D0),        (IEN  | PTD | DIS | M0)) /*GPMC_D0*/\
	MUX_VAL(CP(GPMC_D1),        (IEN  | PTD | DIS | M0)) /*GPMC_D1*/\
	MUX_VAL(CP(GPMC_D2),        (IEN  | PTD | DIS | M0)) /*GPMC_D2*/\
	MUX_VAL(CP(GPMC_D3),        (IEN  | PTD | DIS | M0)) /*GPMC_D3*/\
	MUX_VAL(CP(GPMC_D4),        (IEN  | PTD | DIS | M0)) /*GPMC_D4*/\
	MUX_VAL(CP(GPMC_D5),        (IEN  | PTD | DIS | M0)) /*GPMC_D5*/\
	MUX_VAL(CP(GPMC_D6),        (IEN  | PTD | DIS | M0)) /*GPMC_D6*/\
	MUX_VAL(CP(GPMC_D7),        (IEN  | PTD | DIS | M0)) /*GPMC_D7*/\
	MUX_VAL(CP(GPMC_D8),        (IEN  | PTD | DIS | M0)) /*GPMC_D8*/\
	MUX_VAL(CP(GPMC_D9),        (IEN  | PTD | DIS | M0)) /*GPMC_D9*/\
	MUX_VAL(CP(GPMC_D10),       (IEN  | PTD | DIS | M0)) /*GPMC_D10*/\
	MUX_VAL(CP(GPMC_D11),       (IEN  | PTD | DIS | M0)) /*GPMC_D11*/\
	MUX_VAL(CP(GPMC_D12),       (IEN  | PTD | DIS | M0)) /*GPMC_D12*/\
	MUX_VAL(CP(GPMC_D13),       (IEN  | PTD | DIS | M0)) /*GPMC_D13*/\
	MUX_VAL(CP(GPMC_D14),       (IEN  | PTD | DIS | M0)) /*GPMC_D14*/\
	MUX_VAL(CP(GPMC_D15),       (IEN  | PTD | DIS | M0)) /*GPMC_D15*/\
	MUX_VAL(CP(GPMC_nCS0),      (IDIS | PTU | EN  | M0)) /*GPMC_nCS0*/\
	MUX_VAL(CP(GPMC_nCS1),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(GPMC_nCS2),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(GPMC_nCS3),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(GPMC_nCS4),      (IDIS | PTD | DIS | M2)) /*FM I2S_CLK*/\
	MUX_VAL(CP(GPMC_nCS5),      (IEN  | PTD | DIS | M2)) /*FM I2S_DR*/\
	MUX_VAL(CP(GPMC_nCS6),      (IEN  | PTD | DIS | M2)) /*FM I2S_DX*/\
	MUX_VAL(CP(GPMC_nCS7),      (IEN  | PTU | EN  | M2)) /*FM I2S_FS*/\
	MUX_VAL(CP(GPMC_CLK),       (IDIS | PTD | DIS | M0)) /*GPMC_CLK*/\
	MUX_VAL(CP(GPMC_nADV_ALE),  (IDIS | PTD | DIS | M0)) /*GPMC_nADV_ALE*/\
	MUX_VAL(CP(GPMC_nOE),       (IDIS | PTD | DIS | M0)) /*GPMC_nOE*/\
	MUX_VAL(CP(GPMC_nWE),       (IDIS | PTD | DIS | M0)) /*GPMC_nWE*/\
	MUX_VAL(CP(GPMC_nBE0_CLE),  (IDIS | PTD | DIS | M0)) /*GPMC_nBE0_CLE*/\
	MUX_VAL(CP(GPMC_nBE1),      (IEN  | PTD | DIS | M0)) /*GPMC_nBE1 lab*/\
	MUX_VAL(CP(GPMC_nWP),       (IEN  | PTD | DIS | M0)) /*GPMC_nWP*/\
	MUX_VAL(CP(GPMC_WAIT0),     (IEN  | PTU | EN  | M0)) /*GPMC_WAIT0*/\
	MUX_VAL(CP(GPMC_WAIT1),     (IEN  | PTU | EN  | M0)) /*GPMC_WAIT1*/\
	MUX_VAL(CP(GPMC_WAIT2),     (IEN  | PTU | EN  | M0)) /*GPMC_WAIT2*/\
	MUX_VAL(CP(GPMC_WAIT3),     (IEN  | PTU | DIS | M1)) /*SYS_NDMA_REQ1*/\
	/*DSS*/\
	MUX_VAL(CP(DSS_PCLK),       (IDIS | PTD | DIS | M0)) /*DSS_PCLK*/\
	MUX_VAL(CP(DSS_HSYNC),      (IDIS | PTD | DIS | M0)) /*DSS_HSYNC*/\
	MUX_VAL(CP(DSS_VSYNC),      (IDIS | PTD | DIS | M0)) /*DSS_VSYNC*/\
	MUX_VAL(CP(DSS_ACBIAS),     (IDIS | PTD | DIS | M0)) /*DSS_ACBIAS*/\
	MUX_VAL(CP(DSS_DATA0),      (IEN  | PTD | DIS | M0)) /*DSS_DATA0*/\
	MUX_VAL(CP(DSS_DATA1),      (IEN  | PTD | DIS | M0)) /*DSS_DATA1*/\
	MUX_VAL(CP(DSS_DATA2),      (IEN  | PTD | DIS | M0)) /*DSS_DATA2*/\
	MUX_VAL(CP(DSS_DATA3),      (IEN  | PTD | DIS | M0)) /*DSS_DATA3*/\
	MUX_VAL(CP(DSS_DATA4),      (IEN  | PTD | DIS | M0)) /*DSS_DATA4*/\
	MUX_VAL(CP(DSS_DATA5),      (IEN  | PTD | DIS | M0)) /*DSS_DATA5*/\
	MUX_VAL(CP(DSS_DATA6),      (IEN  | PTD | DIS | M0)) /*DSS_DATA6*/\
	MUX_VAL(CP(DSS_DATA7),      (IEN  | PTD | DIS | M0)) /*DSS_DATA7*/\
	MUX_VAL(CP(DSS_DATA8),      (IEN  | PTD | DIS | M0)) /*DSS_DATA8*/\
	MUX_VAL(CP(DSS_DATA9),      (IEN  | PTD | DIS | M0)) /*DSS_DATA9*/\
	MUX_VAL(CP(DSS_DATA10),     (IEN  | PTD | DIS | M0)) /*DSS_DATA10*/\
	MUX_VAL(CP(DSS_DATA11),     (IEN  | PTD | DIS | M0)) /*DSS_DATA11*/\
	MUX_VAL(CP(DSS_DATA12),     (IEN  | PTD | DIS | M0)) /*DSS_DATA12*/\
	MUX_VAL(CP(DSS_DATA13),     (IEN  | PTD | DIS | M0)) /*DSS_DATA13*/\
	MUX_VAL(CP(DSS_DATA14),     (IEN  | PTD | DIS | M0)) /*DSS_DATA14*/\
	MUX_VAL(CP(DSS_DATA15),     (IEN  | PTD | DIS | M0)) /*DSS_DATA15*/\
	MUX_VAL(CP(DSS_DATA16),     (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(DSS_DATA17),     (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(DSS_DATA18),     (IDIS | PTD | DIS | M7)) /* [NC] */\
	MUX_VAL(CP(DSS_DATA19),     (IDIS | PTD | DIS | M7)) /* [NC] */\
	MUX_VAL(CP(DSS_DATA20),     (IDIS | PTD | DIS | M7)) /* [NC] */\
	MUX_VAL(CP(DSS_DATA21),     (IDIS | PTD | DIS | M7)) /* [NC] */\
	MUX_VAL(CP(DSS_DATA22),     (IDIS | PTD | DIS | M4)) /*LCD_RS GPIO_92*/\
	MUX_VAL(CP(DSS_DATA23),     (IDIS | PTU | EN  | M4)) /*LCD_RST GPIO_93*/\
	/*CAMERA*/\
	MUX_VAL(CP(CAM_HS ),        (IEN  | PTD | DIS | M0)) /*CAM_HS */\
	MUX_VAL(CP(CAM_VS ),        (IEN  | PTD | DIS | M0)) /*CAM_VS */\
	MUX_VAL(CP(CAM_XCLKA),      (IDIS | PTD | DIS | M0)) /*CAM_XCLKA*/\
	MUX_VAL(CP(CAM_PCLK),       (IEN  | PTD | DIS | M0)) /*CAM_PCLK*/\
	MUX_VAL(CP(CAM_FLD),        (IDIS | PTD | EN  | M4)) /*CAM_RST GPIO_98*/\
	MUX_VAL(CP(CAM_D0 ),        (IEN  | PTD | DIS | M0)) /*CAM_D0 */\
	MUX_VAL(CP(CAM_D1 ),        (IEN  | PTD | DIS | M0)) /*CAM_D1 */\
	MUX_VAL(CP(CAM_D2 ),        (IEN  | PTD | DIS | M0)) /*CAM_D2*/\
	MUX_VAL(CP(CAM_D3 ),        (IEN  | PTD | DIS | M0)) /*CAM_D3*/\
	MUX_VAL(CP(CAM_D4 ),        (IEN  | PTD | DIS | M0)) /*CAM_D4*/\
	MUX_VAL(CP(CAM_D5 ),        (IEN  | PTD | DIS | M0)) /*CAM_D5*/\
	MUX_VAL(CP(CAM_D6 ),        (IEN  | PTD | DIS | M0)) /*CAM_D6*/\
	MUX_VAL(CP(CAM_D7 ),        (IEN  | PTD | DIS | M0)) /*CAM_D7*/\
	MUX_VAL(CP(CAM_D8 ),        (IDIS | PTD | DIS | M7)) /*CAM_PWDN GPIO_107*/\
	MUX_VAL(CP(CAM_D9 ),        (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(CAM_D10),        (IDIS | PTU | EN  | M4)) /*BT_RST_N GPIO_109*/\
	MUX_VAL(CP(CAM_D11),        (IDIS | PTD | EN  | M4)) /*CAM_PWDN GPIO_110*/\
	MUX_VAL(CP(CAM_XCLKB),      (IDIS | PTD | EN  | M4)) /* V_CAM_EN GPIO_111*/\
	MUX_VAL(CP(CAM_WEN),        (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(CAM_STROBE),     (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(CSI2_DX0),       (IEN  | PTD | DIS | M4)) /*VFAULT_N GPIO_112*/\
	MUX_VAL(CP(CSI2_DY0),       (IEN  | PTU | EN  | M4)) /* ASL_INT GPIO_113*/\
	MUX_VAL(CP(CSI2_DX1),       (IDIS | PTD | DIS | M7)) /* [NC] GPIO_114 */\
	MUX_VAL(CP(CSI2_DY1),       (IEN  | PTD | EN  | M4)) /*ACC_IRQ2 GPIO_115*/\
	/*Audio Interface */\
	MUX_VAL(CP(McBSP2_FSX),     (IEN  | PTD | DIS | M0)) /*TWL5030 I2S_FSX*/\
	MUX_VAL(CP(McBSP2_CLKX),    (IEN  | PTD | DIS | M0)) /*TWL5030 I2S_CLKX*/\
	MUX_VAL(CP(McBSP2_DR),      (IEN  | PTD | DIS | M0)) /*TWL5030 I2S_DR*/\
	MUX_VAL(CP(McBSP2_DX),      (IDIS | PTD | DIS | M0)) /*TWL5030 I2S_DX*/\
	/* SD card  */\
	MUX_VAL(CP(MMC1_CLK),       (IDIS | PTD | DIS | M0)) /*SD_CLK*/\
	MUX_VAL(CP(MMC1_CMD),       (IEN  | PTU | EN  | M0)) /*SD_CMD*/\
	MUX_VAL(CP(MMC1_DAT0),      (IEN  | PTU | EN  | M0)) /*SD_DAT0*/\
	MUX_VAL(CP(MMC1_DAT1),      (IEN  | PTU | EN  | M0)) /*SD_DAT1*/\
	MUX_VAL(CP(MMC1_DAT2),      (IEN  | PTU | EN  | M0)) /*SD_DAT2*/\
	MUX_VAL(CP(MMC1_DAT3),      (IEN  | PTU | EN  | M0)) /*SD_DAT3*/\
	/* AP Modem status */\
	MUX_VAL(CP(GPIO126),        (IDIS | PTD | EN | M4)) /*MDM_PWR_EN GPIO_126*/\
	MUX_VAL(CP(GPIO127),        (IDIS | PTD | EN | M4)) /*MDM_RSV1 GPIO_127*/\
	MUX_VAL(CP(GPIO128),        (IDIS | PTU | EN | M4)) /*AP_READY GPIO_128*/\
	MUX_VAL(CP(GPIO129),        (IEN  | PTD | EN | M4)) /*MDM_READY GPIO_129*/\
	/*EMMC TBD*/\
	MUX_VAL(CP(MMC2_CLK),       (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_CMD),       (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT0),      (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT1),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT2),      (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT3),      (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT4),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT5),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT6),      (IDIS | PTU | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(MMC2_DAT7),      (IEN  | PTU | DIS | M4)) /*SD_CD GPIO_139*/\
	/* pcm out */\
	MUX_VAL(CP(McBSP3_DX),      (IDIS | PTD | DIS | M0)) /*McBSP3_DX, BT_TWL5030_MODEM_PCM*/\
	MUX_VAL(CP(McBSP3_DR),      (IEN  | PTD | DIS | M0)) /*McBSP3_DR, BT_TWL5030_MODEM_PCM*/\
	MUX_VAL(CP(McBSP3_CLKX),    (IEN  | PTD | DIS | M0)) /*McBSP3_CLKX, BT_TWL5030_MODEM_PCM*/\
	MUX_VAL(CP(McBSP3_FSX),     (IEN  | PTD | DIS | M0)) /*McBSP3_FSX, BT_TWL5030_MODEM_PCM*/\
	/* uart2, BT */\
	MUX_VAL(CP(UART2_CTS),      (IEN  | PTU | EN  | M0)) /*UART2_CTS, BT*/\
	MUX_VAL(CP(UART2_RTS),      (IDIS | PTD | DIS | M0)) /*UART2_RTS, BT*/\
	MUX_VAL(CP(UART2_TX),       (IDIS | PTD | DIS | M0)) /*UART2_TX, BT*/\
	MUX_VAL(CP(UART2_RX),       (IEN  | PTD | DIS | M0)) /*UART2_RX, BT*/\
	/*uart1 Modem */\
	MUX_VAL(CP(UART1_TX),       (IDIS | PTD | DIS | M0)) /*UART1_TX*/\
	MUX_VAL(CP(UART1_RTS),      (IDIS | PTD | EN  | M4)) /*MDM_RST GPIO_149*/\
	MUX_VAL(CP(UART1_CTS),      (IDIS | PTD | EN  | M4)) /*UART1_EN GPIO_150*/\
	MUX_VAL(CP(UART1_RX),       (IEN  | PTD | EN  | M0)) /*UART1_RX*/\
	/*gpio setting */\
	MUX_VAL(CP(McBSP4_CLKX),    (IDIS | PTU | EN  | M4)) /*OVP_REV_N GPIO_152*/\
	MUX_VAL(CP(McBSP4_DR),      (IEN  | PTU | EN  | M4)) /*TOUCH_IRQ_N GPIO_153*/\
	MUX_VAL(CP(McBSP4_DX),      (IDIS | PTD | DIS | M7)) /* [NC] GPIO_154*/\
	MUX_VAL(CP(McBSP4_FSX),     (IDIS | PTD | EN  | M4)) /*LCD_BL_EN GPIO_155*/\
	/* per func*/\
	MUX_VAL(CP(McBSP1_CLKR),    (IDIS | PTD | EN  | M4)) /*FM FM_RX_EN GPIO_156*/\
	MUX_VAL(CP(McBSP1_FSR),     (IDIS | PTD | EN  | M4)) /*FM FM_TX_EN GPIO_157*/\
	MUX_VAL(CP(McBSP1_DX),      (IDIS | PTD | EN  | M4)) /*WL WL_SHDN_N GPIO_158*/\
	MUX_VAL(CP(McBSP1_DR),      (IDIS | PTD | DIS | M7)) /* [NC] GPIO_159*/\
	MUX_VAL(CP(McBSP_CLKS),     (IEN  | PTU | EN  | M0)) /*TWL5030_CLK256FS*/\
	MUX_VAL(CP(McBSP1_FSX),     (IDIS | PTD | EN  | M4)) /*FM FM_EN GPIO_161*/\
	MUX_VAL(CP(McBSP1_CLKX),    (IEN  | PTD | EN  | M4)) /*WLAN IRQ GPIO_162*/\
	/*uart3 console*/\
	MUX_VAL(CP(UART3_CTS_RCTX), (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(UART3_RTS_SD),   (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(UART3_RX_IRRX),  (IEN  | PTU | EN  | M0)) /*UART3_RX_IRRX*/\
	MUX_VAL(CP(UART3_TX_IRTX),  (IDIS | PTU | EN  | M0)) /*UART3_TX_IRTX*/\
	/*husb0*/\
	MUX_VAL(CP(HSUSB0_CLK),     (IEN  | PTD | DIS | M0)) /*HSUSB0_CLK*/\
	MUX_VAL(CP(HSUSB0_STP),     (IDIS | PTU | EN  | M0)) /*HSUSB0_STP*/\
	MUX_VAL(CP(HSUSB0_DIR),     (IEN  | PTD | DIS | M0)) /*HSUSB0_DIR*/\
	MUX_VAL(CP(HSUSB0_NXT),     (IEN  | PTD | DIS | M0)) /*HSUSB0_NXT*/\
	MUX_VAL(CP(HSUSB0_DATA0),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA0 */\
	MUX_VAL(CP(HSUSB0_DATA1),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA1 */\
	MUX_VAL(CP(HSUSB0_DATA2),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA2 */\
	MUX_VAL(CP(HSUSB0_DATA3),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA3 */\
	MUX_VAL(CP(HSUSB0_DATA4),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA4 */\
	MUX_VAL(CP(HSUSB0_DATA5),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA5 */\
	MUX_VAL(CP(HSUSB0_DATA6),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA6 */\
	MUX_VAL(CP(HSUSB0_DATA7),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA7 */\
	/*i2c*/\
	MUX_VAL(CP(I2C1_SCL),       (IEN  | PTU | EN  | M0)) /*I2C1_SCL, TWL5030*/\
	MUX_VAL(CP(I2C1_SDA),       (IEN  | PTU | EN  | M0)) /*I2C1_SDA, TWL5030*/\
	MUX_VAL(CP(I2C2_SCL),       (IEN  | PTU | EN  | M0)) /*I2C2_SCL, FT5X0X Touch*/\
	MUX_VAL(CP(I2C2_SDA),       (IEN  | PTU | EN  | M0)) /*I2C2_SDA, FT5X0X Touch*/\
	MUX_VAL(CP(I2C3_SCL),       (IEN  | PTU | EN  | M0)) /*I2C3_SCL, KXUD9*/\
	MUX_VAL(CP(I2C3_SDA),       (IEN  | PTU | EN  | M0)) /*I2C3_SDA, KXUD9*/\
	MUX_VAL(CP(I2C4_SCL),       (IEN  | PTU | EN  | M0)) /*I2C4_SCL, TWL5030*/\
	MUX_VAL(CP(I2C4_SDA),       (IEN  | PTU | EN  | M0)) /*I2C4_SDA, TWL5030*/\
	MUX_VAL(CP(HDQ_SIO),        (IEN  | PTU | EN  | M4)) /*MANU_MODE GPIO_170*/\
	/*spi1*/\
	MUX_VAL(CP(McSPI1_CLK),     (IDIS | PTD | DIS | M0)) /*LCD LCD_SPI_CLK*/\
	MUX_VAL(CP(McSPI1_SIMO),    (IDIS | PTD | DIS | M0)) /*LCD LCD_SPI_SIMO */\
	MUX_VAL(CP(McSPI1_SOMI),    (IEN  | PTD | EN  | M4)) /*MDM_RSV2 GPIO_173*/\
	MUX_VAL(CP(McSPI1_CS0),     (IDIS | PTD | DIS | M0)) /*LCD LCD_SPI_CS*/\
	MUX_VAL(CP(McSPI1_CS1),     (IDIS | PTD | EN  | M3)) /*WLAN MMC3_CMD*/\
	MUX_VAL(CP(McSPI1_CS2),     (IDIS | PTD | DIS | M3)) /*WLAN MMC3_CLK*/\
	MUX_VAL(CP(McSPI1_CS3),     (IEN  | PTD | DIS | M4)) /*CBP DIGITAL USB, USB2_TXDAT, Input GPIO*/\
	/*spi2 TBD */\
	MUX_VAL(CP(McSPI2_CLK),     (IDIS | PTD | DIS | M7)) /* [NC] C_PCM_GPIO_EN GPIO 178 */\
	MUX_VAL(CP(McSPI2_SIMO),    (IDIS | PTD | EN  | M4)) /* TOUCH_RST GPIO 179 */\
	MUX_VAL(CP(McSPI2_SOMI),    (IDIS | PTD | DIS | M7)) /* [NC] ASL_SCL GPIO 180 */\
	MUX_VAL(CP(McSPI2_CS0),     (IDIS | PTD | DIS | M7)) /* [NC] ASL_SDA GPIO 181 */\
	MUX_VAL(CP(McSPI2_CS1),     (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	/*Control and JTAG*/\
	MUX_VAL(CP(SYS_32K),        (IEN  | PTD | DIS | M0)) /*SYS_32K*/\
	MUX_VAL(CP(SYS_CLKREQ),     (IEN  | PTD | DIS | M0)) /*SYS_CLKREQ*/\
	MUX_VAL(CP(SYS_nIRQ),       (IEN  | PTU | EN  | M0)) /*SYS_nIRQ*/\
	MUX_VAL(CP(SYS_BOOT0),      (IEN  | PTD | DIS | M0)) /*SYSBOOT0*/\
	MUX_VAL(CP(SYS_BOOT1),      (IEN  | PTD | DIS | M0)) /*SYSBOOT1*/\
	MUX_VAL(CP(SYS_BOOT2),      (IEN  | PTD | DIS | M0)) /*SYSBOOT2*/\
	MUX_VAL(CP(SYS_BOOT3),      (IEN  | PTD | DIS | M0)) /*SYSBOOT3*/\
	MUX_VAL(CP(SYS_BOOT4),      (IEN  | PTD | DIS | M0)) /*SYSBOOT4*/\
	MUX_VAL(CP(SYS_BOOT5),      (IEN  | PTD | DIS | M0)) /*SYSBOOT5*/\
	MUX_VAL(CP(SYS_BOOT6),      (IEN  | PTD | DIS | M0)) /*SYSBOOT6*/\
	MUX_VAL(CP(SYS_OFF_MODE),   (IEN  | PTD | DIS | M0)) /*SYS_OFF_MODE*/\
	MUX_VAL(CP(SYS_CLKOUT1),    (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(SYS_CLKOUT2),    (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(JTAG_nTRST),     (IEN  | PTD | DIS | M0)) /*JTAG_nTRST*/\
	MUX_VAL(CP(JTAG_TCK),       (IEN  | PTD | DIS | M0)) /*JTAG_TCK*/\
	MUX_VAL(CP(JTAG_TMS),       (IEN  | PTD | DIS | M0)) /*JTAG_TMS*/\
	MUX_VAL(CP(JTAG_TDI),       (IEN  | PTD | DIS | M0)) /*JTAG_TDI*/\
	MUX_VAL(CP(JTAG_EMU0),      (IDIS | PTD | DIS | M7)) /*emu0/gpio11 lab, not used*/\
	MUX_VAL(CP(JTAG_EMU1),      (IDIS | PTD | DIS | M7)) /*emu1/gpio31 lab, not used*/\
	/*WLAN*/\
	MUX_VAL(CP(ETK_CLK_ES2),    (IDIS | PTD | DIS | M7)) /* [NC] Touch SCL GPIO 12 */\
	MUX_VAL(CP(ETK_CTL_ES2),    (IDIS | PTD | DIS | M7)) /* [NC] Touch SDA GPIO 13 */\
	MUX_VAL(CP(ETK_D0_ES2 ),    (IDIS | PTD | DIS | M4)) /*OVP_DIR_N*/\
	MUX_VAL(CP(ETK_D1_ES2 ),    (IDIS | PTD | DIS | M7)) /* [NC] */\
	MUX_VAL(CP(ETK_D2_ES2 ),    (IDIS | PTD | DIS | M7)) /* [NC] LCD_BL_EN_2 GPIO_16*/\
	MUX_VAL(CP(ETK_D3_ES2 ),    (IEN  | PTD | DIS | M2)) /*WLAN MMC*/\
	MUX_VAL(CP(ETK_D4_ES2 ),    (IEN  | PTD | DIS | M2)) /*WLAN MMC*/\
	MUX_VAL(CP(ETK_D5_ES2 ),    (IEN  | PTD | DIS | M2)) /*WLAN MMC*/\
	MUX_VAL(CP(ETK_D6_ES2 ),    (IEN  | PTD | DIS | M2)) /*WLAN MMC*/\
	MUX_VAL(CP(ETK_D7_ES2 ),    (IDIS | PTD | DIS | M4)) /*CBP DIGITL USB EN*/\
	MUX_VAL(CP(ETK_D8_ES2 ),    (IDIS | PTD | DIS | M4)) /*CBP PCM EN*/\
	MUX_VAL(CP(ETK_D9_ES2 ),    (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(ETK_D10_ES2),    (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(ETK_D11_ES2),    (IEN  | PTD | DIS  | M4)) /*TP317*/\
	MUX_VAL(CP(ETK_D12_ES2),    (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(ETK_D13_ES2),    (IDIS | PTD | DIS | M7)) /*TP318*/\
	MUX_VAL(CP(ETK_D14_ES2),    (IDIS | PTD | DIS | M7)) /*TBD PIN*/\
	MUX_VAL(CP(ETK_D15_ES2),    (IEN  | PTD | DIS | M4)) /*CBP DIGITAL USB, USB2_TXSE0, Input GPIO*/\
	/*Die to Die */\
	MUX_VAL(CP(d2d_mcad0),      (IEN  | PTD | EN  | M0)) /*d2d_mcad0*/\
	MUX_VAL(CP(d2d_mcad1),      (IEN  | PTD | EN  | M0)) /*d2d_mcad1*/\
	MUX_VAL(CP(d2d_mcad2),      (IEN  | PTD | EN  | M0)) /*d2d_mcad2*/\
	MUX_VAL(CP(d2d_mcad3),      (IEN  | PTD | EN  | M0)) /*d2d_mcad3*/\
	MUX_VAL(CP(d2d_mcad4),      (IEN  | PTD | EN  | M0)) /*d2d_mcad4*/\
	MUX_VAL(CP(d2d_mcad5),      (IEN  | PTD | EN  | M0)) /*d2d_mcad5*/\
	MUX_VAL(CP(d2d_mcad6),      (IEN  | PTD | EN  | M0)) /*d2d_mcad6*/\
	MUX_VAL(CP(d2d_mcad7),      (IEN  | PTD | EN  | M0)) /*d2d_mcad7*/\
	MUX_VAL(CP(d2d_mcad8),      (IEN  | PTD | EN  | M0)) /*d2d_mcad8*/\
	MUX_VAL(CP(d2d_mcad9),      (IEN  | PTD | EN  | M0)) /*d2d_mcad9*/\
	MUX_VAL(CP(d2d_mcad10),     (IEN  | PTD | EN  | M0)) /*d2d_mcad10*/\
	MUX_VAL(CP(d2d_mcad11),     (IEN  | PTD | EN  | M0)) /*d2d_mcad11*/\
	MUX_VAL(CP(d2d_mcad12),     (IEN  | PTD | EN  | M0)) /*d2d_mcad12*/\
	MUX_VAL(CP(d2d_mcad13),     (IEN  | PTD | EN  | M0)) /*d2d_mcad13*/\
	MUX_VAL(CP(d2d_mcad14),     (IEN  | PTD | EN  | M0)) /*d2d_mcad14*/\
	MUX_VAL(CP(d2d_mcad15),     (IEN  | PTD | EN  | M0)) /*d2d_mcad15*/\
	MUX_VAL(CP(d2d_mcad16),     (IEN  | PTD | EN  | M0)) /*d2d_mcad16*/\
	MUX_VAL(CP(d2d_mcad17),     (IEN  | PTD | EN  | M0)) /*d2d_mcad17*/\
	MUX_VAL(CP(d2d_mcad18),     (IEN  | PTD | EN  | M0)) /*d2d_mcad18*/\
	MUX_VAL(CP(d2d_mcad19),     (IEN  | PTD | EN  | M0)) /*d2d_mcad19*/\
	MUX_VAL(CP(d2d_mcad20),     (IEN  | PTD | EN  | M0)) /*d2d_mcad20*/\
	MUX_VAL(CP(d2d_mcad21),     (IEN  | PTD | EN  | M0)) /*d2d_mcad21*/\
	MUX_VAL(CP(d2d_mcad22),     (IEN  | PTD | EN  | M0)) /*d2d_mcad22*/\
	MUX_VAL(CP(d2d_mcad23),     (IEN  | PTD | EN  | M0)) /*d2d_mcad23*/\
	MUX_VAL(CP(d2d_mcad24),     (IEN  | PTD | EN  | M0)) /*d2d_mcad24*/\
	MUX_VAL(CP(d2d_mcad25),     (IEN  | PTD | EN  | M0)) /*d2d_mcad25*/\
	MUX_VAL(CP(d2d_mcad26),     (IEN  | PTD | EN  | M0)) /*d2d_mcad26*/\
	MUX_VAL(CP(d2d_mcad27),     (IEN  | PTD | EN  | M0)) /*d2d_mcad27*/\
	MUX_VAL(CP(d2d_mcad28),     (IEN  | PTD | EN  | M0)) /*d2d_mcad28*/\
	MUX_VAL(CP(d2d_mcad29),     (IEN  | PTD | EN  | M0)) /*d2d_mcad29*/\
	MUX_VAL(CP(d2d_mcad30),     (IEN  | PTD | EN  | M0)) /*d2d_mcad30*/\
	MUX_VAL(CP(d2d_mcad31),     (IEN  | PTD | EN  | M0)) /*d2d_mcad31*/\
	MUX_VAL(CP(d2d_mcad32),     (IEN  | PTD | EN  | M0)) /*d2d_mcad32*/\
	MUX_VAL(CP(d2d_mcad33),     (IEN  | PTD | EN  | M0)) /*d2d_mcad33*/\
	MUX_VAL(CP(d2d_mcad34),     (IEN  | PTD | EN  | M0)) /*d2d_mcad34*/\
	MUX_VAL(CP(d2d_mcad35),     (IEN  | PTD | EN  | M0)) /*d2d_mcad35*/\
	MUX_VAL(CP(d2d_mcad36),     (IEN  | PTD | EN  | M0)) /*d2d_mcad36*/\
	MUX_VAL(CP(d2d_clk26mi),    (IEN  | PTD | DIS | M0)) /*d2d_clk26mi  */\
	MUX_VAL(CP(d2d_nrespwron ), (IEN  | PTD | EN  | M0)) /*d2d_nrespwron*/\
	MUX_VAL(CP(d2d_nreswarm),   (IEN  | PTU | EN  | M0)) /*d2d_nreswarm */\
	MUX_VAL(CP(d2d_arm9nirq),   (IEN  | PTD | DIS | M0)) /*d2d_arm9nirq */\
	MUX_VAL(CP(d2d_uma2p6fiq ), (IEN  | PTD | DIS | M0)) /*d2d_uma2p6fiq*/\
	MUX_VAL(CP(d2d_spint),      (IEN  | PTD | EN  | M0)) /*d2d_spint*/\
	MUX_VAL(CP(d2d_frint),      (IEN  | PTD | EN  | M0)) /*d2d_frint*/\
	MUX_VAL(CP(d2d_dmareq0),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq0  */\
	MUX_VAL(CP(d2d_dmareq1),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq1  */\
	MUX_VAL(CP(d2d_dmareq2),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq2  */\
	MUX_VAL(CP(d2d_dmareq3),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq3  */\
	MUX_VAL(CP(d2d_n3gtrst),    (IEN  | PTD | DIS | M0)) /*d2d_n3gtrst  */\
	MUX_VAL(CP(d2d_n3gtdi),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtdi*/\
	MUX_VAL(CP(d2d_n3gtdo),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtdo*/\
	MUX_VAL(CP(d2d_n3gtms),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtms*/\
	MUX_VAL(CP(d2d_n3gtck),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtck*/\
	MUX_VAL(CP(d2d_n3grtck),    (IEN  | PTD | DIS | M0)) /*d2d_n3grtck  */\
	MUX_VAL(CP(d2d_mstdby),     (IEN  | PTU | EN  | M0)) /*d2d_mstdby*/\
	MUX_VAL(CP(d2d_swakeup),    (IEN  | PTD | EN  | M0)) /*d2d_swakeup  */\
	MUX_VAL(CP(d2d_idlereq),    (IEN  | PTD | DIS | M0)) /*d2d_idlereq  */\
	MUX_VAL(CP(d2d_idleack),    (IEN  | PTU | EN  | M0)) /*d2d_idleack  */\
	MUX_VAL(CP(d2d_mwrite),     (IEN  | PTD | DIS | M0)) /*d2d_mwrite*/\
	MUX_VAL(CP(d2d_swrite),     (IEN  | PTD | DIS | M0)) /*d2d_swrite*/\
	MUX_VAL(CP(d2d_mread),      (IEN  | PTD | DIS | M0)) /*d2d_mread*/\
	MUX_VAL(CP(d2d_sread),      (IEN  | PTD | DIS | M0)) /*d2d_sread*/\
	MUX_VAL(CP(d2d_mbusflag),   (IEN  | PTD | DIS | M0)) /*d2d_mbusflag */\
	MUX_VAL(CP(d2d_sbusflag),   (IEN  | PTD | DIS | M0)) /*d2d_sbusflag */\
	MUX_VAL(CP(sdrc_cke0),      (IDIS | PTU | EN  | M0)) /*sdrc_cke0 */\
	MUX_VAL(CP(sdrc_cke1),      (IDIS | PTU | EN  | M0)) /*sdrc_cke1 */ \
	MUX_VAL(CP(gpmc_a11),       (IEN  | PTD | EN  | M4))/*gpmc_all unused*/
