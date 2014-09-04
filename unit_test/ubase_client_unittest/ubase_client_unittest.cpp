// ubase_client_unittest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tchar.h>
#include <conio.h>
#include <string>
#include <Windows.h>


int _tmain(int argc, _TCHAR* argv[])
{
	std::wstring strName = L"\\\\.\\pipe\\Test1";
	HANDLE 	_hServerPipe = CreateFile(strName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING,
		SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
		NULL);

	if (_hServerPipe == INVALID_HANDLE_VALUE) {
		DWORD dwErr = GetLastError();
		printf("Open pipe error: %d.\n", dwErr);
		if (ERROR_PIPE_BUSY == dwErr) {
			printf("1\n");
			if (!WaitNamedPipe(strName.c_str(), 2000)) {
				printf("Still busy.\n");
			}
			else {
				printf("ok.\n");
			}
		}
	}
	else {
		printf("Open pipe success.\n");
	}

	_getch();

	CloseHandle(_hServerPipe);

	return 0;
}

