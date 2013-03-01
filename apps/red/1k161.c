#include <stdio.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "gnss.h"
#include "1k161.h"

#define	BIN_CMD_CHANNEL_SPD_4800	0
#define	BIN_CMD_CHANNEL_SPD_9600	1
#define	BIN_CMD_CHANNEL_SPD_19200	2
#define	BIN_CMD_CHANNEL_SPD_38400	3
#define	BIN_CMD_CHANNEL_SPD_2400	0
#define	BIN_CMD_CHANNEL_SPD_9600	1
#define	BIN_CMD_CHANNEL_SPD_57600	2
#define	BIN_CMD_CHANNEL_SPD_115200	3
#define	BIN_CMD_CHANNEL_PAR_NONE	0
#define	BIN_CMD_CHANNEL_PAR_EVEN	1
#define	BIN_CMD_CHANNEL_PAR_ODD		2

struct conf_1k161 {
	const int portspeed:20;
	const int portconf;
	int speed_code:4;
	int parity:3;
};

#define USART_NONE AT91C_US_USMODE_NORMAL |	\
		AT91C_US_CLKS_CLOCK |		\
		AT91C_US_CHRL_8_BITS |		\
		AT91C_US_PAR_NONE |		\
		AT91C_US_NBSTOP_1_BIT
#define USART_EVEN AT91C_US_USMODE_NORMAL |	\
		AT91C_US_CLKS_CLOCK |		\
		AT91C_US_CHRL_8_BITS |		\
		AT91C_US_PAR_ODD |		\
		AT91C_US_NBSTOP_1_BIT
#define USART_ODD AT91C_US_USMODE_NORMAL |	\
		AT91C_US_CLKS_CLOCK |		\
		AT91C_US_CHRL_8_BITS |		\
		AT91C_US_PAR_ODD |		\
		AT91C_US_NBSTOP_1_BIT

static const struct conf_1k161 configs_1k161[] = {
			{2400, USART_NONE, BIN_CMD_CHANNEL_SPD_2400 | 4,	BIN_CMD_CHANNEL_PAR_NONE},	// [0]
			{2400, USART_EVEN, BIN_CMD_CHANNEL_SPD_2400 | 4,	BIN_CMD_CHANNEL_PAR_EVEN},	// [1]
			{2400, USART_ODD, BIN_CMD_CHANNEL_SPD_2400 | 4,	BIN_CMD_CHANNEL_PAR_ODD},	// [2]
			{4800, USART_NONE, BIN_CMD_CHANNEL_SPD_4800,	BIN_CMD_CHANNEL_PAR_NONE},	// [3]
			{4800, USART_EVEN, BIN_CMD_CHANNEL_SPD_4800,	BIN_CMD_CHANNEL_PAR_EVEN},	// [4]
			{4800, USART_ODD, BIN_CMD_CHANNEL_SPD_4800,	BIN_CMD_CHANNEL_PAR_ODD},	// [5]
			{9600, USART_NONE, BIN_CMD_CHANNEL_SPD_9600,	BIN_CMD_CHANNEL_PAR_NONE},	// [6]
			{9600, USART_EVEN, BIN_CMD_CHANNEL_SPD_9600,	BIN_CMD_CHANNEL_PAR_EVEN},	// [7]
			{9600, USART_ODD, BIN_CMD_CHANNEL_SPD_9600,	BIN_CMD_CHANNEL_PAR_ODD},	// [8]
			{19200, USART_NONE, BIN_CMD_CHANNEL_SPD_19200,	BIN_CMD_CHANNEL_PAR_NONE},	// [9]
			{19200, USART_EVEN, BIN_CMD_CHANNEL_SPD_19200,	BIN_CMD_CHANNEL_PAR_EVEN},	// [10]
			{19200, USART_ODD, BIN_CMD_CHANNEL_SPD_19200,	BIN_CMD_CHANNEL_PAR_ODD},	// [11]
			{38400, USART_NONE, BIN_CMD_CHANNEL_SPD_38400,	BIN_CMD_CHANNEL_PAR_NONE},	// [12]
			{38400, USART_EVEN, BIN_CMD_CHANNEL_SPD_38400,	BIN_CMD_CHANNEL_PAR_EVEN},	// [13]
			{38400, USART_ODD, BIN_CMD_CHANNEL_SPD_38400,	BIN_CMD_CHANNEL_PAR_ODD},	// [14]
			{57600, USART_NONE, BIN_CMD_CHANNEL_SPD_57600 | 4,	BIN_CMD_CHANNEL_PAR_NONE},	// [15]
			{57600, USART_EVEN, BIN_CMD_CHANNEL_SPD_57600 | 4,	BIN_CMD_CHANNEL_PAR_EVEN},	// [16]
			{57600, USART_ODD, BIN_CMD_CHANNEL_SPD_57600 | 4,	BIN_CMD_CHANNEL_PAR_ODD},	// [17]
			{115200, USART_NONE, BIN_CMD_CHANNEL_SPD_115200 | 4,	BIN_CMD_CHANNEL_PAR_NONE},	// [18]
			{115200, USART_EVEN, BIN_CMD_CHANNEL_SPD_115200 | 4,	BIN_CMD_CHANNEL_PAR_EVEN},	// [19]
			{115200, USART_ODD, BIN_CMD_CHANNEL_SPD_115200 | 4,	BIN_CMD_CHANNEL_PAR_ODD},	// [20]
};
static msg_t mbox_geo_buffer[10];
static msg_t mbox_misc_buffer[10];
static MAILBOX_DECL(mbox_geo, mbox_geo_buffer, 10);
static MAILBOX_DECL(mbox_misc, mbox_misc_buffer, 10);

