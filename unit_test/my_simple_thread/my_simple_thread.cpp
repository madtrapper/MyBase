// my_simple_thread.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/simple_thread.h"

class MyThreadRunner : public base::DelegateSimpleThread::Delegate {
public:
	MyThreadRunner() {
		bBreak = false;
	}
	
	virtual void Run() OVERRIDE{
		for (int i = 0; i < 100; i++) {
			Sleep(1000);
			if (bBreak)
				break;
		}
	}

	
public:
	void breakIt() {
		bBreak = true;
	}

protected:
	bool bBreak;
};

int _tmain(int argc, _TCHAR* argv[])
{
	MyThreadRunner runner;

	base::DelegateSimpleThread thread(&runner, "test");

	thread.Start();

	Sleep(1000 * 6);

	//::TerminateThread(thread.)

	runner.breakIt();

	thread.Join();

	return 0;
}

