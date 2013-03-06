#ifndef _1K161_MSG_H
#define _1K161_MSG_H

typedef	unsigned short	UINT16;
typedef	unsigned long	UINT32;
typedef	signed short	SINT16;
typedef	signed long	SINT32;
typedef	unsigned short	BIN16;
typedef	unsigned long	BIN32;
typedef	float		FP;
typedef	double		EFP;
// --------- ответные сообщения от МПВ -------------
#define ANSWER_CODES_START	100

#define BIN_MSG_PPS		(ANSWER_CODES_START + 0 )	// метка времени
#define BIN_MSG_TEST_SELF	(ANSWER_CODES_START + 1 )	// результат самоконтроля
#define BIN_MSG_WARNING		(ANSWER_CODES_START + 2 )	// предупреждение о нарушении целостности
#define BIN_MSG_CMD_NACK	(ANSWER_CODES_START + 3 )	// ошибка при приеме командного сообщения
#define BIN_MSG_TEST_ANT	(ANSWER_CODES_START + 4 )	// результат контроля БА
#define BIN_MSG_CMD_ACK		(ANSWER_CODES_START + 5 )	// подтверждение приема командного сообщения
#define BIN_MSG_STAT_ANT	(ANSWER_CODES_START + 6 )	// статус БА
#define BIN_MSG_SHEDULE		(ANSWER_CODES_START + 7 )	// предупреждение о начале или окончании сессии при работе по расписанию
#define BIN_MSG_READY_PWROFF	(ANSWER_CODES_START + 8 )	// МПВ готов к выключению питания

#define BIN_MSG_COORDS		(ANSWER_CODES_START + 11)	// точные координаты
#define BIN_MSG_COORDS_GK	(ANSWER_CODES_START + 13)	// точные координаты в системе Гаусса-Крюгера

#define BIN_MSG_TPULSE_PARAMS	(ANSWER_CODES_START + 17)	// параметры импульсов времени

#define BIN_MSG_RTCM_LIST	(ANSWER_CODES_START + 28)	// перечень ретранслируемых RTCM-сообщений
#define BIN_MSG_RTCM		(ANSWER_CODES_START + 29)	// RTCM-сообщение

#define BIN_MSG_PPS_PARAMS	(ANSWER_CODES_START + 84)	// режим выдачи сигнала "1 Гц"



#pragma pack(push, _Struct, 1)	// Set the alignment of 1 byte	// структуры упакованы, поскольку данные будут передаваться в устройство

typedef struct {
} Tbin_message_header;

typedef struct							// метка времени в линии обмена		100
{
	Tbin_message_header header;
	BIN16	sings;
}Tbin_msg_pps;

//----------------------------SELFTEST-RESULT---------------------------
/*typedef struct							// результат самоконтроля		101
{
	Tbin_message_header header;
	BIN32	result;
	BIN32	resources;
	BIN16	APP;
	BIN16	SPP;
}Tbin_msg_test_self;*/
typedef struct _Tbin_test_self_msg_new {
	Tbin_message_header header;	//for compatibility
	BIN32	summary;		// Tbin_test_self_msg_new_summary
	BIN32	resources;		// Tbin_test_self_msg_new_resources
	BIN16	UARTs;			// Tbin_test_self_msg_new_UARTs
	BIN16	USRTs;			// Tbin_test_self_msg_new_USRTs
} Tbin_test_self_msg_new;

typedef struct _Tbin_test_self_msg_new_summary {
	unsigned MPV :2;		//00 0=good, 1=not all good, 2=reserved, 3=complete failure
	unsigned testfails :1;	//02 1=failures during test, 0=no failures
	unsigned CPU :1;		//03 0=good, 1=bad
	unsigned eRAM :1;		//04 (external RAM)
	unsigned eROM :1;		//05 (external ROM)
	unsigned EEPROM :1;		//06 (EEPROM-?? - ???)
	unsigned RTC :1;		//07 (real time clock)
	unsigned WDC :1;		//08 (watchdog)
	unsigned UART :1;		//09 (asynchronous port - ???)
	unsigned USRT :1;		//10 (synchronous port - ???)
	unsigned OCP :1;		//11 (once commands ports)
	unsigned DPM :1;		//12 (digital processing module - ???)
	unsigned synt :1;		//13 (syntezhers & reference oscillator - ??????????? ? ??)
	unsigned GPS :1;		//14 (GPS processing L1 module)
	unsigned GLO :1;		//15 (GLONASS processing F1 module)
	unsigned res :2;
	unsigned ant :1;		//18 (antenna module)
	unsigned byWDT :1;		//19 (fault detected by WDT)
	unsigned byPPS :1;		//20 1=bad (???)
	unsigned res1 :11;
} Tbin_test_self_msg_new_summary;

