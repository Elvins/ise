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

class ThreadTimeOutChecker;
class UdpPacket;
class UdpRequestQueue;
class UdpWorkerThread;
class UdpWorkerThreadPool;
class UdpRequestGroup;
class MainUdpServer;

///////////////////////////////////////////////////////////////////////////////
// class ThreadTimeOutChecker - �̳߳�ʱ�����
//
// ˵��:
// ����������� UdpWorkerThread/CTcpWorkerThread�����й������̵߳Ĺ���ʱ�䳬ʱ��⡣
// ���������߳��յ�һ����������Ͻ��빤��״̬��һ����ԣ��������߳�Ϊ�����������������
// ʱ�䲻��̫������̫����ᵼ�·��������й������̶߳�ȱ��ʹ��Ӧ����������������½�������
// ����UDP������˵���������ˡ�ͨ������£��̹߳�����ʱ����������Ϊ��������̺��߼�����
// �������ⲿԭ�򣬱������ݿⷱæ����Դ����������ӵ�µȵȡ����̹߳�����ʱ��Ӧ֪ͨ���˳���
// ����֪ͨ�˳�������ʱ������δ�˳�����ǿ��ɱ�����������̵߳�����������ʱ�����µ��̡߳�

class ThreadTimeOutChecker : public AutoInvokable
{
private:
	Thread *thread_;          // �������߳�
	time_t startTime_;        // ��ʼ��ʱʱ��ʱ���
	bool started_;            // �Ƿ��ѿ�ʼ��ʱ
	UINT timeoutSecs_;        // ������������Ϊ��ʱ (Ϊ0��ʾ�����г�ʱ���)
	CriticalSection lock_;

private:
	void start();
	void stop();

protected:
	virtual void invokeInitialize() { start(); }
	virtual void invokeFinalize() { stop(); }

public:
	explicit ThreadTimeOutChecker(Thread *thread);
	virtual ~ThreadTimeOutChecker() {}

	// ����߳��Ƿ��ѳ�ʱ������ʱ��֪ͨ���˳�
	bool check();

	// ���ó�ʱʱ�䣬��Ϊ0���ʾ�����г�ʱ���
	void setTimeOutSecs(UINT value) { timeoutSecs_ = value; }
	// �����Ƿ��ѿ�ʼ��ʱ
	bool getStarted();
};

///////////////////////////////////////////////////////////////////////////////
// class UdpPacket - UDP���ݰ���

class UdpPacket
{
private:
	void *packetBuffer_;

public:
	UINT recvTimeStamp_;
	InetAddress peerAddr_;
	int packetSize_;

public:
	UdpPacket() :
		packetBuffer_(NULL),
		recvTimeStamp_(0),
		peerAddr_(0, 0),
		packetSize_(0)
	{}
	virtual ~UdpPacket()
		{ if (packetBuffer_) free(packetBuffer_); }

	void setPacketBuffer(void *pPakcetBuffer, int packetSize);
	inline void* getPacketBuffer() const { return packetBuffer_; }
};

///////////////////////////////////////////////////////////////////////////////
// class UdpRequestQueue - UDP���������

class UdpRequestQueue
{
private:
	// ��������ȱ��:
	// deque  - ֧��ͷβ������ɾ������ɾ�м�Ԫ�غ�����֧���±���ʡ�
	// vector - ֧��β��������ɾ��ͷ�����м�Ԫ����ɾ������֧���±���ʡ�
	// list   - ֧���κ�Ԫ�صĿ�����ɾ������֧���±���ʣ���֧�ֿ���ȡ��ǰ����(size())��
	typedef deque<UdpPacket*> PacketList;

	UdpRequestGroup *ownGroup_;    // �������
	PacketList packetList_;        // ���ݰ��б�
	int packetCount_;              // ���������ݰ��ĸ���(Ϊ�˿��ٷ���)
	int capacity_;                 // ���е��������
	int effWaitTime_;              // ���ݰ���Ч�ȴ�ʱ��(��)
	CriticalSection lock_;
	Semaphore semaphore_;

public:
	explicit UdpRequestQueue(UdpRequestGroup *ownGroup);
	virtual ~UdpRequestQueue() { clear(); }

	void addPacket(UdpPacket *pPacket);
	UdpPacket* extractPacket();
	void clear();
	void breakWaiting(int semCount);

	int getCount() { return packetCount_; }
};

