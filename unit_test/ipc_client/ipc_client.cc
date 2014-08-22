#include "build/build_config.h"
#include <windows.h>
#include <conio.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#undef max
#undef min
#include "base/basictypes.h"
#include "base/at_exit.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_listener.h"
#include "base/threading/platform_thread.h"
#include "base/message_loop/message_loop.h"

const char kFuzzerChannel[] = "channelName";

#define IPC_MESSAGE_IMPL
#include "ipc/ipc_message_macros.h"

#include "IPCMessage.h"

/* Listener */
class SimpleListener : public IPC::Listener {
public:
	SimpleListener() : other_(NULL) {}

	void Init(IPC::Sender* s)
	{
		other_ = s;
	}

	virtual bool OnMessageReceived(const IPC::Message& msg) {
		std::cout << "Listener::OnChannelConnected() : Message Received" << std::endl;
		IPC_BEGIN_MESSAGE_MAP(SimpleListener, msg)
		IPC_MESSAGE_HANDLER(MsgClassIS, classIS)
		IPC_MESSAGE_HANDLER(MsgClassSI, classSI)
		IPC_END_MESSAGE_MAP()
		return true;
	}

	virtual void OnChannelConnected(int32 peer_pid)
	{
		std::cout << "Listener::OnChannelConnected() : Channel Connected" << std::endl;
	}

	virtual void OnChannelError()
	{
		std::cout << "Listener::OnChannelConnected() : Channel Error" << std::endl;
		exit(0);
	}

	void classIS(int myint, std::wstring mystring) {
		std::cout << "classIS() : Received == > Int:" << myint << " String : " << mystring << std::endl;
	}

	void classSI(std::wstring mystring, int myint) {
		std::cout << "classSI() : Received == > String:" << mystring << " Int : " << myint << std::endl;
	}

protected:
	IPC::Sender* other_;
};

void main()
{
	base::AtExitManager exit_manager;

	printf("I am ipc client\n");
	SimpleListener serverListener;
	base::MessageLoopForIO main_message_loop;

	std::cout << "Server: Creating IPC channel " << kFuzzerChannel << std::endl;
	//IPC::Channel serverChannel(kFuzzerChannel, IPC::Channel::MODE_CLIENT, &serverListener);
	scoped_ptr<IPC::Channel> serverChannel = 
		IPC::Channel::Create(kFuzzerChannel, IPC::Channel::MODE_CLIENT, &serverListener);
	std::cout << "Server: Connecting to IPC channel " << kFuzzerChannel << std::endl;
	
	if (!serverChannel->Connect()){
		std::cout << "Server: Error in connecting to the channel " << kFuzzerChannel << std::endl;
		return;
	}
	std::cout << "Server: Connected to IPC channel " << kFuzzerChannel << std::endl;

	std::cout << "Initializing listener" << std::endl;
	serverListener.Init(static_cast<IPC::Sender*>(serverChannel.get()));

	std::cout << "Starting the MessageLoop" << std::endl;
	//_getch();
	base::MessageLoop::current()->Run();
}