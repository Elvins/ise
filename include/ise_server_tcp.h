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
// ise_server_tcp.h
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// ˵��:
//
// * �յ����Ӻ�(onTcpConnected)����ʹ�û������� connection->recv()��ISEҲ���ں�̨
//   �Զ��������ݡ����յ��������ݴ��ڻ����С�
//
// * ��ʹ�û������������κζ���(�Ȳ� send Ҳ�� recv)�����Է��Ͽ����� (close/shutdown) ʱ��
//   �ҷ�Ҳ�ܹ���֪����ͨ�� onTcpDisconnected() ֪ͨ�û���
//
// * �������ӵĶϿ�:
//   connection->disconnect() �������ر� (shutdown) ����ͨ���������ܸ����ӵ�
//   ISE��������û��δ����������ݡ�����û�йرս���ͨ�������Դ�ʱ�����Կ��Խ�
//   �նԷ������ݡ���������£��Է��ڼ�⵽�ҷ��رշ��� (read ���� 0) ��Ӧ��
//   ������ (close)�������ҷ������������մ��󣬽��� errorOccurred()���ȶ��������ӡ�
//
//   ���ϣ���ѻ����е����ݷ�����Ϻ��� disconnect()������ onTcpSendComplete()
//   �н��жϿ�������
//
//   TcpConnection �ṩ�˸����� shutdown(bool closeSend, bool closeRecv) ������
//   �û����ϣ���Ͽ�����ʱ˫��رգ���ֱ�ӵ��� connection->shutdown() ������
//   ������ connection->disconnect()��
//
//   �������ISE������˫��ر� (shutdown(true, true)) ����:
//   1. �������д����� (errorOccurred())��
//   2. ���ͻ���ճ�ʱ (checkTimeout())��
//   3. �����˳�ʱ�ر��ִ����� (clearConnections())��
//
// * ���Ӷ��� (TcpConnection) ���� boost::shared_ptr ���������¼�����ɫ����:
//   1. TcpEventLoop.
//      �� TcpEventLoop::tcpConnMap_ ���У�TcpEventLoop::removeConnection() ʱ�ͷš�
//      ������ TcpConnection::setEventLoop(NULL) ʱ�������� removeConnection()��
//      ���������˳� (kill �� iseApp().setTerminated(true)) ʱ��������ȫ������
//      (TcpEventLoop::clearConnections())���Ӷ�ʹ TcpEventLoop �ͷ������е�ȫ��
//      boost::shared_ptr��
//   2. ISE_WINDOWS::IOCP.
//      �� IocpTaskData::callback_ ���С�callback_ ��Ϊһ�� boost::function������
//      ���� TcpConnection::shared_from_this() ���ݹ����� shared_ptr��
//      ��һ��IOCP��ѭ��Ϻ󣬻���� IocpObject::destroyOverlappedData()���˴���
//      ���� callback_���Ӷ��ͷ� shared_ptr��
//      ��˵��Ҫ������һ�����Ӷ�������Ӧ��֤�ڸ�������Ͷ�ݸ�IOCP����������ѭ��
//      �õ��˴���ֻ�д���������IOCP �Ż��ͷ� shared_ptr��
//   3. ISE ���û�.
//      ҵ��ӿ��� IseBusiness::onTcpXXX() ϵ�к��������Ӷ����� shared_ptr ��ʽ
//      ���ݸ��û����󲿷�����£�ISE�����Ӷ�����Զ�����������ʵ����Ҫ�����û�
//      ��Ȼ���Գ��� shared_ptr���������ӱ��Զ����١�
//
// * ���Ӷ��� (TcpConnection) �ļ������ٳ���:
//   1. IseBusiness::onTcpConnected() ֮���û������� connection->disconnect()��
//      ���� IOCP/EPoll ���󣬵��� errorOccurred()��ִ�� onTcpDisconnected() ��
//      TcpConnecton::setEventLoop(NULL)��
//   2. IseBusiness::onTcpConnected() ֮���û�û���κζ��������Է��Ͽ������ӣ�
//      �ҷ���⵽������ IOCP/EPoll ����֮��ͬ1��
//   3. ������ Linux �±� kill���������ִ���� iseApp().setTerminated(true)��
//      ��ǰʣ������ȫ���� TcpEventLoop::clearConnections() �� �� disconnect()��
//      �ڽ���������ѭ�������� IOCP/EPoll ����֮��ͬ1��


