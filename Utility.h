#pragma once

namespace Utility
{
	#define htons(x) ( (((x)<<8)&0xFF00) | (((x)>>8)&0xFF) )
	#define ntohs(x) htons(x)

	#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
					   ((x)<< 8 & 0x00FF0000UL) | \
					   ((x)>> 8 & 0x0000FF00UL) | \
					   ((x)>>24 & 0x000000FFUL) )
	#define ntohl(x) htonl(x)

	size_t readBytes(size_t count, uint8_t*  data);
	void delay(unsigned long ms);
	int dateTimeToMinutesInWeek(const DateTime& dt);
	DateTime addMinutesToDate(int minutes, const DateTime& date);

	const static size_t DATE_TIME_NETWORK_SIZE = sizeof(DateTime::Second) + sizeof(DateTime::Minute) + sizeof(DateTime::Hour) + sizeof(DateTime::Dow) + sizeof(DateTime::Day) + sizeof(DateTime::Month) + sizeof(DateTime::Year);
	static_assert(DATE_TIME_NETWORK_SIZE != 6, "DateTime should be 6");
	template<class T>
	void dateTimeFromBytes(DateTime& dt, const T bytes, int offset = 0);
	template< class T>
	void dateTimeToBytes(const DateTime& dt, T bytes, int offset = 0);
}
