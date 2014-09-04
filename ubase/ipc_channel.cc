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
		if (hPipe_ != INVALID_HANDLE_VALUE)
			bRet = true;
	}
	else { //Server mode
		hPipe_ = CreateNamedPipe(channelName_.c_str(),
								 open_mode,
								 PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
								 1,
								 PIPE_READ_BUFFER_SIZE,
								 PIPE_READ_BUFFER_SIZE,
								 5000 * 10,
								 NULL);
		
		if (hPipe_ != INVALID_HANDLE_VALUE) {
			bRet = true;
			IocpStart();
			CreateIoCompletionPort(hPipe_, iocp_.h_iocp, 0, 0);
			overLap_[CONNECT].hPipe = hPipe_;
			overLap_[CONNECT].pChannel = this;
			BOOL bRes = ConnectNamedPipe(overLap_[CONNECT].hPipe, (LPOVERLAPPED)&overLap_[CONNECT]);
		}
	}

	return bRet;
}

void Channel::IocpStart() {
	iocp_.h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	iocp_.threads_count = 2;
	iocp_.h_threads = (HANDLE*)malloc(sizeof(HANDLE)* iocp_.threads_count);

	for (size_t n = 0; n < iocp_.threads_count; ++n)
		iocp_.h_threads[n] = (HANDLE)_beginthreadex(0, 0, iocp_proc, &iocp_, 0, 0);
}

void Channel::IocpCompletedRoutine(DWORD dwNumberOfBytesTransferred,
								   ULONG_PTR lpCompletionKey,
								   LPOVERLAPPED pOverlapped) {

	LPPIPEOVERLAPPED p = (LPPIPEOVERLAPPED)pOverlapped;
	BOOL bRet;
	if (p && p->hPipe == p->pChannel->overLap_[CONNECT].hPipe) {
		printf("Pipe connected.\n");
		DisconnectNamedPipe(p->hPipe);
		bRet = ConnectNamedPipe(p->hPipe, (LPOVERLAPPED)p);
	}

}

unsigned WINAPI Channel::iocp_proc(void *p)  {
	iocp_info *iocp = (iocp_info*)p;
	while (true) {
		DWORD dwNumberOfBytesTransferred = 0;
		ULONG_PTR lpCompletionKey = 0;
		LPOVERLAPPED pOverlapped = NULL;
		BOOL bRet = GetQueuedCompletionStatus(iocp->h_iocp,
											  &dwNumberOfBytesTransferred,
											  &lpCompletionKey,
											  &pOverlapped,
											  INFINITE);
		if (!bRet) {
			DWORD err = GetLastError();
			
			if (err == 64) {
				//
			}
		}
		if (NULL == lpCompletionKey && NULL == pOverlapped) {
			PostQueuedCompletionStatus(iocp->h_iocp, 0, 0, 0);
			break;
		}

		IocpCompletedRoutine(dwNumberOfBytesTransferred, lpCompletionKey, pOverlapped);
	}
	return 0;
}

} // End of Namespace
