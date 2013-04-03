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
// ise_application.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_APPLICATION_H_
#define _ISE_APPLICATION_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"
#include "ise_server_udp.h"
#include "ise_server_tcp.h"
#include "ise_server_assistor.h"
#include "ise_sys_threads.h"
#include "ise_scheduler.h"

#ifdef ISE_WIN32
#include <stdarg.h>
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class CIseBusiness;
class CIseOptions;
class CIseMainServer;
class CIseApplication;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

// ����״̬
enum STARTUP_STATE
{
	SS_BEFORE_START,     // ����֮ǰ
	SS_AFTER_START,      // ����֮��
	SS_START_FAIL        // ����ʧ��
};

// ����������(�ɶ�ѡ��ѡ)
enum SERVER_TYPE
{
	ST_UDP = 0x0001,     // UDP������
	ST_TCP = 0x0002      // TCP������
};

// �û��źŴ���ص�
typedef void (*USER_SIGNAL_HANDLER_PROC)(void *pParam, int nSignalNumber);

///////////////////////////////////////////////////////////////////////////////
// class CIseBusiness - ISEҵ�����

class CIseBusiness
{
public:
	CIseBusiness() {}
	virtual ~CIseBusiness() {}

	// ��ʼ�� (ʧ�����׳��쳣)
	virtual void Initialize() {}
	// ������ (���۳�ʼ���Ƿ����쳣������ʱ����ִ��)
	virtual void Finalize() {}

	// ���������в�������������ȷ�򷵻� false
	virtual bool ParseArguments(int nArgc, char *sArgv[]) { return true; }
	// ���س���ĵ�ǰ�汾��
	virtual string GetAppVersion() { return "1.0.0.0"; }
	// ���س���İ�����Ϣ
	virtual string GetAppHelp() { return ""; }
	// ��������״̬
	virtual void DoStartupState(STARTUP_STATE nState) {}
	// ��ʼ��ISE������Ϣ
	virtual void InitIseOptions(CIseOptions& IseOpt) {}

	// UDP���ݰ�����
	virtual void ClassifyUdpPacket(void *pPacketBuffer, int nPacketSize, int& nGroupIndex) { nGroupIndex = 0; }
	// UDP���ݰ�����
	virtual void DispatchUdpPacket(CUdpWorkerThread& WorkerThread, int nGroupIndex, CUdpPacket& Packet) {}

	// ������һ���µ�TCP����
	virtual void OnTcpConnection(CTcpConnection *pConnection) {}
	// TCP���Ӵ�����̷����˴��� (ISE����֮ɾ�������Ӷ���)
	virtual void OnTcpError(CTcpConnection *pConnection) {}
	// TCP�����ϵ�һ���������������
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params) {}
	// TCP�����ϵ�һ���������������
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params) {}

	// ���������߳�ִ��(nAssistorIndex: 0-based)
	virtual void AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex) {}
	// ϵͳ�ػ��߳�ִ�� (nSecondCount: 0-based)
	virtual void DaemonThreadExecute(CThread& Thread, int nSecondCount) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CIseOptions - ISE������

class CIseOptions
{
public:
	// ��������������ȱʡֵ
	enum
	{
		DEF_SERVER_TYPE             = ST_UDP,    // ������Ĭ������
		DEF_ADJUST_THREAD_INTERVAL  = 5,         // ��̨���� "�������߳�����" ��ʱ����ȱʡֵ(��)
		DEF_ASSISTOR_THREAD_COUNT   = 0,         // �����̵߳ĸ���
	};

