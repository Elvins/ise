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
// ise_thread.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_THREAD_H_
#define _ISE_THREAD_H_

#include "ise_options.h"

#ifdef ISE_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#endif

#include "ise_global_defs.h"
#include "ise_errmsgs.h"
#include "ise_classes.h"

using namespace std;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
/* ˵��

һ��Win32ƽ̨�º�Linuxƽ̨���̵߳���Ҫ����:

	1. Win32�߳�ӵ��Handle��ThreadId����Linux�߳�ֻ��ThreadId��
	2. Win32�߳�ֻ��ThreadPriority����Linux�߳���ThreadPolicy��ThreadPriority��

*/
///////////////////////////////////////////////////////////////////////////////

class CThread;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

#ifdef ISE_WIN32
// �߳����ȼ�
enum
{
	THREAD_PRI_IDLE         = 0,
	THREAD_PRI_LOWEST       = 1,
	THREAD_PRI_NORMAL       = 2,
	THREAD_PRI_HIGHER       = 3,
	THREAD_PRI_HIGHEST      = 4,
	THREAD_PRI_TIMECRITICAL = 5
};
#endif

#ifdef ISE_LINUX
// �̵߳��Ȳ���
enum
{
	THREAD_POL_DEFAULT      = SCHED_OTHER,
	THREAD_POL_RR           = SCHED_RR,
	THREAD_POL_FIFO         = SCHED_FIFO
};

// �߳����ȼ�
enum
{
	THREAD_PRI_DEFAULT      = 0,
	THREAD_PRI_MIN          = 0,
	THREAD_PRI_MAX          = 99,
	THREAD_PRI_HIGH         = 80
};
#endif

#ifdef ISE_WIN32
typedef DWORD THREAD_ID;
#endif
#ifdef ISE_LINUX
typedef pthread_t THREAD_ID;
#endif

///////////////////////////////////////////////////////////////////////////////
// class CThreadImpl - ƽ̨�߳�ʵ�ֻ���

class CThreadImpl
{
public:
	friend class CThread;
protected:
	CThread& m_Thread;              // ������� CThread ����
	THREAD_ID m_nThreadId;          // �߳�ID
	bool m_bExecuting;              // �߳��Ƿ�����ִ���̺߳���
	bool m_bRunCalled;              // Run() �����Ƿ��ѱ����ù�
	int m_nTermTime;                // ���� Terminate() ʱ��ʱ���
	bool m_bFreeOnTerminate;        // �߳��˳�ʱ�Ƿ�ͬʱ�ͷ������
	bool m_bTerminated;             // �Ƿ�Ӧ�˳��ı�־
	bool m_bSleepInterrupted;       // ˯���Ƿ��ж�
	int m_nReturnValue;             // �̷߳���ֵ (���� Execute �������޸Ĵ�ֵ������ WaitFor ���ش�ֵ)

protected:
	void Execute();
	void BeforeTerminate();
	void BeforeKill();

	void CheckNotRunning();
public:
	CThreadImpl(CThread *pThread);
	virtual ~CThreadImpl() {}

	virtual void Run() = 0;
	virtual void Terminate();
	virtual void Kill() = 0;
	virtual int WaitFor() = 0;

	void Sleep(double fSeconds);
	void InterruptSleep() { m_bSleepInterrupted = true; }
	bool IsRunning() { return m_bExecuting; }

