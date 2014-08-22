#include "build/build_config.h"
#include <windows.h>
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
#include "base/logging.h"
#include "base/threading/platform_thread.h"
#include "base/message_loop/message_loop.h"

const char kFuzzerChannel[] = "channelName";

#define IPC_MESSAGE_IMPL
#include "ipc/ipc_message_macros.h"

#include "../ipc_client/IPCMessage.h"

/* Listener */
class SimpleListener : public IPC::Listener {
public:
	SimpleListener() : other_(NULL) {}

	void Init(IPC::Sender* s)
	{
		other_ = s;
	}

	virtual bool OnMessageReceived(const IPC::Message& msg) {
		std::cout << "Listener::OnMessageReceived() : Message Received" << std::endl;
		return true;
	}

	virtual void OnChannelConnected(int32 peer_pid)
	{
		std::cout << "Listener::OnChannelConnected() : Channel Connected" << std::endl;

		IPC::Message* msg = NULL;
		int value = 43;
		msg = new MsgClassIS(value, L"expect 43");
		if (!other_->Send(msg)){
			std::cout << "Error in sending MsgClassIS" << std::endl;
			return;
		}
		std::cout << "Sent MsgClassIS" << std::endl;

		msg = new MsgClassSI(L"expect 44", ++value);
		if (!other_->Send(msg)){
			std::cout << "Error in sending MsgClassSI" << std::endl;
			return;
		}
		std::cout << "Sent MsgClassSI" << std::endl;
	}

	virtual void OnChannelError()
	{
		std::cout << "Listener::OnChannelError() : Channel Error" << std::endl;
		exit(0);
	}
protected:
	IPC::Sender* other_;
};

void main()
{
	base::AtExitManager exit_manager;
	SimpleListener clientListener;
	base::MessageLoopForIO main_message_loop;

	std::cout << "Client: Creating IPC channel " << kFuzzerChannel << std::endl;
	//IPC::Channel clientChannel(kFuzzerChannel, IPC::Channel::MODE_SERVER, &clientListener);
	scoped_ptr<IPC::Channel> clientChannel = 
		IPC::Channel::Create(kFuzzerChannel, IPC::Channel::MODE_SERVER, &clientListener);
	std::cout << "Waiting for IPC Server to come up " << std::endl;
	//base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(5));
	std::cout << "Client: Connecting to IPC channel " << kFuzzerChannel << std::endl;
	if (!clientChannel->Connect()){
		std::cout << "Error in connecting to the channel " << kFuzzerChannel << std::endl;
		return;
	}
	std::cout << "Client: Connected to IPC channel " << kFuzzerChannel << std::endl;

	std::cout << "Initializing listener" << std::endl;
	clientListener.Init(static_cast<IPC::Sender*>(clientChannel.get()));

	std::cout << "Starting the MessageLoop" << std::endl;
	base::MessageLoop::current()->Run();
}