	// UDP����������ȱʡֵ
	enum
	{
		DEF_UDP_SERVER_PORT         = 9000,      // UDP����Ĭ�϶˿�
		DEF_UDP_LISTENER_THD_COUNT  = 1,         // �����̵߳�����
		DEF_UDP_REQ_GROUP_COUNT     = 1,         // �������������ȱʡֵ
		DEF_UDP_REQ_QUEUE_CAPACITY  = 5000,      // ������е�ȱʡ����(���ܷ��¶������ݰ�)
		DEF_UDP_WORKER_THREADS_MIN  = 1,         // ÿ������й������̵߳�ȱʡ���ٸ���
		DEF_UDP_WORKER_THREADS_MAX  = 8,         // ÿ������й������̵߳�ȱʡ������
		DEF_UDP_REQ_EFF_WAIT_TIME   = 10,        // �����ڶ����е���Ч�ȴ�ʱ��ȱʡֵ(��)
		DEF_UDP_WORKER_THD_TIMEOUT  = 60,        // �������̵߳Ĺ�����ʱʱ��ȱʡֵ(��)
		DEF_UDP_QUEUE_ALERT_LINE    = 500,       // ���������ݰ�����������ȱʡֵ�����������������������߳�
	};

	// TCP����������ȱʡֵ
	enum
	{
		DEF_TCP_SERVER_PORT         = 9000,      // TCP����Ĭ�϶˿�
		DEF_TCP_REQ_GROUP_COUNT     = 1,         // �������������ȱʡֵ
		DEF_TCP_EVENT_LOOP_COUNT    = 8,         // TCP�¼�ѭ���ĸ���
	};

private:
	/* ------------ ϵͳ����: ------------------ */

	string m_strLogFileName;            // ��־�ļ��� (��·��)
	bool m_bLogNewFileDaily;            // �Ƿ�ÿ����һ���������ļ��洢��־
	bool m_bIsDaemon;                   // �Ƿ��̨�ػ�����(daemon)
	bool m_bAllowMultiInstance;         // �Ƿ�����������ʵ��ͬʱ����

	/* ------------ ��������������: ------------ */

	// ���������� (ST_UDP | ST_TCP)
	UINT m_nServerType;
	// ��̨�����������߳�������ʱ����(��)
	int m_nAdjustThreadInterval;
	// �����̵߳ĸ���
	int m_nAssistorThreadCount;

	/* ------------ UDP����������: ------------ */

	struct CUdpRequestGroupOpt
	{
		int nRequestQueueCapacity;      // ������е�����(�������ɶ��ٸ����ݰ�)
		int nMinWorkerThreads;          // �������̵߳����ٸ���
		int nMaxWorkerThreads;          // �������̵߳�������

		CUdpRequestGroupOpt()
		{
			nRequestQueueCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;
			nMinWorkerThreads = DEF_UDP_WORKER_THREADS_MIN;
			nMaxWorkerThreads = DEF_UDP_WORKER_THREADS_MAX;
		}
	};
	typedef vector<CUdpRequestGroupOpt> CUdpRequestGroupOpts;

	// UDP����˿�
	int m_nUdpServerPort;
	// �����̵߳�����
	int m_nUdpListenerThreadCount;
	// ������������
	int m_nUdpRequestGroupCount;
	// ÿ������ڵ�����
	CUdpRequestGroupOpts m_UdpRequestGroupOpts;
	// ���ݰ��ڶ����е���Ч�ȴ�ʱ�䣬��ʱ���账��(��)
	int m_nUdpRequestEffWaitTime;
	// �������̵߳Ĺ�����ʱʱ��(��)����Ϊ0��ʾ�����г�ʱ���
	int m_nUdpWorkerThreadTimeOut;
	// ������������ݰ����������ߣ����������������������߳�
	int m_nUdpRequestQueueAlertLine;

	/* ------------ TCP����������: ------------ */

	struct CTcpServerOpt
	{
		int nTcpServerPort;             // TCP����˿ں�

		CTcpServerOpt()
		{
			nTcpServerPort = 0;
		}
	};
	typedef vector<CTcpServerOpt> CTcpServerOpts;

	// TCP������������ (һ��TCP������ռ��һ��TCP�˿�)
	int m_nTcpServerCount;
	// ÿ��TCP������������
	CTcpServerOpts m_TcpServerOpts;
	// TCP�¼�ѭ���ĸ���
	int m_nTcpEventLoopCount;

public:
	CIseOptions();