#ifndef _ISE_SERVER_TCP_H_
#define _ISE_SERVER_TCP_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sys_utils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

#ifdef ISE_WINDOWS
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <sys/epoll.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class IoBuffer;
class TcpEventLoopThread;
class TcpEventLoop;
class TcpEventLoopList;
class TcpConnection;
class TcpClient;
class TcpServer;
class TcpConnector;

#ifdef ISE_WINDOWS
class WinTcpConnection;
class WinTcpEventLoop;
class IocpTaskData;
class IocpBufferAllocator;
class IocpPendingCounter;
class IocpObject;
#endif

#ifdef ISE_LINUX
class LinuxTcpConnection;
class LinuxTcpEventLoop;
class EpollObject;
#endif

class MainTcpServer;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

// ���ݰ��ֽ���
typedef boost::function<void (
    const char *data,   // �����п������ݵ����ֽ�ָ��
    int bytes,          // �����п������ݵ��ֽ���
    int& retrieveBytes  // ���ط�����������ݰ���С������0��ʾ�ִ��������в����Է����һ���������ݰ�
)> PacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// Ԥ�������ݰ��ֽ���

void bytePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void linePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void nullTerminatedPacketSplitter(const char *data, int bytes, int& retrieveBytes);
void anyPacketSplitter(const char *data, int bytes, int& retrieveBytes);

// ÿ�ν���һ���ֽڵ����ݰ��ֽ���
const PacketSplitter BYTE_PACKET_SPLITTER = &ise::bytePacketSplitter;
// �� '\r'��'\n' �������Ϊ�ֽ��ַ������ݰ��ֽ���
const PacketSplitter LINE_PACKET_SPLITTER = &ise::linePacketSplitter;
// �� '\0' Ϊ�ֽ��ַ������ݰ��ֽ���
const PacketSplitter NULL_TERMINATED_PACKET_SPLITTER = &ise::nullTerminatedPacketSplitter;
// �����յ������ֽڶ�������ȡ�����ݰ��ֽ���
const PacketSplitter ANY_PACKET_SPLITTER = &ise::anyPacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// class TcpInspectInfo

class TcpInspectInfo : boost::noncopyable
{
public:
    AtomicInt tcpConnCreateCount;    // TcpConnection ����Ĵ�������
    AtomicInt tcpConnDestroyCount;   // TcpConnection ��������ٴ���
    AtomicInt errorOccurredCount;    // TcpConnection::errorOccurred() �ĵ��ô���
    AtomicInt addConnCount;          // TcpEventLoop::addConnection() �ĵ��ô���
    AtomicInt removeConnCount;       // TcpEventLoop::removeConnection() �ĵ��ô���
public:
    static TcpInspectInfo& instance()
    {
        static TcpInspectInfo obj;
        return obj;
    }
};

///////////////////////////////////////////////////////////////////////////////
// class IoBuffer - �����������
//
// +-----------------+------------------+------------------+
// |  useless bytes  |  readable bytes  |  writable bytes  |
// |                 |     (CONTENT)    |                  |
// +-----------------+------------------+------------------+
// |                 |                  |                  |
// 0     <=     readerIndex   <=   writerIndex    <=    size

class IoBuffer
{
public:
    enum { INITIAL_SIZE = 1024 };

public:
    IoBuffer();
    ~IoBuffer();

    int getReadableBytes() const { return writerIndex_ - readerIndex_; }
    int getWritableBytes() const { return (int)buffer_.size() - writerIndex_; }
    int getUselessBytes() const { return readerIndex_; }

    void append(const string& str);
    void append(const void *data, int bytes);
    void append(int bytes);

    void retrieve(int bytes);
    void retrieveAll(string& str);
    void retrieveAll();

