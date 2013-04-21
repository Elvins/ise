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
// �ļ�����: ise_server_tcp.cpp
// ��������: TCP��������ʵ��
///////////////////////////////////////////////////////////////////////////////

#include "ise_server_tcp.h"
#include "ise_errmsgs.h"
#include "ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// Ԥ�������ݰ��ֽ���

void bytePacketSplitter(const char *data, int bytes, int& splitBytes)
{
    splitBytes = (bytes > 0 ? 1 : 0);
}

void linePacketSplitter(const char *data, int bytes, int& splitBytes)
{
    splitBytes = 0;

    const char *p = data;
    int i = 0;
    while (i < bytes)
    {
        if (*p == '\r' || *p == '\n')
        {
            splitBytes = i + 1;
            if (i < bytes - 1)
            {
                char next = *(p+1);
                if ((next == '\r' || next == '\n') && next != *p)
                    ++splitBytes;
            }
            break;
        }

        ++p;
        ++i;
    }
}

//-----------------------------------------------------------------------------

void nullTerminatedPacketSplitter(const char *data, int bytes, int& splitBytes)
{
    const char DELIMITER = '\0';

    splitBytes = 0;
    for (int i = 0; i < bytes; ++i)
    {
        if (data[i] == DELIMITER)
        {
            splitBytes = i + 1;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class IoBuffer

IoBuffer::IoBuffer() :
    buffer_(INITIAL_SIZE),
    readerIndex_(0),
    writerIndex_(0)
{
    // nothing
}

IoBuffer::~IoBuffer()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: �򻺴�׷��д������
//-----------------------------------------------------------------------------
void IoBuffer::append(const string& str)
{
    append(str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------
// ����: �򻺴�׷��д������
//-----------------------------------------------------------------------------
void IoBuffer::append(const void *data, int bytes)
{
    if (data && bytes > 0)
    {
        if (getWritableBytes() < bytes)
            makeSpace(bytes);

        ISE_ASSERT(getWritableBytes() >= bytes);

        memmove(getWriterPtr(), data, bytes);
        writerIndex_ += bytes;
    }
}

//-----------------------------------------------------------------------------
// ����: �򻺴�׷�� bytes ���ֽڲ����Ϊ'\0'
//-----------------------------------------------------------------------------
void IoBuffer::append(int bytes)
{
    if (bytes > 0)
    {
        string str;
        str.resize(bytes, 0);
        append(str);
    }
}

//-----------------------------------------------------------------------------
// ����: �ӻ����ȡ bytes ���ֽ�����
//-----------------------------------------------------------------------------
void IoBuffer::retrieve(int bytes)
{
    if (bytes > 0)
    {
        ISE_ASSERT(bytes <= getReadableBytes());
        readerIndex_ += bytes;
    }
}

//-----------------------------------------------------------------------------
// ����: �ӻ����ȡȫ���ɶ����ݲ����� str ��
//-----------------------------------------------------------------------------
void IoBuffer::retrieveAll(string& str)
{
    if (getReadableBytes() > 0)
        str.assign(peek(), getReadableBytes());
    else
        str.clear();

    retrieveAll();
}

//-----------------------------------------------------------------------------
// ����: �ӻ����ȡȫ���ɶ�����
//-----------------------------------------------------------------------------
void IoBuffer::retrieveAll()
{
    readerIndex_ = 0;
    writerIndex_ = 0;
}

//-----------------------------------------------------------------------------

void IoBuffer::swap(IoBuffer& rhs)
{
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}

//-----------------------------------------------------------------------------
// ����: ��չ����ռ��Ա����д�� moreBytes ���ֽ�
//-----------------------------------------------------------------------------
void IoBuffer::makeSpace(int moreBytes)
{
    if (getWritableBytes() + getUselessBytes() < moreBytes)
    {
        buffer_.resize(writerIndex_ + moreBytes);
    }
    else
    {
        // ��ȫ���ɶ������������濪ʼ��
        int readableBytes = getReadableBytes();
        char *buffer = getBufferPtr();
        memmove(buffer, buffer + readerIndex_, readableBytes);
        readerIndex_ = 0;
        writerIndex_ = readerIndex_ + readableBytes;

        ISE_ASSERT(readableBytes == getReadableBytes());
    }
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopThread

TcpEventLoopThread::TcpEventLoopThread(TcpEventLoop& eventLoop) :
    eventLoop_(eventLoop)
{
    setFreeOnTerminate(false);
}

//-----------------------------------------------------------------------------

void TcpEventLoopThread::execute()
{
    eventLoop_.loopThreadId_ = getThreadId();
    eventLoop_.runLoop(this);
}

//-----------------------------------------------------------------------------

void TcpEventLoopThread::afterExecute()
{
    eventLoop_.loopThreadId_ = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop

TcpEventLoop::TcpEventLoop() :
    thread_(NULL),
    loopThreadId_(0)
{
    // nothing
}

TcpEventLoop::~TcpEventLoop()
{
    stop(false, true);
}

//-----------------------------------------------------------------------------
// ����: ���������߳�
//-----------------------------------------------------------------------------
void TcpEventLoop::start()
{
    if (!thread_)
    {
        thread_ = new TcpEventLoopThread(*this);
        thread_->run();
    }
}

//-----------------------------------------------------------------------------
// ����: ֹͣ�����߳�
//-----------------------------------------------------------------------------
void TcpEventLoop::stop(bool force, bool waitFor)
{
    if (thread_ && thread_->isRunning())
    {
        if (force)
        {
            thread_->kill();
            thread_ = NULL;
            waitFor = false;
        }
        else
        {
            thread_->terminate();
            wakeupLoop();
        }

        if (waitFor)
        {
            thread_->waitFor();
            delete thread_;
            thread_ = NULL;
        }
    }
}

//-----------------------------------------------------------------------------
// ����: �жϹ����̵߳�ǰ�Ƿ���������
//-----------------------------------------------------------------------------
bool TcpEventLoop::isRunning()
{
    return (thread_ != NULL && thread_->isRunning());
}

//-----------------------------------------------------------------------------
// ����: �жϵ�ǰ���ô˷������̺߳ʹ� eventLoop �����߳��ǲ���ͬһ���߳�
//-----------------------------------------------------------------------------
bool TcpEventLoop::isInLoopThread()
{
    return loopThreadId_ == getCurThreadId();
}

//-----------------------------------------------------------------------------
// ����: ���¼�ѭ���߳�������ִ��ָ���ķº���
// ��ע: �̰߳�ȫ
//-----------------------------------------------------------------------------
void TcpEventLoop::executeInLoop(const Functor& functor)
{
    if (isInLoopThread())
        functor();
    else
        delegateToLoop(functor);
}

//-----------------------------------------------------------------------------
// ����: ��ָ���ķº���ί�и��¼�ѭ���߳�ִ�С��߳�����ɵ�ǰһ���¼�ѭ������
//       ִ�б�ί�еķº�����
// ��ע: �̰߳�ȫ
//-----------------------------------------------------------------------------
void TcpEventLoop::delegateToLoop(const Functor& functor)
{
    {
        AutoLocker locker(delegatedFunctors_.lock);
        delegatedFunctors_.items.push_back(functor);
    }

    wakeupLoop();
}

//-----------------------------------------------------------------------------
// ����: ���һ�������� (finalizer) ���¼�ѭ���У���ÿ��ѭ��������ִ������
//-----------------------------------------------------------------------------
void TcpEventLoop::addFinalizer(const Functor& finalizer)
{
    AutoLocker locker(finalizers_.lock);
    finalizers_.items.push_back(finalizer);
}

//-----------------------------------------------------------------------------
// ����: ��ָ������ע�ᵽ�� eventLoop ��
//-----------------------------------------------------------------------------
void TcpEventLoop::addConnection(TcpConnection *connection)
{
    TcpConnectionPtr connPtr(connection);
    tcpConnMap_[connection->getConnectionName()] = connPtr;

    registerConnection(connection);
    delegateToLoop(boost::bind(&IseBusiness::onTcpConnect, &iseApp().getIseBusiness(), connPtr));
}

//-----------------------------------------------------------------------------
// ����: ��ָ�����ӴӴ� eventLoop ��ע��
//-----------------------------------------------------------------------------
void TcpEventLoop::removeConnection(TcpConnection *connection)
{
    unregisterConnection(connection);
    tcpConnMap_.erase(connection->getConnectionName());
}

//-----------------------------------------------------------------------------
// ����: ִ���¼�ѭ��
//-----------------------------------------------------------------------------
void TcpEventLoop::runLoop(Thread *thread)
{
    while (!thread->isTerminated())
    {
        doLoopWork(thread);
        executeDelegatedFunctors();
        executeFinalizer();
    }
}

//-----------------------------------------------------------------------------
// ����: ִ�б�ί�еķº���
//-----------------------------------------------------------------------------
void TcpEventLoop::executeDelegatedFunctors()
{
    Functors functors;
    {
        AutoLocker locker(delegatedFunctors_.lock);
        functors.swap(delegatedFunctors_.items);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
}

//-----------------------------------------------------------------------------
// ����: ִ������������
//-----------------------------------------------------------------------------
void TcpEventLoop::executeFinalizer()
{
    Functors finalizers;
    {
        AutoLocker locker(finalizers_.lock);
        finalizers.swap(finalizers_.items);
    }

    for (size_t i = 0; i < finalizers.size(); ++i)
        finalizers[i]();
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList

TcpEventLoopList::TcpEventLoopList(int loopCount) :
    items_(false, true)
{
    setCount(loopCount);
}

TcpEventLoopList::~TcpEventLoopList()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: ����ȫ�� eventLoop �Ĺ����߳�
//-----------------------------------------------------------------------------
void TcpEventLoopList::start()
{
    for (int i = 0; i < items_.getCount(); i++)
        items_[i]->start();
}

//-----------------------------------------------------------------------------
// ����: ֹͣȫ�� eventLoop �Ĺ����߳�
//-----------------------------------------------------------------------------
void TcpEventLoopList::stop()
{
    const int MAX_WAIT_FOR_SECS = 5;    // (��)
    const double SLEEP_INTERVAL = 0.5;  // (��)

    // ֹ֪ͨͣ
    for (int i = 0; i < items_.getCount(); i++)
        items_[i]->stop(false, false);

    // �ȴ�ֹͣ
    double waitSecs = 0;
    while (waitSecs < MAX_WAIT_FOR_SECS)
    {
        int runningCount = 0;
        for (int i = 0; i < items_.getCount(); i++)
            if (items_[i]->isRunning()) runningCount++;

        if (runningCount == 0) break;

        sleepSec(SLEEP_INTERVAL, true);
        waitSecs += SLEEP_INTERVAL;
    }

    // ǿ��ֹͣ
    for (int i = 0; i < items_.getCount(); i++)
        items_[i]->stop(true, true);
}

//-----------------------------------------------------------------------------
// ����: ���� eventLoop �ĸ���
//-----------------------------------------------------------------------------
void TcpEventLoopList::setCount(int count)
{
    count = ensureRange(count, 1, (int)MAX_LOOP_COUNT);

    for (int i = 0; i < count; i++)
    {
#ifdef ISE_WINDOWS
        items_.add(new WinTcpEventLoop());
#endif
#ifdef ISE_LINUX
        items_.add(new LinuxTcpEventLoop());
#endif
    }
}

///////////////////////////////////////////////////////////////////////////////
// class TcpServer

//-----------------------------------------------------------------------------
// ����: �������Ӷ���
//-----------------------------------------------------------------------------
BaseTcpConnection* TcpServer::createConnection(SOCKET socketHandle)
{
    BaseTcpConnection *result = NULL;
    string connectionName = generateConnectionName(socketHandle);

#ifdef ISE_WINDOWS
    result = new WinTcpConnection(this, socketHandle, connectionName);
#endif
#ifdef ISE_LINUX
    result = new LinuxTcpConnection(this, socketHandle, connectionName);
#endif

    return result;
}

//-----------------------------------------------------------------------------
// ����: ����һ�� TcpServer ��Χ��Ψһ����������
//-----------------------------------------------------------------------------
string TcpServer::generateConnectionName(SOCKET socketHandle)
{
    string result = formatString("%s-%s#%u",
        getSocket().getLocalAddr().getDisplayStr().c_str(),
        getSocketPeerAddr(socketHandle).getDisplayStr().c_str(),
        (UINT)connIdAlloc_.allocId());
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection

TcpConnection::TcpConnection(TcpServer *tcpServer, SOCKET socketHandle,
    const string& connectionName) :
        BaseTcpConnection(socketHandle),
        tcpServer_(tcpServer),
        eventLoop_(NULL),
        connectionName_(connectionName)
{
    tcpServer_->incConnCount();
}

TcpConnection::~TcpConnection()
{
    logger().writeFmt("destroy conn: %s", getConnectionName().c_str());  // debug

    setEventLoop(NULL);
    tcpServer_->decConnCount();
}

//-----------------------------------------------------------------------------
// ����: �ύһ���������� (�̰߳�ȫ)
//-----------------------------------------------------------------------------
void TcpConnection::send(const void *buffer, int size, const Context& context)
{
    if (!buffer || size <= 0) return;

    if (getEventLoop()->isInLoopThread())
        postSendTask(buffer, size, context);
    else
    {
        string data((const char*)buffer, size);
        getEventLoop()->delegateToLoop(
            boost::bind(&TcpConnection::postSendTask, this, data, context));
    }
}

//-----------------------------------------------------------------------------
// ����: �ύһ���������� (�̰߳�ȫ)
//-----------------------------------------------------------------------------
void TcpConnection::recv(const PacketSplitter& packetSplitter, const Context& context)
{
    if (!packetSplitter) return;

    if (getEventLoop()->isInLoopThread())
        postRecvTask(packetSplitter, context);
    else
    {
        getEventLoop()->delegateToLoop(
            boost::bind(&TcpConnection::postRecvTask, this, packetSplitter, context));
    }
}

//-----------------------------------------------------------------------------
// ����: �Ͽ�����
// ��ע:
//   ���ֱ�� close socket��Linux�� EPoll ���������֪ͨ���Ӷ��޷��������
//   �� shutdown ��û�����⡣�� Windows �£������� close ���� shutdown��
//   ֻҪ�����ϴ��ڽ��ջ��Ͷ�����IOCP �����Բ������
//-----------------------------------------------------------------------------
void TcpConnection::doDisconnect()
{
    getSocket().shutdown();

    // ���û������� recv �������ȷ������������
    recv(BYTE_PACKET_SPLITTER);
}

//-----------------------------------------------------------------------------
// ����: ���ӷ����˴���
// ��ע:
//   �����ڷ����������û�����ü�ֵ��Ӧ�����٣�����Ӧ�������٣���Ϊ�Ժ�ִ�е�
//   ί�и��¼�ѭ���ķº����п��ܴ��ڶԴ����ӵĻص������ԣ�Ӧ���� addFinalizer()
//   �ķ�ʽ���ٴν����ٶ��������ί�и��¼�ѭ������ÿ��ѭ����ĩβִ�С�
//-----------------------------------------------------------------------------
void TcpConnection::errorOccurred()
{
    ISE_ASSERT(eventLoop_ != NULL);

    getSocket().shutdown();

    getEventLoop()->executeInLoop(
        boost::bind(&IseBusiness::onTcpDisconnect,
        &iseApp().getIseBusiness(), shared_from_this()));

    // setEventLoop(NULL) ��ʹ shared_ptr<TcpConnection> �������ü������������ٶ���
    getEventLoop()->addFinalizer(boost::bind(&TcpConnection::setEventLoop, this, (TcpEventLoop*)NULL));
}

//-----------------------------------------------------------------------------

void TcpConnection::postSendTask(const string& data, const Context& context)
{
    postSendTask(data.c_str(), (int)data.size(), context);
}

//-----------------------------------------------------------------------------
// ����: ���ô����Ӵ������ĸ� eventLoop
//-----------------------------------------------------------------------------
void TcpConnection::setEventLoop(TcpEventLoop *eventLoop)
{
    if (eventLoop_)
    {
        TcpEventLoop *temp = eventLoop_;
        eventLoop_ = NULL;
        temp->removeConnection(this);
    }

    if (eventLoop)
    {
        eventLoop_ = eventLoop;
        eventLoop->addConnection(this);
    }
}

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// class WinTcpConnection

WinTcpConnection::WinTcpConnection(TcpServer *tcpServer, SOCKET socketHandle,
    const string& connectionName) :
        TcpConnection(tcpServer, socketHandle, connectionName),
        isSending_(false),
        isRecving_(false),
        bytesSent_(0),
        bytesRecved_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void WinTcpConnection::postSendTask(const void *buffer, int size, const Context& context)
{
    sendBuffer_.append(buffer, size);

    SendTask task;
    task.bytes = size;
    task.context = context;

    sendTaskQueue_.push_back(task);

    trySend();
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void WinTcpConnection::postRecvTask(const PacketSplitter& packetSplitter, const Context& context)
{
    RecvTask task;
    task.packetSplitter = packetSplitter;
    task.context = context;

    recvTaskQueue_.push_back(task);

    tryRecv();
}

//-----------------------------------------------------------------------------

void WinTcpConnection::trySend()
{
    if (isSending_) return;

    int readableBytes = sendBuffer_.getReadableBytes();
    if (readableBytes > 0)
    {
        const int MAX_SEND_SIZE = 1024*32;

        const char *buffer = sendBuffer_.peek();
        int sendSize = ise::min(readableBytes, MAX_SEND_SIZE);

        isSending_ = true;
        getEventLoop()->getIocpObject()->send(getSocket().getHandle(),
            (PVOID)buffer, sendSize, 0,
            boost::bind(&WinTcpConnection::onIocpCallback, this, _1),
            this, EMPTY_CONTEXT);
    }
}

//-----------------------------------------------------------------------------

void WinTcpConnection::tryRecv()
{
    if (isRecving_) return;

    if (!recvTaskQueue_.empty())
    {
        const int MAX_RECV_SIZE = 1024*16;

        isRecving_ = true;
        recvBuffer_.append(MAX_RECV_SIZE);
        const char *buffer = recvBuffer_.peek() + bytesRecved_;

        getEventLoop()->getIocpObject()->recv(getSocket().getHandle(),
            (PVOID)buffer, MAX_RECV_SIZE, 0,
            boost::bind(&WinTcpConnection::onIocpCallback, this, _1),
            this, EMPTY_CONTEXT);
    }
}

//-----------------------------------------------------------------------------

void WinTcpConnection::onIocpCallback(const IocpTaskData& taskData)
{
    if (taskData.getErrorCode() == 0)
    {
        switch (taskData.getTaskType())
        {
        case ITT_SEND:
            onSendCallback(taskData);
            break;
        case ITT_RECV:
            onRecvCallback(taskData);
            break;
        }
    }
    else
    {
        errorOccurred();
    }
}

//-----------------------------------------------------------------------------

void WinTcpConnection::onSendCallback(const IocpTaskData& taskData)
{
    ISE_ASSERT(taskData.getErrorCode() == 0);

    if (taskData.getBytesTrans() < taskData.getDataSize())
    {
        getEventLoop()->getIocpObject()->send(
            (SOCKET)taskData.getFileHandle(),
            taskData.getEntireDataBuf(),
            taskData.getEntireDataSize(),
            taskData.getDataBuf() - taskData.getEntireDataBuf() + taskData.getBytesTrans(),
            taskData.getCallback(), taskData.getCaller(), taskData.getContext());
    }
    else
    {
        isSending_ = false;
        sendBuffer_.retrieve(taskData.getEntireDataSize());
    }

    bytesSent_ += taskData.getBytesTrans();

    while (!sendTaskQueue_.empty())
    {
        const SendTask& task = sendTaskQueue_.front();
        if (bytesSent_ >= task.bytes)
        {
            bytesSent_ -= task.bytes;
            iseApp().getIseBusiness().onTcpSendComplete(shared_from_this(), task.context);
            sendTaskQueue_.pop_front();
        }
        else
            break;
    }

    if (!sendTaskQueue_.empty())
        trySend();
}

//-----------------------------------------------------------------------------

void WinTcpConnection::onRecvCallback(const IocpTaskData& taskData)
{
    ISE_ASSERT(taskData.getErrorCode() == 0);

    if (taskData.getBytesTrans() < taskData.getDataSize())
    {
        getEventLoop()->getIocpObject()->recv(
            (SOCKET)taskData.getFileHandle(),
            taskData.getEntireDataBuf(),
            taskData.getEntireDataSize(),
            taskData.getDataBuf() - taskData.getEntireDataBuf() + taskData.getBytesTrans(),
            taskData.getCallback(), taskData.getCaller(), taskData.getContext());
    }
    else
    {
        isRecving_ = false;
    }

    bytesRecved_ += taskData.getBytesTrans();

    while (!recvTaskQueue_.empty())
    {
        RecvTask& task = recvTaskQueue_.front();
        const char *buffer = recvBuffer_.peek();
        bool packetRecved = false;

        if (bytesRecved_ > 0)
        {
            int packetSize = 0;
            task.packetSplitter(buffer, bytesRecved_, packetSize);
            if (packetSize > 0)
            {
                bytesRecved_ -= packetSize;
                iseApp().getIseBusiness().onTcpRecvComplete(shared_from_this(),
                    (void*)buffer, packetSize, task.context);
                recvTaskQueue_.pop_front();
                recvBuffer_.retrieve(packetSize);
                packetRecved = true;
            }
        }

        if (!packetRecved)
            break;
    }

    if (!recvTaskQueue_.empty())
        tryRecv();
}

///////////////////////////////////////////////////////////////////////////////
// class IocpTaskData

IocpTaskData::IocpTaskData() :
    iocpHandle_(INVALID_HANDLE_VALUE),
    fileHandle_(INVALID_HANDLE_VALUE),
    taskType_((IOCP_TASK_TYPE)0),
    taskSeqNum_(0),
    caller_(0),
    entireDataBuf_(0),
    entireDataSize_(0),
    bytesTrans_(0),
    errorCode_(0)
{
    wsaBuffer_.buf = NULL;
    wsaBuffer_.len = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class IocpBufferAllocator

IocpBufferAllocator::IocpBufferAllocator(int bufferSize) :
    bufferSize_(bufferSize),
    usedCount_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------

IocpBufferAllocator::~IocpBufferAllocator()
{
    clear();
}

//-----------------------------------------------------------------------------

PVOID IocpBufferAllocator::allocBuffer()
{
    AutoLocker locker(lock_);
    PVOID result;

    if (!items_.isEmpty())
    {
        result = items_.last();
        items_.del(items_.getCount() - 1);
    }
    else
    {
        result = new char[bufferSize_];
        if (result == NULL)
            iseThrowMemoryException();
    }

    usedCount_++;
    return result;
}

//-----------------------------------------------------------------------------

void IocpBufferAllocator::returnBuffer(PVOID buffer)
{
    AutoLocker locker(lock_);

    if (buffer != NULL && items_.indexOf(buffer) == -1)
    {
        items_.add(buffer);
        usedCount_--;
    }
}

//-----------------------------------------------------------------------------

void IocpBufferAllocator::clear()
{
    AutoLocker locker(lock_);

    for (int i = 0; i < items_.getCount(); i++)
        delete[] (char*)items_[i];
    items_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

void IocpPendingCounter::inc(PVOID caller, IOCP_TASK_TYPE taskType)
{
    AutoLocker locker(lock_);

    Items::iterator iter = items_.find(caller);
    if (iter == items_.end())
    {
        CountData Data = {0, 0};
        iter = items_.insert(std::make_pair(caller, Data)).first;
    }

    if (taskType == ITT_SEND)
        iter->second.sendCount++;
    else if (taskType == ITT_RECV)
        iter->second.recvCount++;
}

//-----------------------------------------------------------------------------

void IocpPendingCounter::dec(PVOID caller, IOCP_TASK_TYPE taskType)
{
    AutoLocker locker(lock_);

    Items::iterator iter = items_.find(caller);
    if (iter != items_.end())
    {
        if (taskType == ITT_SEND)
            iter->second.sendCount--;
        else if (taskType == ITT_RECV)
            iter->second.recvCount--;

        if (iter->second.sendCount <= 0 && iter->second.recvCount <= 0)
            items_.erase(iter);
    }
}

//-----------------------------------------------------------------------------

int IocpPendingCounter::get(PVOID caller)
{
    AutoLocker locker(lock_);

    Items::iterator iter = items_.find(caller);
    if (iter == items_.end())
        return 0;
    else
        return ise::max(0, iter->second.sendCount + iter->second.recvCount);
}

//-----------------------------------------------------------------------------

int IocpPendingCounter::get(IOCP_TASK_TYPE taskType)
{
    AutoLocker locker(lock_);

    int result = 0;
    if (taskType == ITT_SEND)
    {
        for (Items::iterator iter = items_.begin(); iter != items_.end(); ++iter)
            result += iter->second.sendCount;
    }
    else if (taskType == ITT_RECV)
    {
        for (Items::iterator iter = items_.begin(); iter != items_.end(); ++iter)
            result += iter->second.recvCount;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

IocpBufferAllocator IocpObject::bufferAlloc_(sizeof(CIocpOverlappedData));
SeqNumberAlloc IocpObject::taskSeqAlloc_(0);
IocpPendingCounter IocpObject::pendingCounter_;

//-----------------------------------------------------------------------------

IocpObject::IocpObject() :
    iocpHandle_(0)
{
    initialize();
}

//-----------------------------------------------------------------------------

IocpObject::~IocpObject()
{
    finalize();
}

//-----------------------------------------------------------------------------

bool IocpObject::associateHandle(SOCKET socketHandle)
{
    HANDLE h = ::CreateIoCompletionPort((HANDLE)socketHandle, iocpHandle_, 0, 0);
    return (h != 0);
}

//-----------------------------------------------------------------------------

bool IocpObject::isComplete(PVOID caller)
{
    return (pendingCounter_.get(caller) <= 0);
}

//-----------------------------------------------------------------------------

void IocpObject::work()
{
    const int IOCP_WAIT_TIMEOUT = 1000*1;  // ms

    CIocpOverlappedData *overlappedPtr = NULL;
    DWORD bytesTransferred = 0, nTemp = 0;
    int errorCode = 0;

    struct AutoFinalizer
    {
    private:
        IocpObject& iocpObject_;
        CIocpOverlappedData*& ovPtr_;
    public:
        AutoFinalizer(IocpObject& iocpObject, CIocpOverlappedData*& ovPtr) :
            iocpObject_(iocpObject), ovPtr_(ovPtr) {}
        ~AutoFinalizer()
        {
            if (ovPtr_)
            {
                iocpObject_.pendingCounter_.dec(
                    ovPtr_->taskData.getCaller(),
                    ovPtr_->taskData.getTaskType());
                iocpObject_.destroyOverlappedData(ovPtr_);
            }
        }
    } finalizer(*this, overlappedPtr);

    /*
    FROM MSDN:

    If the function dequeues a completion packet for a successful I/O operation from the completion port,
    the return value is nonzero. The function stores information in the variables pointed to by the
    lpNumberOfBytes, lpCompletionKey, and lpOverlapped parameters.

    If *lpOverlapped is NULL and the function does not dequeue a completion packet from the completion port,
    the return value is zero. The function does not store information in the variables pointed to by the
    lpNumberOfBytes and lpCompletionKey parameters. To get extended error information, call GetLastError.
    If the function did not dequeue a completion packet because the wait timed out, GetLastError returns
    WAIT_TIMEOUT.

    If *lpOverlapped is not NULL and the function dequeues a completion packet for a failed I/O operation
    from the completion port, the return value is zero. The function stores information in the variables
    pointed to by lpNumberOfBytes, lpCompletionKey, and lpOverlapped. To get extended error information,
    call GetLastError.

    If a socket handle associated with a completion port is closed, GetQueuedCompletionStatus returns
    ERROR_SUCCESS (0), with *lpOverlapped non-NULL and lpNumberOfBytes equal zero.
    */

    if (::GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &nTemp,
        (LPOVERLAPPED*)&overlappedPtr, IOCP_WAIT_TIMEOUT))
    {
        if (overlappedPtr != NULL && bytesTransferred == 0)
        {
            errorCode = overlappedPtr->taskData.getErrorCode();
            if (errorCode == 0)
                errorCode = GetLastError();
            if (errorCode == 0)
                errorCode = SOCKET_ERROR;
        }
    }
    else
    {
        if (overlappedPtr != NULL)
            errorCode = GetLastError();
        else
        {
            if (GetLastError() != WAIT_TIMEOUT)
                throwGeneralError();
        }
    }

    if (overlappedPtr != NULL)
    {
        IocpTaskData *taskPtr = &overlappedPtr->taskData;
        taskPtr->bytesTrans_ = bytesTransferred;
        if (taskPtr->errorCode_ == 0)
            taskPtr->errorCode_ = errorCode;

        invokeCallback(*taskPtr);
    }
}

//-----------------------------------------------------------------------------

void IocpObject::wakeup()
{
    ::PostQueuedCompletionStatus(iocpHandle_, 0, 0, NULL);
}

//-----------------------------------------------------------------------------

void IocpObject::send(SOCKET socketHandle, PVOID buffer, int size, int offset,
    const IocpCallback& callback, PVOID caller, const Context& context)
{
    CIocpOverlappedData *ovDataPtr;
    IocpTaskData *taskPtr;
    DWORD numberOfBytesSent;

    pendingCounter_.inc(caller, ITT_SEND);

    ovDataPtr = createOverlappedData(ITT_SEND, (HANDLE)socketHandle, buffer, size,
        offset, callback, caller, context);
    taskPtr = &(ovDataPtr->taskData);

    if (::WSASend(socketHandle, &taskPtr->wsaBuffer_, 1, &numberOfBytesSent, 0,
        (LPWSAOVERLAPPED)ovDataPtr, NULL) == SOCKET_ERROR)
    {
        if (GetLastError() != ERROR_IO_PENDING)
            postError(GetLastError(), ovDataPtr);
    }
}

//-----------------------------------------------------------------------------

void IocpObject::recv(SOCKET socketHandle, PVOID buffer, int size, int offset,
    const IocpCallback& callback, PVOID caller, const Context& context)
{
    CIocpOverlappedData *ovDataPtr;
    IocpTaskData *taskPtr;
    DWORD nNumberOfBytesRecvd, flags = 0;

    pendingCounter_.inc(caller, ITT_RECV);

    ovDataPtr = createOverlappedData(ITT_RECV, (HANDLE)socketHandle, buffer, size,
        offset, callback, caller, context);
    taskPtr = &(ovDataPtr->taskData);

    if (::WSARecv(socketHandle, &taskPtr->wsaBuffer_, 1, &nNumberOfBytesRecvd, &flags,
        (LPWSAOVERLAPPED)ovDataPtr, NULL) == SOCKET_ERROR)
    {
        if (GetLastError() != ERROR_IO_PENDING)
            postError(GetLastError(), ovDataPtr);
    }
}

//-----------------------------------------------------------------------------

void IocpObject::initialize()
{
    iocpHandle_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);
    if (iocpHandle_ == 0)
        throwGeneralError();
}

//-----------------------------------------------------------------------------

void IocpObject::finalize()
{
    CloseHandle(iocpHandle_);
    iocpHandle_ = 0;
}

//-----------------------------------------------------------------------------

void IocpObject::throwGeneralError()
{
    iseThrowException(formatString(SEM_IOCP_ERROR, GetLastError()).c_str());
}

//-----------------------------------------------------------------------------

CIocpOverlappedData* IocpObject::createOverlappedData(IOCP_TASK_TYPE taskType,
    HANDLE fileHandle, PVOID buffer, int size, int offset,
    const IocpCallback& callback, PVOID caller, const Context& context)
{
    ISE_ASSERT(buffer != NULL);
    ISE_ASSERT(size >= 0);
    ISE_ASSERT(offset >= 0);
    ISE_ASSERT(offset < size);

    CIocpOverlappedData *result = (CIocpOverlappedData*)bufferAlloc_.allocBuffer();
    memset(result, 0, sizeof(*result));

    result->taskData.iocpHandle_ = iocpHandle_;
    result->taskData.fileHandle_ = fileHandle;
    result->taskData.taskType_ = taskType;
    result->taskData.taskSeqNum_ = taskSeqAlloc_.allocId();
    result->taskData.caller_ = caller;
    result->taskData.context_ = context;
    result->taskData.entireDataBuf_ = buffer;
    result->taskData.entireDataSize_ = size;
    result->taskData.wsaBuffer_.buf = (char*)buffer + offset;
    result->taskData.wsaBuffer_.len = size - offset;
    result->taskData.callback_ = callback;

    return result;
}

//-----------------------------------------------------------------------------

void IocpObject::destroyOverlappedData(CIocpOverlappedData *ovDataPtr)
{
    bufferAlloc_.returnBuffer(ovDataPtr);
}

//-----------------------------------------------------------------------------

void IocpObject::postError(int errorCode, CIocpOverlappedData *ovDataPtr)
{
    ovDataPtr->taskData.errorCode_ = errorCode;
    ::PostQueuedCompletionStatus(iocpHandle_, 0, 0, LPOVERLAPPED(ovDataPtr));
}

//-----------------------------------------------------------------------------

void IocpObject::invokeCallback(const IocpTaskData& taskData)
{
    const IocpCallback& callback = taskData.getCallback();
    if (callback)
        callback(taskData);
}

///////////////////////////////////////////////////////////////////////////////
// class WinTcpEventLoop

WinTcpEventLoop::WinTcpEventLoop()
{
    iocpObject_ = new IocpObject();
}

WinTcpEventLoop::~WinTcpEventLoop()
{
    stop(false, true);
    delete iocpObject_;
}

//-----------------------------------------------------------------------------
// ����: ִ�е����¼�ѭ���еĹ���
//-----------------------------------------------------------------------------
void WinTcpEventLoop::doLoopWork(Thread *thread)
{
    iocpObject_->work();
}

//-----------------------------------------------------------------------------
// ����: �����¼�ѭ���е���������
//-----------------------------------------------------------------------------
void WinTcpEventLoop::wakeupLoop()
{
    iocpObject_->wakeup();
}

//-----------------------------------------------------------------------------
// ����: ��������ע�ᵽ�¼�ѭ����
//-----------------------------------------------------------------------------
void WinTcpEventLoop::registerConnection(TcpConnection *connection)
{
    iocpObject_->associateHandle(connection->getSocket().getHandle());
}

//-----------------------------------------------------------------------------
// ����: ���¼�ѭ����ע������
//-----------------------------------------------------------------------------
void WinTcpEventLoop::unregisterConnection(TcpConnection *connection)
{
    // nothing
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WINDOWS */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpConnection

LinuxTcpConnection::LinuxTcpConnection(TcpServer *tcpServer, SOCKET socketHandle,
    const string& connectionName) :
        TcpConnection(tcpServer, socketHandle, connectionName),
        bytesSent_(0),
        enableSend_(false),
        enableRecv_(false)
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void LinuxTcpConnection::postSendTask(const void *buffer, int size, const Context& context)
{
    sendBuffer_.append(buffer, size);

    SendTask task;
    task.bytes = size;
    task.context = context;

    sendTaskQueue_.push_back(task);

    if (!enableSend_)
        setSendEnabled(true);
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void LinuxTcpConnection::postRecvTask(const PacketSplitter& packetSplitter, const Context& context)
{
    RecvTask task;
    task.packetSplitter = packetSplitter;
    task.context = context;

    recvTaskQueue_.push_back(task);

    if (!enableRecv_)
        setRecvEnabled(true);
}

//-----------------------------------------------------------------------------
// ����: ���á��Ƿ���ӿɷ����¼���
//-----------------------------------------------------------------------------
void LinuxTcpConnection::setSendEnabled(bool enabled)
{
    ISE_ASSERT(eventLoop_ != NULL);

    enableSend_ = enabled;
    getEventLoop()->updateConnection(this, enableSend_, enableRecv_);
}

//-----------------------------------------------------------------------------
// ����: ���á��Ƿ���ӿɽ����¼���
//-----------------------------------------------------------------------------
void LinuxTcpConnection::setRecvEnabled(bool enabled)
{
    ISE_ASSERT(eventLoop_ != NULL);

    enableRecv_ = enabled;
    getEventLoop()->updateConnection(this, enableSend_, enableRecv_);
}

//-----------------------------------------------------------------------------
// ����: �����ɷ��͡��¼�����ʱ�����Է�������
//-----------------------------------------------------------------------------
void LinuxTcpConnection::trySend()
{
    int readableBytes = sendBuffer_.getReadableBytes();
    if (readableBytes <= 0)
    {
        setSendEnabled(false);
        return;
    }

    const char *buffer = sendBuffer_.peek();
    int bytesSent = sendBuffer((void*)buffer, readableBytes, false);
    if (bytesSent < 0)
    {
        errorOccurred();
        return;
    }

    if (bytesSent > 0)
    {
        sendBuffer_.retrieve(bytesSent);
        bytesSent_ += bytesSent;

        while (!sendTaskQueue_.empty())
        {
            const SendTask& task = sendTaskQueue_.front();
            if (bytesSent_ >= task.bytes)
            {
                bytesSent_ -= task.bytes;
                iseApp().getIseBusiness().onTcpSendComplete(shared_from_this(), task.context);
                sendTaskQueue_.pop_front();
            }
            else
                break;
        }
    }
}

//-----------------------------------------------------------------------------
// ����: �����ɽ��ա��¼�����ʱ�����Խ�������
//-----------------------------------------------------------------------------
void LinuxTcpConnection::tryRecv()
{
    if (recvTaskQueue_.empty())
    {
        setRecvEnabled(false);
        return;
    }

    const int BUFFER_SIZE = 1024*16;
    char dataBuf[BUFFER_SIZE];

    int bytesRecved = recvBuffer(dataBuf, BUFFER_SIZE, false);
    if (bytesRecved < 0)
    {
        errorOccurred();
        return;
    }

    if (bytesRecved > 0)
        recvBuffer_.append(dataBuf, bytesRecved);

    while (!recvTaskQueue_.empty())
    {
        RecvTask& task = recvTaskQueue_.front();
        const char *buffer = recvBuffer_.peek();
        int readableBytes = recvBuffer_.getReadableBytes();
        bool packetRecved = false;

        if (readableBytes > 0)
        {
            int packetSize = 0;
            task.packetSplitter(buffer, readableBytes, packetSize);
            if (packetSize > 0)
            {
                iseApp().getIseBusiness().onTcpRecvComplete(shared_from_this(),
                    (void*)buffer, packetSize, task.context);
                recvTaskQueue_.pop_front();
                recvBuffer_.retrieve(packetSize);
                packetRecved = true;
            }
        }

        if (!packetRecved)
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// class EpollObject

EpollObject::EpollObject()
{
    events_.resize(INITIAL_EVENT_SIZE);
    createEpoll();
    createPipe();
}

EpollObject::~EpollObject()
{
    destroyPipe();
    destroyEpoll();
}

//-----------------------------------------------------------------------------
// ����: ִ��һ�� EPoll ��ѭ
//-----------------------------------------------------------------------------
void EpollObject::poll()
{
    const int EPOLL_WAIT_TIMEOUT = 1000*1;  // ms

    int eventCount = ::epoll_wait(epollFd_, &events_[0], (int)events_.size(), EPOLL_WAIT_TIMEOUT);
    if (eventCount > 0)
    {
        processEvents(eventCount);

        if (eventCount == (int)events_.size())
            events_.resize(eventCount * 2);
    }
    else if (eventCount < 0)
    {
        logger().writeStr(SEM_EPOLL_WAIT_ERROR);
    }
}

//-----------------------------------------------------------------------------
// ����: �������������� Poll() ����
//-----------------------------------------------------------------------------
void EpollObject::wakeup()
{
    BYTE val = 0;
    ::write(pipeFds_[1], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// ����: �� EPoll �����һ������
//-----------------------------------------------------------------------------
void EpollObject::addConnection(TcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollControl(
        EPOLL_CTL_ADD, connection, connection->getSocket().getHandle(),
        enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// ����: ���� EPoll �е�һ������
//-----------------------------------------------------------------------------
void EpollObject::updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollControl(
        EPOLL_CTL_MOD, connection, connection->getSocket().getHandle(),
        enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// ����: �� EPoll ��ɾ��һ������
//-----------------------------------------------------------------------------
void EpollObject::removeConnection(TcpConnection *connection)
{
    epollControl(
        EPOLL_CTL_DEL, connection, connection->getSocket().getHandle(),
        false, false);
}

//-----------------------------------------------------------------------------
// ����: ���ûص�
//-----------------------------------------------------------------------------
void EpollObject::setNotifyEventCallback(const NotifyEventCallback& callback)
{
    onNotifyEvent_ = callback;
}

//-----------------------------------------------------------------------------

void EpollObject::createEpoll()
{
    epollFd_ = ::epoll_create(1024);
    if (epollFd_ < 0)
        logger().writeStr(SEM_CREATE_EPOLL_ERROR);
}

//-----------------------------------------------------------------------------

void EpollObject::destroyEpoll()
{
    if (epollFd_ > 0)
        ::close(epollFd_);
}

//-----------------------------------------------------------------------------

void EpollObject::createPipe()
{
    // pipeFds_[0] for reading, pipeFds_[1] for writing.
    memset(pipeFds_, 0, sizeof(pipeFds_));
    if (::pipe(pipeFds_) == 0)
        epollControl(EPOLL_CTL_ADD, NULL, pipeFds_[0], false, true);
    else
        logger().writeStr(SEM_CREATE_PIPE_ERROR);
}

//-----------------------------------------------------------------------------

void EpollObject::destroyPipe()
{
    epollControl(EPOLL_CTL_DEL, NULL, pipeFds_[0], false, false);

    if (pipeFds_[0]) close(pipeFds_[0]);
    if (pipeFds_[1]) close(pipeFds_[1]);

    memset(pipeFds_, 0, sizeof(pipeFds_));
}

//-----------------------------------------------------------------------------

void EpollObject::epollControl(int operation, void *param, int handle,
    bool enableSend, bool enableRecv)
{
    // ע: ���� Level Triggered (LT, Ҳ�� "��ƽ����") ģʽ

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = param;
    if (enableSend)
        event.events |= EPOLLOUT;
    if (enableRecv)
        event.events |= (EPOLLIN | EPOLLPRI);

    if (::epoll_ctl(epollFd_, operation, handle, &event) < 0)
    {
        logger().writeFmt(SEM_EPOLL_CTRL_ERROR, operation);
    }
}

//-----------------------------------------------------------------------------
// ����: ����ܵ��¼�
//-----------------------------------------------------------------------------
void EpollObject::processPipeEvent()
{
    BYTE val;
    ::read(pipeFds_[0], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// ����: ���� EPoll ��ѭ����¼�
//-----------------------------------------------------------------------------
void EpollObject::processEvents(int eventCount)
{
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

    for (int i = 0; i < eventCount; i++)
    {
        epoll_event& ev = events_[i];
        if (ev.data.ptr == NULL)  // for pipe
        {
            processPipeEvent();
        }
        else
        {
            TcpConnection *connection = (TcpConnection*)ev.data.ptr;
            EVENT_TYPE eventType = ET_NONE;

            if (ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                eventType = ET_ERROR;
            else if (ev.events & (EPOLLIN | EPOLLPRI))
                eventType = ET_ALLOW_RECV;
            else if (ev.events & EPOLLOUT)
                eventType = ET_ALLOW_SEND;

            if (eventType != ET_NONE && onNotifyEvent_)
                onNotifyEvent_(connection, eventType);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpEventLoop

LinuxTcpEventLoop::LinuxTcpEventLoop()
{
    epollObject_ = new EpollObject();
    epollObject_->setNotifyEventCallback(boost::bind(&LinuxTcpEventLoop::onEpollNotifyEvent, this, _1, _2));
}

LinuxTcpEventLoop::~LinuxTcpEventLoop()
{
    stop(false, true);
    delete epollObject_;
}

//-----------------------------------------------------------------------------
// ����: ���´� eventLoop �е�ָ�����ӵ�����
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollObject_->updateConnection(connection, enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// ����: ִ�е����¼�ѭ���еĹ���
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::doLoopWork(Thread *thread)
{
    epollObject_->poll();
}

//-----------------------------------------------------------------------------
// ����: �����¼�ѭ���е���������
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::wakeupLoop()
{
    epollObject_->wakeup();
}

//-----------------------------------------------------------------------------
// ����: ��������ע�ᵽ�¼�ѭ����
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::registerConnection(TcpConnection *connection)
{
    epollObject_->addConnection(connection, false, false);
}

//-----------------------------------------------------------------------------
// ����: ���¼�ѭ����ע������
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::unregisterConnection(TcpConnection *connection)
{
    epollObject_->removeConnection(connection);
}

//-----------------------------------------------------------------------------
// ����: EPoll �¼��ص�
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::onEpollNotifyEvent(TcpConnection *connection, EpollObject::EVENT_TYPE eventType)
{
    LinuxTcpConnection *theConn = static_cast<LinuxTcpConnection*>(connection);

    if (eventType == EpollObject::ET_ALLOW_SEND)
        theConn->trySend();
    else if (eventType == EpollObject::ET_ALLOW_RECV)
        theConn->tryRecv();
    else if (eventType == EpollObject::ET_ERROR)
        theConn->errorOccurred();
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class MainTcpServer

MainTcpServer::MainTcpServer() :
    eventLoopList_(iseApp().getIseOptions().getTcpEventLoopCount()),
    isActive_(false)
{
    createTcpServerList();
}

MainTcpServer::~MainTcpServer()
{
    close();
    destroyTcpServerList();
}

//-----------------------------------------------------------------------------
// ����: ����������
//-----------------------------------------------------------------------------
void MainTcpServer::open()
{
    if (!isActive_)
    {
        try
        {
            doOpen();
            isActive_ = true;
        }
        catch (...)
        {
            doClose();
            throw;
        }
    }
}

//-----------------------------------------------------------------------------
// ����: �رշ�����
//-----------------------------------------------------------------------------
void MainTcpServer::close()
{
    if (isActive_)
    {
        doClose();
        isActive_ = false;
    }
}

//-----------------------------------------------------------------------------
// ����: ����TCP������
//-----------------------------------------------------------------------------
void MainTcpServer::createTcpServerList()
{
    int serverCount = iseApp().getIseOptions().getTcpServerCount();
    ISE_ASSERT(serverCount >= 0);

    tcpServerList_.resize(serverCount);
    for (int i = 0; i < serverCount; i++)
    {
        TcpServer *tcpServer = new TcpServer();
        tcpServer->setContext(i);

        tcpServerList_[i] = tcpServer;

        tcpServer->setAcceptConnCallback(boost::bind(&MainTcpServer::onAcceptConnection, this, _1, _2));
        tcpServer->setLocalPort(iseApp().getIseOptions().getTcpServerPort(i));
    }
}

//-----------------------------------------------------------------------------
// ����: ����TCP������
//-----------------------------------------------------------------------------
void MainTcpServer::destroyTcpServerList()
{
    for (int i = 0; i < (int)tcpServerList_.size(); i++)
        delete tcpServerList_[i];
    tcpServerList_.clear();
}

//-----------------------------------------------------------------------------
// ����: ����������
//-----------------------------------------------------------------------------
void MainTcpServer::doOpen()
{
    for (int i = 0; i < (int)tcpServerList_.size(); i++)
        tcpServerList_[i]->open();

    eventLoopList_.start();
}

//-----------------------------------------------------------------------------
// ����: �رշ�����
//-----------------------------------------------------------------------------
void MainTcpServer::doClose()
{
    eventLoopList_.stop();

    for (int i = 0; i < (int)tcpServerList_.size(); i++)
        tcpServerList_[i]->close();
}

//-----------------------------------------------------------------------------
// ����: �յ��µ�����
// ע��:
//   �˻ص���TCP�����������߳�(TcpListenerThread)��ִ�С�
//   Ϊ�˱���TCP�������ļ����̳߳�Ϊϵͳ��ƿ�����ʲ�Ӧ�ڴ˼����߳��д���
//   iseApp().getIseBusiness().onTcpConnect() �ص�����ȷ�������ǣ����²���
//   ������֪ͨ�� TcpEventLoop�������¼�ѭ���߳�(TcpEventLoopThread)�����ص���
//   ����������һ���ô��ǣ�����ͬһ��TCP���ӣ�IseBusiness::onTcpXXX() ϵ�лص�
//   ����ͬһ���߳���ִ�С�
//-----------------------------------------------------------------------------
void MainTcpServer::onAcceptConnection(BaseTcpServer *tcpServer, BaseTcpConnection *connection)
{
    // round-robin
    static int index = 0;
    TcpEventLoop *eventLoop = eventLoopList_[index];
    index = (index >= eventLoopList_.getCount() - 1 ? 0 : index + 1);

    ((TcpConnection*)connection)->setEventLoop(eventLoop);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