	// ϵͳ��������/��ȡ-------------------------------------------------------

	void SetLogFileName(const string& strValue, bool bLogNewFileDaily = false)
		{ m_strLogFileName = strValue; m_bLogNewFileDaily = bLogNewFileDaily; }
	void SetIsDaemon(bool bValue) { m_bIsDaemon = bValue; }
	void SetAllowMultiInstance(bool bValue) { m_bAllowMultiInstance = bValue; }

	string GetLogFileName() { return m_strLogFileName; }
	bool GetLogNewFileDaily() { return m_bLogNewFileDaily; }
	bool GetIsDaemon() { return m_bIsDaemon; }
	bool GetAllowMultiInstance() { return m_bAllowMultiInstance; }

	// ��������������----------------------------------------------------------

	// ���÷���������(ST_UDP | ST_TCP)
	void SetServerType(UINT nSvrType);
	// ���ú�̨�����������߳�������ʱ����(��)
	void SetAdjustThreadInterval(int nSecs);
	// ���ø����̵߳�����
	void SetAssistorThreadCount(int nCount);

	// ����UDP����˿ں�
	void SetUdpServerPort(int nPort);
	// ����UDP�����̵߳�����
	void SetUdpListenerThreadCount(int nCount);
	// ����UDP������������
	void SetUdpRequestGroupCount(int nCount);
	// ����UDP������е�������� (�������ɶ��ٸ����ݰ�)
	void SetUdpRequestQueueCapacity(int nGroupIndex, int nCapacity);
	// ����UDP�������̸߳�����������
	void SetUdpWorkerThreadCount(int nGroupIndex, int nMinThreads, int nMaxThreads);
	// ����UDP�����ڶ����е���Ч�ȴ�ʱ�䣬��ʱ���账��(��)
	void SetUdpRequestEffWaitTime(int nSecs);
	// ����UDP�������̵߳Ĺ�����ʱʱ��(��)����Ϊ0��ʾ�����г�ʱ���
	void SetUdpWorkerThreadTimeOut(int nSecs);
	// ����UDP������������ݰ�����������
	void SetUdpRequestQueueAlertLine(int nCount);

	// ����TCP������������
	void SetTcpServerCount(int nCount);
	// ����TCP����˿ں�
	void SetTcpServerPort(int nServerIndex, int nPort);
	// ����TCP�¼�ѭ���ĸ���
	void SetTcpEventLoopCount(int nCount);

	// ���������û�ȡ----------------------------------------------------------

	UINT GetServerType() { return m_nServerType; }
	int GetAdjustThreadInterval() { return m_nAdjustThreadInterval; }
	int GetAssistorThreadCount() { return m_nAssistorThreadCount; }

	int GetUdpServerPort() { return m_nUdpServerPort; }
	int GetUdpListenerThreadCount() { return m_nUdpListenerThreadCount; }
	int GetUdpRequestGroupCount() { return m_nUdpRequestGroupCount; }
	int GetUdpRequestQueueCapacity(int nGroupIndex);
	void GetUdpWorkerThreadCount(int nGroupIndex, int& nMinThreads, int& nMaxThreads);
	int GetUdpRequestEffWaitTime() { return m_nUdpRequestEffWaitTime; }
	int GetUdpWorkerThreadTimeOut() { return m_nUdpWorkerThreadTimeOut; }
	int GetUdpRequestQueueAlertLine() { return m_nUdpRequestQueueAlertLine; }

