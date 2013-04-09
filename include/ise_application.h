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

class IseBusiness;
class IseOptions;
class IseMainServer;
class IseApplication;

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
typedef void (*USER_SIGNAL_HANDLER_PROC)(void *param, int signalNumber);

///////////////////////////////////////////////////////////////////////////////
// class IseBusiness - ISEҵ�����

class IseBusiness
{
public:
	IseBusiness() {}
	virtual ~IseBusiness() {}

	// ��ʼ�� (ʧ�����׳��쳣)
	virtual void initialize() {}
	// ������ (���۳�ʼ���Ƿ����쳣������ʱ����ִ��)
	virtual void finalize() {}

	// ���������в�������������ȷ�򷵻� false
	virtual bool parseArguments(int argc, char *argv[]) { return true; }
	// ���س���ĵ�ǰ�汾��
	virtual string getAppVersion() { return "1.0.0.0"; }
	// ���س���İ�����Ϣ
	virtual string getAppHelp() { return ""; }
	// ��������״̬
	virtual void doStartupState(STARTUP_STATE state) {}
	// ��ʼ��ISE������Ϣ
	virtual void initIseOptions(IseOptions& options) {}

	// UDP���ݰ�����
	virtual void classifyUdpPacket(void *packetBuffer, int packetSize, int& groupIndex) { groupIndex = 0; }
	// UDP���ݰ�����
	virtual void dispatchUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet) {}

	// ������һ���µ�TCP����
	virtual void onTcpConnection(TcpConnection *connection) {}
	// TCP���Ӵ�����̷����˴��� (ISE����֮ɾ�������Ӷ���)
	virtual void onTcpError(TcpConnection *connection) {}
	// TCP�����ϵ�һ���������������
	virtual void onTcpRecvComplete(TcpConnection *connection, void *packetBuffer,
		int packetSize, const CustomParams& params) {}
	// TCP�����ϵ�һ���������������
	virtual void onTcpSendComplete(TcpConnection *connection, const CustomParams& params) {}

	// ���������߳�ִ��(assistorIndex: 0-based)
	virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex) {}
	// ϵͳ�ػ��߳�ִ�� (secondCount: 0-based)
	virtual void daemonThreadExecute(Thread& thread, int secondCount) {}
};

///////////////////////////////////////////////////////////////////////////////
// class IseOptions - ISE������

class IseOptions
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

	string logFileName_;              // ��־�ļ��� (��·��)
	bool logNewFileDaily_;            // �Ƿ�ÿ����һ���������ļ��洢��־
	bool isDaemon_;                   // �Ƿ��̨�ػ�����(daemon)
	bool allowMultiInstance_;         // �Ƿ�����������ʵ��ͬʱ����

	/* ------------ ��������������: ------------ */

	// ���������� (ST_UDP | ST_TCP)
	UINT serverType_;
	// ��̨�����������߳�������ʱ����(��)
	int adjustThreadInterval_;
	// �����̵߳ĸ���
	int assistorThreadCount_;

	/* ------------ UDP����������: ------------ */

	struct CUdpRequestGroupOpt
	{
		int requestQueueCapacity;      // ������е�����(�������ɶ��ٸ����ݰ�)
		int minWorkerThreads;          // �������̵߳����ٸ���
		int maxWorkerThreads;          // �������̵߳�������

		CUdpRequestGroupOpt()
		{
			requestQueueCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;
			minWorkerThreads = DEF_UDP_WORKER_THREADS_MIN;
			maxWorkerThreads = DEF_UDP_WORKER_THREADS_MAX;
		}
	};
	typedef vector<CUdpRequestGroupOpt> CUdpRequestGroupOpts;

	// UDP����˿�
	int udpServerPort_;
	// �����̵߳�����
	int udpListenerThreadCount_;
	// ������������
	int udpRequestGroupCount_;
	// ÿ������ڵ�����
	CUdpRequestGroupOpts udpRequestGroupOpts_;
	// ���ݰ��ڶ����е���Ч�ȴ�ʱ�䣬��ʱ���账��(��)
	int udpRequestEffWaitTime_;
	// �������̵߳Ĺ�����ʱʱ��(��)����Ϊ0��ʾ�����г�ʱ���
	int udpWorkerThreadTimeOut_;
	// ������������ݰ����������ߣ����������������������߳�
	int udpRequestQueueAlertLine_;

	/* ------------ TCP����������: ------------ */

	struct CTcpServerOpt
	{
		int tcpServerPort;             // TCP����˿ں�

		CTcpServerOpt()
		{
			tcpServerPort = 0;
		}
	};
	typedef vector<CTcpServerOpt> CTcpServerOpts;

	// TCP������������ (һ��TCP������ռ��һ��TCP�˿�)
	int tcpServerCount_;
	// ÿ��TCP������������
	CTcpServerOpts tcpServerOpts_;
	// TCP�¼�ѭ���ĸ���
	int tcpEventLoopCount_;

