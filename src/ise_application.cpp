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
// �ļ�����: ise_application.cpp
// ��������: ������Ӧ������Ԫ
///////////////////////////////////////////////////////////////////////////////

#include "ise_application.h"
#include "ise_errmsgs.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////
// �ⲿ��������

CIseBusiness* CreateIseBusinessObject();

///////////////////////////////////////////////////////////////////////////////
// ������

#ifdef ISE_WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		if (IseApplication.ParseArguments(__argc, __argv))
		{
			struct CAppFinalizer {
				~CAppFinalizer() { IseApplication.Finalize(); }
			} AppFinalizer;

			IseApplication.Initialize();
			IseApplication.Run();
		}
	}
	catch (CException& e)
	{
		Logger().WriteException(e);
	}

	return 0;
}

#endif
#ifdef ISE_LINUX

int main(int argc, char *argv[])
{
	try
	{
		if (IseApplication.ParseArguments(argc, argv))
		{
			struct CAppFinalizer {
				~CAppFinalizer() { IseApplication.Finalize(); }
			} AppFinalizer;

			IseApplication.Initialize();
			IseApplication.Run();
		}
	}
	catch (CException& e)
	{
		cout << e.MakeLogStr() << endl << endl;
		Logger().WriteException(e);
	}

	return 0;
}

#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ȫ�ֱ�������

// Ӧ�ó������
CIseApplication IseApplication;
// ISEҵ�����ָ��
CIseBusiness *pIseBusiness;

#ifdef ISE_LINUX
// ���ڽ����˳�ʱ����ת
static sigjmp_buf ProcExitJmpBuf;
#endif

// �����ڴ治�������³����˳�
static char *pReservedMemoryForExit;

///////////////////////////////////////////////////////////////////////////////
// �źŴ�����

