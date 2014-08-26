#include "stdafx.h"
#include "platform_thread.h"

namespace base{
namespace {
// The information on how to set the thread name comes from
// a MSDN article: http://msdn2.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD kVCThreadNameException = 0x406D1388;
typedef struct tagTHREADNAME_INFO {
	DWORD dwType;  // Must be 0x1000.
	LPCSTR szName;  // Pointer to name (in user addr space).
	DWORD dwThreadID;  // Thread ID (-1=caller thread).
	DWORD dwFlags;  // Reserved for future use, must be zero.
} THREADNAME_INFO;

// This function has try handling, so it is separated out of its caller.
void SetNameInternal(PlatformThreadId thread_id, const char* name) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = thread_id;
	info.dwFlags = 0;

	__try {
		RaiseException(kVCThreadNameException, 0, sizeof(info) / sizeof(DWORD),
			reinterpret_cast<DWORD_PTR*>(&info));
	}
	__except (EXCEPTION_CONTINUE_EXECUTION) {
	}
}

struct ThreadParams {
	PlatformThread::Delegate* delegate;
	bool joinable;
};

DWORD __stdcall ThreadFunc(void* params) {
	ThreadParams* thread_params = static_cast<ThreadParams*>(params);
	PlatformThread::Delegate* delegate = thread_params->delegate;
	if (!thread_params->joinable)
		base::ThreadRestrictions::SetSingletonAllowed(false);

	// Retrieve a copy of the thread handle to use as the key in the
	// thread name mapping.
	PlatformThreadHandle::Handle platform_handle;
	BOOL did_dup = DuplicateHandle(GetCurrentProcess(),
		GetCurrentThread(),
		GetCurrentProcess(),
		&platform_handle,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);

	win::ScopedHandle scoped_platform_handle;

	if (did_dup) {
		scoped_platform_handle.Set(platform_handle);
		ThreadIdNameManager::GetInstance()->RegisterThread(
			scoped_platform_handle.Get(),
			PlatformThread::CurrentId());
	}

	delete thread_params;
	delegate->ThreadMain();

	if (did_dup) {
		ThreadIdNameManager::GetInstance()->RemoveName(
			scoped_platform_handle.Get(),
			PlatformThread::CurrentId());
	}

	return NULL;
}

}

}