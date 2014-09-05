#include "stdafx.h"
#include "ipc_channel.h"
#include <process.h>

namespace IPC {
//
bool Channel::InitChannel() {
	bool bRet = false;
	const DWORD open_mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;

	if (channelMode_ == MODE_CLIENT) {
		hPipe_ = CreateFile(channelName_.c_str(),
						    GENERIC_READ | GENERIC_WRITE,
			                0, NULL, OPEN_EXISTING,
			                SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
							NULL);
		if (hPipe_ != INVALID_HANDLE_VALUE) {
			pipeStatus_ = PIPE_CONNECTED;
			bRet = true;
			printf("Connected with server\n");
		}
		else {
			printf("Connect fail\n");
		}
	}
	else { //Server mode
		hPipe_ = CreateNamedPipe(channelName_.c_str(),
								 open_mode,
								 PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
								 1,
								 PIPE_READ_BUFFER_SIZE,
								 PIPE_READ_BUFFER_SIZE,
								 1000 * 5,
								 NULL);
		
		if (hPipe_ != INVALID_HANDLE_VALUE) {
			bRet = true;
			BOOL fConnected = ConnectNamedPipe(hPipe_, &pipe_overlap_.overLap);
			if (fConnected) {
				pipeStatus_ = PIPE_CONNECTED;
				printf("Pipe connected.\n");
				return bRet;
			}
			switch (GetLastError()) {
			case ERROR_IO_PENDING:
				pipeStatus_ = PIPE_WAIT_FOR_CONNECT;
				printf("Wait for connected...\n");
				break;
			case ERROR_PIPE_CONNECTED:
				pipeStatus_ = PIPE_DISCONNECTED;
				bRet = false;
				printf("Pipe err disconnected...\n");
				break;
			default:
				pipeStatus_ = PIPE_DISCONNECTED;
				bRet = false;
				printf("Pipe error:%d\n", GetLastError());
			}

			if (pipeStatus_ == PIPE_WAIT_FOR_CONNECT) {
				DWORD cbRet = 0;
				/*
				DWORD dwWait = WaitForSingleObject(overLap_.hEvent, 5000);

				if (dwWait == WAIT_OBJECT_0) {
					pipeStatus_ = PIPE_CONNECTED;
					bRet = true;
					printf("Pipe connected.\n");
				}
				else {
					pipeStatus_ = PIPE_DISCONNECTED;
					bRet = false;
					printf("Pipe error:%d\n", GetLastError());
				}
				*/
				
				BOOL fSuccess = GetOverlappedResult(hPipe_, 
													&pipe_overlap_.overLap,
													&cbRet, 
													TRUE);
				if (fSuccess) {
					pipeStatus_ = PIPE_CONNECTED;
					bRet = true;
					printf("Pipe connected.\n");
				}
				else {
					pipeStatus_ = PIPE_DISCONNECTED;
					bRet = false;
					printf("Pipe error:%d\n", GetLastError());
				}
				
			}
		}
	}

	return bRet;
}

void WINAPI WriteCallback(DWORD dwError, DWORD cbTransferred, LPOVERLAPPED lpo)
{
	PIPE_OVERLAP* pPipeOverlap = (PIPE_OVERLAP*)lpo;
	if (pPipeOverlap) {
		pPipeOverlap->byteTransfered = cbTransferred;
		SetEvent(pPipeOverlap->hEvent);
	}
}

BOOL Channel::Send(const char* buf, int len, int timeout) 
{
	BOOL bRet = FALSE;
	DWORD dwWrite = 0;
	DWORD dwWait;
	
	if (pipeStatus_ != PIPE_CONNECTED || !buf || !len) { 
		return bRet;
	}

	ResetEvent(pipe_overlap_.hEvent);

	bRet = WriteFileEx(hPipe_, buf, len, (LPOVERLAPPED)&pipe_overlap_, &WriteCallback);

	if (bRet) {
		dwWait = WaitForSingleObjectEx(pipe_overlap_.hEvent, timeout, TRUE);
		if (dwWait == WAIT_OBJECT_0 ||
			dwWait == WAIT_IO_COMPLETION) {
			bRet = TRUE;
		}
		else {
			bRet = FALSE;
		}
		return bRet;
	}

	DWORD err = GetLastError();

	if (ERROR_BROKEN_PIPE == err) {
		pipeStatus_ = PIPE_DISCONNECTED;
		bRet = FALSE;
		return bRet;
	}

	if (ERROR_IO_PENDING == GetLastError()) {
		dwWait = WaitForSingleObjectEx(pipe_overlap_.hEvent, timeout, TRUE);
		if (dwWait == WAIT_OBJECT_0 ||
			dwWait == WAIT_IO_COMPLETION) {
			bRet = TRUE;
		}
		else {
			bRet = FALSE;
		}
	}

	return bRet;
}

void WINAPI ReadCallback(DWORD dwError, DWORD cbTransferred, LPOVERLAPPED lpo)
{
	PIPE_OVERLAP* pPipeOverlap = (PIPE_OVERLAP*)lpo;
	if (pPipeOverlap) {
		pPipeOverlap->byteTransfered = cbTransferred;
		SetEvent(pPipeOverlap->hEvent);
	}
}

BOOL Channel::Read(char* buf, int len, int* out_len, int timeout)
{
	BOOL bRet = FALSE;
	DWORD dwRead = 0;
	DWORD dwWait;

	if (pipeStatus_ != PIPE_CONNECTED) {
		return bRet;
	}
	ResetEvent(pipe_overlap_.hEvent);
	bRet = ReadFileEx(hPipe_, buf, len, (LPOVERLAPPED)&pipe_overlap_, &ReadCallback);
	
	if (bRet) {
		dwWait = WaitForSingleObjectEx(pipe_overlap_.hEvent, timeout, TRUE);
		if (dwWait == WAIT_OBJECT_0 ||
			dwWait == WAIT_IO_COMPLETION) {
			*out_len = pipe_overlap_.byteTransfered;
			bRet = TRUE;
		}
		else {
			bRet = FALSE;
		}
		return bRet;
	}

	DWORD err = GetLastError();

	if (ERROR_BROKEN_PIPE == err) {
		pipeStatus_ = PIPE_DISCONNECTED;
		bRet = FALSE;
		return bRet;
	}

	if (ERROR_IO_PENDING == GetLastError()) {
		dwWait = WaitForSingleObjectEx(pipe_overlap_.hEvent, timeout, TRUE);
		if (dwWait == WAIT_OBJECT_0 ||
			dwWait == WAIT_IO_COMPLETION) {
			bRet = TRUE;
			*out_len = pipe_overlap_.byteTransfered;
		}
		else {
			bRet = FALSE;
		}		
	}


	return bRet;
}


} // End of Namespace
