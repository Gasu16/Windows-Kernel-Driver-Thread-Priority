#pragma once
#include <windows.h>

struct ThreadData {
	ULONG ThreadId;
	int Priority;
};

#define MY_DRIVER_DEVICE 0x8000

#define IOCTL_MY_DRIVER_SET_PRIORITY CTL_CODE(MY_DRIVER_DEVICE, \
	0x800, METHOD_NEITHER, FILE_ANY_ACCESS)