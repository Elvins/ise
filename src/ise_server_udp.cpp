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
// �ļ�����: ise_server_udp.cpp
// ��������: UDP��������ʵ��
///////////////////////////////////////////////////////////////////////////////

#include "ise_server_udp.h"
#include "ise_errmsgs.h"
#include "ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CThreadTimeOutChecker

CThreadTimeOutChecker::CThreadTimeOutChecker(CThread *pThread) :
	m_pThread(pThread),
	m_tStartTime(0),
	m_bStarted(false),
	m_nTimeOutSecs(0)
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ��ʼ��ʱ
//-----------------------------------------------------------------------------
void CThreadTimeOutChecker::Start()
{
	CAutoLocker Locker(m_Lock);

	m_tStartTime = time(NULL);
	m_bStarted = true;
}

//-----------------------------------------------------------------------------
// ����: ֹͣ��ʱ
//-----------------------------------------------------------------------------
void CThreadTimeOutChecker::Stop()
{
	CAutoLocker Locker(m_Lock);

	m_bStarted = false;
}

//-----------------------------------------------------------------------------
// ����: ����߳��Ƿ��ѳ�ʱ������ʱ��֪ͨ���˳�
//-----------------------------------------------------------------------------
bool CThreadTimeOutChecker::Check()
{
	bool bResult = false;

	if (m_bStarted && m_nTimeOutSecs > 0)
	{
		if ((UINT)time(NULL) - m_tStartTime >= m_nTimeOutSecs)
		{
			if (!m_pThread->GetTerminated()) m_pThread->Terminate();
			bResult = true;
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ��ѿ�ʼ��ʱ
//-----------------------------------------------------------------------------
bool CThreadTimeOutChecker::GetStarted()
{
	CAutoLocker Locker(m_Lock);

	return m_bStarted;
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpPacket

void CUdpPacket::SetPacketBuffer(void *pPakcetBuffer, int nPacketSize)
{
	if (m_pPacketBuffer)
	{
		free(m_pPacketBuffer);
		m_pPacketBuffer = NULL;
	}

	m_pPacketBuffer = malloc(nPacketSize);
	if (!m_pPacketBuffer)
		IseThrowMemoryException();

	memcpy(m_pPacketBuffer, pPakcetBuffer, nPacketSize);
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestQueue

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   pOwnGroup - ָ���������
//-----------------------------------------------------------------------------
CUdpRequestQueue::CUdpRequestQueue(CUdpRequestGroup *pOwnGroup)
{
	int nGroupIndex;

	m_pOwnGroup = pOwnGroup;
	nGroupIndex = pOwnGroup->GetGroupIndex();
	m_nCapacity = IseApplication.GetIseOptions().GetUdpRequestQueueCapacity(nGroupIndex);
	m_nEffWaitTime = IseApplication.GetIseOptions().GetUdpRequestEffWaitTime();
	m_nPacketCount = 0;
}

//-----------------------------------------------------------------------------
// ����: �������������ݰ�
//-----------------------------------------------------------------------------
void CUdpRequestQueue::AddPacket(CUdpPacket *pPacket)
{
	if (m_nCapacity <= 0) return;
	bool bRemoved = false;

	{
		CAutoLocker Locker(m_Lock);

		if (m_nPacketCount >= m_nCapacity)
		{
			CUdpPacket *p;
			p = m_PacketList.front();
			delete p;
			m_PacketList.pop_front();
			m_nPacketCount--;
			bRemoved = true;
		}

		m_PacketList.push_back(pPacket);
		m_nPacketCount++;
	}

	if (!bRemoved) m_Semaphore.Increase();
}

//-----------------------------------------------------------------------------
// ����: �Ӷ�����ȡ�����ݰ� (ȡ����Ӧ�����ͷţ���ʧ���򷵻� NULL)
// ��ע: ����������û�����ݰ�����һֱ�ȴ���
//-----------------------------------------------------------------------------
CUdpPacket* CUdpRequestQueue::ExtractPacket()
{
	m_Semaphore.Wait();

	{
		CAutoLocker Locker(m_Lock);
		CUdpPacket *p, *pResult = NULL;

		while (m_nPacketCount > 0)
		{
			p = m_PacketList.front();
			m_PacketList.pop_front();
			m_nPacketCount--;

			if (time(NULL) - (UINT)p->m_nRecvTimeStamp <= (UINT)m_nEffWaitTime)
			{
				pResult = p;
				break;
			}
			else
			{
				delete p;
			}
		}

		return pResult;
	}
}

//-----------------------------------------------------------------------------
// ����: ��ն���
//-----------------------------------------------------------------------------
void CUdpRequestQueue::Clear()
{
	CAutoLocker Locker(m_Lock);
	CUdpPacket *p;

	for (UINT i = 0; i < m_PacketList.size(); i++)
	{
		p = m_PacketList[i];
		delete p;
	}

	m_PacketList.clear();
	m_nPacketCount = 0;
	m_Semaphore.Reset();
}

//-----------------------------------------------------------------------------
// ����: �����ź�����ֵ��ʹ�ȴ����ݵ��߳��жϵȴ�
//-----------------------------------------------------------------------------
void CUdpRequestQueue::BreakWaiting(int nSemCount)
{
	for (int i = 0; i < nSemCount; i++)
		m_Semaphore.Increase();
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThread

CUdpWorkerThread::CUdpWorkerThread(CUdpWorkerThreadPool *pThreadPool) :
	m_pOwnPool(pThreadPool),
	m_TimeOutChecker(this)
{
	SetFreeOnTerminate(true);
	// ���ó�ʱ���
	m_TimeOutChecker.SetTimeOutSecs(IseApplication.GetIseOptions().GetUdpWorkerThreadTimeOut());

	m_pOwnPool->RegisterThread(this);
}

CUdpWorkerThread::~CUdpWorkerThread()
{
	m_pOwnPool->UnregisterThread(this);
}

//-----------------------------------------------------------------------------
// ����: �̵߳�ִ�к���
//-----------------------------------------------------------------------------
void CUdpWorkerThread::Execute()
{
	int nGroupIndex;
	CUdpRequestQueue *pRequestQueue;
	CUdpPacket *pPacket;

	nGroupIndex = m_pOwnPool->GetRequestGroup().GetGroupIndex();
	pRequestQueue = &(m_pOwnPool->GetRequestGroup().GetRequestQueue());

	while (!GetTerminated())
	try
	{
		pPacket = pRequestQueue->ExtractPacket();
		if (pPacket)
		{
			std::auto_ptr<CUdpPacket> AutoPtr(pPacket);
			CAutoInvoker AutoInvoker(m_TimeOutChecker);

			// �������ݰ�
			if (!GetTerminated())
				pIseBusiness->DispatchUdpPacket(*this, nGroupIndex, *pPacket);
		}
	}
	catch (CException&)
	{}
}

//-----------------------------------------------------------------------------
// ����: ִ�� Terminate() ǰ�ĸ��Ӳ���
//-----------------------------------------------------------------------------
void CUdpWorkerThread::DoTerminate()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ִ�� Kill() ǰ�ĸ��Ӳ���
//-----------------------------------------------------------------------------
void CUdpWorkerThread::DoKill()
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThreadPool

CUdpWorkerThreadPool::CUdpWorkerThreadPool(CUdpRequestGroup *pOwnGroup) :
	m_pOwnGroup(pOwnGroup)
{
	// nothing
}

CUdpWorkerThreadPool::~CUdpWorkerThreadPool()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ���� nCount ���߳�
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::CreateThreads(int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		CUdpWorkerThread *pThread;
		pThread = new CUdpWorkerThread(this);
		pThread->Run();
	}
}

//-----------------------------------------------------------------------------
// ����: ��ֹ nCount ���߳�
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::TerminateThreads(int nCount)
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	int nTermCount = 0;
	if (nCount > m_ThreadList.GetCount())
		nCount = m_ThreadList.GetCount();

	for (int i = m_ThreadList.GetCount() - 1; i >= 0; i--)
	{
		CUdpWorkerThread *pThread;
		pThread = (CUdpWorkerThread*)m_ThreadList[i];
		if (pThread->IsIdle())
		{
			pThread->Terminate();
			nTermCount++;
			if (nTermCount >= nCount) break;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ����߳��Ƿ�����ʱ (��ʱ�߳�: ��ĳһ������빤��״̬������δ��ɵ��߳�)
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::CheckThreadTimeOut()
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	for (int i = 0; i < m_ThreadList.GetCount(); i++)
	{
		CUdpWorkerThread *pThread;
		pThread = (CUdpWorkerThread*)m_ThreadList[i];
		pThread->GetTimeOutChecker().Check();
	}
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���������߳� (�����߳�: �ѱ�֪ͨ�˳������ò��˳����߳�)
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::KillZombieThreads()
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	for (int i = m_ThreadList.GetCount() - 1; i >= 0; i--)
	{
		CUdpWorkerThread *pThread;
		pThread = (CUdpWorkerThread*)m_ThreadList[i];
		if (pThread->GetTermElapsedSecs() >= MAX_THREAD_TERM_SECS)
		{
			pThread->Kill();
			m_ThreadList.Remove(pThread);
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::RegisterThread(CUdpWorkerThread *pThread)
{
	m_ThreadList.Add(pThread);
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::UnregisterThread(CUdpWorkerThread *pThread)
{
	m_ThreadList.Remove(pThread);
}

//-----------------------------------------------------------------------------
// ����: ���ݸ��������̬�����߳�����
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::AdjustThreadCount()
{
	int nPacketCount, nThreadCount;
	int nMinThreads, nMaxThreads, nDeltaThreads;
	int nPacketAlertLine;

	// ȡ���߳�����������
	IseApplication.GetIseOptions().GetUdpWorkerThreadCount(
		m_pOwnGroup->GetGroupIndex(), nMinThreads, nMaxThreads);
	// ȡ��������������ݰ�����������
	nPacketAlertLine = IseApplication.GetIseOptions().GetUdpRequestQueueAlertLine();

	// ����߳��Ƿ�����ʱ
	CheckThreadTimeOut();
	// ǿ��ɱ���������߳�
	KillZombieThreads();

	// ȡ�����ݰ��������߳�����
	nPacketCount = m_pOwnGroup->GetRequestQueue().GetCount();
	nThreadCount = m_ThreadList.GetCount();

	// ��֤�߳������������޷�Χ֮��
	if (nThreadCount < nMinThreads)
	{
		CreateThreads(nMinThreads - nThreadCount);
		nThreadCount = nMinThreads;
	}
	if (nThreadCount > nMaxThreads)
	{
		TerminateThreads(nThreadCount - nMaxThreads);
		nThreadCount = nMaxThreads;
	}

	// �����������е��������������ߣ����������߳�����
	if (nThreadCount < nMaxThreads && nPacketCount >= nPacketAlertLine)
	{
		nDeltaThreads = Min(nMaxThreads - nThreadCount, 3);
		CreateThreads(nDeltaThreads);
	}

	// ����������Ϊ�գ����Լ����߳�����
	if (nThreadCount > nMinThreads && nPacketCount == 0)
	{
		nDeltaThreads = 1;
		TerminateThreads(nDeltaThreads);
	}
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�����߳��˳�
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::TerminateAllThreads()
{
	m_ThreadList.TerminateAllThreads();

	CAutoLocker Locker(m_ThreadList.GetLock());

	// ʹ�̴߳ӵȴ��н��ѣ������˳�
	GetRequestGroup().GetRequestQueue().BreakWaiting(m_ThreadList.GetCount());
}

//-----------------------------------------------------------------------------
// ����: �ȴ������߳��˳�
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::WaitForAllThreads()
{
	TerminateAllThreads();

	int nKilledCount = 0;
	m_ThreadList.WaitForAllThreads(MAX_THREAD_WAIT_FOR_SECS, &nKilledCount);

	if (nKilledCount)
		Logger().WriteFmt(SEM_THREAD_KILLED, nKilledCount, "udp worker");
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestGroup

CUdpRequestGroup::CUdpRequestGroup(CMainUdpServer *pOwnMainUdpSvr, int nGroupIndex) :
	m_pOwnMainUdpSvr(pOwnMainUdpSvr),
	m_nGroupIndex(nGroupIndex),
	m_RequestQueue(this),
	m_ThreadPool(this)
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////
// class CMainUdpServer

CMainUdpServer::CMainUdpServer()
{
	InitUdpServer();
	InitRequestGroupList();
}

CMainUdpServer::~CMainUdpServer()
{
	ClearRequestGroupList();
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� m_UdpServer
//-----------------------------------------------------------------------------
void CMainUdpServer::InitUdpServer()
{
	m_UdpServer.SetOnRecvDataCallBack(OnRecvData, this);
}

//-----------------------------------------------------------------------------
// ����: ��ʼ����������б�
//-----------------------------------------------------------------------------
void CMainUdpServer::InitRequestGroupList()
{
	ClearRequestGroupList();

	m_nRequestGroupCount = IseApplication.GetIseOptions().GetUdpRequestGroupCount();
	for (int nGroupIndex = 0; nGroupIndex < m_nRequestGroupCount; nGroupIndex++)
	{
		CUdpRequestGroup *p;
		p = new CUdpRequestGroup(this, nGroupIndex);
		m_RequestGroupList.push_back(p);
	}
}

//-----------------------------------------------------------------------------
// ����: �����������б�
//-----------------------------------------------------------------------------
void CMainUdpServer::ClearRequestGroupList()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		delete m_RequestGroupList[i];
	m_RequestGroupList.clear();
}

//-----------------------------------------------------------------------------
// ����: �յ����ݰ�
//-----------------------------------------------------------------------------
void CMainUdpServer::OnRecvData(void *pParam, void *pPacketBuffer, int nPacketSize,
	const CPeerAddress& PeerAddr)
{
	CMainUdpServer *pThis = (CMainUdpServer*)pParam;
	int nGroupIndex;

	// �Ƚ������ݰ����࣬�õ�����
	pIseBusiness->ClassifyUdpPacket(pPacketBuffer, nPacketSize, nGroupIndex);

	// ������źϷ�
	if (nGroupIndex >= 0 && nGroupIndex < pThis->m_nRequestGroupCount)
	{
		CUdpPacket *p = new CUdpPacket();
		if (p)
		{
			p->m_nRecvTimeStamp = (UINT)time(NULL);
			p->m_PeerAddr = PeerAddr;
			p->m_nPacketSize = nPacketSize;
			p->SetPacketBuffer(pPacketBuffer, nPacketSize);

			// ��ӵ����������
			pThis->m_RequestGroupList[nGroupIndex]->GetRequestQueue().AddPacket(p);
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ����������
//-----------------------------------------------------------------------------
void CMainUdpServer::Open()
{
	m_UdpServer.Open();
}

//-----------------------------------------------------------------------------
// ����: �رշ�����
//-----------------------------------------------------------------------------
void CMainUdpServer::Close()
{
	TerminateAllWorkerThreads();
	WaitForAllWorkerThreads();

	m_UdpServer.Close();
}

//-----------------------------------------------------------------------------
// ����: ���ݸ��������̬�����������߳�����
//-----------------------------------------------------------------------------
void CMainUdpServer::AdjustWorkerThreadCount()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		m_RequestGroupList[i]->GetThreadPool().AdjustThreadCount();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ���й������߳��˳�
//-----------------------------------------------------------------------------
void CMainUdpServer::TerminateAllWorkerThreads()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		m_RequestGroupList[i]->GetThreadPool().TerminateAllThreads();
}

//-----------------------------------------------------------------------------
// ����: �ȴ����й������߳��˳�
//-----------------------------------------------------------------------------
void CMainUdpServer::WaitForAllWorkerThreads()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		m_RequestGroupList[i]->GetThreadPool().WaitForAllThreads();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
