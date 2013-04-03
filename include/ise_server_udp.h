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
// ise_server_udp.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SERVER_UDP_H_
#define _ISE_SERVER_UDP_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class CThreadTimeOutChecker;
class CUdpPacket;
class CUdpRequestQueue;
class CUdpWorkerThread;
class CUdpWorkerThreadPool;
class CUdpRequestGroup;
class CMainUdpServer;

///////////////////////////////////////////////////////////////////////////////
// class CThreadTimeOutChecker - �̳߳�ʱ�����
//
// ˵��:
// ����������� CUdpWorkerThread/CTcpWorkerThread�����й������̵߳Ĺ���ʱ�䳬ʱ��⡣
// ���������߳��յ�һ����������Ͻ��빤��״̬��һ����ԣ��������߳�Ϊ�����������������
// ʱ�䲻��̫������̫����ᵼ�·��������й������̶߳�ȱ��ʹ��Ӧ����������������½�������
// ����UDP������˵���������ˡ�ͨ������£��̹߳�����ʱ����������Ϊ��������̺��߼�����
// �������ⲿԭ�򣬱������ݿⷱæ����Դ����������ӵ�µȵȡ����̹߳�����ʱ��Ӧ֪ͨ���˳���
// ����֪ͨ�˳�������ʱ������δ�˳�����ǿ��ɱ�����������̵߳�����������ʱ�����µ��̡߳�

class CThreadTimeOutChecker : public CAutoInvokable
{
private:
	CThread *m_pThread;         // �������߳�
	time_t m_tStartTime;        // ��ʼ��ʱʱ��ʱ���
	bool m_bStarted;            // �Ƿ��ѿ�ʼ��ʱ
	UINT m_nTimeOutSecs;        // ������������Ϊ��ʱ (Ϊ0��ʾ�����г�ʱ���)
	CCriticalSection m_Lock;

private:
	void Start();
	void Stop();

protected:
	virtual void InvokeInitialize() { Start(); }
	virtual void InvokeFinalize() { Stop(); }

public:
	explicit CThreadTimeOutChecker(CThread *pThread);
	virtual ~CThreadTimeOutChecker() {}

	// ����߳��Ƿ��ѳ�ʱ������ʱ��֪ͨ���˳�
	bool Check();

	// ���ó�ʱʱ�䣬��Ϊ0���ʾ�����г�ʱ���
	void SetTimeOutSecs(UINT nValue) { m_nTimeOutSecs = nValue; }
	// �����Ƿ��ѿ�ʼ��ʱ
	bool GetStarted();
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpPacket - UDP���ݰ���

class CUdpPacket
{
private:
	void *m_pPacketBuffer;

public:
	UINT m_nRecvTimeStamp;
	CPeerAddress m_PeerAddr;
	int m_nPacketSize;

public:
	CUdpPacket() :
		m_pPacketBuffer(NULL),
		m_nRecvTimeStamp(0),
		m_PeerAddr(0, 0),
		m_nPacketSize(0)
	{}
	virtual ~CUdpPacket()
	{ if (m_pPacketBuffer) free(m_pPacketBuffer); }

	void SetPacketBuffer(void *pPakcetBuffer, int nPacketSize);
	inline void* GetPacketBuffer() const { return m_pPacketBuffer; }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestQueue - UDP���������

class CUdpRequestQueue
{
private:
	// ��������ȱ��:
	// deque  - ֧��ͷβ������ɾ������ɾ�м�Ԫ�غ�����֧���±���ʡ�
	// vector - ֧��β��������ɾ��ͷ�����м�Ԫ����ɾ������֧���±���ʡ�
	// list   - ֧���κ�Ԫ�صĿ�����ɾ������֧���±���ʣ���֧�ֿ���ȡ��ǰ����(size())��
	typedef deque<CUdpPacket*> PacketList;

	CUdpRequestGroup *m_pOwnGroup;   // �������
	PacketList m_PacketList;         // ���ݰ��б�
	int m_nPacketCount;              // ���������ݰ��ĸ���(Ϊ�˿��ٷ���)
	int m_nCapacity;                 // ���е��������
	int m_nEffWaitTime;              // ���ݰ���Ч�ȴ�ʱ��(��)
	CCriticalSection m_Lock;
	CSemaphore m_Semaphore;

public:
	explicit CUdpRequestQueue(CUdpRequestGroup *pOwnGroup);
	virtual ~CUdpRequestQueue() { Clear(); }

	void AddPacket(CUdpPacket *pPacket);
	CUdpPacket* ExtractPacket();
	void Clear();
	void BreakWaiting(int nSemCount);