///////////////////////////////////////////////////////////////////////////////
// class UdpWorkerThread - UDP�������߳���
//
// ˵��:
// 1. ȱʡ����£�UDP�������߳�������г�ʱ��⣬��ĳЩ���������ó�ʱ��⣬����:
//    UdpWorkerThread::GetTimeOutChecker().SetTimeOutSecs(0);
//
// ���ʽ���:
// 1. ��ʱ�߳�: ��ĳһ������빤��״̬������δ��ɵ��̡߳�
// 2. �����߳�: �ѱ�֪ͨ�˳������ò��˳����̡߳�

class UdpWorkerThread : public Thread
{
private:
	UdpWorkerThreadPool *ownPool_;         // �����̳߳�
	ThreadTimeOutChecker timeoutChecker_;  // ��ʱ�����
protected:
	virtual void execute();
	virtual void doTerminate();
	virtual void doKill();
public:
	explicit UdpWorkerThread(UdpWorkerThreadPool *threadPool);
	virtual ~UdpWorkerThread();

	// ���س�ʱ�����
	ThreadTimeOutChecker& getTimeoutChecker() { return timeoutChecker_; }
	// ���ظ��߳��Ƿ����״̬(���ڵȴ�����)
	bool isIdle() { return !timeoutChecker_.getStarted(); }
};

///////////////////////////////////////////////////////////////////////////////
// class UdpWorkerThreadPool - UDP�������̳߳���

class UdpWorkerThreadPool
{
public:
	enum
	{
		MAX_THREAD_TERM_SECS     = 60*3,    // �̱߳�֪ͨ�˳���������(��)
		MAX_THREAD_WAIT_FOR_SECS = 2        // �̳߳����ʱ���ȴ�ʱ��(��)
	};

private:
	UdpRequestGroup *ownGroup_;           // �������
	ThreadList threadList_;               // �߳��б�
private:
	void createThreads(int count);
	void terminateThreads(int count);
	void checkThreadTimeout();
	void killZombieThreads();
public:
	explicit UdpWorkerThreadPool(UdpRequestGroup *ownGroup);
	virtual ~UdpWorkerThreadPool();

	void registerThread(UdpWorkerThread *thread);
	void unregisterThread(UdpWorkerThread *thread);

	// ���ݸ��������̬�����߳�����
	void AdjustThreadCount();
	// ֪ͨ�����߳��˳�
	void terminateAllThreads();
	// �ȴ������߳��˳�
	void waitForAllThreads();

	// ȡ�õ�ǰ�߳�����
	int getThreadCount() { return threadList_.getCount(); }
	// ȡ���������
	UdpRequestGroup& getRequestGroup() { return *ownGroup_; }
};

///////////////////////////////////////////////////////////////////////////////
// class UdpRequestGroup - UDP���������

class UdpRequestGroup
{
private:
	MainUdpServer *ownMainUdpSvr_;         // ����UDP������
	int groupIndex_;                       // ����(0-based)
	UdpRequestQueue requestQueue_;         // �������
	UdpWorkerThreadPool threadPool_;       // �������̳߳�

public:
	UdpRequestGroup(MainUdpServer *ownMainUdpSvr, int groupIndex);
	virtual ~UdpRequestGroup() {}

	int getGroupIndex() { return groupIndex_; }
	UdpRequestQueue& getRequestQueue() { return requestQueue_; }
	UdpWorkerThreadPool& getThreadPool() { return threadPool_; }

	// ȡ������UDP������
	MainUdpServer& getMainUdpServer() { return *ownMainUdpSvr_; }
};

///////////////////////////////////////////////////////////////////////////////
// class MainUdpServer - UDP����������

class MainUdpServer
{
private:
	UdpServer udpServer_;
	vector<UdpRequestGroup*> requestGroupList_;    // ��������б�
	int requestGroupCount_;                        // �����������
private:
	void initUdpServer();
	void initRequestGroupList();
	void clearRequestGroupList();
private:
	static void onRecvData(void *param, void *packetBuffer, int packetSize,
		const InetAddress& peerAddr);
public:
	explicit MainUdpServer();
	virtual ~MainUdpServer();

	void open();
	void close();

	void setLocalPort(int value) { udpServer_.setLocalPort(value); }
	void setListenerThreadCount(int value) { udpServer_.setListenerThreadCount(value); }

	// ���ݸ��������̬�����������߳�����
	void adjustWorkerThreadCount();
	// ֪ͨ���й������߳��˳�
	void terminateAllWorkerThreads();
	// �ȴ����й������߳��˳�
	void waitForAllWorkerThreads();

	UdpServer& getUdpServer() { return udpServer_; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_UDP_H_