/* XXX */
#if 0
#define print_gnss(...)		//{	if(!vt100_is_present){	printf(str_gnss);	printf(__VA_ARGS__);	}}
#define NEXT_GNSS_TIMEOUT(_timeout)	status_gnss.time = ((_timeout) * 1000)
#define NEXT_GNSS_STATE(_state)		status_gnss.state = (_state)
#define IF_GNSS_TIMEOUT_OR(...)	if(!status_gnss.time || (__VA_ARGS__))
#define IF_GNSS_TIMEOUT			if(!status_gnss.time)
#define print_gnss_noanswer()	print_gnss(" %s"CRLF, str_notanswer)
#define print_gnss_cmd(...)	{	print_gnss(str_gnss_cmd);	printf(__VA_ARGS__);	printf(CRLF);	}
static void do_1k161_states(void)
{
	static int state = GNSS_STATE_RESET;
	static int timeout = 0; /* Seconds */
	static int present = 0;
	static int usart_conf = GNSS_PORT_CFG_DEFAULT;
	switch(state) {
	case GNSS_STATE_RESET:
		timeout = 15;
		state = GNSS_STATE_WAIT_FOR_RESET;
		break;
	case GNSS_STATE_WAIT_FOR_RESET:
		if (!timeout || present) {
			if (!timeout)
				state = GNSS_STATE_SET_BINARY;
			else
				state = GNSS_STATE_FOUND;
		}
		break;
	case GNSS_STATE_SET_BINARY:
		set_protocol_1k161(1, 0, 0, 0, 0);
		timeout = 4;
		state = GNSS_STATE_WAIT_FOR_BINARY;
		break;
	case GNSS_STATE_WAIT_FOR_BINARY:
		if (!timeout || present) {
			if (!timeout)
				state = GNSS_STATE_INC_USART_CFG;
			else
				state = GNSS_STATE_FOUND;
		}
		break;
	case GNSS_STATE_INC_USART_CFG:
		usart_conf++;
		if (usart_conf >=
			sizeof(usart_configs)/sizeof(usart_configs[0]))
			usart_conf = 0;
		state = GNSS_STATE_USART_RECONFIG;
		break;
	case GNSS_STATE_USART_RECONFIG:
		present = 0;
		sdStop(&SD1);
		sdStart(&SD1, &usart_configs[usart_conf]);
		state = GNSS_STATE_SET_BINARY;
		if (present) {
			input_1k161_parser_reset();
			present = 0;
		}
		
/* XXX implement XXX */
#if 0
			configure_for_GNSS(status_gnss.port_cfg);	// USART = current USART cfg
			set_1k161_port(AT91C_BASE_US0);
			NEXT_GNSS_STATE(GNSS_STATE_SET_BINARY);
			if(status_gnss.is_present) {
				print_gnss(" lost"CRLF);
				// ñþäà íóæíî äîáàâèòü î÷èñòêó áóôåðîâ USART
				//GNSS_port_fifo_flush();
				gnss_dma_fifo_clear();
				input_1k161_parser_reset();
/*!!!*/				status_gnss.is_present = 0;
			}
#endif
		break;
	case GNSS_STATE_FOUND:
		if (usart_conf == GNSS_PORT_CFG_DEFAULT) {
			if (!tested) {
				selftest_1k161(0);
				timeout = 25;
			}
			state = GNSS_STATE_SELFTEST_RESULT;
		} else {
			int speed_code = GNSS_1K161_SPEED_CODE;
			int parity = GNSS_1K161_PARITY;
			int channel = 1;
			config_channel_1k161(
				channel, speed_code & 3,
				speed_code >> 2,
				1,
				parity,
				SAVE_1K161_SETTINGS_NVRAM);
			input_1k161_parser_reset();
			present = 0;
			timeout = 6;
			state = GNSS_STATE_CHANNEL_CFG_RESULT;
		}
/* XXX implement XXX */
#if 0
			print_gnss(" found!"CRLF);
			if(status_gnss.port_cfg == GNSS_PORT_CFG_DEFAULT) {// USART config == default USART cfg?
				print_gnss(" on default channel configuration"CRLF);
				//NEXT_GNSS_STATE(GNSS_STATE_QUIET);
				if(!status_gnss.tested) {		// çàïðîñ ñàìîòåñòèðîâàíèÿ ÃÍÑÑ
					print_gnss_cmd(str_gnss_self_test);
					selftest_1k161(0);		// çàïðàøèâàåì ðåçóëüòàò ñàìîòåñòèðîâàíèÿ
					NEXT_GNSS_TIMEOUT(25);		// timeout = ...(âðåìÿ(ñåê) íà âûäà÷ó ðåçóëüòàòà ñàìîòåñòèðîâàíèÿ)
				}
				NEXT_GNSS_STATE(GNSS_STATE_SELFTEST_RESULT);
			} else {
				char speed_code = gnss_get_port_speed(GNSS_PORT_CFG_DEFAULT);
				char parity = gnss_get_port_parity(GNSS_PORT_CFG_DEFAULT);
				char channel = 1;
				print_gnss_cmd(str_gnss_conf_channel);
				print_gnss_cmd(str_gnss_conf_channel);
				print_gnss_cmd(str_gnss_conf_channel);
				print_gnss_cmd("%s#%d spdc:%d add:%d par:%d (%s)", str_gnss_conf_channel, channel, speed_code & 3, speed_code >> 2, parity, SAVE_1K161_SETTINGS_NVRAM ? str_gnss_save : str_gnss_nosave);
				config_channel_1k161(channel, speed_code & 3, speed_code >> 2, 1, parity, SAVE_1K161_SETTINGS_NVRAM);// set GNSS module channel config to default USART cfg
				print_gnss(" lost"CRLF);
				// ñþäà íóæíî äîáàâèòü î÷èñòêó áóôåðîâ USART
				//GNSS_port_fifo_flush();
				gnss_dma_fifo_clear();
				input_1k161_parser_reset();
/*!!!*/				status_gnss.is_present = 0;
				NEXT_GNSS_TIMEOUT(6);			// timeout = ...(âðåìÿ(ñåê) íà óñòàíîâêó ïàðàìåòðîâ ïîðòà ÃÍÑÑ)
				NEXT_GNSS_STATE(GNSS_STATE_CHANNEL_CFG_RESULT);
			}
#endif
		break;
	case GNSS_STATE_CHANNEL_CFG_RESULT:
		if (!timeout || present)
			state = GNSS_STATE_INC_USART_CFG;
		else {
			usart_conf = GNSS_PORT_CFG_DEFAULT;
			state = GNSS_STATE_USART_RECONFIG;
		}
/* XXX implement XXX */
#if 0
			IF_GNSS_TIMEOUT_OR(status_gnss.is_present) {	// Waiting for ACK msg (105, 162?)
				IF_GNSS_TIMEOUT {
					print_gnss_noanswer();
					NEXT_GNSS_STATE(GNSS_STATE_INC_USART_CFG);
				} else {
					status_gnss.port_cfg = GNSS_PORT_CFG_DEFAULT;// current USART cfg = default USART cfg
					NEXT_GNSS_STATE(GNSS_STATE_USART_RECONFIG);
				}
			}
#endif
		break;
	case GNSS_STATE_SELFTEST_RESULT:
		if (!timeout)
			state = GNSS_STATE_RESET;
		else if (tested) {
			set_protocol_1k161(1, 1, 0, 0, SAVE_1K161_SETTINGS_NVRAM);
			Tprotocol_config protocol_config;
			memset(&protocol_config, 0, sizeof(protocol_config));
			protocol_config.chanel = 1;
			protocol_config.data_time = 1;
			protocol_config.at_gsm_iec = 0;
			protocol_config.at_gsm_sms = 0;
			Tprotocol_bin_config protocol_bin_config;
			memset(&protocol_bin_config, 0, sizeof(protocol_bin_config));
			protocol_bin_config.result_freq = 1;
			protocol_bin_config.about_result_freq = 1;
			protocol_bin_config.time_scale_freq = 0;
			protocol_bin_config.time_scale_type = 0;
			protocol_bin_config.radio_meas_freq = 0;
			protocol_bin_config.ant_status_freq = 1;
			protocol_bin_config.result_type = 1;
			config_bin_protocol_1k161(&protocol_config, &protocol_bin_config,
				SAVE_1K161_SETTINGS_NVRAM);
			state = GNSS_STATE_QUIET;
			timer_handler(NULL);
		}
/* XXX implement XXX */
#if 0
			IF_GNSS_TIMEOUT_OR(status_gnss.tested) {		// Waiting for selftest (101)
				IF_GNSS_TIMEOUT {
					print_gnss_noanswer();
					NEXT_GNSS_STATE(GNSS_STATE_RESET);
				} else {
					print_gnss(" setftest result - device:%d, antenna:%d ~~~"CRLF, status_gnss.test, status_gnss.ant);
					print_gnss_cmd("%s (%s)", str_gnss_bin_prot, SAVE_1K161_SETTINGS_NVRAM ? str_gnss_save : str_gnss_nosave);
					set_protocol_1k161(1, 1, 0, 0, SAVE_1K161_SETTINGS_NVRAM);
					print_gnss_cmd(" %s (%s)", str_config, SAVE_1K161_SETTINGS_NVRAM ? str_gnss_save : str_gnss_nosave);
					Tprotocol_config protocol_config;
					memset(&protocol_config, 0, sizeof(protocol_config));	// âñå ïîëÿ, êðîìå èíèöèàëèçèðîâàííûõ íèæå çàïîëíèòü 0
					protocol_config.chanel = 1;			// íà êàêîé ïîðò âûâîäèòü äàííûå
					protocol_config.data_time = 1;			// 1 - âûâîäèòü äàííûå íà ìîìåíò èçìåðåíèÿ, áåç ýêñòðàïîëÿöèè
					protocol_config.at_gsm_iec = 0;
					protocol_config.at_gsm_sms = 0;
					
					Tprotocol_bin_config protocol_bin_config;
					memset(&protocol_bin_config, 0, sizeof(protocol_bin_config));	// âñå ïîëÿ, êðîìå èíèöèàëèçèðîâàííûõ íèæå çàïîëíèòü 0
					protocol_bin_config.result_freq = 1;		//Áèò 2-0 âûäà÷à ñîîáùåíèÿ ñ ÊÑÂ-ðåøåíèåì (ID=147, ID=149 èëè ID=151) (1-1Hz, 2-2Hz, 3-5Hz, 4-10Hz)
					protocol_bin_config.about_result_freq = 1;	//Áèò 4-3 âûäà÷à ñîîáùåíèÿ ñ îïåðàòèâíûìè äàííûìè î ÊÑÂ-ðåøåíèè (ID=152) (1-1Hz, 2-ñ ÷àñòîòîé íàâèãàöèîííûõ îïðåäåëåíèé)
					protocol_bin_config.time_scale_freq = 0;	//Áèò 5 âûäà÷à ñîîáùåíèÿ ñ ïàðàìåòðàìè âûõîäíîé ØÂ (ID=153, ID=154)
					protocol_bin_config.time_scale_type = 0;	//Áèò 6 âèä âûâîäèìîãî ñîîáùåíèÿ ñ ïàðàìåòðàìè âûõîäíîé ØÂ: 0 = ID=153; 1 = ID=154;
					protocol_bin_config.radio_meas_freq = 0;	//Áèò 8-7 âûäà÷à ñîîáùåíèÿ ñ èçìåðåíèÿìè ÐÍÏ (ID=150 èëè ID=148, ñì. Áèò 31)
					protocol_bin_config.ant_status_freq = 1;	//Áèò 25-24 âûäà÷à ñîîáùåíèÿ ñî ñòàòóñîì àíòåííû (ID=106) (1-ïî îáíîâë., 2-1Ãö)
					protocol_bin_config.result_type = 1;		//Áèò 26 âèä âûâîäèìîãî ñîîáùåíèÿ ñ ÊÑÂ-ðåøåíèåì: 0 - ID=151; 1 - ID=149;
					
					config_bin_protocol_1k161(&protocol_config, &protocol_bin_config, SAVE_1K161_SETTINGS_NVRAM);
					if(errno) {
						
					}
					NEXT_GNSS_STATE(GNSS_STATE_QUIET);
					timer_handler(NULL);
				}
			}
#endif
		break;
	}
}
#endif

