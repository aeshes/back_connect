#include "sock.h"

DWORD WINAPI SendData(SOCKET sock, char *data)
{
	int ret = send(sock, data, lstrlenA(data), 0);

	if (ret <= 0)
	{
		FormatError(WSAGetLastError());
	}
	return 0;
}