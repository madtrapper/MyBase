// ubase_unittest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>

#include <ubase/ipc_channel.h>

IPC::Channel srvChannel(L"Test1", IPC::Channel::MODE_SERVER);

int _tmain(int argc, _TCHAR* argv[])
{
	srvChannel.InitChannel();

	_getch();
	return 0;
}