public:
	IseOptions();

	// ϵͳ��������/��ȡ-------------------------------------------------------

	void setLogFileName(const string& value, bool logNewFileDaily = false)
		{ logFileName_ = value; logNewFileDaily_ = logNewFileDaily; }
	void setIsDaemon(bool value) { isDaemon_ = value; }
	void setAllowMultiInstance(bool value) { allowMultiInstance_ = value; }

	string getLogFileName() { return logFileName_; }
	bool getLogNewFileDaily() { return logNewFileDaily_; }
	bool getIsDaemon() { return isDaemon_; }
	bool getAllowMultiInstance() { return allowMultiInstance_; }

	// ��������������----------------------------------------------------------

	// ���÷���������(ST_UDP | ST_TCP)
	void setServerType(UINT serverType);
	// ���ú�̨�����������߳�������ʱ����(��)
	void setAdjustThreadInterval(int seconds);
	// ���ø����̵߳�����
	void setAssistorThreadCount(int count);

	// ����UDP����˿ں�
	void setUdpServerPort(int port);
	// ����UDP�����̵߳�����
	void setUdpListenerThreadCount(int count);
	// ����UDP������������
	void setUdpRequestGroupCount(int count);
	// ����UDP������е�������� (�������ɶ��ٸ����ݰ�)
	void setUdpRequestQueueCapacity(int groupIndex, int capacity);
	// ����UDP�������̸߳�����������
	void setUdpWorkerThreadCount(int groupIndex, int minThreads, int maxThreads);
	// ����UDP�����ڶ����е���Ч�ȴ�ʱ�䣬��ʱ���账��(��)
	void setUdpRequestEffWaitTime(int seconds);
	// ����UDP�������̵߳Ĺ�����ʱʱ��(��)����Ϊ0��ʾ�����г�ʱ���
	void setUdpWorkerThreadTimeOut(int seconds);
	// ����UDP������������ݰ�����������
	void SetUdpRequestQueueAlertLine(int count);

	// ����TCP������������
	void setTcpServerCount(int count);
	// ����TCP����˿ں�
	void setTcpServerPort(int serverIndex, int port);
	// ����TCP�¼�ѭ���ĸ���
	void setTcpEventLoopCount(int count);

	// ���������û�ȡ----------------------------------------------------------

	UINT getServerType() { return serverType_; }
	int getAdjustThreadInterval() { return adjustThreadInterval_; }
	int getAssistorThreadCount() { return assistorThreadCount_; }

	int getUdpServerPort() { return udpServerPort_; }
	int getUdpListenerThreadCount() { return udpListenerThreadCount_; }
	int getUdpRequestGroupCount() { return udpRequestGroupCount_; }
	int getUdpRequestQueueCapacity(int groupIndex);
	void getUdpWorkerThreadCount(int groupIndex, int& minThreads, int& maxThreads);
	int getUdpRequestEffWaitTime() { return udpRequestEffWaitTime_; }
	int getUdpWorkerThreadTimeOut() { return udpWorkerThreadTimeOut_; }
	int getUdpRequestQueueAlertLine() { return udpRequestQueueAlertLine_; }

	int getTcpServerCount() { return tcpServerCount_; }
	int getTcpServerPort(int serverIndex);
	int getTcpEventLoopCount() { return tcpEventLoopCount_; }
};

