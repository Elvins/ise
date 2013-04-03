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
// class CThreadImpl

CThreadImpl::CThreadImpl(CThread *pThread) :
	m_Thread(*pThread),
	m_nThreadId(0),
	m_bExecuting(false),
	m_bRunCalled(false),
	m_nTermTime(0),
	m_bFreeOnTerminate(false),
	m_bTerminated(false),
	m_bSleepInterrupted(false),
	m_nReturnValue(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

void CThreadImpl::Execute()
{
	m_Thread.Execute();
}

//-----------------------------------------------------------------------------

void CThreadImpl::BeforeTerminate()
{
	m_Thread.BeforeTerminate();
}

//-----------------------------------------------------------------------------

void CThreadImpl::BeforeKill()
{
	m_Thread.BeforeKill();
}

//-----------------------------------------------------------------------------
// ����: ����߳������У����׳��쳣
//-----------------------------------------------------------------------------
void CThreadImpl::CheckNotRunning()
{
	if (m_bRunCalled)
		IseThrowThreadException(SEM_THREAD_RUN_ONCE);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void CThreadImpl::Terminate()
{
	if (!m_bTerminated)
	{
		BeforeTerminate();
		m_nTermTime = (int)time(NULL);
		m_bTerminated = true;
	}
}

//-----------------------------------------------------------------------------
// ����: ȡ�ôӵ��� Terminate ����ǰ����������ʱ��(��)
//-----------------------------------------------------------------------------
int CThreadImpl::GetTermElapsedSecs() const
{
	int nResult = 0;

	// ����Ѿ�֪ͨ�˳������̻߳�����
	if (m_bTerminated && m_nThreadId != 0)
	{
		nResult = (int)(time(NULL) - m_nTermTime);
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ� Terminate
//-----------------------------------------------------------------------------
void CThreadImpl::SetTerminated(bool bValue)
{
	if (bValue != m_bTerminated)
	{
		if (bValue)
			Terminate();
		else
		{
			m_bTerminated = false;
			m_nTermTime = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ����˯��״̬ (˯�߹����л��� m_bTerminated ��״̬)
// ����:
//   fSeconds - ˯�ߵ���������ΪС�����ɾ�ȷ������
// ע��:
//   ���ڽ�˯��ʱ��ֳ������ɷݣ�ÿ��˯��ʱ���С����ۼ���������������
//-----------------------------------------------------------------------------
void CThreadImpl::Sleep(double fSeconds)
{
	const double SLEEP_INTERVAL = 0.5;      // ÿ��˯�ߵ�ʱ��(��)
	double fOnceSecs;

	m_bSleepInterrupted = false;

	while (!GetTerminated() && fSeconds > 0 && !m_bSleepInterrupted)
	{
		fOnceSecs = (fSeconds >= SLEEP_INTERVAL ? SLEEP_INTERVAL : fSeconds);
		fSeconds -= fOnceSecs;

		SleepSec(fOnceSecs, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CWin32ThreadImpl

#ifdef ISE_WIN32

//-----------------------------------------------------------------------------
// ����: �߳�ִ�к���
// ����:
//   pParam - �̲߳������˴�ָ�� CWin32ThreadImpl ����
//-----------------------------------------------------------------------------
UINT __stdcall ThreadExecProc(void *pParam)
{
	CWin32ThreadImpl *pThreadImpl = (CWin32ThreadImpl*)pParam;
	int nReturnValue = 0;

	{
		pThreadImpl->SetExecuting(true);

		// ���� AutoFinalizer �����Զ����ƺ���
		struct CAutoFinalizer
		{
			CWin32ThreadImpl *m_pThreadImpl;
			CAutoFinalizer(CWin32ThreadImpl *pThreadImpl) { m_pThreadImpl = pThreadImpl; }
			~CAutoFinalizer()
			{
				m_pThreadImpl->SetExecuting(false);
				if (m_pThreadImpl->GetFreeOnTerminate())
					delete m_pThreadImpl->GetThread();
			}
		} AutoFinalizer(pThreadImpl);

		if (!pThreadImpl->m_bTerminated)
		{
			try { pThreadImpl->Execute(); } catch (CException&) {}

			// �����̷߳���ֵ
			nReturnValue = pThreadImpl->m_nReturnValue;
		}
	}

	// ע��: ����� _endthreadex �� ExitThread ������
	_endthreadex(nReturnValue);
	//ExitThread(nReturnValue);

	return nReturnValue;
}

//-----------------------------------------------------------------------------
// ����: ���캯��
//-----------------------------------------------------------------------------
CWin32ThreadImpl::CWin32ThreadImpl(CThread *pThread) :
	CThreadImpl(pThread),
	m_nHandle(0),
	m_nPriority(THREAD_PRI_NORMAL)
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
CWin32ThreadImpl::~CWin32ThreadImpl()
{
	if (m_nThreadId != 0)
	{
		if (m_bExecuting)
			Terminate();
		if (!m_bFreeOnTerminate)
			WaitFor();
	}

	if (m_nThreadId != 0)
	{
		CloseHandle(m_nHandle);
	}
}

//-----------------------------------------------------------------------------
// ����: �̴߳�����
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::CheckThreadError(bool bSuccess)
{
	if (!bSuccess)
	{
		string strErrMsg = SysErrorMessage(GetLastError());
		Logger().WriteStr(strErrMsg.c_str());
		IseThrowThreadException(strErrMsg.c_str());
	}
}

//-----------------------------------------------------------------------------
// ����: �����̲߳�ִ��
// ע��: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::Run()
{
	CheckNotRunning();
	m_bRunCalled = true;

	// ע��: ����� CRT::_beginthreadex �� API::CreateThread ������ǰ�߼�����CRT��
	m_nHandle = (HANDLE)_beginthreadex(NULL, 0, ThreadExecProc, (LPVOID)this,
		CREATE_SUSPENDED, (UINT*)&m_nThreadId);
	//m_nHandle = CreateThread(NULL, 0, ThreadExecProc, (LPVOID)this, CREATE_SUSPENDED, (LPDWORD)&m_nThreadId);

	CheckThreadError(m_nHandle != 0);

	// �����߳����ȼ�
	if (m_nPriority != THREAD_PRI_NORMAL)
		SetPriority(m_nPriority);

	::ResumeThread(m_nHandle);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::Terminate()
{
	CThreadImpl::Terminate();
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���߳�
// ע��:
//   1. ���ô˺����󣬶��߳�������һ�в����Բ�����(Terminate(); WaitFor(); delete pThread; ��)��
//   2. �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ (��δ���ü�����
//      �㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� BeforeKill �н��С�
//   3. Win32 ��ǿɱ�̣߳��߳�ִ�й����е�ջ���󲻻�������
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::Kill()
{
	if (m_nThreadId != 0)
	{
		BeforeKill();

		m_nThreadId = 0;
		TerminateThread(m_nHandle, 0);
		::CloseHandle(m_nHandle);
	}

	delete (CThread*)&m_Thread;
}

//-----------------------------------------------------------------------------
// ����: �ȴ��߳��˳�
// ����: �̷߳���ֵ
//-----------------------------------------------------------------------------
int CWin32ThreadImpl::WaitFor()
{
	ISE_ASSERT(m_bFreeOnTerminate == false);

	if (m_nThreadId != 0)
	{
		WaitForSingleObject(m_nHandle, INFINITE);
		GetExitCodeThread(m_nHandle, (LPDWORD)&m_nReturnValue);
	}

	return m_nReturnValue;
}

//-----------------------------------------------------------------------------
// ����: �����̵߳����ȼ�
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::SetPriority(int nValue)
{
	int nPriorities[7] = {
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_TIME_CRITICAL
	};

	nValue = (int)EnsureRange((int)nValue, 0, 6);
	m_nPriority = nValue;
	if (m_nThreadId != 0)
		SetThreadPriority(m_nHandle, nPriorities[nValue]);
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class CLinuxThreadImpl

#ifdef ISE_LINUX

//-----------------------------------------------------------------------------
// ����: �߳�������
// ����:
//   pParam - �̲߳������˴�ָ�� CLinuxThreadImpl ����
//-----------------------------------------------------------------------------
void ThreadFinalProc(void *pParam)
{
	CLinuxThreadImpl *pThreadImpl = (CLinuxThreadImpl*)pParam;

	pThreadImpl->SetExecuting(false);
	if (pThreadImpl->GetFreeOnTerminate())
		delete pThreadImpl->GetThread();
}

//-----------------------------------------------------------------------------
// ����: �߳�ִ�к���
// ����:
//   pParam - �̲߳������˴�ָ�� CLinuxThreadImpl ����
//-----------------------------------------------------------------------------
void* ThreadExecProc(void *pParam)
{
	CLinuxThreadImpl *pThreadImpl = (CLinuxThreadImpl*)pParam;
	int nReturnValue = 0;

	{
		// �ȴ��̶߳���׼������
		pThreadImpl->m_pExecSem->Wait();
		delete pThreadImpl->m_pExecSem;
		pThreadImpl->m_pExecSem = NULL;

		pThreadImpl->SetExecuting(true);

		// �̶߳� cancel �źŵ���Ӧ��ʽ������: (1)����Ӧ (2)�Ƴٵ�ȡ��������Ӧ (3)����������Ӧ��
		// �˴������߳�Ϊ��(3)�ַ�ʽ���������ϱ� cancel �ź���ֹ��
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		// ע��������
		pthread_cleanup_push(ThreadFinalProc, pThreadImpl);

		if (!pThreadImpl->m_bTerminated)
		{
			// �߳���ִ�й��������յ� cancel �źţ����׳�һ���쳣(������ʲô���͵��쳣��)��
			// ���쳣ǧ�򲻿�ȥ������( try{}catch(...){} )��ϵͳ���������쳣�Ƿ񱻳���������
			// ���ǣ���ᷢ�� SIGABRT �źţ������ն���� "FATAL: exception not rethrown"��
			// ���Դ˴��Ĳ�����ֻ���� CException �쳣(��ISE�������쳣�Դ� CException �̳�)��
			// �� pThread->Execute() ��ִ�й����У��û�Ӧ��ע����������:
			// 1. ����մ˴�������ȥ�����쳣�����в��������������͵��쳣( ��catch(...) );
			// 2. �����׳� CException ��������֮����쳣�������׳�һ������( �� throw 5; )��
			//    ϵͳ����Ϊû�д��쳣�Ĵ����������� abort��(������ˣ�ThreadFinalProc
			//    �Ի��� pthread_cleanup_push ����ŵ��������ִ�е���)
			try { pThreadImpl->Execute(); } catch (CException& e) {}

			// �����̷߳���ֵ
			nReturnValue = pThreadImpl->m_nReturnValue;
		}

		pthread_cleanup_pop(1);

		// ���� cancel �źţ����˵�ǰ scope �󽫲��ɱ�ǿ����ֹ
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}

	pthread_exit((void*)nReturnValue);
	return NULL;
}

//-----------------------------------------------------------------------------
// ����: ���캯��
//-----------------------------------------------------------------------------
CLinuxThreadImpl::CLinuxThreadImpl(CThread *pThread) :
	CThreadImpl(pThread),
	m_nPolicy(THREAD_POL_DEFAULT),
	m_nPriority(THREAD_PRI_DEFAULT),
	m_pExecSem(NULL)
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
CLinuxThreadImpl::~CLinuxThreadImpl()
{
	delete m_pExecSem;
	m_pExecSem = NULL;

	if (m_nThreadId != 0)
	{
		if (m_bExecuting)
			Terminate();
		if (!m_bFreeOnTerminate)
			WaitFor();
	}

	if (m_nThreadId != 0)
	{
		pthread_detach(m_nThreadId);
	}
}

//-----------------------------------------------------------------------------
// ����: �̴߳�����
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::CheckThreadError(int nErrorCode)
{
	if (nErrorCode != 0)
	{
		string strErrMsg = SysErrorMessage(nErrorCode);
		Logger().WriteStr(strErrMsg.c_str());
		IseThrowThreadException(strErrMsg.c_str());
	}
}

//-----------------------------------------------------------------------------
// ����: �����̲߳�ִ��
// ע��: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::Run()
{
	CheckNotRunning();
	m_bRunCalled = true;

	delete m_pExecSem;
	m_pExecSem = new CSemaphore(0);

	// �����߳�
	CheckThreadError(pthread_create((pthread_t*)&m_nThreadId, NULL, ThreadExecProc, (void*)this));

	// �����̵߳��Ȳ���
	if (m_nPolicy != THREAD_POL_DEFAULT)
		SetPolicy(m_nPolicy);
	// �����߳����ȼ�
	if (m_nPriority != THREAD_PRI_DEFAULT)
		SetPriority(m_nPriority);

	// �̶߳�����׼���������̺߳������Կ�ʼ����
	m_pExecSem->Increase();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::Terminate()
{
	CThreadImpl::Terminate();
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���߳�
// ע��:
//   1. ���ô˺������̶߳��󼴱����٣����߳�������һ�в����Բ�����(Terminate();
//      WaitFor(); delete pThread; ��)��
//   2. ��ɱ���߳�ǰ��m_bFreeOnTerminate ���Զ���Ϊ true���Ա�������Զ��ͷš�
//   3. �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ (��δ���ü�����
//      �㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� BeforeKill �н��С�
//   4. pthread û�й涨���߳��յ� cancel �źź��Ƿ���� C++ stack unwinding��Ҳ����˵ջ����
//      ������������һ����ִ�С�ʵ��������� RedHat AS (glibc-2.3.4) �»���� stack unwinding��
//      ���� Debian 4.0 (glibc-2.3.6) ���򲻻ᡣ
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::Kill()
{
	if (m_nThreadId != 0)
	{
		BeforeKill();

		if (m_bExecuting)
		{
			SetFreeOnTerminate(true);
			pthread_cancel(m_nThreadId);
			return;
		}
	}

	delete this;
}

//-----------------------------------------------------------------------------
// ����: �ȴ��߳��˳�
// ����: �̷߳���ֵ
//-----------------------------------------------------------------------------
int CLinuxThreadImpl::WaitFor()
{
	ISE_ASSERT(m_bFreeOnTerminate == false);

	pthread_t nThreadId = m_nThreadId;

	if (m_nThreadId != 0)
	{
		m_nThreadId = 0;
		CheckThreadError(pthread_join(nThreadId, (void**)&m_nReturnValue));
	}

	return m_nReturnValue;
}

//-----------------------------------------------------------------------------
// ����: �����̵߳ĵ��Ȳ���
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::SetPolicy(int nValue)
{
	if (nValue != THREAD_POL_DEFAULT &&
		nValue != THREAD_POL_RR &&
		nValue != THREAD_POL_FIFO)
	{
		nValue = THREAD_POL_DEFAULT;
	}

	m_nPolicy = nValue;

	if (m_nThreadId != 0)
	{
		struct sched_param param;
		param.sched_priority = m_nPriority;
		pthread_setschedparam(m_nThreadId, m_nPolicy, &param);
	}
}

//-----------------------------------------------------------------------------
// ����: �����̵߳����ȼ�
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::SetPriority(int nValue)
{
	if (nValue < THREAD_PRI_MIN || nValue > THREAD_PRI_MAX)
		nValue = THREAD_PRI_DEFAULT;

	m_nPriority = nValue;

	if (m_nThreadId != 0)
	{
		struct sched_param param;
		param.sched_priority = m_nPriority;
		pthread_setschedparam(m_nThreadId, m_nPolicy, &param);
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class CThread

//-----------------------------------------------------------------------------
// ����: ����һ���̲߳�����ִ��
//-----------------------------------------------------------------------------
void CThread::Create(THREAD_EXEC_PROC pExecProc, void *pParam)
{
	CThread *pThread = new CThread();

	pThread->SetFreeOnTerminate(true);
	pThread->m_pExecProc = pExecProc;
	pThread->m_pThreadParam = pParam;

	pThread->Run();
}

///////////////////////////////////////////////////////////////////////////////
// class CThreadList

CThreadList::CThreadList() :
	m_Items(false, false)
{
	// nothing
}

CThreadList::~CThreadList()
{
	// nothing
}

//-----------------------------------------------------------------------------

void CThreadList::Add(CThread *pThread)
{
	CAutoLocker Locker(m_Lock);
	m_Items.Add(pThread, false);
}

//-----------------------------------------------------------------------------

void CThreadList::Remove(CThread *pThread)
{
	CAutoLocker Locker(m_Lock);
	m_Items.Remove(pThread);
}

//-----------------------------------------------------------------------------

bool CThreadList::Exists(CThread *pThread)
{
	CAutoLocker Locker(m_Lock);
	return m_Items.Exists(pThread);
}

//-----------------------------------------------------------------------------

void CThreadList::Clear()
{
	CAutoLocker Locker(m_Lock);
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�����߳��˳�
//-----------------------------------------------------------------------------
void CThreadList::TerminateAllThreads()
{
	CAutoLocker Locker(m_Lock);

	for (int i = 0; i < m_Items.GetCount(); i++)
	{
		CThread *pThread = m_Items[i];
		pThread->Terminate();
	}
}

//-----------------------------------------------------------------------------
// ����: �ȴ������߳��˳�
// ����:
//   nMaxWaitForSecs - ��ȴ�ʱ��(��) (Ϊ -1 ��ʾ���޵ȴ�)
//   pKilledCount    - ��������ǿɱ�˶��ٸ��߳�
// ע��:
//   �˺���Ҫ���б��и��߳����˳�ʱ�Զ������Լ������б����Ƴ���
//-----------------------------------------------------------------------------
void CThreadList::WaitForAllThreads(int nMaxWaitForSecs, int *pKilledCount)
{
	const double SLEEP_INTERVAL = 0.1;  // (��)
	double nWaitSecs = 0;
	int nKilledCount = 0;

	// ֪ͨ�����߳��˳�
	TerminateAllThreads();

	// �ȴ��߳��˳�
	while (nWaitSecs < (UINT)nMaxWaitForSecs)
	{
		if (m_Items.GetCount() == 0) break;
		SleepSec(SLEEP_INTERVAL, true);
		nWaitSecs += SLEEP_INTERVAL;
	}

	// ���ȴ���ʱ����ǿ��ɱ�����߳�
	if (m_Items.GetCount() > 0)
	{
		CAutoLocker Locker(m_Lock);

		nKilledCount = m_Items.GetCount();
		for (int i = 0; i < m_Items.GetCount(); i++)
			m_Items[i]->Kill();

		m_Items.Clear();
	}

	if (pKilledCount)
		*pKilledCount = nKilledCount;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
