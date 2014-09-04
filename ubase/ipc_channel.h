#ifndef __IPC_CHANNEL__
#define __IPC_CHANNEL__

#include "ubase.h"
#include <Windows.h>
#include <string>

namespace IPC {
//
#define PIPE_READ_BUFFER_SIZE	(4 * 1024)
//
class ChannelEvtDelegate
{
public:
	ChannelEvtDelegate(){};
	virtual ~ChannelEvtDelegate(){};

	virtual void OnPipeConnect(){};
	virtual void OnPipeDisconnect(){};
	virtual void OnPipeRead(const char* buf, int len){};
	virtual void OnPipeWrite(){};
};

class UBASE_API Channel
{
public:
	enum Mode {
		MODE_SERVER,
		MODE_CLIENT
	};

	enum OVERLAPPED_TYPE{
		RECV = 0,
		SEND,
		CONNECT
	};

	Channel(const std::wstring &name, Channel::Mode mode)
		: channelMode_(mode),
		  hPipe_(NULL){
		channelName_ = L"\\\\.\\pipe\\" + name;
		memset(&overLap_, 0, sizeof(overLap_));
	}

	virtual ~Channel() {}

	bool InitChannel();
	void IocpStart();

	

protected:
	std::wstring channelName_;
	Mode channelMode_;
	HANDLE hPipe_;
	ChannelEvtDelegate* delegate;

	struct iocp_info {
		HANDLE h_iocp;
		size_t threads_count;
		HANDLE *h_threads;
	};

	iocp_info iocp_;

	typedef struct _PipeOVERLAPPED{
		OVERLAPPED	over;
		HANDLE		hPipe;
		Channel*	pChannel;
	}PIPEOVERLAPPED, *LPPIPEOVERLAPPED;

	PIPEOVERLAPPED overLap_[3];		// Connect, Read, Write

	static unsigned WINAPI iocp_proc(void *p);
	static void IocpCompletedRoutine(DWORD dwNumberOfBytesTransferred, 
							         ULONG_PTR lpCompletionKey, 
							         LPOVERLAPPED pOverlapped);

	DISALLOW_COPY_AND_ASSIGN(Channel);
};

}

#endif