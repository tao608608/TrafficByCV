#include "thread.h"


thread::thread()
:m_stopFlag(false),
m_hThread(INVALID_HANDLE_VALUE)
{
}

thread::~thread(void)
{
	Stop();
}

void thread::Start()
{
	unsigned long *p = NULL;
	m_hThread = ::CreateThread(NULL, 0, ThreadProc, this, 0, p);
}

DWORD WINAPI thread::ThreadProc(LPVOID p)
{
	thread* Thread = (thread*)p;
	Thread->Run();

	CloseHandle(Thread->m_hThread);
	Thread->m_hThread = INVALID_HANDLE_VALUE;

	return 0;
}

void thread::Stop()
{
	m_stopFlag = true;

	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(m_hThread, INFINITE) != WAIT_ABANDONED)
		{
			CloseHandle(m_hThread);
		}
		m_hThread = INVALID_HANDLE_VALUE;
	}
}


bool thread::IsStop()
{
	return m_stopFlag;
}