typedef struct _Tbin_test_self_msg_new_resources {
	unsigned DPMGPS :6;		// usable DPM channels for GPS L1 module
	unsigned DPMGLO :6;		// usable DPM channels for GLONASS F1 module
	unsigned res :10;
} Tbin_test_self_msg_new_resources;

typedef struct _Tbin_test_self_msg_new_UARTs {
	unsigned UART1 :3;		// (asynchronous port 1 - ???1)
	unsigned UART2 :3;		// (asynchronous port 2 - ???2)
	unsigned res :10;
} Tbin_test_self_msg_new_UARTs;

typedef struct Tbin_test_self_msg_new_USRTs {
	unsigned USRT1 :3;		// (synchronous port 1 - ???1)
	unsigned res :13;
} Tbin_test_self_msg_new_USRTs;
//---------------------------/SELFTEST-RESULT---------------------------





typedef struct							// предупреждение о нарушении целостности	102
{
	Tbin_message_header header;
	BIN16	status;
	UINT16	err_num;
	BIN32	err_descr1;
	BIN16	err_descr2;
}Tbin_msg_warning;
typedef struct							// ошибка при приеме команды	(NACK)	103
{
	Tbin_message_header header;
	UINT16	cmd;
	SINT16	checksum;
	BIN16	err;
}Tbin_msg_nack;	
typedef struct							// результат контроля антенны		104
{
	Tbin_message_header header;
	BIN16	control;
}Tbin_msg_test_ant;
typedef struct							// подтверждение приема команды	(ACK)	105
{
	Tbin_message_header header;
	UINT16	cmd;
}Tbin_msg_ack;
typedef struct							// статус антенны			106
{
	Tbin_message_header header;
	BIN16	status;
}Tbin_msg_stat_ant;






//--------------------------DECISION-RESULT-149--------------------------
typedef struct							// решение: координаты, скорость, время	149
{
	Tbin_message_header header;
	BIN16	attr;
	BIN32	time;
	UINT32	time_ns;
	BIN32	date;
	EFP	lat;			// рад (м)
	EFP	lon;			// рад (м)
	EFP	alt;			// м
	FP	spd_lat;		// м/с
	FP	spd_lon;		// м/с
	FP	spd_alt;		// м/с
	FP	rms_err_coord;		// м
	FP	rms_err_alt;		// м
	FP	rms_err_time;		// нс
	FP	rms_err_spd;		// м/с
	FP	rms_err_spd_alt;	// м/с
}Tbin_msg_result;

typedef struct _Tbin_msg_result_nav_attr {
	unsigned dim :1;		//00 current dimension mode: 0=2D, 1=3D
	unsigned dimctrl :1;	//01 dimension mode control: 0=external, 1=auto
	unsigned altsrc :1;		//02 altitude source (only with .dim==0): 0=external, 1=last decision with valid VDOP
	unsigned offs :1;		//03 time? offset: 0=used fixed, 1=determined by measuring
	unsigned offsctrl :1;	//04 time offset control: 0=external, 1=auto
	unsigned offssrc :1;	//05 offset source (only with .offs==0): 0=external, 1=last decision with valid TDOP
	unsigned recurs :1;		//06 recursive algorithm: 0=not use - measuring only, 1=use - not only measuring - also extrapolated
	unsigned coordsyst :1;	//07 0=geodesic (polar), 1=cartesian
	unsigned lopwr :1;		//08 0=normal mode, 1=low power mode
	unsigned timing :1;		//09 0=decision data extrapolated to the output moment, 1=data refer to the measuring moment
	unsigned navfield :1;	//10 0=bad, 1=good
	unsigned secready :1;	//11 date & time ready with 1s precision: 0=not ready, 1=ready
	unsigned :1;			//12 reserved
	unsigned timescale :3;	//13-15 time scale: 0=any-previously setted by cmd66, 1=UTC(US), 2=UTC(RUS), 3=TS GPS, 4=TS GLONASS
} Tbin_msg_result_nav_attr;