    void swap(IoBuffer& rhs);
    const char* peek() const { return getBufferPtr() + readerIndex_; }

private:
    char* getBufferPtr() const { return (char*)&*buffer_.begin(); }
    char* getWriterPtr() const { return getBufferPtr() + writerIndex_; }
    void makeSpace(int moreBytes);

private:
    vector<char> buffer_;
    int readerIndex_;
    int writerIndex_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopThread - �¼�ѭ��ִ���߳�

class TcpEventLoopThread : public Thread
{
public:
    TcpEventLoopThread(TcpEventLoop& eventLoop);
protected:
    virtual void execute();
    virtual void afterExecute();
private:
    TcpEventLoop& eventLoop_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop - �¼�ѭ������

class TcpEventLoop : boost::noncopyable
{
public:
    typedef vector<Functor> Functors;
    typedef map<string, TcpConnectionPtr> TcpConnectionMap;  // <connectionName, TcpConnectionPtr>

    struct FunctorList
    {
        Functors items;
        CriticalSection lock;
    };

public:
    TcpEventLoop();
    virtual ~TcpEventLoop();

    void start();
    void stop(bool force, bool waitFor);

    bool isRunning();
    bool isInLoopThread();
    void assertInLoopThread();
    void executeInLoop(const Functor& functor);
    void delegateToLoop(const Functor& functor);
    void addFinalizer(const Functor& finalizer);

    void addConnection(TcpConnection *connection);
    void removeConnection(TcpConnection *connection);
    void clearConnections();

protected:
    virtual void doLoopWork(Thread *thread) = 0;
    virtual void wakeupLoop() {}
    virtual void registerConnection(TcpConnection *connection) = 0;
    virtual void unregisterConnection(TcpConnection *connection) = 0;

protected:
    void runLoop(Thread *thread);
    void executeDelegatedFunctors();
    void executeFinalizer();

private:
    void checkTimeout();

private:
    TcpEventLoopThread *thread_;
    THREAD_ID loopThreadId_;
    TcpConnectionMap tcpConnMap_;
    FunctorList delegatedFunctors_;
    FunctorList finalizers_;
    UINT lastCheckTimeoutTicks_;

    friend class TcpEventLoopThread;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList - �¼�ѭ���б�

class TcpEventLoopList : boost::noncopyable
{
public:
    enum { MAX_LOOP_COUNT = 64 };

public:
    TcpEventLoopList(int loopCount);
    virtual ~TcpEventLoopList();

    void start();
    void stop();

    int getCount() { return items_.getCount(); }
    TcpEventLoop* getItem(int index) { return items_[index]; }
    TcpEventLoop* operator[] (int index) { return getItem(index); }

private:
    void setCount(int count);

private:
    ObjectList<TcpEventLoop> items_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection - Proactorģ���µ�TCP����

class TcpConnection :
    public BaseTcpConnection,
    public boost::enable_shared_from_this<TcpConnection>
{
public:
    struct SendTask
    {
    public:
        int bytes;
        Context context;
        int timeout;
        UINT startTicks;
    public:
        SendTask()
        {
            bytes = 0;
            timeout = 0;
            startTicks = 0;
        }
    };

    struct RecvTask
    {
    public:
        PacketSplitter packetSplitter;
        Context context;
        int timeout;
        UINT startTicks;
    public:
        RecvTask()
        {
            timeout = 0;
            startTicks = 0;
        }
    };

    typedef deque<SendTask> SendTaskQueue;
    typedef deque<RecvTask> RecvTaskQueue;

public:
    TcpConnection();
    TcpConnection(TcpServer *tcpServer, SOCKET socketHandle);
    virtual ~TcpConnection();

    void send(
        const void *buffer,
        int size,
        const Context& context = EMPTY_CONTEXT,
        int timeout = TIMEOUT_INFINITE
        );

    void recv(
        const PacketSplitter& packetSplitter = ANY_PACKET_SPLITTER,
        const Context& context = EMPTY_CONTEXT,
        int timeout = TIMEOUT_INFINITE
        );

    const string& getConnectionName() const;
    int getServerIndex() const;
    int getServerPort() const;
    int getServerConnCount() const;

protected:
    virtual void doDisconnect();
    virtual void eventLoopChanged() {}
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout) = 0;
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout) = 0;

protected:
    void errorOccurred();
    void checkTimeout(UINT curTicks);
    void postSendTask(const string& data, const Context& context, int timeout);