#ifdef ISE_LINUX
//-----------------------------------------------------------------------------
// ����: �����˳����� �źŴ�����
//-----------------------------------------------------------------------------
void ExitProgramSignalHandler(int nSigNo)
{
	static bool bInHandler = false;
	if (bInHandler) return;
	bInHandler = true;

	// ֹͣ���߳�ѭ��
	IseApplication.SetTerminated(true);

	Logger().WriteFmt(SEM_SIG_TERM, nSigNo);

	siglongjmp(ProcExitJmpBuf, 1);
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ����� �źŴ�����
//-----------------------------------------------------------------------------
void FatalErrorSignalHandler(int nSigNo)
{
	static bool bInHandler = false;
	if (bInHandler) return;
	bInHandler = true;

	// ֹͣ���߳�ѭ��
	IseApplication.SetTerminated(true);

	Logger().WriteFmt(SEM_SIG_FATAL_ERROR, nSigNo);
	abort();
}
#endif

//-----------------------------------------------------------------------------
// ����: �û��źŴ�����
//-----------------------------------------------------------------------------
void UserSignalHandler(int nSigNo)
{
	const CCallBackList<USER_SIGNAL_HANDLER_PROC>& ProcList = IseApplication.m_OnUserSignal;

	for (int i = 0; i < ProcList.GetCount(); i++)
	{
		const CCallBackDef<USER_SIGNAL_HANDLER_PROC>& ProcItem = ProcList.GetItem(i);
		ProcItem.pProc(ProcItem.pParam, nSigNo);
	}
}

//-----------------------------------------------------------------------------
// ����: �ڴ治���������
// ��ע:
//   ��δ��װ��������(set_new_handler)���� new ����ʧ��ʱ�׳� bad_alloc �쳣��
//   ����װ����������new �����������׳��쳣�����ǵ��ô�����������
//-----------------------------------------------------------------------------
void OutOfMemoryHandler()
{
	static bool bInHandler = false;
	if (bInHandler) return;
	bInHandler = true;

	// �ͷű����ڴ棬��������˳��������ٴγ����ڴ治��
	delete[] pReservedMemoryForExit;
	pReservedMemoryForExit = NULL;

	Logger().WriteStr(SEM_OUT_OF_MEMORY);
	abort();
}

///////////////////////////////////////////////////////////////////////////////
// class CIseOptions

CIseOptions::CIseOptions()
{
	m_strLogFileName = "";
	m_bLogNewFileDaily = false;
	m_bIsDaemon = false;
	m_bAllowMultiInstance = false;

	SetServerType(DEF_SERVER_TYPE);
	SetAdjustThreadInterval(DEF_ADJUST_THREAD_INTERVAL);
	SetAssistorThreadCount(DEF_ASSISTOR_THREAD_COUNT);

	SetUdpServerPort(DEF_UDP_SERVER_PORT);
	SetUdpListenerThreadCount(DEF_UDP_LISTENER_THD_COUNT);
	SetUdpRequestGroupCount(DEF_UDP_REQ_GROUP_COUNT);
	for (int i = 0; i < DEF_UDP_REQ_GROUP_COUNT; i++)
	{
		SetUdpRequestQueueCapacity(i, DEF_UDP_REQ_QUEUE_CAPACITY);
		SetUdpWorkerThreadCount(i, DEF_UDP_WORKER_THREADS_MIN, DEF_UDP_WORKER_THREADS_MAX);
	}
	SetUdpRequestEffWaitTime(DEF_UDP_REQ_EFF_WAIT_TIME);
	SetUdpWorkerThreadTimeOut(DEF_UDP_WORKER_THD_TIMEOUT);
	SetUdpRequestQueueAlertLine(DEF_UDP_QUEUE_ALERT_LINE);

	SetTcpServerCount(DEF_TCP_REQ_GROUP_COUNT);
	for (int i = 0; i < DEF_TCP_REQ_GROUP_COUNT; i++)
		SetTcpServerPort(i, DEF_TCP_SERVER_PORT);
	SetTcpEventLoopCount(DEF_TCP_EVENT_LOOP_COUNT);
}

//-----------------------------------------------------------------------------
// ����: ���÷���������(UDP|TCP)
// ����:
//   nSvrType - ������������(�ɶ�ѡ��ѡ)
// ʾ��:
//   SetServerType(ST_UDP | ST_TCP);
//-----------------------------------------------------------------------------
void CIseOptions::SetServerType(UINT nSvrType)
{
	m_nServerType = nSvrType;
}

//-----------------------------------------------------------------------------
// ����: ���ú�̨ά���������߳�������ʱ����(��)
//-----------------------------------------------------------------------------
void CIseOptions::SetAdjustThreadInterval(int nSecs)
{
	if (nSecs <= 0) nSecs = DEF_ADJUST_THREAD_INTERVAL;
	m_nAdjustThreadInterval = nSecs;
}

//-----------------------------------------------------------------------------
// ����: ���ø����̵߳�����
//-----------------------------------------------------------------------------
void CIseOptions::SetAssistorThreadCount(int nCount)
{
	if (nCount < 0) nCount = 0;
	m_nAssistorThreadCount = nCount;
}

//-----------------------------------------------------------------------------
// ����: ����UDP����˿ں�
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpServerPort(int nPort)
{
	m_nUdpServerPort = nPort;
}

//-----------------------------------------------------------------------------
// ����: ����UDP�����̵߳�����
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpListenerThreadCount(int nCount)
{
	if (nCount < 1) nCount = 1;

	m_nUdpListenerThreadCount = nCount;
}

//-----------------------------------------------------------------------------
// ����: ����UDP���ݰ����������
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestGroupCount(int nCount)
{
	if (nCount <= 0) nCount = DEF_UDP_REQ_GROUP_COUNT;

	m_nUdpRequestGroupCount = nCount;
	m_UdpRequestGroupOpts.resize(nCount);
}

//-----------------------------------------------------------------------------
// ����: ����UDP������е�������� (�������ɶ��ٸ����ݰ�)
// ����:
//   nGroupIndex - ���� (0-based)
//   nCapacity   - ����
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestQueueCapacity(int nGroupIndex, int nCapacity)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return;

	if (nCapacity <= 0) nCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;

	m_UdpRequestGroupOpts[nGroupIndex].nRequestQueueCapacity = nCapacity;
}

