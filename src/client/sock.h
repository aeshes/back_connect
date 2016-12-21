#ifndef SOCK_H
#define SOCK_H

#include <WinSock2.h>
#include "error.h"

DWORD WINAPI SendData(SOCKET sock, char *data);

#endif