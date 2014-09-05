// ubase_client_unittest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tchar.h>
#include <conio.h>
#include <string>
#include <Windows.h>

#include <ubase/ipc_channel.h>

IPC::Channel clientChannel(L"Test1", IPC::Channel::MODE_CLIENT);

int _tmain(int argc, _TCHAR* argv[])
{
	char buf[1024*64] = { 0 };
	clientChannel.InitChannel();

	for (int i = 0; i < 1000000; i++) {
		//sprintf_s(buf, "%d", i);
		if (!clientChannel.Send(buf, sizeof(buf), 1000))
			printf("Pipe Write Error.\n");
		
		memset(buf, 0, sizeof(buf));
		int outLen;
		if (!clientChannel.Read(buf, sizeof(buf), &outLen, 1000)) {
			printf("Pipe Read Error.\n");
		}
		else {
			if (outLen != sizeof(buf))
				printf("Outlen :%d\n");
			/*
			if (0 == strlen(buf)) {
				printf("Nothing in buf\n");
			}
			*/
			//printf("Buf:%s \n", buf);			
		}
	}
	
	printf("Done.\n");
	_getch();

	return 0;
}

