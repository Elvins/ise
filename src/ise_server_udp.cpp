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
// class ThreadTimeOutChecker

ThreadTimeOutChecker::ThreadTimeOutChecker(Thread *thread) :
    thread_(thread),
    startTime_(0),
    started_(false),
    timeoutSecs_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: ����߳��Ƿ��ѳ�ʱ������ʱ��֪ͨ���˳�
//-----------------------------------------------------------------------------
bool ThreadTimeOutChecker::check()
{
    bool result = false;

    if (started_ && timeoutSecs_ > 0)
    {
        if ((UINT)time(NULL) - startTime_ >= timeoutSecs_)
        {
            if (!thread_->isTerminated()) thread_->terminate();
            result = true;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ��ѿ�ʼ��ʱ
//-----------------------------------------------------------------------------
bool ThreadTimeOutChecker::getStarted()
{
    AutoLocker locker(lock_);

    return started_;
}

///////////////////////////////////////////////////////////////////////////////
// class UdpPacket

void UdpPacket::setPacketBuffer(void *pPakcetBuffer, int packetSize)
{
    if (packetBuffer_)
    {
        free(packetBuffer_);
        packetBuffer_ = NULL;
    }

    packetBuffer_ = malloc(packetSize);
    if (!packetBuffer_)
        iseThrowMemoryException();

    memcpy(packetBuffer_, pPakcetBuffer, packetSize);
}

//-----------------------------------------------------------------------------
// ����: ��ʼ��ʱ
//-----------------------------------------------------------------------------
void ThreadTimeOutChecker::start()
{
    AutoLocker locker(lock_);

    startTime_ = time(NULL);
    started_ = true;
}

//-----------------------------------------------------------------------------
// ����: ֹͣ��ʱ
//-----------------------------------------------------------------------------
void ThreadTimeOutChecker::stop()
{
    AutoLocker locker(lock_);

    started_ = false;
}

///////////////////////////////////////////////////////////////////////////////
// class UdpRequestQueue

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   ownGroup - ָ���������
//-----------------------------------------------------------------------------
UdpRequestQueue::UdpRequestQueue(UdpRequestGroup *ownGroup)
{
    int groupIndex;

    ownGroup_ = ownGroup;
    groupIndex = ownGroup->getGroupIndex();
    capacity_ = iseApp().getIseOptions().getUdpRequestQueueCapacity(groupIndex);
    effWaitTime_ = iseApp().getIseOptions().getUdpRequestEffWaitTime();
    packetCount_ = 0;
}

//-----------------------------------------------------------------------------
// ����: �������������ݰ�
//-----------------------------------------------------------------------------
void UdpRequestQueue::addPacket(UdpPacket *pPacket)
{
    if (capacity_ <= 0) return;
    bool removed = false;

    {
        AutoLocker locker(lock_);

        if (packetCount_ >= capacity_)
        {
            UdpPacket *p;
            p = packetList_.front();
            delete p;
            packetList_.pop_front();
            packetCount_--;
            removed = true;
        }

        packetList_.push_back(pPacket);
        packetCount_++;
    }

    if (!removed) semaphore_.increase();
}

//-----------------------------------------------------------------------------
// ����: �Ӷ�����ȡ�����ݰ� (ȡ����Ӧ�����ͷţ���ʧ���򷵻� NULL)
// ��ע: ����������û�����ݰ�����һֱ�ȴ���
//-----------------------------------------------------------------------------
UdpPacket* UdpRequestQueue::extractPacket()
{
    semaphore_.wait();

    {
        AutoLocker locker(lock_);
        UdpPacket *p, *result = NULL;

        while (packetCount_ > 0)
        {
            p = packetList_.front();
            packetList_.pop_front();
            packetCount_--;

            if (time(NULL) - (UINT)p->recvTimeStamp_ <= (UINT)effWaitTime_)
            {
                result = p;
                break;
            }
            else
            {
                delete p;
            }
        }

        return result;
    }
}

//-----------------------------------------------------------------------------
// ����: ��ն���
//-----------------------------------------------------------------------------
void UdpRequestQueue::clear()
{
    AutoLocker locker(lock_);
    UdpPacket *p;

    for (UINT i = 0; i < packetList_.size(); i++)
    {
        p = packetList_[i];
        delete p;
    }

    packetList_.clear();
    packetCount_ = 0;
    semaphore_.reset();
}

//-----------------------------------------------------------------------------
// ����: �����ź�����ֵ��ʹ�ȴ����ݵ��߳��жϵȴ�
//-----------------------------------------------------------------------------
void UdpRequestQueue::breakWaiting(int semCount)
{
    for (int i = 0; i < semCount; i++)
        semaphore_.increase();
}

///////////////////////////////////////////////////////////////////////////////
// class UdpWorkerThread

UdpWorkerThread::UdpWorkerThread(UdpWorkerThreadPool *threadPool) :
    ownPool_(threadPool),
    timeoutChecker_(this)
{
    setFreeOnTerminate(true);
    // ���ó�ʱ���
    timeoutChecker_.setTimeOutSecs(iseApp().getIseOptions().getUdpWorkerThreadTimeOut());

    ownPool_->registerThread(this);
}

UdpWorkerThread::~UdpWorkerThread()
{
    ownPool_->unregisterThread(this);
}

//-----------------------------------------------------------------------------
// ����: �̵߳�ִ�к���
//-----------------------------------------------------------------------------
void UdpWorkerThread::execute()
{
    int groupIndex;
    UdpRequestQueue *requestQueue;
    UdpPacket *packet;

    groupIndex = ownPool_->getRequestGroup().getGroupIndex();
    requestQueue = &(ownPool_->getRequestGroup().getRequestQueue());

    while (!isTerminated())
    try
    {
        packet = requestQueue->extractPacket();
        if (packet)
        {
            std::auto_ptr<UdpPacket> AutoPtr(packet);
            AutoInvoker autoInvoker(timeoutChecker_);

            // �������ݰ�
            if (!isTerminated())
                iseApp().getIseBusiness().dispatchUdpPacket(*this, groupIndex, *packet);
        }
    }
    catch (Exception&)
    {}
}

//-----------------------------------------------------------------------------
// ����: ִ�� terminate() ǰ�ĸ��Ӳ���
//-----------------------------------------------------------------------------
void UdpWorkerThread::doTerminate()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: ִ�� kill() ǰ�ĸ��Ӳ���
//-----------------------------------------------------------------------------
void UdpWorkerThread::doKill()
{
    // nothing
}

///////////////////////////////////////////////////////////////////////////////
// class UdpWorkerThreadPool

UdpWorkerThreadPool::UdpWorkerThreadPool(UdpRequestGroup *ownGroup) :
    ownGroup_(ownGroup)
{
    // nothing
}

UdpWorkerThreadPool::~UdpWorkerThreadPool()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::registerThread(UdpWorkerThread *thread)
{
    threadList_.add(thread);
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::unregisterThread(UdpWorkerThread *thread)
{
    threadList_.remove(thread);
}

//-----------------------------------------------------------------------------
// ����: ���ݸ��������̬�����߳�����
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::AdjustThreadCount()
{
    int packetCount, threadCount;
    int minThreads, maxThreads, deltaThreads;
    int packetAlertLine;

    // ȡ���߳�����������
    iseApp().getIseOptions().getUdpWorkerThreadCount(
        ownGroup_->getGroupIndex(), minThreads, maxThreads);
    // ȡ��������������ݰ�����������
    packetAlertLine = iseApp().getIseOptions().getUdpRequestQueueAlertLine();

    // ����߳��Ƿ�����ʱ
    checkThreadTimeout();
    // ǿ��ɱ���������߳�
    killZombieThreads();

    // ȡ�����ݰ��������߳�����
    packetCount = ownGroup_->getRequestQueue().getCount();
    threadCount = threadList_.getCount();

    // ��֤�߳������������޷�Χ֮��
    if (threadCount < minThreads)
    {
        createThreads(minThreads - threadCount);
        threadCount = minThreads;
    }
    if (threadCount > maxThreads)
    {
        terminateThreads(threadCount - maxThreads);
        threadCount = maxThreads;
    }

    // �����������е��������������ߣ����������߳�����
    if (threadCount < maxThreads && packetCount >= packetAlertLine)
    {
        deltaThreads = ise::min(maxThreads - threadCount, 3);
        createThreads(deltaThreads);
    }

    // ����������Ϊ�գ����Լ����߳�����
    if (threadCount > minThreads && packetCount == 0)
    {
        deltaThreads = 1;
        terminateThreads(deltaThreads);
    }
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�����߳��˳�
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::terminateAllThreads()
{
    threadList_.terminateAllThreads();

    AutoLocker locker(threadList_.getLock());

    // ʹ�̴߳ӵȴ��н��ѣ������˳�
    getRequestGroup().getRequestQueue().breakWaiting(threadList_.getCount());
}

//-----------------------------------------------------------------------------
// ����: �ȴ������߳��˳�
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::waitForAllThreads()
{
    terminateAllThreads();

    int killedCount = 0;
    threadList_.waitForAllThreads(MAX_THREAD_WAIT_FOR_SECS, &killedCount);

    if (killedCount)
        logger().writeFmt(SEM_THREAD_KILLED, killedCount, "udp worker");
}

//-----------------------------------------------------------------------------
// ����: ���� count ���߳�
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::createThreads(int count)
{
    for (int i = 0; i < count; i++)
    {
        UdpWorkerThread *thread;
        thread = new UdpWorkerThread(this);
        thread->run();
    }
}

//-----------------------------------------------------------------------------
// ����: ��ֹ count ���߳�
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::terminateThreads(int count)
{
    AutoLocker locker(threadList_.getLock());

    int termCount = 0;
    if (count > threadList_.getCount())
        count = threadList_.getCount();

    for (int i = threadList_.getCount() - 1; i >= 0; i--)
    {
        UdpWorkerThread *thread;
        thread = (UdpWorkerThread*)threadList_[i];
        if (thread->isIdle())
        {
            thread->terminate();
            termCount++;
            if (termCount >= count) break;
        }
    }
}

//-----------------------------------------------------------------------------
// ����: ����߳��Ƿ�����ʱ (��ʱ�߳�: ��ĳһ������빤��״̬������δ��ɵ��߳�)
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::checkThreadTimeout()
{
    AutoLocker locker(threadList_.getLock());

    for (int i = 0; i < threadList_.getCount(); i++)
    {
        UdpWorkerThread *thread;
        thread = (UdpWorkerThread*)threadList_[i];
        thread->getTimeoutChecker().check();
    }
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���������߳� (�����߳�: �ѱ�֪ͨ�˳������ò��˳����߳�)
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::killZombieThreads()
{
    AutoLocker locker(threadList_.getLock());

    for (int i = threadList_.getCount() - 1; i >= 0; i--)
    {
        UdpWorkerThread *thread;
        thread = (UdpWorkerThread*)threadList_[i];
        if (thread->getTermElapsedSecs() >= MAX_THREAD_TERM_SECS)
        {
            thread->kill();
            threadList_.remove(thread);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class UdpRequestGroup

UdpRequestGroup::UdpRequestGroup(MainUdpServer *ownMainUdpSvr, int groupIndex) :
    ownMainUdpSvr_(ownMainUdpSvr),
    groupIndex_(groupIndex),
    requestQueue_(this),
    threadPool_(this)
{
    // nothing
}

///////////////////////////////////////////////////////////////////////////////
// class MainUdpServer

MainUdpServer::MainUdpServer()
{
    initUdpServer();
    initRequestGroupList();
}

MainUdpServer::~MainUdpServer()
{
    clearRequestGroupList();
}

//-----------------------------------------------------------------------------
// ����: ����������
//-----------------------------------------------------------------------------
void MainUdpServer::open()
{
    udpServer_.open();
}

//-----------------------------------------------------------------------------
// ����: �رշ�����
//-----------------------------------------------------------------------------
void MainUdpServer::close()
{
    terminateAllWorkerThreads();
    waitForAllWorkerThreads();

    udpServer_.close();
}

//-----------------------------------------------------------------------------
// ����: ���ݸ��������̬�����������߳�����
//-----------------------------------------------------------------------------
void MainUdpServer::adjustWorkerThreadCount()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        requestGroupList_[i]->getThreadPool().AdjustThreadCount();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ���й������߳��˳�
//-----------------------------------------------------------------------------
void MainUdpServer::terminateAllWorkerThreads()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        requestGroupList_[i]->getThreadPool().terminateAllThreads();
}

//-----------------------------------------------------------------------------
// ����: �ȴ����й������߳��˳�
//-----------------------------------------------------------------------------
void MainUdpServer::waitForAllWorkerThreads()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        requestGroupList_[i]->getThreadPool().waitForAllThreads();
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� udpServer_
//-----------------------------------------------------------------------------
void MainUdpServer::initUdpServer()
{
    udpServer_.setRecvDataCallback(boost::bind(&MainUdpServer::onRecvData, this, _1, _2, _3));
}

//-----------------------------------------------------------------------------
// ����: ��ʼ����������б�
//-----------------------------------------------------------------------------
void MainUdpServer::initRequestGroupList()
{
    clearRequestGroupList();

    requestGroupCount_ = iseApp().getIseOptions().getUdpRequestGroupCount();
    for (int groupIndex = 0; groupIndex < requestGroupCount_; groupIndex++)
    {
        UdpRequestGroup *p;
        p = new UdpRequestGroup(this, groupIndex);
        requestGroupList_.push_back(p);
    }
}

//-----------------------------------------------------------------------------
// ����: �����������б�
//-----------------------------------------------------------------------------
void MainUdpServer::clearRequestGroupList()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        delete requestGroupList_[i];
    requestGroupList_.clear();
}

//-----------------------------------------------------------------------------
// ����: �յ����ݰ�
//-----------------------------------------------------------------------------
void MainUdpServer::onRecvData(void *packetBuffer, int packetSize, const InetAddress& peerAddr)
{
    int groupIndex;

    // �Ƚ������ݰ����࣬�õ�����
    iseApp().getIseBusiness().classifyUdpPacket(packetBuffer, packetSize, groupIndex);

    // ������źϷ�
    if (groupIndex >= 0 && groupIndex < requestGroupCount_)
    {
        UdpPacket *p = new UdpPacket();
        if (p)
        {
            p->recvTimeStamp_ = (UINT)time(NULL);
            p->peerAddr_ = peerAddr;
            p->packetSize_ = packetSize;
            p->setPacketBuffer(packetBuffer, packetSize);

            // ��ӵ����������
            requestGroupList_[groupIndex]->getRequestQueue().addPacket(p);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
