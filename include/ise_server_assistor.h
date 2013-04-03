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
// ise_server_assistor.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SERVER_ASSISTOR_H_
#define _ISE_SERVER_ASSISTOR_H_

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

class CAssistorThread;
class CAssistorThreadPool;
class CAssistorServer;

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThread - �����߳���
//
// ˵��:
// 1. ���ڷ���˳��򣬳����������֮�⣬һ�㻹��Ҫ���ɺ�̨�ػ��̣߳����ں�̨ά��������
//    �����������ݻ��ա����ݿ������������ȵȡ���������߳�ͳ��Ϊ assistor thread.

class CAssistorThread : public CThread
{
private:
	CAssistorThreadPool *m_pOwnPool;        // �����̳߳�
	int m_nAssistorIndex;                   // �����������(0-based)
protected:
	virtual void Execute();
	virtual void DoKill();
public:
	CAssistorThread(CAssistorThreadPool *pThreadPool, int nAssistorIndex);
	virtual ~CAssistorThread();

	int GetIndex() const { return m_nAssistorIndex; }
};

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThreadPool - �����̳߳���

class CAssistorThreadPool
{
private:
	CAssistorServer *m_pOwnAssistorSvr;     // ��������������
	CThreadList m_ThreadList;               // �߳��б�

public:
	explicit CAssistorThreadPool(CAssistorServer *pOwnAssistorServer);
	virtual ~CAssistorThreadPool();

	void RegisterThread(CAssistorThread *pThread);
	void UnregisterThread(CAssistorThread *pThread);

	// ֪ͨ�����߳��˳�
	void TerminateAllThreads();
	// �ȴ������߳��˳�
	void WaitForAllThreads();
	// ���ָ���̵߳�˯��
	void InterruptThreadSleep(int nAssistorIndex);

	// ȡ�õ�ǰ�߳�����
	int GetThreadCount() { return m_ThreadList.GetCount(); }
	// ȡ����������������
	CAssistorServer& GetAssistorServer() { return *m_pOwnAssistorSvr; }
};

///////////////////////////////////////////////////////////////////////////////
// class CAssistorServer - ����������

class CAssistorServer
{
private:
	bool m_bActive;                         // �������Ƿ�����
	CAssistorThreadPool m_ThreadPool;       // �����̳߳�

public:
	explicit CAssistorServer();
	virtual ~CAssistorServer();

	// ����������
	void Open();
	// �رշ�����
	void Close();

	// ���������߳�ִ�к���
	void OnAssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex);

	// ֪ͨ���и����߳��˳�
	void TerminateAllAssistorThreads();
	// �ȴ����и����߳��˳�
	void WaitForAllAssistorThreads();
	// ���ָ�������̵߳�˯��
	void InterruptAssistorThreadSleep(int nAssistorIndex);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_ASSISTOR_H_
