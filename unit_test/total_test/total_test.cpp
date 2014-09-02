// total_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/files/file_util.h"
#include "base/files/file_enumerator.h"
#include "base/message_loop/message_loop.h"

static base::AtExitManager exit_manager;

int _tmain(int argc, _TCHAR* argv[])
{
	base::MessageLoop mainLoop;
	CommandLine cl(argc, argv);
	
	base::FilePath exePath;
	wchar_t szExeName[MAX_PATH];

	::GetModuleFileName(NULL, szExeName, MAX_PATH);	

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
		base::LaunchProcess(s, options, NULL);

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

	}
	else if (cl.HasSwitch("client")) {		
		printf("Client mode.\n");		
	}
	else {
		printf("cmd error\n");
	}

	printf("Press enter to exit.\n");
	_getch();

	return 0;
}

