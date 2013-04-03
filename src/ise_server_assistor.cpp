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
// �ļ�����: ise_server_assistor.cpp
// ��������: ������������ʵ��
///////////////////////////////////////////////////////////////////////////////

#include "ise_server_assistor.h"
#include "ise_errmsgs.h"
#include "ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThread

CAssistorThread::CAssistorThread(CAssistorThreadPool *pThreadPool, int nAssistorIndex) :
	m_pOwnPool(pThreadPool),
	m_nAssistorIndex(nAssistorIndex)
{
	SetFreeOnTerminate(true);
	m_pOwnPool->RegisterThread(this);
}

CAssistorThread::~CAssistorThread()
{
	m_pOwnPool->UnregisterThread(this);
}

//-----------------------------------------------------------------------------
// ����: �߳�ִ�к���
//-----------------------------------------------------------------------------
void CAssistorThread::Execute()
{
	m_pOwnPool->GetAssistorServer().OnAssistorThreadExecute(*this, m_nAssistorIndex);
}

//-----------------------------------------------------------------------------
// ����: ִ�� Kill() ǰ�ĸ��Ӳ���
//-----------------------------------------------------------------------------
void CAssistorThread::DoKill()
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThreadPool

CAssistorThreadPool::CAssistorThreadPool(CAssistorServer *pOwnAssistorServer) :
	m_pOwnAssistorSvr(pOwnAssistorServer)
{
	// nothing
}

CAssistorThreadPool::~CAssistorThreadPool()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void CAssistorThreadPool::RegisterThread(CAssistorThread *pThread)
{
	m_ThreadList.Add(pThread);
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void CAssistorThreadPool::UnregisterThread(CAssistorThread *pThread)
{
	m_ThreadList.Remove(pThread);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�����߳��˳�
//-----------------------------------------------------------------------------
void CAssistorThreadPool::TerminateAllThreads()
{
	m_ThreadList.TerminateAllThreads();
}

//-----------------------------------------------------------------------------
// ����: �ȴ������߳��˳�
//-----------------------------------------------------------------------------
void CAssistorThreadPool::WaitForAllThreads()
{
	const int MAX_WAIT_FOR_SECS = 5;
	int nKilledCount = 0;

	m_ThreadList.WaitForAllThreads(MAX_WAIT_FOR_SECS, &nKilledCount);

	if (nKilledCount > 0)
		Logger().WriteFmt(SEM_THREAD_KILLED, nKilledCount, "assistor");
}

//-----------------------------------------------------------------------------
// ����: ���ָ���̵߳�˯��
//-----------------------------------------------------------------------------
void CAssistorThreadPool::InterruptThreadSleep(int nAssistorIndex)
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	for (int i = 0; i < m_ThreadList.GetCount(); i++)
	{
		CAssistorThread *pThread = (CAssistorThread*)m_ThreadList[i];
		if (pThread->GetIndex() == nAssistorIndex)
		{
			pThread->InterruptSleep();
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CAssistorServer

CAssistorServer::CAssistorServer() :
	m_bActive(false),
	m_ThreadPool(this)
{
	// nothing
}

CAssistorServer::~CAssistorServer()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ����������
//-----------------------------------------------------------------------------
void CAssistorServer::Open()
{
	if (!m_bActive)
	{
		int nCount = IseApplication.GetIseOptions().GetAssistorThreadCount();

		for (int i = 0; i < nCount; i++)
		{
			CAssistorThread *pThread;
			pThread = new CAssistorThread(&m_ThreadPool, i);
			pThread->Run();
		}

		m_bActive = true;
	}
}

//-----------------------------------------------------------------------------
// ����: �رշ�����
//-----------------------------------------------------------------------------
void CAssistorServer::Close()
{
	if (m_bActive)
	{
		WaitForAllAssistorThreads();
		m_bActive = false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���������߳�ִ�к���
// ����:
//   nAssistorIndex - �����߳����(0-based)
//-----------------------------------------------------------------------------
void CAssistorServer::OnAssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex)
{
	pIseBusiness->AssistorThreadExecute(AssistorThread, nAssistorIndex);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ���и����߳��˳�
//-----------------------------------------------------------------------------
void CAssistorServer::TerminateAllAssistorThreads()
{
	m_ThreadPool.TerminateAllThreads();
}

//-----------------------------------------------------------------------------
// ����: �ȴ����и����߳��˳�
//-----------------------------------------------------------------------------
void CAssistorServer::WaitForAllAssistorThreads()
{
	m_ThreadPool.WaitForAllThreads();
}

//-----------------------------------------------------------------------------
// ����: ���ָ�������̵߳�˯��
//-----------------------------------------------------------------------------
void CAssistorServer::InterruptAssistorThreadSleep(int nAssistorIndex)
{
	m_ThreadPool.InterruptThreadSleep(nAssistorIndex);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
