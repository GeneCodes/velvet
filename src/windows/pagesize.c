#include <Windows.h>

unsigned int get_pagesize()
{
	SYSTEM_INFO info;
	GetNativeSystemInfo(&info);
	return (unsigned int)info.dwPageSize;
}