//-----------------------------------------------------------------------------
// ����: ����UDP�������̸߳�����������
// ����:
//   nGroupIndex - ���� (0-based)
//   nMinThreads - �̸߳���������
//   nMaxThreads - �̸߳���������
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpWorkerThreadCount(int nGroupIndex, int nMinThreads, int nMaxThreads)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return;

	if (nMinThreads < 1) nMinThreads = 1;
	if (nMaxThreads < nMinThreads) nMaxThreads = nMinThreads;

	m_UdpRequestGroupOpts[nGroupIndex].nMinWorkerThreads = nMinThreads;
	m_UdpRequestGroupOpts[nGroupIndex].nMaxWorkerThreads = nMaxThreads;
}

//-----------------------------------------------------------------------------
// ����: ����UDP�����ڶ����е���Ч�ȴ�ʱ�䣬��ʱ���账��
// ����:
//   nMSecs - �ȴ�����
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestEffWaitTime(int nSecs)
{
	if (nSecs <= 0) nSecs = DEF_UDP_REQ_EFF_WAIT_TIME;
	m_nUdpRequestEffWaitTime = nSecs;
}

//-----------------------------------------------------------------------------
// ����: ����UDP�������̵߳Ĺ�����ʱʱ��(��)����Ϊ0��ʾ�����г�ʱ���
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpWorkerThreadTimeOut(int nSecs)
{
	if (nSecs < 0) nSecs = 0;
	m_nUdpWorkerThreadTimeOut = nSecs;
}

//-----------------------------------------------------------------------------
// ����: ����������������ݰ�����������
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestQueueAlertLine(int nCount)
{
	if (nCount < 1) nCount = 1;
	m_nUdpRequestQueueAlertLine = nCount;
}

//-----------------------------------------------------------------------------
// ����: ����TCP���ݰ����������
//-----------------------------------------------------------------------------
void CIseOptions::SetTcpServerCount(int nCount)
{
	if (nCount < 0) nCount = 0;

	m_nTcpServerCount = nCount;
	m_TcpServerOpts.resize(nCount);
}

//-----------------------------------------------------------------------------
// ����: ����TCP����˿ں�
// ����:
//   nServerIndex - TCP��������� (0-based)
//   nPort        - �˿ں�
//-----------------------------------------------------------------------------
void CIseOptions::SetTcpServerPort(int nServerIndex, int nPort)
{
	if (nServerIndex < 0 || nServerIndex >= m_nTcpServerCount) return;

	m_TcpServerOpts[nServerIndex].nTcpServerPort = nPort;
}

//-----------------------------------------------------------------------------
// ����: ����TCP�¼�ѭ���ĸ���
//-----------------------------------------------------------------------------
void CIseOptions::SetTcpEventLoopCount(int nCount)
{
	if (nCount < 1) nCount = 1;
	m_nTcpEventLoopCount = nCount;
}

//-----------------------------------------------------------------------------
// ����: ȡ��UDP������е�������� (�������ɶ��ٸ����ݰ�)
// ����:
//   nGroupIndex - ���� (0-based)
//-----------------------------------------------------------------------------
int CIseOptions::GetUdpRequestQueueCapacity(int nGroupIndex)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return -1;

	return m_UdpRequestGroupOpts[nGroupIndex].nRequestQueueCapacity;
}

//-----------------------------------------------------------------------------
// ����: ȡ��UDP�������̸߳�����������
// ����:
//   nGroupIndex - ���� (0-based)
//   nMinThreads - ����̸߳���������
//   nMaxThreads - ����̸߳���������
//-----------------------------------------------------------------------------
void CIseOptions::GetUdpWorkerThreadCount(int nGroupIndex, int& nMinThreads, int& nMaxThreads)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return;

	nMinThreads = m_UdpRequestGroupOpts[nGroupIndex].nMinWorkerThreads;
	nMaxThreads = m_UdpRequestGroupOpts[nGroupIndex].nMaxWorkerThreads;
}