static int packet_header_1k161(int c)
{
	char header[] = {0x57, 0xf1};
	static int header_step = 0;
	static int skipped_chars = 0;
	if (header[header_step] == c) {
		header_step++;
		if (header_step == sizeof(header)) {
#if 0
			if (skipped_chars > 0)
				printf("Skip %d\r\n", skipped_chars);
#endif
			header_step = 0;
			skipped_chars = 0;
			return 1;
		}
	} else {
		header_step = 0;
		skipped_chars++;
	}
	return 0;
}

struct packet_msg {
	int id;
	int len;
	uint16_t crc;
	uint8_t data[];
};

static uint16_t cksum_calc(uint16_t oldsum, uint16_t data)
{
	int sum = (oldsum + data) & 0x1FFFF;
	sum +=  (sum >> 16) & 1;
	return sum & 0xFFFF;
}

static uint16_t cksum(struct packet_msg *data)
{
	int i;
	uint16_t sum;
	uint16_t *pdata;
	int size = (data->len >> 1);
	sum = cksum_calc(0, 0xf157);
	sum = cksum_calc(sum, (data->id << 8) | (data->len >> 1));
	pdata = (uint16_t *)data->data;
	for (i = 0; i < size; i++, pdata++)
		sum = cksum_calc(sum, *pdata);

	return sum;
}

