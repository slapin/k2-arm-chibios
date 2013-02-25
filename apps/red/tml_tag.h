/*
 * tml_tag.h
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 02.09.2010
 *      Author: A.Afanasiev
 */

#ifndef TML_TAG_H_
#define TML_TAG_H_

typedef enum {
	TML_NONE = 0,		/* No tag */
	TML_LAT = 0x01,		/* Latitude */
	TML_LON = 0x02,		/* Longitude */
	POSDECISION = 0x03,		/* PosDecision */
	POSDIRECTION = 0x04,		/* PosDirection */
	POSSPEED = 0x05,		/* PosSpeed */
	POSSATEL = 0x06,		/* PosSatel */
	POSSATELDECISION = 0x07,		/* PosSatelDecision */
	DEVICEID = 0x08,		/* DeviceID */
	TIMEBEGIN = 0x10,		/* TimeBegin */
	TIMEEND = 0x11,		/* TimeEnd */
	ITEMSCOUNT = 0x12,		/* ItemsCount */
	DEVICEUPTIME = 0x14,		/* DeviceUptime */
	DEVICEGNSS = 0x16,		/* DeviceGNSS */
	DEVICEGSM = 0x17,		/* DeviceGSM */
	DEVICEGPS = 0x18,		/* DeviceGPS */
	DEVICESEKOP = 0x19,		/* DeviceSEKOP */
	DEVICETETRA = 0x1A,		/* DeviceTETRA */
	DEVICEETH = 0x1B,		/* DeviceEth */
	DEVICERS485 = 0x1C,		/* DeviceRs485 */
	ADCDATA = 0x1D,		/* ADCData */
	KMZDATA = 0x1E,		/* KMZData */
	KMZERRORCODE = 0x1F,		/* KMZErrorCode */
	PROTOCOLVERSION = 0x20,		/* ProtocolVersion */
	ALARM = 0x21,		/* Alarm */
	RADIOALARM = 0x22,		/* RadioAlarm */
	RFIDDATA = 0x23,		/* RFID_Data */
	RFIDSTATUS = 0x24,		/* RFID_Status */
	SERIALNUMBER = 0x25,		/* SerialNumber */
	MODVERSION = 0x26,		/* ModVersion */
	GEOPDOP = 0x27,		/* PDOP */
	DEVICEPOWER = 0x28,		/* DevicePower */
	CHANNELID = 0x33,		/* ChannelID */
	ISTLV = 0x80,
	POSNMEA = 0x81,		/* PosNMEA */
	NETADDRESS = 0x82,		/* NetAddress */
	SEKOPTEXT = 0x83,		/* SekopText */
	SEKOPSTATUS = 0x84,		/* SekopStatus */
	SEKOPTRANSACTIONFILE = 0x85,		/* SekopTransactionFile */
	VERSION = 0x86,		/* Version */
	MODNAME = 0x87,		/* ModName */
	SMSTEXT = 0x88,		/* SMSText */
	QUEUELIST = 0x89,		/* QueueList */
	SEKOPMZ = 0x8A,		/* SekopMZ */
	CHANNELBIN = 0x8B,		/* ChannelBIN */
	SEKOPDATABIN = 0x8D,		/* SekopDataBin */
	CETUSMZDATA = 0x91,		/* CetusMZData */
	DIAGGNSS = 0xB1,		/* DiagGNSS */
	DIAGGSM = 0xB2,		/* DiagGSM */
	DIAGGPRS = 0xB3,		/* DiagGPRS */
	DIAGSEKOP = 0xB4,		/* DiagSEKOP */
	DIAGTETRA = 0xB5,		/* DiagTETRA */
	DIAGETH = 0xB6,		/* DiagEth */
	DIAGRS485 = 0xB7,		/* DiagRs485 */
	SEKOPTRANSACTION = 0xB8,		/* SekopTransaction */
	DEVICESWVERSION = 0xB9,		/* DeviceSWVersion */
	MODINFO = 0xBA,		/* ModInfo */
	SEKOPSTATUSDATA = 0xBB,		/* SekopStatusData */
	GEODATA = 0xBC,		/* GeoData */
	SENSORSDATA = 0xBD,		/* SensorsData */
	ALARMDATA = 0xBE,		/* AlarmData */
	SEKOPMZDATA = 0xBF,		/* SekopMZData */
	AUTHDATA = 0xC0,		/* AuthData */
	RFID = 0xC1,		/* RFID */
	QUEUESTATUS = 0xC2,		/* QueueStatus */
	QUEUECLEAN = 0xC3,		/* QueueClean */
	OLDPROTOCOL = 0xC4,		/* OldProtocol */
	DIAGPOWER = 0xC5,		/* DiagPower */
	CHANNELDATA = 0xC6,		/* ChannelData */
	SEKOPDATAIN = 0xC8,		/* SekopDataIn */
	SEKOPDATAOUT = 0xC9,		/* SekopDataOut */
	CONTAINERCRC16 = 0xE1,		/* ContainerCrc16 */
	ERR = 0xE9,		/* Error */
	REQREAD = 0xEA,		/* ReqRead */
	REPLY = 0xEB,		/* Reply */
	REQWRITE = 0xEC,		/* ReqWrite */
	CETUSMZ = 0xF2,		/* CetusMZ */
	TML_TAGS_RANGE,
} TML_TAG;

#endif /* TML_TAG_H_ */