    void setEventLoop(TcpEventLoop *eventLoop);
    TcpEventLoop* getEventLoop() { return eventLoop_; }

private:
    void init();

protected:
    TcpServer *tcpServer_;           // ���� TcpServer
    TcpEventLoop *eventLoop_;        // ���� TcpEventLoop
    mutable string connectionName_;  // ��������
    IoBuffer sendBuffer_;            // ���ݷ��ͻ���
    IoBuffer recvBuffer_;            // ���ݽ��ջ���
    SendTaskQueue sendTaskQueue_;    // �����������
    RecvTaskQueue recvTaskQueue_;    // �����������
    bool isErrorOccurred_;           // �������Ƿ����˴���

    friend class MainTcpServer;
    friend class TcpEventLoop;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpClient

class TcpClient : public BaseTcpClient
{
public:
    TcpConnection& getConnection() { return *static_cast<TcpConnection*>(connection_); }
protected:
    virtual BaseTcpConnection* createConnection();
private:
    bool registerToEventLoop(int index = -1);
private:
    friend class TcpConnector;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpServer

class TcpServer : public BaseTcpServer
{
public:
    int getConnectionCount() const { return connCount_.get(); }

protected:
    virtual BaseTcpConnection* createConnection(SOCKET socketHandle);

private:
    void incConnCount() { connCount_.increment(); }
    void decConnCount() { connCount_.decrement(); }

private:
    mutable AtomicInt connCount_;
    friend class TcpConnection;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpConnector - TCP��������

class TcpConnector : boost::noncopyable
{
public:
    typedef boost::function<void (bool success, TcpConnection *connection,
        const InetAddress& peerAddr, const Context& context)> CompleteCallback;

private:
    typedef vector<SOCKET> FdList;

    struct TaskItem
    {
        TcpClient tcpClient;
        InetAddress peerAddr;
        CompleteCallback completeCallback;
        ASYNC_CONNECT_STATE state;
        Context context;
    };

    typedef ObjectList<TaskItem> TaskList;

    class WorkerThread : public Thread
    {
    public:
        WorkerThread(TcpConnector& owner) : owner_(owner) {}
    protected:
        virtual void execute() { owner_.work(*this); }
        virtual void afterExecute() { owner_.thread_ = NULL; }
    private:
        TcpConnector& owner_;
    };

    friend class WorkerThread;

public:
    ~TcpConnector();
    static TcpConnector& instance();

    void connect(const InetAddress& peerAddr,
        const CompleteCallback& completeCallback,
        const Context& context = EMPTY_CONTEXT);
    void clear();

private:
    TcpConnector();

private:
    void start();
    void stop();
    void work(WorkerThread& thread);

    void tryConnect();
    void getPendingFdsFromTaskList(int& fromIndex, FdList& fds);
    void checkAsyncConnectState(const FdList& fds, FdList& connectedFds, FdList& failedFds);
    TaskItem* findTask(SOCKET fd);
    void invokeCompleteCallback();

private:
    TaskList taskList_;
    CriticalSection lock_;
    WorkerThread *thread_;
};

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

enum IOCP_TASK_TYPE
{
    ITT_SEND = 1,
    ITT_RECV = 2,
};

typedef boost::function<void (const IocpTaskData& taskData)> IocpCallback;

///////////////////////////////////////////////////////////////////////////////
// class WinTcpConnection

class WinTcpConnection : public TcpConnection
{
public:
    WinTcpConnection();
    WinTcpConnection(TcpServer *tcpServer, SOCKET socketHandle);

protected:
    virtual void eventLoopChanged();
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout);

private:
    void init();

    WinTcpEventLoop* getEventLoop() { return (WinTcpEventLoop*)eventLoop_; }

    void trySend();
    void tryRecv();

