// ipc_sync_client.cpp : Defines the entry point for the console application.
//

#include <tchar.h>

#include <iostream>
#include <string.h>

#include "base/basictypes.h"
#include "base/at_exit.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
//#include "base/process/process_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_channel.h"
#include "base/logging.h"
#include "base/threading/platform_thread.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/ipc_sync_channel.h"

const char kFuzzerChannel[] = "channelName";

#define IPC_MESSAGE_IMPL
#include "ipc/ipc_message_macros.h"

#include "../ipc_sync_server/IPCSyncMessage.h"

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

		IPC::SyncMessage* msg = NULL;
		int out2 = 0;
		bool out1;
		msg = new Msg_C_1_2(true, &out1, &out2);
		if (!other_->Send(msg)){
			std::cout << "Error in sending Msg_C_1_2" << std::endl;
			return;
		}
		std::cout << "Sent Msg_C_1_2 and received the response also" << std::endl;
		std::cout << "out1 is : " << out1 << " and out2 is : " << out2 << std::endl;
	}

	virtual void OnChannelError()
	{
		std::cout << "Listener::OnChannelError() : Channel Error" << std::endl;
		exit(0);
	}

protected:
	IPC::Sender* other_;
};

int _tmain(int argc, _TCHAR* argv[])
{
	base::AtExitManager				  exit_manager;
	SimpleListener                    clientListener;
	scoped_ptr<base::MessageLoopForUI>      message_loop_(new base::MessageLoopForUI());
	scoped_ptr<base::Thread>          thread_(new base::Thread("New_Thread"));
	scoped_ptr<base::WaitableEvent>   wEvent_(new base::WaitableEvent(false, false));
	base::Thread::Options             options;

	options.message_loop_type = base::MessageLoop::TYPE_IO;

	std::cout << ::GetCurrentThreadId() << ": Main Thread : Starting IO Thread " << std::endl;
	bool thread_result = thread_->StartWithOptions(options);
	if (!thread_result){
		std::cout << "failed" << std::endl;
		return 0;
	}

	std::cout << ::GetCurrentThreadId() << ": Main Thread : Creating IPC channel " << kFuzzerChannel << std::endl;
	scoped_ptr<IPC::SyncChannel> syncChannel = 
		IPC::SyncChannel::Create(kFuzzerChannel, 
								 IPC::Channel::MODE_CLIENT, 
								 &clientListener, 
								 thread_->message_loop_proxy(), 
								 true, 
								 wEvent_.get());
	std::cout << ::GetCurrentThreadId() << ": Main Thread : Connected to IPC channel " << kFuzzerChannel << std::endl;

	std::cout << "Initializing listener" << std::endl;
	clientListener.Init(static_cast<IPC::Sender*>(syncChannel.get()));

	std::cout << "Starting the MessageLoop" << std::endl;
	base::MessageLoop::current()->Run();
}