//-----------------------------------------------------------------------------
// ����: ȡ��TCP����˿ں�
// ����:
//   nServerIndex - TCP����������� (0-based)
//-----------------------------------------------------------------------------
int CIseOptions::GetTcpServerPort(int nServerIndex)
{
	if (nServerIndex < 0 || nServerIndex >= m_nTcpServerCount) return -1;

	return m_TcpServerOpts[nServerIndex].nTcpServerPort;
}

///////////////////////////////////////////////////////////////////////////////
// class CIseMainServer

CIseMainServer::CIseMainServer() :
	m_pUdpServer(NULL),
	m_pTcpServer(NULL),
	m_pAssistorServer(NULL),
	m_pSysThreadMgr(NULL)
{
	// nothing
}

CIseMainServer::~CIseMainServer()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ��������ʼ���к����߳̽��к�̨�ػ�����
//-----------------------------------------------------------------------------
void CIseMainServer::RunBackground()
{
	int nAdjustThreadInterval = IseApplication.GetIseOptions().GetAdjustThreadInterval();
	int nSecondCount = 0;

	while (!IseApplication.GetTerminated())
	try
	{
		try
		{
			// ÿ�� nAdjustThreadInterval ��ִ��һ��
			if ((nSecondCount % nAdjustThreadInterval) == 0)
			{
#ifdef ISE_LINUX
				// ��ʱ�����˳��ź�
				CSignalMasker SigMasker(true);
				SigMasker.SetSignals(1, SIGTERM);
				SigMasker.Block();
#endif

				// ά���������̵߳�����
				if (m_pUdpServer) m_pUdpServer->AdjustWorkerThreadCount();
			}
		}
		catch (...)
		{}

		nSecondCount++;
		SleepSec(1, true);  // 1��
	}
	catch (...)
	{}
}

//-----------------------------------------------------------------------------
// ����: ��������ʼ�� (����ʼ��ʧ�����׳��쳣)
// ��ע: �� IseApplication.Initialize() ����
//-----------------------------------------------------------------------------
void CIseMainServer::Initialize()
{
	// ��ʼ�� UDP ������
	if (IseApplication.GetIseOptions().GetServerType() & ST_UDP)
	{
		m_pUdpServer = new CMainUdpServer();
		m_pUdpServer->SetLocalPort(IseApplication.GetIseOptions().GetUdpServerPort());
		m_pUdpServer->SetListenerThreadCount(IseApplication.GetIseOptions().GetUdpListenerThreadCount());
		m_pUdpServer->Open();
	}

	// ��ʼ�� TCP ������
	if (IseApplication.GetIseOptions().GetServerType() & ST_TCP)
	{
		m_pTcpServer = new CMainTcpServer();
		m_pTcpServer->Open();
	}

	// ��ʼ������������
	m_pAssistorServer = new CAssistorServer();
	m_pAssistorServer->Open();

	// ��ʼ��ϵͳ�̹߳�����
	m_pSysThreadMgr = new CSysThreadMgr();
	m_pSysThreadMgr->Initialize();
}