    static void onIocpCallback(const TcpConnectionPtr& thisObj, const IocpTaskData& taskData);
    void onSendCallback(const IocpTaskData& taskData);
    void onRecvCallback(const IocpTaskData& taskData);

private:
    bool isSending_;       // �Ƿ�����IOCP�ύ����������δ�յ��ص�֪ͨ
    bool isRecving_;       // �Ƿ�����IOCP�ύ����������δ�յ��ص�֪ͨ
    int bytesSent_;        // �Դ��ϴη���������ɻص������������˶����ֽ�
    int bytesRecved_;      // �Դ��ϴν���������ɻص������������˶����ֽ�
};

///////////////////////////////////////////////////////////////////////////////
// class IocpTaskData

class IocpTaskData
{
public:
    IocpTaskData();

    HANDLE getIocpHandle() const { return iocpHandle_; }
    HANDLE getFileHandle() const { return fileHandle_; }
    IOCP_TASK_TYPE getTaskType() const { return taskType_; }
    UINT getTaskSeqNum() const { return taskSeqNum_; }
    PVOID getCaller() const { return caller_; }
    const Context& getContext() const { return context_; }
    char* getEntireDataBuf() const { return (char*)entireDataBuf_; }
    int getEntireDataSize() const { return entireDataSize_; }
    char* getDataBuf() const { return (char*)wsaBuffer_.buf; }
    int getDataSize() const { return wsaBuffer_.len; }
    int getBytesTrans() const { return bytesTrans_; }
    int getErrorCode() const { return errorCode_; }
    const IocpCallback& getCallback() const { return callback_; }

private:
    HANDLE iocpHandle_;
    HANDLE fileHandle_;
    IOCP_TASK_TYPE taskType_;
    UINT taskSeqNum_;
    PVOID caller_;
    Context context_;
    PVOID entireDataBuf_;
    int entireDataSize_;
    WSABUF wsaBuffer_;
    int bytesTrans_;
    int errorCode_;
    IocpCallback callback_;

    friend class IocpObject;
};

#pragma pack(1)
struct IocpOverlappedData
{
    OVERLAPPED overlapped;
    IocpTaskData taskData;
};
#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class IocpBufferAllocator

class IocpBufferAllocator : boost::noncopyable
{
public:
    IocpBufferAllocator(int bufferSize);
    ~IocpBufferAllocator();

    PVOID allocBuffer();
    void returnBuffer(PVOID buffer);

    int getUsedCount() const { return usedCount_; }

private:
    void clear();

private:
    int bufferSize_;
    PointerList items_;
    int usedCount_;
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

class IocpPendingCounter : boost::noncopyable
{
public:
    IocpPendingCounter() {}
    virtual ~IocpPendingCounter() {}

    void inc(PVOID caller, IOCP_TASK_TYPE taskType);
    void dec(PVOID caller, IOCP_TASK_TYPE taskType);
    int get(PVOID caller);
    int get(IOCP_TASK_TYPE taskType);

private:
    struct CountData
    {
        int sendCount;
        int recvCount;
    };

    typedef std::map<PVOID, CountData> Items;   // <caller, CountData>

    Items items_;
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

class IocpObject : boost::noncopyable
{
public:
    IocpObject();
    virtual ~IocpObject();

    bool associateHandle(SOCKET socketHandle);
    bool isComplete(PVOID caller);

    void work();
    void wakeup();

