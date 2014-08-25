// ipc_sync_server.cpp : Defines the entry point for the console application.
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

#include "IPCSyncMessage.h"

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
		IPC_MESSAGE_HANDLER(Msg_C_1_2, classMsg_C_1_2)
		IPC_END_MESSAGE_MAP()
		return true;
	}

	void classMsg_C_1_2(bool in1, bool *out1, int *out2)
	{
		std::cout << "Boolean received : " << in1 << std::endl;
		*out1 = false;
		*out2 = 255;
		std::cout << "Out1: " << *out1 << std::endl;
		std::cout << "Out2: " << *out2 << std::endl;
	}

	void Send(const IPC::Message *msg)
	{
		other_->Send((IPC::Message *)msg);
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

protected:
	IPC::Sender* other_;
};


int _tmain(int argc, _TCHAR* argv[])
{
	base::AtExitManager					exit_manager;
	SimpleListener						serverListener;
	scoped_ptr<base::MessageLoopForUI>  message_loop_(new base::MessageLoopForUI());
	scoped_ptr<base::Thread>            thread_(new base::Thread("New_Thread"));
	scoped_ptr<base::WaitableEvent>     wEvent_(new base::WaitableEvent(false, false));
	base::Thread::Options               options;

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
								 IPC::Channel::MODE_SERVER, 
								 &serverListener, 
								 thread_->message_loop_proxy(), 
								 true, 
								 wEvent_.get());
	std::cout << ::GetCurrentThreadId() << ": Main Thread : Connected to IPC channel " << kFuzzerChannel << std::endl;

	std::cout << "Initializing listener" << std::endl;
	serverListener.Init(static_cast<IPC::Sender*>(syncChannel.get()));

	std::cout << "Starting the MessageLoop" << std::endl;
	base::MessageLoop::current()->Run();
}

