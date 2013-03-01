#ifndef GNSS_H
#define GNSS_H
typedef struct _status_gnss_t {							// ~~~ ÃÍÑÑ ~~~
	uint8_t	is_present:1;			// ðåçóëüòàò ïîèñêà ìîäóëÿ ÃÍÑÑ
	uint8_t	state:4;				// ýòàï ïîèñêà è êîíôèãóðàöèè ìîäóëÿ ÃÍÑÑ
	uint16_t time:16;				// ìàêñèìàëüíîå âðåìÿ (ìñ) äî ñëåäóþùåãî ýòàïà ïîèñêà è êîíôèãóðàöèè ìîäóëÿ ÃÍÑÑ
	uint8_t port_cfg:5;				// òåêóùàÿ êîíôèãóðàöèÿ ïîðòà ÃÍÑÑ (èíäåêñ çàïèñè â ìàññèâå gnss_port_cfg)
	uint8_t test:2;					// ðåçóëüòàò ñàìîòåñòèðîâàíèÿ ìîäóëÿ
	uint8_t tested:1;				// ïîëó÷åíû ëè ðåçóëüòàòû ñàìîòåñòèðîâàíèÿ
	uint8_t ant_status:2;	//0 - good, 1 - shaded, 2 - failure
	uint8_t valid :1;
} status_gnss_t;
void packet_detector_geos(int c);
int gnss_init(void);
#endif