	int GetTcpServerCount() { return m_nTcpServerCount; }
	int GetTcpServerPort(int nServerIndex);
	int GetTcpEventLoopCount() { return m_nTcpEventLoopCount; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseMainServer - ����������

class CIseMainServer
{
private:
	CMainUdpServer *m_pUdpServer;        // UDP������
	CMainTcpServer *m_pTcpServer;        // TCP������
	CAssistorServer *m_pAssistorServer;  // ����������
	CSysThreadMgr *m_pSysThreadMgr;      // ϵͳ�̹߳�����
private:
	void RunBackground();
public:
	CIseMainServer();
	virtual ~CIseMainServer();

	void Initialize();
	void Finalize();
	void Run();

	CMainUdpServer& GetMainUdpServer() { return *m_pUdpServer; }
	CMainTcpServer& GetMainTcpServer() { return *m_pTcpServer; }
	CAssistorServer& GetAssistorServer() { return *m_pAssistorServer; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseApplication - ISEӦ�ó�����
//
// ˵��:
// 1. ����������������������ܣ�ȫ�ֵ�������(Application)�ڳ�������ʱ����������
// 2. һ����˵��������������ⲿ��������(kill)���˳��ġ���ISE�У������յ�kill�˳������
//    (��ʱ��ǰִ�е�һ���� IseApplication.Run() ��)���ᴥ�� ExitProgramSignalHandler
//    �źŴ��������������� longjmp ����ʹִ�е�ģ��� Run() ���˳����̶�ִ�� Finalize��
// 3. �������������ķǷ��������󣬻��ȴ��� FatalErrorSignalHandler �źŴ�������
//    Ȼ��ͬ����������������˳���˳���
// 4. �������ڲ��������˳������Ƽ�ʹ��exit���������� IseApplication.SetTeraminted(true).
//    ���������ó���������������˳���˳���

class CIseApplication
{
private:
	friend void UserSignalHandler(int nSigNo);

private:
	CIseOptions m_IseOptions;                       // ISE����
	CIseMainServer *m_pMainServer;                  // ��������
	StringArray m_ArgList;                          // �����в���
	string m_strExeName;                            // ��ִ���ļ���ȫ��(������·��)
	time_t m_nAppStartTime;                         // ��������ʱ��ʱ��
	bool m_bInitialized;                            // �Ƿ�ɹ���ʼ��
	bool m_bTerminated;                             // �Ƿ�Ӧ�˳��ı�־
	CCallBackList<USER_SIGNAL_HANDLER_PROC> m_OnUserSignal;  // �û��źŴ���ص�

private:
	bool ProcessStandardArgs();
	void CheckMultiInstance();
	void ApplyIseOptions();
	void CreateMainServer();
	void FreeMainServer();
	void CreateIseBusiness();
	void FreeIseBusiness();
	void InitExeName();
	void InitDaemon();
	void InitSignals();
	void InitNewOperHandler();
	void CloseTerminal();
	void DoFinalize();

public:
	CIseApplication();
	virtual ~CIseApplication();

	bool ParseArguments(int nArgc, char *sArgv[]);
	void Initialize();
	void Finalize();
	void Run();

	inline CIseOptions& GetIseOptions() { return m_IseOptions; }
	inline CIseMainServer& GetMainServer() { return *m_pMainServer; }
	inline CIseScheduleTaskMgr& GetScheduleTaskMgr() { return CIseScheduleTaskMgr::Instance(); }

	inline void SetTerminated(bool bValue) { m_bTerminated = bValue; }
	inline bool GetTerminated() { return m_bTerminated; }

	// ȡ�ÿ�ִ���ļ���ȫ��(������·��)
	string GetExeName() { return m_strExeName; }
	// ȡ�ÿ�ִ���ļ����ڵ�·��
	string GetExePath();
	// ȡ�������в�������(�׸�����Ϊ����·���ļ���)
	int GetArgCount() { return (int)m_ArgList.size(); }
	// ȡ�������в����ַ��� (nIndex: 0-based)
	string GetArgString(int nIndex);
	// ȡ�ó�������ʱ��ʱ��
	time_t GetAppStartTime() { return m_nAppStartTime; }

	// ע���û��źŴ�����
	void RegisterUserSignalHandler(USER_SIGNAL_HANDLER_PROC pProc, void *pParam = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// ȫ�ֱ�������

// Ӧ�ó������
extern CIseApplication IseApplication;
// ISEҵ�����ָ��
extern CIseBusiness *pIseBusiness;

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_APPLICATION_H_
