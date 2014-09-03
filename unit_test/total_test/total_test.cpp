// total_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include "base/at_exit.h"
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


static base::AtExitManager exit_manager;

bool launch_child_process(const wchar_t* cmd) 
{
	STARTUPINFO			si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	BOOL bRet = CreateProcess(NULL,
							  (wchar_t*)cmd,
							  NULL,
							  NULL,
							  FALSE,
							  CREATE_NEW_CONSOLE,
							  NULL,
							  NULL,
							  &si,
							  &pi);
	return bRet;
}

class SrvWorker : public IPC::Listener {
public:
	SrvWorker() : other_(NULL) {}
	void Init(IPC::Sender* s) {
		other_ = s;
	}

	virtual void OnChannelConnected(int32 peer_pid) {
		printf("OnChannelConnected peer pid : %d\n", peer_pid);
	}

	virtual void OnChannelError() {
		printf("Listener::OnChannelError() : Channel Error\n");		
	}

	virtual bool OnMessageReceived(const IPC::Message& msg) {
		printf("Listener::OnChannelConnected() : Message Received\n");
		return true;
	}

protected:
	IPC::Sender* other_;
};

class ClientWorker : public IPC::Listener {
public:
	ClientWorker() : other_(NULL) {}

	void Init(IPC::Sender* s) {
		other_ = s;
	}

	virtual bool OnMessageReceived(const IPC::Message& msg) {
		printf("Client OnMessageReceived\n");
		return true;
	}

	virtual void OnChannelConnected(int32 peer_pid) {
		printf("Listener::OnChannelConnected() : Channel Connected\n");
	}

	virtual void OnChannelError() {
		printf("Listener::OnChannelConnected() : Channel Error\n");
	}
protected:
	IPC::Sender* other_;
};

class MsgThread : public base::Thread {
public:
	MsgThread() :base::Thread("main") {	}
};

MsgThread client_srv_thread;
SrvWorker serverListener;
scoped_ptr<IPC::SyncChannel> serverChannel;

base::WaitableEvent* wEvent_;

void runas_server() {
	base::Thread::Options options;
	wEvent_ = new base::WaitableEvent(false, false);
	

	options.message_loop_type = base::MessageLoop::TYPE_IO;
	client_srv_thread.StartWithOptions(options);

	serverChannel =	IPC::SyncChannel::Create("Channel_1",
		IPC::Channel::MODE_SERVER,
		&serverListener,
		client_srv_thread.message_loop_proxy().get(),
		true,
		wEvent_);
	serverChannel.release();
	serverListener.Init(static_cast<IPC::Sender*>(serverChannel.get()));
}

ClientWorker clientListener;
scoped_ptr<IPC::SyncChannel> clientChannel;
base:: WaitableEvent quit_ev(false, false);
base::MessageLoop mainLoop;

void runas_client() {
	base::Thread::Options options;
	wEvent_ = new base::WaitableEvent(false, false);
	

	options.message_loop_type = base::MessageLoop::TYPE_IO;
	client_srv_thread.StartWithOptions(options);
	clientChannel = IPC::SyncChannel::Create("Channel_1",
		IPC::Channel::MODE_CLIENT,
		&clientListener,
		client_srv_thread.message_loop_proxy().get(),
		true,
		wEvent_);
	
	clientChannel.release();
	clientListener.Init(static_cast<IPC::Sender*>(clientChannel.get()));
}
void shit() {
	mainLoop.QuitWhenIdle();
}
BOOL WINAPI HandlerRoutine(_In_  DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_C_EVENT ||
		dwCtrlType == CTRL_BREAK_EVENT ||
		dwCtrlType == CTRL_CLOSE_EVENT ||
		dwCtrlType == CTRL_LOGOFF_EVENT ||
		dwCtrlType == CTRL_SHUTDOWN_EVENT) {
		printf("1-------\n");
		mainLoop.PostTask(FROM_HERE, base::Bind(&shit));
		printf("2-------\n");
	}

	return TRUE;
}


int _tmain(int argc, _TCHAR* argv[])
{
	CommandLine cl(argc, argv);
	
	base::FilePath exePath;
	wchar_t szExeName[MAX_PATH];

	::GetModuleFileName(NULL, szExeName, MAX_PATH);	
	::SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	if (cl.HasSwitch("server")) {
		printf("Server mode.\n");
		base::UserTokenHandle token;
		OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &token);
		base::LaunchOptions options;
		CommandLine client_cl = CommandLine::FromString(szExeName);

		client_cl.AppendSwitch("--client");
		CommandLine::StringType s = client_cl.GetCommandLineString();
		//options.as_user = token;
		//options.
		//base::LaunchProcess(s, options, NULL);
		if (!cl.HasSwitch("standlone")) {
			launch_child_process(s.c_str());
		}

		base::FilePath scan_dir = cl.GetSwitchValuePath("d");
		_tprintf(L"Target : %s\n", scan_dir.AsUTF16Unsafe().c_str());

		base::FileEnumerator scan_enum(scan_dir, 
									   true, 
									   base::FileEnumerator::FILES,
									   FILE_PATH_LITERAL("*.*"));
		int total_file_count = 0;
		for (base::FilePath name = scan_enum.Next(); 
			!name.empty(); 
			name = scan_enum.Next(), total_file_count++) {
			_tprintf(L"File : %s\n", name.AsUTF16Unsafe().c_str());
		}

		printf("Total : %d\n", total_file_count);

		runas_server();

	}
	else if (cl.HasSwitch("client")) {		
		printf("Client mode.\n");
		runas_client();
	}
	else {
		printf("cmd error\n");
	}

	printf("Press enter to exit.\n");

	//_getch();
	//base::MessageLoop::current()->PostTask(FROM_HERE, 
	//									   base::Bind(&WaitEndEvent));
	
	mainLoop.Run();
	printf("quit\n");
	//run_loop.Run();

	client_srv_thread.Stop();

	return 0;
}