static void process_packet(struct packet_msg *packet)
{
	int res;
	pr_debug("Packet id %d size %d crc %u\r\n", packet->id, packet->len, packet->crc);
	switch(packet->id) {
	case 102: /* Consistency warning */
		res = chMBPost(&mbox_misc, (msg_t)packet, TIME_IMMEDIATE);
		break;
	case 149: /* decision */
	case 152: /* run-time decision data */
		res = chMBPost(&mbox_geo, (msg_t)packet, TIME_IMMEDIATE);
		break;
	default:
		pr_debug("Packet id %d size %d crc %u\r\n", packet->id, packet->len, packet->crc);
		pr_debug("CKSUM %u\r\n", cksum(packet));
		chHeapFree(packet);
		break;
	}
}

void packet_detector_1k161(int c)
{
	static int state = 0;
	static int packetsize = 0;
	static int packetnum = 0;
	static uint16_t crc;
	static struct packet_msg *packet = NULL;
	static int packet_count = 0;
	switch (state) {
	case 0: /* Header magic */
		if(packet_header_1k161(c))
			state = 1;
		break;
	case 1: /* Size */
		packetsize = c;
		state = 2;
		break;
	case 2: /* ID */
		packetnum = c;
		state = 3;
		break;
	case 3: /* CRC low */
		crc = c;
		state = 4;
		break;
	case 4: /* CRC high */
		crc |= (c << 8);
		state = 5;
		packet = chHeapAlloc(NULL,
			sizeof(struct packet_msg) + packetsize * 2);
		if (!packet) {
			printf("Drop\r\n");
			state = 0;
			break;
		}
		packet->len = packetsize *2;
		packet->id = packetnum;
		packet->crc = crc;
		packet_count = 0;
		break;
	case 5: /* Even byte */
	case 6: /* Odd byte */
		if (packet) {
			if (packet->len > packet_count) {
				packet->data[packet_count] = c;
				packet_count++;
			}
		}
		if (state == 6) {
			packetsize--;
			if(!packetsize) {
				state = 0;
				/* Check checksum here */
				/* see bin_message_checksum() in 1k-161.c */
				if (cksum(packet) == packet->crc)
					process_packet(packet);
				else
					chHeapFree(packet);
			} else
				state = 5;
		} else
			state++;
		break;
	}
}

