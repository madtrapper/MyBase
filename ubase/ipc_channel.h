#ifndef __IPC_CHANNEL__
#define __IPC_CHANNEL__

#include "ubase.h"
#include <Windows.h>
#include <string>

namespace IPC {
//
#define PIPE_READ_BUFFER_SIZE	(4 * 1024)
//
typedef struct _PIPE_OVERLAP {
	OVERLAPPED overLap;
	HANDLE hEvent;
	char buf[1024 * 32];
	int byteTransfered;
}PIPE_OVERLAP;

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

	enum PIPE_STATUS {
		PIPE_WAIT_FOR_CONNECT,
		PIPE_CONNECTED,
		PIPE_DISCONNECTED
	};

	Channel(const std::wstring &name, Channel::Mode mode)
		: channelMode_(mode),
		  hPipe_(NULL),
		  pipeStatus_(PIPE_DISCONNECTED) {
		channelName_ = L"\\\\.\\pipe\\" + name;
		memset(&pipe_overlap_, 0, sizeof(pipe_overlap_));
		//memset(&overLap_, 0, sizeof(overLap_));
		pipe_overlap_.overLap.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		pipe_overlap_.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	virtual ~Channel() {}

	bool InitChannel();
	void CloseChannel();

	BOOL Send(const char* buf, int len, int timeout);
	BOOL Read(char* buf, int len, int* out_len, int timeout);

protected:
	std::wstring channelName_;
	Mode channelMode_;
	HANDLE hPipe_;
	PIPE_STATUS pipeStatus_;
	ChannelEvtDelegate* delegate_;

	PIPE_OVERLAP pipe_overlap_;

	DISALLOW_COPY_AND_ASSIGN(Channel);
};

}

#endif