	// ���� (getter)
	CThread* GetThread() { return (CThread*)&m_Thread; }
	THREAD_ID GetThreadId() const { return m_nThreadId; }
	int GetTerminated() const { return m_bTerminated; }
	int GetReturnValue() const { return m_nReturnValue; }
	bool GetFreeOnTerminate() const { return m_bFreeOnTerminate; }
	int GetTermElapsedSecs() const;
	// ���� (setter)
	void SetThreadId(THREAD_ID nValue) { m_nThreadId = nValue; }
	void SetExecuting(bool bValue) { m_bExecuting = bValue; }
	void SetTerminated(bool bValue);
	void SetReturnValue(int nValue) { m_nReturnValue = nValue; }
	void SetFreeOnTerminate(bool bValue) { m_bFreeOnTerminate = bValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CWin32ThreadImpl - Win32ƽ̨�߳�ʵ����

#ifdef ISE_WIN32
class CWin32ThreadImpl : public CThreadImpl
{
public:
	friend UINT __stdcall ThreadExecProc(void *pParam);

protected:
	HANDLE m_nHandle;               // �߳̾��
	int m_nPriority;                // �߳����ȼ�

private:
	void CheckThreadError(bool bSuccess);

public:
	CWin32ThreadImpl(CThread *pThread);
	virtual ~CWin32ThreadImpl();

	virtual void Run();
	virtual void Terminate();
	virtual void Kill();
	virtual int WaitFor();

	int GetPriority() const { return m_nPriority; }
	void SetPriority(int nValue);
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class CLinuxThreadImpl - Linuxƽ̨�߳�ʵ����

#ifdef ISE_LINUX
class CLinuxThreadImpl : public CThreadImpl
{
public:
	friend void ThreadFinalProc(void *pParam);
	friend void* ThreadExecProc(void *pParam);

protected:
	int m_nPolicy;                  // �̵߳��Ȳ��� (THREAD_POLICY_XXX)
	int m_nPriority;                // �߳����ȼ� (0..99)
	CSemaphore *m_pExecSem;         // ���������̺߳���ʱ��ʱ����

private:
	void CheckThreadError(int nErrorCode);

public:
	CLinuxThreadImpl(CThread *pThread);
	virtual ~CLinuxThreadImpl();

	virtual void Run();
	virtual void Terminate();
	virtual void Kill();
	virtual int WaitFor();

	int GetPolicy() const { return m_nPolicy; }
	int GetPriority() const { return m_nPriority; }
	void SetPolicy(int nValue);
	void SetPriority(int nValue);
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class CThread - �߳���

typedef void (*THREAD_EXEC_PROC)(void *pParam);

class CThread
{
public:
	friend class CThreadImpl;

private:
#ifdef ISE_WIN32
	CWin32ThreadImpl m_ThreadImpl;
#endif
#ifdef ISE_LINUX
	CLinuxThreadImpl m_ThreadImpl;
#endif

	THREAD_EXEC_PROC m_pExecProc;
	void *m_pThreadParam;

protected:
	// �̵߳�ִ�к��������������д��
	virtual void Execute() { if (m_pExecProc != NULL) (*m_pExecProc)(m_pThreadParam); }

	// ִ�� Terminate() ǰ�ĸ��Ӳ�����
	// ע: ���� Terminate() ������Ը�˳����ƣ�Ϊ�������߳��ܾ����˳�������
	// m_bTerminated ��־����Ϊ true ֮�⣬��ʱ��Ӧ������һЩ���ӵĲ�����
	// �������߳̾�������������н��ѳ�����
	virtual void BeforeTerminate() {}

	// ִ�� Kill() ǰ�ĸ��Ӳ�����
	// ע: �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ
	// (��δ���ü������㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� BeforeKill �н��С�
	virtual void BeforeKill() {}
public:
	CThread() : m_ThreadImpl(this), m_pExecProc(NULL), m_pThreadParam(NULL) {}
	virtual ~CThread() {}

	// ����һ���̲߳�����ִ��
	static void Create(THREAD_EXEC_PROC pExecProc, void *pParam = NULL);

	// ������ִ���̡߳�
	// ע: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
	void Run() { m_ThreadImpl.Run(); }

	// ֪ͨ�߳��˳� (��Ը�˳�����)
	// ע: ���߳�����ĳЩ����ʽ�����ٳٲ��˳����ɵ��� Kill() ǿ���˳���
	void Terminate() { m_ThreadImpl.Terminate(); }

	// ǿ��ɱ���߳� (ǿ���˳�����)
	void Kill() { m_ThreadImpl.Kill(); }

	// �ȴ��߳��˳�
	int WaitFor() { return m_ThreadImpl.WaitFor(); }

	// ����˯��״̬ (˯�߹����л��� m_bTerminated ��״̬)
	// ע: �˺����������߳��Լ����÷�����Ч��
	void Sleep(double fSeconds) { m_ThreadImpl.Sleep(fSeconds); }
	// ���˯��
	void InterruptSleep() { m_ThreadImpl.InterruptSleep(); }

	// �ж��߳��Ƿ���������
	bool IsRunning() { return m_ThreadImpl.IsRunning(); }

	// ���� (getter)
	THREAD_ID GetThreadId() const { return m_ThreadImpl.GetThreadId(); }
	int GetTerminated() const { return m_ThreadImpl.GetTerminated(); }
	int GetReturnValue() const { return m_ThreadImpl.GetReturnValue(); }
	bool GetFreeOnTerminate() const { return m_ThreadImpl.GetFreeOnTerminate(); }
	int GetTermElapsedSecs() const { return m_ThreadImpl.GetTermElapsedSecs(); }
#ifdef ISE_WIN32
	int GetPriority() const { return m_ThreadImpl.GetPriority(); }
#endif
#ifdef ISE_LINUX
	int GetPolicy() const { return m_ThreadImpl.GetPolicy(); }
	int GetPriority() const { return m_ThreadImpl.GetPriority(); }
#endif
	// ���� (setter)
	void SetTerminated(bool bValue) { m_ThreadImpl.SetTerminated(bValue); }
	void SetReturnValue(int nValue) { m_ThreadImpl.SetReturnValue(nValue); }
	void SetFreeOnTerminate(bool bValue) { m_ThreadImpl.SetFreeOnTerminate(bValue); }
#ifdef ISE_WIN32
	void SetPriority(int nValue) { m_ThreadImpl.SetPriority(nValue); }
#endif
#ifdef ISE_LINUX
	void SetPolicy(int nValue) { m_ThreadImpl.SetPolicy(nValue); }
	void SetPriority(int nValue) { m_ThreadImpl.SetPriority(nValue); }
#endif
};

///////////////////////////////////////////////////////////////////////////////
// class CThreadList - �߳��б���

class CThreadList
{
protected:
	CObjectList<CThread> m_Items;
	mutable CCriticalSection m_Lock;
public:
	CThreadList();
	virtual ~CThreadList();

	void Add(CThread *pThread);
	void Remove(CThread *pThread);
	bool Exists(CThread *pThread);
	void Clear();

	void TerminateAllThreads();
	void WaitForAllThreads(int nMaxWaitForSecs = 5, int *pKilledCount = NULL);

	int GetCount() const { return m_Items.GetCount(); }
	CThread* GetItem(int nIndex) const { return m_Items[nIndex]; }
	CThread* operator[] (int nIndex) const { return GetItem(nIndex); }

	CCriticalSection& GetLock() const { return m_Lock; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_THREAD_H_
