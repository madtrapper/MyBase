#ifndef __SRV_WORKER__
#define __SRV_WORKER__
#include "base/run_loop.h"
#include "base/macros.h"
#include "base/basictypes.h"
#include "base/synchronization/waitable_event.h"
#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/files/file_util.h"
#include "base/files/file_enumerator.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_sync_channel.h"
#include "ipc/ipc_listener.h"
#include "ipc_test_msg.h"

class SrvWorker : public IPC::Listener, public IPC::Sender {
public:
	// Will create a channel without a name.
	SrvWorker(IPC::Channel::Mode mode, const std::string& thread_name)
		: done_(new base::WaitableEvent(false, false)),
		channel_created_(new base::WaitableEvent(false, false)),
		mode_(mode),
		ipc_thread_((thread_name + "_ipc").c_str()),
		listener_thread_((thread_name + "_listener").c_str()),
		overrided_thread_(NULL),
		shutdown_event_(true, false),
		is_shutdown_(false) {
	}

	// Will create a named channel and use this name for the threads' name.
	SrvWorker(const std::string& channel_name, IPC::Channel::Mode mode)
		: done_(new base::WaitableEvent(false, false)),
		channel_created_(new base::WaitableEvent(false, false)),
		channel_name_(channel_name),
		mode_(mode),
		ipc_thread_((channel_name + "_ipc").c_str()),
		listener_thread_((channel_name + "_listener").c_str()),
		overrided_thread_(NULL),
		shutdown_event_(true, false),
		is_shutdown_(false) {
	}

	virtual ~SrvWorker() {
		// Shutdown() must be called before destruction.
		CHECK(is_shutdown_);
	}
	void AddRef() { }
	void Release() { }
	virtual bool Send(IPC::Message* msg) OVERRIDE{ return channel_->Send(msg); }
	void WaitForChannelCreation() { channel_created_->Wait(); }
	void CloseChannel() {
		DCHECK(base::MessageLoop::current() == ListenerThread()->message_loop());
		channel_->Close();
	}
	void Start() {
		StartThread(&listener_thread_, base::MessageLoop::TYPE_DEFAULT);
		ListenerThread()->message_loop()->PostTask(
			FROM_HERE, base::Bind(&SrvWorker::OnStart, this));
	}
	void Shutdown() {
		// The IPC thread needs to outlive SyncChannel. We can't do this in
		// ~Worker(), since that'll reset the vtable pointer (to Worker's), which
		// may result in a race conditions. See http://crbug.com/25841.
		base::WaitableEvent listener_done(false, false), ipc_done(false, false);
		ListenerThread()->message_loop()->PostTask(
			FROM_HERE, base::Bind(&SrvWorker::OnListenerThreadShutdown1, this,
			&listener_done, &ipc_done));
		listener_done.Wait();
		ipc_done.Wait();
		ipc_thread_.Stop();
		listener_thread_.Stop();
		is_shutdown_ = true;
	}
	void OverrideThread(base::Thread* overrided_thread) {
		DCHECK(overrided_thread_ == NULL);
		overrided_thread_ = overrided_thread;
	}
	bool SendAnswerToLife(bool pump, bool succeed) {
		int answer = 0;
		IPC::SyncMessage* msg = new SyncChannelTestMsg_AnswerToLife(&answer);
		if (pump)
			msg->EnableMessagePumping();
		bool result = Send(msg);
		DCHECK_EQ(result, succeed);
		DCHECK_EQ(answer, (succeed ? 42 : 0));
		return result;
	}
	bool SendDouble(bool pump, bool succeed) {
		int answer = 0;
		IPC::SyncMessage* msg = new SyncChannelTestMsg_Double(5, &answer);
		if (pump)
			msg->EnableMessagePumping();
		bool result = Send(msg);
		DCHECK_EQ(result, succeed);
		DCHECK_EQ(answer, (succeed ? 10 : 0));
		return result;
	}

	virtual void OnChannelConnected(int32 peer_pid) {
		printf("Listener::OnChannelConnected() : Channel Connected \n");
	}

	virtual void OnChannelError() {
		printf("Listener::OnChannelConnected() : Channel Error\n");
	}