//-----------------------------------------------------------------------------
// ����: ������������
// ��ע: �� IseApplication.Finalize() ���ã��� CIseMainServer �����������в��ص���
//-----------------------------------------------------------------------------
void CIseMainServer::Finalize()
{
	if (m_pAssistorServer)
	{
		m_pAssistorServer->Close();
		delete m_pAssistorServer;
		m_pAssistorServer = NULL;
	}

	if (m_pUdpServer)
	{
		m_pUdpServer->Close();
		delete m_pUdpServer;
		m_pUdpServer = NULL;
	}

	if (m_pTcpServer)
	{
		m_pTcpServer->Close();
		delete m_pTcpServer;
		m_pTcpServer = NULL;
	}

	if (m_pSysThreadMgr)
	{
		m_pSysThreadMgr->Finalize();
		delete m_pSysThreadMgr;
		m_pSysThreadMgr = NULL;
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ���з�����
// ��ע: �� IseApplication.Run() ����
//-----------------------------------------------------------------------------
void CIseMainServer::Run()
{
	RunBackground();
}

///////////////////////////////////////////////////////////////////////////////
// class CIseApplication

CIseApplication::CIseApplication() :
	m_pMainServer(NULL),
	m_nAppStartTime(time(NULL)),
	m_bInitialized(false),
	m_bTerminated(false)
{
	CreateIseBusiness();
}

CIseApplication::~CIseApplication()
{
	Finalize();
}

//-----------------------------------------------------------------------------
// ����: �����׼�����в���
// ����:
//   true  - ��ǰ�����в����Ǳ�׼����
//   false - �����෴
//-----------------------------------------------------------------------------
bool CIseApplication::ProcessStandardArgs()
{
	if (GetArgCount() == 2)
	{
		string strArg = GetArgString(1);
		if (strArg == "--version")
		{
			string strVersion = pIseBusiness->GetAppVersion();
			printf("%s\n", strVersion.c_str());
			return true;
		}
		if (strArg == "--help")
		{
			string strHelp = pIseBusiness->GetAppHelp();
			printf("%s\n", strHelp.c_str());
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// ����: ����Ƿ������˶������ʵ��
//-----------------------------------------------------------------------------
void CIseApplication::CheckMultiInstance()
{
	if (m_IseOptions.GetAllowMultiInstance()) return;

#ifdef ISE_WIN32
	CreateMutexA(NULL, false, GetExeName().c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		IseThrowException(SEM_ALREADY_RUNNING);
#endif
#ifdef ISE_LINUX
	umask(0);
	int fd = open(GetExeName().c_str(), O_RDONLY, 0666);
	if (fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) != 0)
		IseThrowException(SEM_ALREADY_RUNNING);
#endif
}

//-----------------------------------------------------------------------------
// ����: Ӧ�� ISE ����
//-----------------------------------------------------------------------------
void CIseApplication::ApplyIseOptions()
{
	Logger().SetFileName(m_IseOptions.GetLogFileName(), m_IseOptions.GetLogNewFileDaily());
}

//-----------------------------------------------------------------------------
// ����: ������������
//-----------------------------------------------------------------------------
void CIseApplication::CreateMainServer()
{
	if (!m_pMainServer)
		m_pMainServer = new CIseMainServer;
}

//-----------------------------------------------------------------------------
// ����: �ͷ���������
//-----------------------------------------------------------------------------
void CIseApplication::FreeMainServer()
{
	delete m_pMainServer;
	m_pMainServer = NULL;
}

//-----------------------------------------------------------------------------
// ����: ���� CIseBusiness ����
//-----------------------------------------------------------------------------
void CIseApplication::CreateIseBusiness()
{
	if (!pIseBusiness)
		pIseBusiness = CreateIseBusinessObject();
}

//-----------------------------------------------------------------------------
// ����: �ͷ� CIseBusiness ����
//-----------------------------------------------------------------------------
void CIseApplication::FreeIseBusiness()
{
	delete pIseBusiness;
	pIseBusiness = NULL;
}

//-----------------------------------------------------------------------------
// ����: ȡ�������ļ���ȫ��������ʼ�� m_strExeName
//-----------------------------------------------------------------------------
void CIseApplication::InitExeName()
{
	m_strExeName = GetAppExeName();
}

//-----------------------------------------------------------------------------
// ����: �ػ�ģʽ��ʼ��
//-----------------------------------------------------------------------------
void CIseApplication::InitDaemon()
{
#ifdef ISE_WIN32
#endif
#ifdef ISE_LINUX
	int r;

	r = fork();
	if (r < 0)
		IseThrowException(SEM_INIT_DAEMON_ERROR);
	else if (r != 0)
		exit(0);

	// ��һ�ӽ��̺�̨����ִ��

	// ��һ�ӽ��̳�Ϊ�µĻỰ�鳤�ͽ����鳤
	r = setsid();
	if (r < 0) exit(1);

	// ���� SIGHUP �ź� (ע: �������ն˶Ͽ�ʱ�������������������ر�telnet)
	signal(SIGHUP, SIG_IGN);

	// ��һ�ӽ����˳�
	r = fork();
	if (r < 0) exit(1);
	else if (r != 0) exit(0);

	// �ڶ��ӽ��̼���ִ�У��������ǻỰ�鳤

	// �ı䵱ǰ����Ŀ¼ (core dump ���������Ŀ¼��)
	// chdir("/");

	// �����ļ�������ģ
	umask(0);
#endif
}

//-----------------------------------------------------------------------------
// ����: ��ʼ���ź� (�źŵİ�װ�����Ե�)
//
//  �ź�����    ֵ                         �ź�˵��
// ---------  ----  -----------------------------------------------------------
// # SIGHUP    1    ���ź����û��ն�����(�����������)����ʱ������ͨ�������ն˵Ŀ��ƽ���
//                  ����ʱ��֪ͨͬһ session �ڵĸ�����ҵ����ʱ����������ն˲��ٹ�����
// # SIGINT    2    ������ֹ(interrupt)�źţ����û�����INTR�ַ�(ͨ����Ctrl-C)ʱ������
// # SIGQUIT   3    �� SIGINT ���ƣ�����QUIT�ַ� (ͨ���� Ctrl-\) �����ơ����������յ�
//                  ���ź��˳�ʱ�����core�ļ��������������������һ����������źš�
// # SIGILL    4    ִ���˷Ƿ�ָ�ͨ������Ϊ��ִ���ļ�������ִ��󣬻�����ͼִ�����ݶΡ�
//                  ��ջ���ʱҲ�п��ܲ�������źš�
// # SIGTRAP   5    �ɶϵ�ָ������� trap ָ��������� debugger ʹ�á�
// # SIGABRT   6    �����Լ����ִ��󲢵��� abort ʱ������
// # SIGIOT    6    ��PDP-11����iotָ������������������Ϻ� SIGABRT һ����
// # SIGBUS    7    �Ƿ���ַ�������ڴ��ַ����(alignment)����eg: ����һ���ĸ��ֳ���
//                  �����������ַ���� 4 �ı�����
// # SIGFPE    8    �ڷ��������������������ʱ������������������������󣬻������������
//                  ��Ϊ 0 ���������е������Ĵ���
// # SIGKILL   9    ��������������������С����źŲ��ܱ�����������ͺ��ԡ�
// # SIGUSR1   10   �����û�ʹ�á�
// # SIGSEGV   11   ��ͼ����δ������Լ����ڴ棬����ͼ��û��дȨ�޵��ڴ��ַд���ݡ�
// # SIGUSR2   12   �����û�ʹ�á�
// # SIGPIPE   13   �ܵ�����(broken pipe)��дһ��û�ж��˿ڵĹܵ���
// # SIGALRM   14   ʱ�Ӷ�ʱ�źţ��������ʵ�ʵ�ʱ���ʱ��ʱ�䡣alarm ����ʹ�ø��źš�
// # SIGTERM   15   �������(terminate)�źţ��� SIGKILL ��ͬ���Ǹ��źſ��Ա������ʹ���
//                  ͨ������Ҫ������Լ������˳���shell ���� kill ȱʡ��������źš�
// # SIGSTKFLT 16   Э��������ջ����(stack fault)��
// # SIGCHLD   17   �ӽ��̽���ʱ�������̻��յ�����źš�
// # SIGCONT   18   ��һ��ֹͣ(stopped)�Ľ��̼���ִ�С����źŲ��ܱ�������������һ��
//                  handler ���ó������� stopped ״̬��Ϊ����ִ��ʱ����ض��Ĺ���������
//                  ������ʾ��ʾ����
// # SIGSTOP   19   ֹͣ(stopped)���̵�ִ�С�ע������terminate�Լ�interrupt������:
//                  �ý��̻�δ������ֻ����ִͣ�С����źŲ��ܱ��������������ԡ�
// # SIGTSTP   20   ֹͣ���̵����У������źſ��Ա�����ͺ��ԡ��û�����SUSP�ַ�ʱ(ͨ����^Z)
//                  ��������źš�
// # SIGTTIN   21   ����̨��ҵҪ���û��ն˶�����ʱ������ҵ�е����н��̻��յ����źš�ȱʡʱ
//                  ��Щ���̻�ִֹͣ�С�
// # SIGTTOU   22   ������SIGTTIN������д�ն�(���޸��ն�ģʽ)ʱ�յ���
// # SIGURG    23   �� "����" ���ݻ����(out-of-band) ���ݵ��� socket ʱ������
// # SIGXCPU   24   ����CPUʱ����Դ���ơ�������ƿ�����getrlimit/setrlimit����ȡ�͸ı䡣
// # SIGXFSZ   25   �����ļ���С��Դ���ơ�
// # SIGVTALRM 26   ����ʱ���źš������� SIGALRM�����Ǽ�����Ǹý���ռ�õ�CPUʱ�䡣
// # SIGPROF   27   ������SIGALRM/SIGVTALRM���������ý����õ�CPUʱ���Լ�ϵͳ���õ�ʱ�䡣
// # SIGWINCH  28   �ն��Ӵ��ĸı�ʱ������
// # SIGIO     29   �ļ�������׼�����������Կ�ʼ��������/���������
// # SIGPWR    30   Power failure.
// # SIGSYS    31   �Ƿ���ϵͳ���á�
//-----------------------------------------------------------------------------
void CIseApplication::InitSignals()
{
#ifdef ISE_WIN32
#endif
#ifdef ISE_LINUX
	int i;

	// ����ĳЩ�ź�
	int nIgnoreSignals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTSTP, SIGTTIN,
		SIGTTOU, SIGXCPU, SIGCHLD, SIGPWR, SIGALRM, SIGVTALRM, SIGIO};
	for (i = 0; i < sizeof(nIgnoreSignals)/sizeof(int); i++)
		signal(nIgnoreSignals[i], SIG_IGN);

	// ��װ�����Ƿ������źŴ�����
	int nFatalSignals[] = {SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGPROF, SIGSYS};
	for (i = 0; i < sizeof(nFatalSignals)/sizeof(int); i++)
		signal(nFatalSignals[i], FatalErrorSignalHandler);

	// ��װ�����˳��źŴ�����
	int nExitSignals[] = {SIGTERM/*, SIGABRT*/};
	for (i = 0; i < sizeof(nExitSignals)/sizeof(int); i++)
		signal(nExitSignals[i], ExitProgramSignalHandler);

	// ��װ�û��źŴ�����
	int nUserSignals[] = {SIGUSR1, SIGUSR2};
	for (i = 0; i < sizeof(nUserSignals)/sizeof(int); i++)
		signal(nUserSignals[i], UserSignalHandler);
#endif
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� new �������Ĵ�������
//-----------------------------------------------------------------------------
void CIseApplication::InitNewOperHandler()
{
	const int RESERVED_MEM_SIZE = 1024*1024*2;     // 2M

	set_new_handler(OutOfMemoryHandler);

	// �����ڴ治�������³����˳�
	pReservedMemoryForExit = new char[RESERVED_MEM_SIZE];
}

//-----------------------------------------------------------------------------
// ����: �ر��ն�
//-----------------------------------------------------------------------------
void CIseApplication::CloseTerminal()
{
#ifdef ISE_WIN32
#endif
#ifdef ISE_LINUX
	close(0);  // �رձ�׼����(stdin)
	/*
	close(1);  // �رձ�׼���(stdout)
	close(2);  // �رձ�׼�������(stderr)
	*/
#endif
}

//-----------------------------------------------------------------------------
// ����: Ӧ�ó�������� (����� m_bInitialized ��־)
//-----------------------------------------------------------------------------
void CIseApplication::DoFinalize()
{
	try { if (m_pMainServer) m_pMainServer->Finalize(); } catch (...) {}
	try { pIseBusiness->Finalize(); } catch (...) {}
	try { FreeMainServer(); } catch (...) {}
	try { FreeIseBusiness(); } catch (...) {}
	try { NetworkFinalize(); } catch (...) {}
}

//-----------------------------------------------------------------------------
// ����: ���������в���
// ����:
//   true  - ��������ִ��
//   false - ����Ӧ�˳� (���������в�������ȷ)
//-----------------------------------------------------------------------------
bool CIseApplication::ParseArguments(int nArgc, char *sArgv[])
{
	// �ȼ�¼�����в���
	m_ArgList.clear();
	for (int i = 0; i < nArgc; i++)
		m_ArgList.push_back(sArgv[i]);

	// �����׼�����в���
	if (ProcessStandardArgs()) return false;

	// ���� pIseBusiness ����
	return pIseBusiness->ParseArguments(nArgc, sArgv);
}

//-----------------------------------------------------------------------------
// ����: Ӧ�ó����ʼ�� (����ʼ��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CIseApplication::Initialize()
{
	try
	{
#ifdef ISE_LINUX
		// �ڳ�ʼ���׶�Ҫ�����˳��ź�
		CSignalMasker SigMasker(true);
		SigMasker.SetSignals(1, SIGTERM);
		SigMasker.Block();
#endif

		NetworkInitialize();
		InitExeName();
		pIseBusiness->DoStartupState(SS_BEFORE_START);
		pIseBusiness->InitIseOptions(m_IseOptions);
		CheckMultiInstance();
		if (m_IseOptions.GetIsDaemon()) InitDaemon();
		InitSignals();
		InitNewOperHandler();
		ApplyIseOptions();
		CreateMainServer();
		pIseBusiness->Initialize();
		m_pMainServer->Initialize();
		pIseBusiness->DoStartupState(SS_AFTER_START);
		if (m_IseOptions.GetIsDaemon()) CloseTerminal();
		m_bInitialized = true;
	}
	catch (CException&)
	{
		pIseBusiness->DoStartupState(SS_START_FAIL);
		DoFinalize();
		throw;
	}
}

//-----------------------------------------------------------------------------
// ����: Ӧ�ó��������
//-----------------------------------------------------------------------------
void CIseApplication::Finalize()
{
	if (m_bInitialized)
	{
		DoFinalize();
		m_bInitialized = false;
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ����Ӧ�ó���
//-----------------------------------------------------------------------------
void CIseApplication::Run()
{
#ifdef ISE_LINUX
	// ���̱���ֹʱ����ת���˴�����������
	if (sigsetjmp(ProcExitJmpBuf, 0)) return;
#endif

	if (m_pMainServer)
		m_pMainServer->Run();
}

//-----------------------------------------------------------------------------
// ����: ȡ�ÿ�ִ���ļ����ڵ�·��
//-----------------------------------------------------------------------------
string CIseApplication::GetExePath()
{
	return ExtractFilePath(m_strExeName);
}

//-----------------------------------------------------------------------------
// ����: ȡ�������в����ַ��� (nIndex: 0-based)
//-----------------------------------------------------------------------------
string CIseApplication::GetArgString(int nIndex)
{
	if (nIndex >= 0 && nIndex < (int)m_ArgList.size())
		return m_ArgList[nIndex];
	else
		return "";
}

//-----------------------------------------------------------------------------
// ����: ע���û��źŴ�����
//-----------------------------------------------------------------------------
void CIseApplication::RegisterUserSignalHandler(USER_SIGNAL_HANDLER_PROC pProc, void *pParam)
{
	m_OnUserSignal.Register(pProc, pParam);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