    void send(SOCKET socketHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void recv(SOCKET socketHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);

private:
    void initialize();
    void finalize();
    void throwGeneralError();
    IocpOverlappedData* createOverlappedData(IOCP_TASK_TYPE taskType,
        HANDLE fileHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void destroyOverlappedData(IocpOverlappedData *ovDataPtr);
    void postError(int errorCode, IocpOverlappedData *ovDataPtr);
    void invokeCallback(const IocpTaskData& taskData);

private:
    static IocpBufferAllocator bufferAlloc_;
    static SeqNumberAlloc taskSeqAlloc_;
    static IocpPendingCounter pendingCounter_;

    HANDLE iocpHandle_;

    friend class AutoFinalizer;
};

///////////////////////////////////////////////////////////////////////////////
// class WinTcpEventLoop

class WinTcpEventLoop : public TcpEventLoop
{
public:
    WinTcpEventLoop();
    virtual ~WinTcpEventLoop();

    IocpObject* getIocpObject() { return iocpObject_; }

protected:
    virtual void doLoopWork(Thread *thread);
    virtual void wakeupLoop();
    virtual void registerConnection(TcpConnection *connection);
    virtual void unregisterConnection(TcpConnection *connection);

private:
    IocpObject *iocpObject_;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WINDOWS */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpConnection

class LinuxTcpConnection : public TcpConnection
{
public:
    LinuxTcpConnection();
    LinuxTcpConnection(TcpServer *tcpServer, SOCKET socketHandle);

protected:
    virtual void eventLoopChanged();
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout);

private:
    void init();

    LinuxTcpEventLoop* getEventLoop() { return (LinuxTcpEventLoop*)eventLoop_; }

    void setSendEnabled(bool enabled);
    void setRecvEnabled(bool enabled);

    void trySend();
    void tryRecv();

    bool tryRetrievePacket();

private:
    int bytesSent_;                  // �Դ��ϴη���������ɻص������������˶����ֽ�
    bool enableSend_;                // �Ƿ���ӿɷ����¼�
    bool enableRecv_;                // �Ƿ���ӿɽ����¼�

    friend class LinuxTcpEventLoop;
};

///////////////////////////////////////////////////////////////////////////////
// class EpollObject - Linux EPoll ���ܷ�װ

class EpollObject
{
public:
    enum { INITIAL_EVENT_SIZE = 32 };

    enum EVENT_TYPE
    {
        ET_NONE        = 0,
        ET_ERROR       = 1,
        ET_ALLOW_SEND  = 2,
        ET_ALLOW_RECV  = 3,
    };

    typedef vector<struct epoll_event> EventList;
    typedef int EventPipe[2];

    typedef boost::function<void (TcpConnection *connection, EVENT_TYPE eventType)> NotifyEventCallback;

public:
    EpollObject();
    ~EpollObject();

    void poll();
    void wakeup();

    void addConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
    void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
    void removeConnection(TcpConnection *connection);

    void setNotifyEventCallback(const NotifyEventCallback& callback);

private:
    void createEpoll();
    void destroyEpoll();
    void createPipe();
    void destroyPipe();

    void epollControl(int operation, void *param, int handle, bool enableSend, bool enableRecv);

    void processPipeEvent();
    void processEvents(int eventCount);

private:
    int epollFd_;                    // EPoll ���ļ�������
    EventList events_;               // ��� epoll_wait() ���ص��¼�
    EventPipe pipeFds_;              // ���ڻ��� epoll_wait() �Ĺܵ�
    NotifyEventCallback onNotifyEvent_;
};

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpEventLoop

class LinuxTcpEventLoop : public TcpEventLoop
{
public:
    LinuxTcpEventLoop();
    virtual ~LinuxTcpEventLoop();

    void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);

protected:
    virtual void doLoopWork(Thread *thread);
    virtual void wakeupLoop();
    virtual void registerConnection(TcpConnection *connection);
    virtual void unregisterConnection(TcpConnection *connection);

private:
    void onEpollNotifyEvent(TcpConnection *connection, EpollObject::EVENT_TYPE eventType);

private:
    EpollObject *epollObject_;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class MainTcpServer - TCP����������

class MainTcpServer
{
public:
    explicit MainTcpServer();
    virtual ~MainTcpServer();

    void open();
    void close();

    bool registerToEventLoop(BaseTcpConnection *connection, int eventLoopIndex = -1);

private:
    void createTcpServerList();
    void destroyTcpServerList();
    void doOpen();
    void doClose();

    void onAcceptConnection(BaseTcpServer *tcpServer, BaseTcpConnection *connection);

private:
    typedef vector<TcpServer*> TcpServerList;

    bool isActive_;
    TcpServerList tcpServerList_;
    TcpEventLoopList eventLoopList_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_TCP_H_
