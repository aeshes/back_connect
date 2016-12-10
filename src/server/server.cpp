#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 666
#define DEFAULT_PORT "666"	/* Must be the same as PORT */
#define MAX_CONNECTIONS 10

void WINAPI FormatError(DWORD error);
SOCKET WINAPI BindSocket(uint16_t port);
DWORD WINAPI HandleConnection(LPVOID sock);
void WINAPI PrintClientInfo(sockaddr_in *addr);
void WINAPI BackConnect(char *hostname);

int main(int argc, char *argv[])
{
	WSADATA wsadata;
	SOCKET server;
	SOCKET client;
	DWORD ThreadID;
	sockaddr_in client_addr;
	int addr_len = sizeof(client_addr);

	if (FAILED(WSAStartup(MAKEWORD(2, 0), &wsadata)))
	{
		FormatError(WSAGetLastError());
	}
	else
	{
		server = BindSocket(PORT);
		if (listen(server, MAX_CONNECTIONS) == SOCKET_ERROR)
		{
			FormatError(WSAGetLastError());
			WSACleanup();
			return -1;
		}

		while (TRUE)
		{
			client = accept(server, (struct sockaddr *)&client_addr, &addr_len);
			if (client != INVALID_SOCKET)
			{
				PrintClientInfo(&client_addr);
				CreateThread(NULL, 0, HandleConnection, &client, NULL, &ThreadID);
			}
		}
		WSACleanup();
	}
}

void WINAPI FormatError(DWORD errCode)
{
	char error[1000]; 
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		error, sizeof(error), NULL);
	printf("\nError: %s\n", error);
	getchar();
}

SOCKET WINAPI BindSocket(uint16_t port)
{
	SOCKET sock = INVALID_SOCKET;
	sockaddr_in local_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		FormatError(WSAGetLastError());
	}
	else
	{
		RtlZeroMemory(&local_addr, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_port = htons(port);
		local_addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) == SOCKET_ERROR)
		{
			FormatError(WSAGetLastError());
		}
		else
		{
			printf("Server running...\n");
		}
	}

	return sock;
}

DWORD WINAPI HandleConnection(LPVOID sock)
{
	SOCKET client;
	HANDLE hFile = NULL;
	char buffer[200];
	int recv_bytes = 0;
	DWORD bytes_written = 0;

	client = *(SOCKET *)sock;

	hFile = CreateFile(TEXT("recv.exe"),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		closesocket(client);
		return -1;
	}

	while (TRUE)
	{
		recv_bytes = recv(client, buffer, 200, 0);
		if (recv_bytes > 0)
		{
			WriteFile(hFile, buffer, recv_bytes, &bytes_written, NULL);
		}
		else if (recv_bytes == 0)
		{
			break;
		}
		else if (recv_bytes == SOCKET_ERROR)
		{
			FormatError(WSAGetLastError());
			return -1;
		}
	}

	printf("Connection closed\n");

	CloseHandle(hFile);
	closesocket(client);

	return 0;
}

void WINAPI PrintClientInfo(sockaddr_in *addr)
{
	char hostname[NI_MAXHOST];

	getnameinfo((struct sockaddr *)addr, sizeof(*addr), hostname, sizeof(hostname), NULL, 0, 0);
	printf("+ %s [%s] new connection\n", hostname, inet_ntoa(addr->sin_addr));
}