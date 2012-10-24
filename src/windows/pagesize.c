#include <Windows.h>
#include <sys/types.h>

size_t get_pagesize()
{
	SYSTEM_INFO info;
	GetNativeSystemInfo(&info);
	return info.dwAllocationGranularity;
}