	const std::string& channel_name() { return channel_name_; }
	IPC::Channel::Mode mode() { return mode_; }
	base::WaitableEvent* done_event() { return done_.get(); }
	base::WaitableEvent* shutdown_event() { return &shutdown_event_; }
	void ResetChannel() { channel_.reset(); }
	// Derived classes need to call this when they've completed their part of
	// the test.
	void Done() { done_->Signal(); }

protected:
	IPC::SyncChannel* channel() { return channel_.get(); }
	// Functions for dervied classes to implement if they wish.
	virtual void Run() { }
	virtual void OnAnswer(int* answer) { NOTREACHED(); }
	virtual void OnAnswerDelay(IPC::Message* reply_msg) {
		// The message handler map below can only take one entry for
		// SyncChannelTestMsg_AnswerToLife, so since some classes want
		// the normal version while other want the delayed reply, we
		// call the normal version if the derived class didn't override
		// this function.
		int answer;
		OnAnswer(&answer);
		SyncChannelTestMsg_AnswerToLife::WriteReplyParams(reply_msg, answer);
		Send(reply_msg);
	}
	virtual void OnDouble(int in, int* out) { NOTREACHED(); }
	virtual void OnDoubleDelay(int in, IPC::Message* reply_msg) {
		int result;
		OnDouble(in, &result);
		SyncChannelTestMsg_Double::WriteReplyParams(reply_msg, result);
		Send(reply_msg);
	}

	virtual void OnNestedTestMsg(IPC::Message* reply_msg) {
		NOTREACHED();
	}

	virtual IPC::SyncChannel* CreateChannel() {
		scoped_ptr<IPC::SyncChannel> channel = IPC::SyncChannel::Create(
			channel_name_, mode_, this, ipc_thread_.message_loop_proxy().get(),
			true, &shutdown_event_);
		return channel.release();
	}

	base::Thread* ListenerThread() {
		return overrided_thread_ ? overrided_thread_ : &listener_thread_;
	}

	const base::Thread& ipc_thread() const { return ipc_thread_; }

private:
	// Called on the listener thread to create the sync channel.
	void OnStart() {
		// Link ipc_thread_, listener_thread_ and channel_ altogether.
		StartThread(&ipc_thread_, base::MessageLoop::TYPE_IO);
		channel_.reset(CreateChannel());
		channel_created_->Signal();
		Run();
	}

	void OnListenerThreadShutdown1(base::WaitableEvent* listener_event,
		base::WaitableEvent* ipc_event) {
		// SyncChannel needs to be destructed on the thread that it was created on.
		channel_.reset();

		base::RunLoop().RunUntilIdle();

		ipc_thread_.message_loop()->PostTask(
			FROM_HERE, base::Bind(&SrvWorker::OnIPCThreadShutdown, this,
			listener_event, ipc_event));
	}

	void OnIPCThreadShutdown(base::WaitableEvent* listener_event,
		base::WaitableEvent* ipc_event) {
		base::RunLoop().RunUntilIdle();
		ipc_event->Signal();

		listener_thread_.message_loop()->PostTask(
			FROM_HERE, base::Bind(&SrvWorker::OnListenerThreadShutdown2, this,
			listener_event));
	}

	void OnListenerThreadShutdown2(base::WaitableEvent* listener_event) {
		base::RunLoop().RunUntilIdle();
		listener_event->Signal();
	}

	virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE{
		IPC_BEGIN_MESSAGE_MAP(SrvWorker, message)
		IPC_MESSAGE_HANDLER_DELAY_REPLY(SyncChannelTestMsg_Double, OnDoubleDelay)
		IPC_MESSAGE_HANDLER_DELAY_REPLY(SyncChannelTestMsg_AnswerToLife,
		OnAnswerDelay)
		IPC_MESSAGE_HANDLER_DELAY_REPLY(SyncChannelNestedTestMsg_String,
		OnNestedTestMsg)
		IPC_END_MESSAGE_MAP()
		return true;
	}

	void StartThread(base::Thread* thread, base::MessageLoop::Type type) {
		base::Thread::Options options;
		options.message_loop_type = type;
		thread->StartWithOptions(options);
	}

	scoped_ptr<base::WaitableEvent> done_;
	scoped_ptr<base::WaitableEvent> channel_created_;
	std::string channel_name_;
	IPC::Channel::Mode mode_;
	scoped_ptr<IPC::SyncChannel> channel_;
	base::Thread ipc_thread_;
	base::Thread listener_thread_;
	base::Thread* overrided_thread_;

	base::WaitableEvent shutdown_event_;

	bool is_shutdown_;

	DISALLOW_COPY_AND_ASSIGN(SrvWorker);
};


#endif