typedef struct _Tbin_msg_result_nav_time {
	unsigned hour :8;		//00-07 hour 0...23
	unsigned min :8;		//08-15 minute 0...59
	unsigned sec :8;		//16-23 second 0...60
	unsigned :8;			//24-31 reserved
} Tbin_msg_result_nav_time;

typedef struct _Tbin_msg_result_nav_date {
	unsigned year :8;		//00-07 year 0...55 (2000...2055)
	unsigned month :8;		//08-15 month 1...12
	unsigned day :8;		//16-23 day 1...31
	unsigned :8;			//24-31 reserved
} Tbin_msg_result_nav_date;


//-------------------------/DECISION-RESULT-149--------------------------





typedef struct							// решение: координаты, скорость, время	151
{
	Tbin_message_header header;
	BIN16	nav_attr;
	EFP	time;			// сек
	UINT16	day;			// день, номер недели
	EFP	lat;			// рад (м)
	EFP	lon;			// рад (м)
	EFP	alt;			// м
	FP	spd_lat;		// м/с
	FP	spd_lon;		// м/с
	FP	spd_alt;		// м/с
	FP	rms_err_coord;		// м
	FP	rms_err_alt;		// м
	FP	rms_err_time;		// нс
	FP	rms_err_spd;		// м/с
	FP	rms_err_spd_alt;	// м/с
}Tbin_msg_result_tf;





typedef struct							// оперативные данные о решении		152
{
	Tbin_message_header header;
	BIN16	nav_attr;
	BIN32	sats_GPS;		// бит = 1 означает наличие спутника с номером (номер бита + 1)
	BIN32	sats_GLO;		// бит = 1 означает наличие спутника с номером (номер бита + 1)
	BIN32	sats_GEO;		// бит = 1 означает наличие спутника с номером (номер бита + 120)
	BIN32 reserved_1;
	BIN32	sats_GPS_nav;
	BIN32	sats_GLO_nav;
	BIN32	sats_GEO_nav;
	BIN32 reserved_2;
	BIN32	sats_GPS_wst;
	BIN32	sats_GLO_wst;
	BIN32	sats_GEO_wst;
	BIN32 reserved_3;
	BIN32	sats_GPS_wst_RAIM;
	BIN32	sats_GLO_wst_RAIM;
	BIN32	sats_GEO_wst_RAIM;
	BIN32 reserved_4;
	BIN32	diff_GPS;		//
	BIN32	diff_GLO;		//
	BIN32	diff_GEO_GPS;		//
	BIN32	diff_GEO_GLO;		//
	BIN32 reserved_5;		//
	BIN32 reserved_6;		//
	FP	GDOP;			// 0...1000	==0 ? -> невозможно определить координаты в тек. условиях
	FP	PDOP;			// 0...1000
	FP	HDOP;			// 0...1000
	FP	VDOP;			// 0...1000
	FP	TDOP;			// 0...1000
}Tbin_msg_about_result;




typedef struct{				// информация о спутнике (входит в состав 156-го сообщения bin_msg_sats)
	BIN16 num;
	BIN16 state;
	SINT16 alm_old;
	FP elevation;			// +/- pi/2	(угол над горизонтом)
	FP azimuth;			// +/- pi	(азимут)
}Tbin_msg_sats_sat;
typedef struct							// данные о НКА				156
{
	Tbin_message_header header;
	BIN16	state;
	UINT16	nka_num;		// 1...32
	FP	view_angle;		// 0...pi/2
	UINT16	nka_in_view_num;	// 1...16
	Tbin_msg_sats_sat   sat[32];	//
}Tbin_msg_sats;




typedef struct							// атрибуты комплекта			167
{
	Tbin_message_header header;
	UINT32	firmware_reg;
	UINT16	firmware_ver;
	UINT32	operating_time;
	BIN16	manufact_num_hi;
	BIN32	manufact_num_lo;
	BIN32	firmware_date;
	BIN16	firmware_time;
}Tbin_msg_attr_device;




















#pragma pack(pop, _Struct)	// Set the default alignment
#endif
