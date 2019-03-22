#ifndef _TIMER_H_
#define _TIMER_H_

#include <Windows.h>
#include "thread.h"

class Timer : public thread
{
	typedef void(CALLBACK *Timerfunc)(void* p);
	typedef Timerfunc TimerHandler;
public:
	Timer()
		:m_handler(0)
		, m_interval(-1)
	{
	}

	void registerHandler(TimerHandler handler, void* p)
	{
		m_handler = handler;
		m_parameter = p;
	}

	void setInterval(int millisecond)
	{
		m_interval = millisecond;
	}

	void Run()
	{
		unsigned long tickNow = ::GetTickCount();
		unsigned long tickLastTime = tickNow;

		while (!IsStop())
		{
			tickNow = ::GetTickCount();//返回从操作系统启动到当前所经过的毫秒数
			if (tickNow - tickLastTime > m_interval)
			{
				if (m_handler)
				{
					(*m_handler)(m_parameter);
				}
				tickLastTime = ::GetTickCount();
			}
		}
	}

	void Cancel()
	{
		Stop();
	}

private:
	TimerHandler m_handler;
	int             m_interval;
	void*         m_parameter;
};

#endif