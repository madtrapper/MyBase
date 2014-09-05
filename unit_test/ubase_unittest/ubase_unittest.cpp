// ubase_unittest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>

#include <ubase/ipc_channel.h>

IPC::Channel srvChannel(L"Test1", IPC::Channel::MODE_SERVER);

int _tmain(int argc, _TCHAR* argv[])
{
	char buf[1024*64] = { 0 };
	srvChannel.InitChannel();

	for (int i = 0; i < 1000000; i++) {
		memset(buf, 0, sizeof(buf));
		int outLen;
		if (!srvChannel.Read(buf, sizeof(buf), &outLen, 1000)) {
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
		memset(buf, 0, sizeof(buf));
		//sprintf_s(buf, "%d", i);
		if (!srvChannel.Send(buf, sizeof(buf), 1000)) {
			printf("Pipe Write Error.\n");
		}
	}

	printf("Done.\n");

	_getch();
	return 0;
}