///////////////////////////////////////////////////////////////////////////////
// class IseMainServer - ����������

class IseMainServer
{
private:
	MainUdpServer *udpServer_;        // UDP������
	MainTcpServer *tcpServer_;        // TCP������
	AssistorServer *assistorServer_;  // ����������
	SysThreadMgr *sysThreadMgr_;      // ϵͳ�̹߳�����
private:
	void runBackground();
public:
	IseMainServer();
	virtual ~IseMainServer();

	void initialize();
	void finalize();
	void run();

	MainUdpServer& getMainUdpServer() { return *udpServer_; }
	MainTcpServer& getMainTcpServer() { return *tcpServer_; }
	AssistorServer& getAssistorServer() { return *assistorServer_; }
};

///////////////////////////////////////////////////////////////////////////////
// class IseApplication - ISEӦ�ó�����
//
// ˵��:
// 1. ����������������������ܣ�ȫ�ֵ�������(Application)�ڳ�������ʱ����������
// 2. һ����˵��������������ⲿ��������(kill)���˳��ġ���ISE�У������յ�kill�˳������
//    (��ʱ��ǰִ�е�һ���� iseApplication.run() ��)���ᴥ�� ExitProgramSignalHandler
//    �źŴ��������������� longjmp ����ʹִ�е�ģ��� run() ���˳����̶�ִ�� finalize��
// 3. �������������ķǷ��������󣬻��ȴ��� FatalErrorSignalHandler �źŴ�������
//    Ȼ��ͬ����������������˳���˳���
// 4. �������ڲ��������˳������Ƽ�ʹ��exit���������� iseApplication.SetTeraminted(true).
//    ���������ó���������������˳���˳���

class IseApplication
{
private:
	friend void userSignalHandler(int sigNo);

private:
	IseOptions iseOptions_;                       // ISE����
	IseMainServer *mainServer_;                   // ��������
	StringArray argList_;                         // �����в���
	string exeName_;                              // ��ִ���ļ���ȫ��(������·��)
	time_t appStartTime_;                         // ��������ʱ��ʱ��
	bool initialized_;                            // �Ƿ�ɹ���ʼ��
	bool terminated_;                             // �Ƿ�Ӧ�˳��ı�־
	CallbackList<USER_SIGNAL_HANDLER_PROC> onUserSignal_;  // �û��źŴ���ص�

private:
	bool processStandardArgs();
	void checkMultiInstance();
	void applyIseOptions();
	void createMainServer();
	void freeMainServer();
	void createIseBusiness();
	void freeIseBusiness();
	void initExeName();
	void initDaemon();
	void initSignals();
	void initNewOperHandler();
	void closeTerminal();
	void doFinalize();

public:
	IseApplication();
	virtual ~IseApplication();

	bool parseArguments(int argc, char *argv[]);
	void initialize();
	void finalize();
	void run();

	inline IseOptions& getIseOptions() { return iseOptions_; }
	inline IseMainServer& getMainServer() { return *mainServer_; }
	inline IseScheduleTaskMgr& getScheduleTaskMgr() { return IseScheduleTaskMgr::instance(); }

	inline void setTerminated(bool value) { terminated_ = value; }
	inline bool getTerminated() { return terminated_; }

	// ȡ�ÿ�ִ���ļ���ȫ��(������·��)
	string getExeName() { return exeName_; }
	// ȡ�ÿ�ִ���ļ����ڵ�·��
	string getExePath();
	// ȡ�������в�������(�׸�����Ϊ����·���ļ���)
	int getArgCount() { return (int)argList_.size(); }
	// ȡ�������в����ַ��� (index: 0-based)
	string getArgString(int index);
	// ȡ�ó�������ʱ��ʱ��
	time_t getAppStartTime() { return appStartTime_; }

	// ע���û��źŴ�����
	void registerUserSignalHandler(USER_SIGNAL_HANDLER_PROC proc, void *param = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// ȫ�ֱ�������

// Ӧ�ó������
extern IseApplication iseApplication;
// ISEҵ�����ָ��
extern IseBusiness *iseBusiness;

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_APPLICATION_H_
