/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// �ļ�����: ise_thread.cpp
// ��������: �߳���
///////////////////////////////////////////////////////////////////////////////

#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_classes.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class ThreadImpl

ThreadImpl::ThreadImpl(Thread *thread) :
	thread_(*thread),
	threadId_(0),
	isExecuting_(false),
	isRunCalled_(false),
	termTime_(0),
	isFreeOnTerminate_(false),
	terminated_(false),
	isSleepInterrupted_(false),
	returnValue_(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

void ThreadImpl::execute()
{
	thread_.execute();
}

//-----------------------------------------------------------------------------

void ThreadImpl::beforeTerminate()
{
	thread_.beforeTerminate();
}

//-----------------------------------------------------------------------------

void ThreadImpl::beforeKill()
{
	thread_.beforeKill();
}

//-----------------------------------------------------------------------------
// ����: ����߳������У����׳��쳣
//-----------------------------------------------------------------------------
void ThreadImpl::checkNotRunning()
{
	if (isRunCalled_)
		iseThrowThreadException(SEM_THREAD_RUN_ONCE);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void ThreadImpl::terminate()
{
	if (!terminated_)
	{
		beforeTerminate();
		termTime_ = (int)time(NULL);
		terminated_ = true;
	}
}

//-----------------------------------------------------------------------------
// ����: ȡ�ôӵ��� terminate ����ǰ����������ʱ��(��)
//-----------------------------------------------------------------------------
int ThreadImpl::getTermElapsedSecs() const
{
	int result = 0;

	// ����Ѿ�֪ͨ�˳������̻߳�����
	if (terminated_ && threadId_ != 0)
	{
		result = (int)(time(NULL) - termTime_);
	}

	return result;
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ� terminate
//-----------------------------------------------------------------------------
void ThreadImpl::setTerminated(bool value)
{
	if (value != terminated_)
	{
		if (value)
			terminate();
		else
		{
			terminated_ = false;
			termTime_ = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ����˯��״̬ (˯�߹����л��� terminated_ ��״̬)
// ����:
//   seconds - ˯�ߵ���������ΪС�����ɾ�ȷ������
// ע��:
//   ���ڽ�˯��ʱ��ֳ������ɷݣ�ÿ��˯��ʱ���С����ۼ���������������
//-----------------------------------------------------------------------------
void ThreadImpl::sleep(double seconds)
{
	const double SLEEP_INTERVAL = 0.5;      // ÿ��˯�ߵ�ʱ��(��)
	double onceSecs;

	isSleepInterrupted_ = false;

	while (!isTerminated() && seconds > 0 && !isSleepInterrupted_)
	{
		onceSecs = (seconds >= SLEEP_INTERVAL ? SLEEP_INTERVAL : seconds);
		seconds -= onceSecs;

		sleepSec(onceSecs, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class Win32ThreadImpl

#ifdef ISE_WIN32

//-----------------------------------------------------------------------------
// ����: �߳�ִ�к���
// ����:
//   param - �̲߳������˴�ָ�� Win32ThreadImpl ����
//-----------------------------------------------------------------------------
UINT __stdcall threadExecProc(void *param)
{
	Win32ThreadImpl *threadImpl = (Win32ThreadImpl*)param;
	int returnValue = 0;

	{
		threadImpl->setExecuting(true);

		// ���� autoFinalizer �����Զ����ƺ���
		struct AutoFinalizer
		{
			Win32ThreadImpl *threadImpl_;
			AutoFinalizer(Win32ThreadImpl *threadImpl) { threadImpl_ = threadImpl; }
			~AutoFinalizer()
			{
				threadImpl_->setExecuting(false);
				if (threadImpl_->isFreeOnTerminate())
					delete threadImpl_->getThread();
			}
		} autoFinalizer(threadImpl);

		if (!threadImpl->terminated_)
		{
			try { threadImpl->execute(); } catch (Exception&) {}

			// �����̷߳���ֵ
			returnValue = threadImpl->returnValue_;
		}
	}

	// ע��: ����� _endthreadex �� ExitThread ������
	_endthreadex(returnValue);
	//ExitThread(returnValue);

	return returnValue;
}

//-----------------------------------------------------------------------------
// ����: ���캯��
//-----------------------------------------------------------------------------
Win32ThreadImpl::Win32ThreadImpl(Thread *thread) :
	ThreadImpl(thread),
	handle_(0),
	priority_(THREAD_PRI_NORMAL)
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
Win32ThreadImpl::~Win32ThreadImpl()
{
	if (threadId_ != 0)
	{
		if (isExecuting_)
			terminate();
		if (!isFreeOnTerminate_)
			waitFor();
	}

	if (threadId_ != 0)
	{
		CloseHandle(handle_);
	}
}

//-----------------------------------------------------------------------------
// ����: �̴߳�����
//-----------------------------------------------------------------------------
void Win32ThreadImpl::checkThreadError(bool success)
{
	if (!success)
	{
		string errMsg = sysErrorMessage(GetLastError());
		logger().writeStr(errMsg.c_str());
		iseThrowThreadException(errMsg.c_str());
	}
}

//-----------------------------------------------------------------------------
// ����: �����̲߳�ִ��
// ע��: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
//-----------------------------------------------------------------------------
void Win32ThreadImpl::run()
{
	checkNotRunning();
	isRunCalled_ = true;

	// ע��: ����� CRT::_beginthreadex �� API::CreateThread ������ǰ�߼�����CRT��
	handle_ = (HANDLE)_beginthreadex(NULL, 0, threadExecProc, (LPVOID)this,
		CREATE_SUSPENDED, (UINT*)&threadId_);
	//handle_ = CreateThread(NULL, 0, threadExecProc, (LPVOID)this, CREATE_SUSPENDED, (LPDWORD)&threadId_);

	checkThreadError(handle_ != 0);

	// �����߳����ȼ�
	if (priority_ != THREAD_PRI_NORMAL)
		setPriority(priority_);

	::ResumeThread(handle_);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void Win32ThreadImpl::terminate()
{
	ThreadImpl::terminate();
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���߳�
// ע��:
//   1. ���ô˺����󣬶��߳�������һ�в����Բ�����(terminate(); waitFor(); delete thread; ��)��
//   2. �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ (��δ���ü�����
//      �㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� beforeKill �н��С�
//   3. Win32 ��ǿɱ�̣߳��߳�ִ�й����е�ջ���󲻻�������
//-----------------------------------------------------------------------------
void Win32ThreadImpl::kill()
{
	if (threadId_ != 0)
	{
		beforeKill();

		threadId_ = 0;
		TerminateThread(handle_, 0);
		::CloseHandle(handle_);
	}

	delete (Thread*)&thread_;
}

//-----------------------------------------------------------------------------
// ����: �ȴ��߳��˳�
// ����: �̷߳���ֵ
//-----------------------------------------------------------------------------
int Win32ThreadImpl::waitFor()
{
	ISE_ASSERT(isFreeOnTerminate_ == false);

	if (threadId_ != 0)
	{
		WaitForSingleObject(handle_, INFINITE);
		GetExitCodeThread(handle_, (LPDWORD)&returnValue_);
	}

	return returnValue_;
}

//-----------------------------------------------------------------------------
// ����: �����̵߳����ȼ�
//-----------------------------------------------------------------------------
void Win32ThreadImpl::setPriority(int value)
{
	int priorities[7] = {
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_TIME_CRITICAL
	};

	value = (int)ensureRange((int)value, 0, 6);
	priority_ = value;
	if (threadId_ != 0)
		SetThreadPriority(handle_, priorities[value]);
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class LinuxThreadImpl

#ifdef ISE_LINUX

//-----------------------------------------------------------------------------
// ����: �߳�������
// ����:
//   param - �̲߳������˴�ָ�� LinuxThreadImpl ����
//-----------------------------------------------------------------------------
void threadFinalProc(void *param)
{
	LinuxThreadImpl *threadImpl = (LinuxThreadImpl*)param;

	threadImpl->setExecuting(false);
	if (threadImpl->isFreeOnTerminate())
		delete threadImpl->getThread();
}

//-----------------------------------------------------------------------------
// ����: �߳�ִ�к���
// ����:
//   param - �̲߳������˴�ָ�� LinuxThreadImpl ����
//-----------------------------------------------------------------------------
void* threadExecProc(void *param)
{
	LinuxThreadImpl *threadImpl = (LinuxThreadImpl*)param;
	int returnValue = 0;

	{
		// �ȴ��̶߳���׼������
		threadImpl->execSem_->wait();
		delete threadImpl->execSem_;
		threadImpl->execSem_ = NULL;

		threadImpl->setExecuting(true);

		// �̶߳� cancel �źŵ���Ӧ��ʽ������: (1)����Ӧ (2)�Ƴٵ�ȡ��������Ӧ (3)����������Ӧ��
		// �˴������߳�Ϊ��(3)�ַ�ʽ���������ϱ� cancel �ź���ֹ��
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		// ע��������
		pthread_cleanup_push(threadFinalProc, threadImpl);

		if (!threadImpl->terminated_)
		{
			// �߳���ִ�й��������յ� cancel �źţ����׳�һ���쳣(������ʲô���͵��쳣��)��
			// ���쳣ǧ�򲻿�ȥ������( try{}catch(...){} )��ϵͳ���������쳣�Ƿ񱻳���������
			// ���ǣ���ᷢ�� SIGABRT �źţ������ն���� "FATAL: exception not rethrown"��
			// ���Դ˴��Ĳ�����ֻ���� Exception �쳣(��ISE�������쳣�Դ� Exception �̳�)��
			// �� thread->execute() ��ִ�й����У��û�Ӧ��ע����������:
			// 1. ����մ˴�������ȥ�����쳣�����в��������������͵��쳣( ��catch(...) );
			// 2. �����׳� Exception ��������֮����쳣�������׳�һ������( �� throw 5; )��
			//    ϵͳ����Ϊû�д��쳣�Ĵ����������� abort��(������ˣ�threadFinalProc
			//    �Ի��� pthread_cleanup_push ����ŵ��������ִ�е���)
			try { threadImpl->execute(); } catch (Exception& e) {}

			// �����̷߳���ֵ
			returnValue = threadImpl->returnValue_;
		}

		pthread_cleanup_pop(1);

		// ���� cancel �źţ����˵�ǰ scope �󽫲��ɱ�ǿ����ֹ
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}

	pthread_exit((void*)returnValue);
	return NULL;
}

//-----------------------------------------------------------------------------
// ����: ���캯��
//-----------------------------------------------------------------------------
LinuxThreadImpl::LinuxThreadImpl(Thread *thread) :
	ThreadImpl(thread),
	policy_(THREAD_POL_DEFAULT),
	priority_(THREAD_PRI_DEFAULT),
	execSem_(NULL)
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
LinuxThreadImpl::~LinuxThreadImpl()
{
	delete execSem_;
	execSem_ = NULL;

	if (threadId_ != 0)
	{
		if (isExecuting_)
			terminate();
		if (!isFreeOnTerminate_)
			waitFor();
	}

	if (threadId_ != 0)
	{
		pthread_detach(threadId_);
	}
}

//-----------------------------------------------------------------------------
// ����: �̴߳�����
//-----------------------------------------------------------------------------
void LinuxThreadImpl::checkThreadError(int errorCode)
{
	if (errorCode != 0)
	{
		string errMsg = sysErrorMessage(errorCode);
		logger().writeStr(errMsg.c_str());
		iseThrowThreadException(errMsg.c_str());
	}
}

//-----------------------------------------------------------------------------
// ����: �����̲߳�ִ��
// ע��: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
//-----------------------------------------------------------------------------
void LinuxThreadImpl::run()
{
	checkNotRunning();
	isRunCalled_ = true;

	delete execSem_;
	execSem_ = new Semaphore(0);

	// �����߳�
	checkThreadError(pthread_create((pthread_t*)&threadId_, NULL, threadExecProc, (void*)this));

	// �����̵߳��Ȳ���
	if (policy_ != THREAD_POL_DEFAULT)
		setPolicy(policy_);
	// �����߳����ȼ�
	if (priority_ != THREAD_PRI_DEFAULT)
		setPriority(priority_);

	// �̶߳�����׼���������̺߳������Կ�ʼ����
	execSem_->increase();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void LinuxThreadImpl::terminate()
{
	ThreadImpl::terminate();
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���߳�
// ע��:
//   1. ���ô˺������̶߳��󼴱����٣����߳�������һ�в����Բ�����(terminate();
//      waitFor(); delete thread; ��)��
//   2. ��ɱ���߳�ǰ��isFreeOnTerminate_ ���Զ���Ϊ true���Ա�������Զ��ͷš�
//   3. �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ (��δ���ü�����
//      �㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� beforeKill �н��С�
//   4. pthread û�й涨���߳��յ� cancel �źź��Ƿ���� C++ stack unwinding��Ҳ����˵ջ����
//      ������������һ����ִ�С�ʵ��������� RedHat AS (glibc-2.3.4) �»���� stack unwinding��
//      ���� Debian 4.0 (glibc-2.3.6) ���򲻻ᡣ
//-----------------------------------------------------------------------------
void LinuxThreadImpl::kill()
{
	if (threadId_ != 0)
	{
		beforeKill();

		if (isExecuting_)
		{
			setFreeOnTerminate(true);
			pthread_cancel(threadId_);
			return;
		}
	}

	delete this;
}

//-----------------------------------------------------------------------------
// ����: �ȴ��߳��˳�
// ����: �̷߳���ֵ
//-----------------------------------------------------------------------------
int LinuxThreadImpl::waitFor()
{
	ISE_ASSERT(isFreeOnTerminate_ == false);

	pthread_t threadId = threadId_;

	if (threadId_ != 0)
	{
		threadId_ = 0;
		checkThreadError(pthread_join(threadId, (void**)&returnValue_));
	}

	return returnValue_;
}

//-----------------------------------------------------------------------------
// ����: �����̵߳ĵ��Ȳ���
//-----------------------------------------------------------------------------
void LinuxThreadImpl::setPolicy(int value)
{
	if (value != THREAD_POL_DEFAULT &&
		value != THREAD_POL_RR &&
		value != THREAD_POL_FIFO)
	{
		value = THREAD_POL_DEFAULT;
	}

	policy_ = value;

	if (threadId_ != 0)
	{
		struct sched_param param;
		param.sched_priority = priority_;
		pthread_setschedparam(threadId_, policy_, &param);
	}
}

//-----------------------------------------------------------------------------
// ����: �����̵߳����ȼ�
//-----------------------------------------------------------------------------
void LinuxThreadImpl::setPriority(int value)
{
	if (value < THREAD_PRI_MIN || value > THREAD_PRI_MAX)
		value = THREAD_PRI_DEFAULT;

	priority_ = value;

	if (threadId_ != 0)
	{
		struct sched_param param;
		param.sched_priority = priority_;
		pthread_setschedparam(threadId_, policy_, &param);
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class Thread

//-----------------------------------------------------------------------------
// ����: ����һ���̲߳�����ִ��
//-----------------------------------------------------------------------------
void Thread::create(THREAD_EXEC_PROC execProc, void *param)
{
	Thread *thread = new Thread();

	thread->setFreeOnTerminate(true);
	thread->execProc_ = execProc;
	thread->threadParam_ = param;

	thread->run();
}

///////////////////////////////////////////////////////////////////////////////
// class ThreadList

ThreadList::ThreadList() :
	items_(false, false)
{
	// nothing
}

ThreadList::~ThreadList()
{
	// nothing
}

//-----------------------------------------------------------------------------

void ThreadList::add(Thread *thread)
{
	AutoLocker locker(lock_);
	items_.add(thread, false);
}

//-----------------------------------------------------------------------------

void ThreadList::remove(Thread *thread)
{
	AutoLocker locker(lock_);
	items_.remove(thread);
}

//-----------------------------------------------------------------------------

bool ThreadList::exists(Thread *thread)
{
	AutoLocker locker(lock_);
	return items_.exists(thread);
}

//-----------------------------------------------------------------------------

void ThreadList::clear()
{
	AutoLocker locker(lock_);
	items_.clear();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�����߳��˳�
//-----------------------------------------------------------------------------
void ThreadList::terminateAllThreads()
{
	AutoLocker locker(lock_);

	for (int i = 0; i < items_.getCount(); i++)
	{
		Thread *thread = items_[i];
		thread->terminate();
	}
}

//-----------------------------------------------------------------------------
// ����: �ȴ������߳��˳�
// ����:
//   maxWaitForSecs - ��ȴ�ʱ��(��) (Ϊ -1 ��ʾ���޵ȴ�)
//   killedCountPtr  - ��������ǿɱ�˶��ٸ��߳�
// ע��:
//   �˺���Ҫ���б��и��߳����˳�ʱ�Զ������Լ������б����Ƴ���
//-----------------------------------------------------------------------------
void ThreadList::waitForAllThreads(int maxWaitForSecs, int *killedCountPtr)
{
	const double SLEEP_INTERVAL = 0.1;  // (��)
	double waitSecs = 0;
	int killedCount = 0;

	// ֪ͨ�����߳��˳�
	terminateAllThreads();

	// �ȴ��߳��˳�
	while (waitSecs < (UINT)maxWaitForSecs)
	{
		if (items_.getCount() == 0) break;
		sleepSec(SLEEP_INTERVAL, true);
		waitSecs += SLEEP_INTERVAL;
	}

	// ���ȴ���ʱ����ǿ��ɱ�����߳�
	if (items_.getCount() > 0)
	{
		AutoLocker locker(lock_);

		killedCount = items_.getCount();
		for (int i = 0; i < items_.getCount(); i++)
			items_[i]->kill();

		items_.clear();
	}

	if (killedCountPtr)
		*killedCountPtr = killedCount;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