	int GetCount() { return m_nPacketCount; }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThread - UDP�������߳���
//
// ˵��:
// 1. ȱʡ����£�UDP�������߳�������г�ʱ��⣬��ĳЩ���������ó�ʱ��⣬����:
//    CUdpWorkerThread::GetTimeOutChecker().SetTimeOutSecs(0);
//
// ���ʽ���:
// 1. ��ʱ�߳�: ��ĳһ������빤��״̬������δ��ɵ��̡߳�
// 2. �����߳�: �ѱ�֪ͨ�˳������ò��˳����̡߳�

class CUdpWorkerThread : public CThread
{
private:
	CUdpWorkerThreadPool *m_pOwnPool;        // �����̳߳�
	CThreadTimeOutChecker m_TimeOutChecker;  // ��ʱ�����
protected:
	virtual void Execute();
	virtual void DoTerminate();
	virtual void DoKill();
public:
	explicit CUdpWorkerThread(CUdpWorkerThreadPool *pThreadPool);
	virtual ~CUdpWorkerThread();

	// ���س�ʱ�����
	CThreadTimeOutChecker& GetTimeOutChecker() { return m_TimeOutChecker; }
	// ���ظ��߳��Ƿ����״̬(���ڵȴ�����)
	bool IsIdle() { return !m_TimeOutChecker.GetStarted(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThreadPool - UDP�������̳߳���

class CUdpWorkerThreadPool
{
public:
	enum
	{
		MAX_THREAD_TERM_SECS     = 60*3,    // �̱߳�֪ͨ�˳���������(��)
		MAX_THREAD_WAIT_FOR_SECS = 2        // �̳߳����ʱ���ȴ�ʱ��(��)
	};

private:
	CUdpRequestGroup *m_pOwnGroup;          // �������
	CThreadList m_ThreadList;               // �߳��б�
private:
	void CreateThreads(int nCount);
	void TerminateThreads(int nCount);
	void CheckThreadTimeOut();
	void KillZombieThreads();
public:
	explicit CUdpWorkerThreadPool(CUdpRequestGroup *pOwnGroup);
	virtual ~CUdpWorkerThreadPool();

	void RegisterThread(CUdpWorkerThread *pThread);
	void UnregisterThread(CUdpWorkerThread *pThread);

	// ���ݸ��������̬�����߳�����
	void AdjustThreadCount();
	// ֪ͨ�����߳��˳�
	void TerminateAllThreads();
	// �ȴ������߳��˳�
	void WaitForAllThreads();

	// ȡ�õ�ǰ�߳�����
	int GetThreadCount() { return m_ThreadList.GetCount(); }
	// ȡ���������
	CUdpRequestGroup& GetRequestGroup() { return *m_pOwnGroup; }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestGroup - UDP���������

class CUdpRequestGroup
{
private:
	CMainUdpServer *m_pOwnMainUdpSvr;       // ����UDP������
	int m_nGroupIndex;                      // ����(0-based)
	CUdpRequestQueue m_RequestQueue;        // �������
	CUdpWorkerThreadPool m_ThreadPool;      // �������̳߳�

public:
	CUdpRequestGroup(CMainUdpServer *pOwnMainUdpSvr, int nGroupIndex);
	virtual ~CUdpRequestGroup() {}

	int GetGroupIndex() { return m_nGroupIndex; }
	CUdpRequestQueue& GetRequestQueue() { return m_RequestQueue; }
	CUdpWorkerThreadPool& GetThreadPool() { return m_ThreadPool; }

	// ȡ������UDP������
	CMainUdpServer& GetMainUdpServer() { return *m_pOwnMainUdpSvr; }
};

///////////////////////////////////////////////////////////////////////////////
// class CMainUdpServer - UDP����������

class CMainUdpServer
{
private:
	CUdpServer m_UdpServer;
	vector<CUdpRequestGroup*> m_RequestGroupList;   // ��������б�
	int m_nRequestGroupCount;                       // �����������
private:
	void InitUdpServer();
	void InitRequestGroupList();
	void ClearRequestGroupList();
private:
	static void OnRecvData(void *pParam, void *pPacketBuffer, int nPacketSize,
		const CPeerAddress& PeerAddr);
public:
	explicit CMainUdpServer();
	virtual ~CMainUdpServer();

	void Open();
	void Close();

	void SetLocalPort(int nValue) { m_UdpServer.SetLocalPort(nValue); }
	void SetListenerThreadCount(int nValue) { m_UdpServer.SetListenerThreadCount(nValue); }

	// ���ݸ��������̬�����������߳�����
	void AdjustWorkerThreadCount();
	// ֪ͨ���й������߳��˳�
	void TerminateAllWorkerThreads();
	// �ȴ����й������߳��˳�
	void WaitForAllWorkerThreads();

	CUdpServer& GetUdpServer() { return m_UdpServer; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_UDP_H_