void reset_1k161(void)
{
	palClearPad(IOPORT1, PIOA_GPS_NRST);
        chThdSleepMilliseconds(100);
	palSetPad(IOPORT1, PIOA_GPS_NRST);
        chThdSleepMilliseconds(5000);
}

int detect_1k161(void)
{
	int i, j, k, conf;
	int detected = 0;
	uint8_t *gnss_buffer = chHeapAlloc(NULL, 512);
	static SerialConfig us_conf = {
		38400 /* depends */,
		AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
		AT91C_US_CHRL_8_BITS | AT91C_US_PAR_ODD | AT91C_US_NBSTOP_1_BIT
	};

	k2_usart1_1k161();
	conf = 14;
	for (k = 0;
		k < sizeof(configs_1k161)/sizeof(configs_1k161[0]);
		k++) {
		us_conf.sc_speed = configs_1k161[conf].portspeed;
		us_conf.sc_mr = configs_1k161[conf].portconf;
		sdStop(&SD1);
		sdStart(&SD1, &us_conf);
		reset_1k161();
		for (i = 0; i < 20 && !detected; i++) {
			int t = sdReadTimeout(&SD1, gnss_buffer, 512, 500);
			if (t > 0) {
				for (j = 0; j < t; j++) {
					if (packet_header_1k161(gnss_buffer[j]))
						detected = 1;
				}
				if (!detected)
					dbg_hex_dump(gnss_buffer, t);
			}
		}
		if (detected) {
			if (conf == 14) {
				/* TODO: run selftests here */
				break;
			} else {
				/* TODO: Reconfigure device here */
				continue;
			}
		} else {
			conf++;
			if (conf >=
				sizeof(configs_1k161)/sizeof(configs_1k161[0]))
				conf = 0;
		}
	}
	chHeapFree(gnss_buffer);
	return detected;
}
msg_t geo_thread_1k161(void *p)
{
	(void)p;
	msg_t msg, result;
	struct packet_msg *pkt;
	while (TRUE) {
		result = chMBFetch(&mbox_geo, &msg, TIME_INFINITE);
		pkt = (struct packet_msg *) msg;
		pr_debug("packet id = %d\n", pkt->id);
		dbg_hex_dump(pkt->data, pkt->len);
		chHeapFree(pkt);
	}
	return 0;
}
msg_t misc_thread_1k161(void *p)
{
	(void)p;
	msg_t msg, result;
	struct packet_msg *pkt;
	while (TRUE) {
		result = chMBFetch(&mbox_geo, &msg, TIME_INFINITE);
		pkt = (struct packet_msg *) msg;
		pr_debug("packet id = %d\n", pkt->id);
		dbg_hex_dump(pkt->data, pkt->len);
		chHeapFree(pkt);
	}
	return 0;
}

