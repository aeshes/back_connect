#include "error.h"

void WINAPI FormatError(DWORD errCode)
{
	char error[1000]; 
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL,
		errCode,
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		error, sizeof(error), NULL);
	printf("\nError: %s\n", error);
}