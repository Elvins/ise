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

#ifndef _ISE_SERVER_TCP_H_
#define _ISE_SERVER_TCP_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

#ifdef ISE_WIN32
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <sys/epoll.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class PacketMeasurer;
class IoBuffer;
class TcpConnection;
class TcpEventLoopThread;
class BaseTcpEventLoop;
class TcpEventLoop;
class TcpEventLoopList;

#ifdef ISE_WIN32
class IocpTaskData;
class IocpBufferAllocator;
class IocpPendingCounter;
class IocpObject;
#endif

#ifdef ISE_LINUX
class EpollObject;
#endif

class MainTcpServer;

///////////////////////////////////////////////////////////////////////////////
// class PacketMeasurer - ���ݰ�������

class PacketMeasurer
{
public:
	virtual ~PacketMeasurer() {}
	virtual bool isCompletePacket(const char *data, int bytes, int& packetSize) = 0;
};

class DelimiterPacketMeasurer : public PacketMeasurer
{
private:
	char delimiter_;
public:
	DelimiterPacketMeasurer(char delimiter) : delimiter_(delimiter) {}
	virtual bool isCompletePacket(const char *data, int bytes, int& packetSize);
};

class LinePacketMeasurer : public DelimiterPacketMeasurer
{
private:
	LinePacketMeasurer() : DelimiterPacketMeasurer('\n') {}
public:
	static LinePacketMeasurer& instance()
	{
		static LinePacketMeasurer obj;
		return obj;
	}
};

class NullTerminatedPacketMeasurer : public DelimiterPacketMeasurer
{
private:
	NullTerminatedPacketMeasurer() : DelimiterPacketMeasurer('\0') {}
public:
	static NullTerminatedPacketMeasurer& instance()
	{
		static NullTerminatedPacketMeasurer obj;
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
// 0     <=    nReaderIndex   <=   nWriterIndex    <=    size

class IoBuffer
{
public:
	enum { INITIAL_SIZE = 1024 };
private:
	vector<char> buffer_;
	int readerIndex_;
	int writerIndex_;
private:
	char* getBufferPtr() const { return (char*)&*buffer_.begin(); }
	char* getWriterPtr() const { return getBufferPtr() + writerIndex_; }
	void makeSpace(int moreBytes);
public:
	IoBuffer();
	~IoBuffer();

	int getReadableBytes() const { return writerIndex_ - readerIndex_; }
	int getWritableBytes() const { return (int)buffer_.size() - writerIndex_; }
	int getUselessBytes() const { return readerIndex_; }

	void append(const string& str);
	void append(const char *data, int bytes);
	void append(int bytes);

	void retrieve(int bytes);
	void retrieveAll(string& str);
	void retrieveAll();

	const char* peek() const { return getBufferPtr() + readerIndex_; }
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopThread - �¼�ѭ��ִ���߳�

class TcpEventLoopThread : public Thread
{
private:
	BaseTcpEventLoop& eventLoop_;
protected:
	virtual void execute();
public:
	TcpEventLoopThread(BaseTcpEventLoop& eventLoop);
};

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpEventLoop - �¼�ѭ������

class BaseTcpEventLoop
{
public:
	friend class TcpEventLoopThread;
private:
	TcpEventLoopThread *thread_;
	ObjectList<TcpConnection> acceptedConnList_;  // ��TcpServer�²�����TCP����
protected:
	virtual void executeLoop(Thread *thread) = 0;
	virtual void wakeupLoop() {}
	virtual void registerConnection(TcpConnection *connection) = 0;
	virtual void unregisterConnection(TcpConnection *connection) = 0;
protected:
	void processAcceptedConnList();
public:
	BaseTcpEventLoop();
	virtual ~BaseTcpEventLoop();

	void start();
	void stop(bool force, bool waitFor);
	bool isRunning();

	void addConnection(TcpConnection *connection);
	void removeConnection(TcpConnection *connection);
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList - �¼�ѭ���б�

class TcpEventLoopList
{
public:
	enum { MAX_LOOP_COUNT = 64 };
private:
	ObjectList<BaseTcpEventLoop> items_;
private:
	void setCount(int count);
public:
	TcpEventLoopList(int nLoopCount);
	virtual ~TcpEventLoopList();

	void start();
	void stop();

	int getCount() { return items_.getCount(); }
	BaseTcpEventLoop* getItem(int index) { return items_[index]; }
	BaseTcpEventLoop* operator[] (int index) { return getItem(index); }
};

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WIN32

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

enum IOCP_TASK_TYPE
{
	ITT_SEND = 1,
	ITT_RECV = 2,
};

typedef void (*IOCP_CALLBACK_PROC)(const IocpTaskData& taskData, PVOID param);
typedef CallbackDef<IOCP_CALLBACK_PROC> IOCP_CALLBACK_DEF;

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection - Proactorģ���µ�TCP����

class TcpConnection : public BaseTcpConnection
{
public:
	friend class MainTcpServer;
public:
	struct SEND_TASK
	{
		int bytes;
		CustomParams params;
	};

	struct RECV_TASK
	{
		PacketMeasurer *packetMeasurer;
		CustomParams params;
	};

	typedef deque<SEND_TASK> SEND_TASK_QUEUE;
	typedef deque<RECV_TASK> RECV_TASK_QUEUE;

private:
	TcpServer *tcpServer_;            // ���� TcpServer
	TcpEventLoop *eventLoop_;         // ���� TcpEventLoop
	IoBuffer sendBuffer_;             // ���ݷ��ͻ���
	IoBuffer recvBuffer_;             // ���ݽ��ջ���
	SEND_TASK_QUEUE sendTaskQueue_;   // �����������
	RECV_TASK_QUEUE recvTaskQueue_;   // �����������
	bool isSending_;                  // �Ƿ�����IOCP�ύ����������δ�յ��ص�֪ͨ
	bool isRecving_;                  // �Ƿ�����IOCP�ύ����������δ�յ��ص�֪ͨ
	int bytesSent_;                   // �Դ��ϴη���������ɻص������������˶����ֽ�
	int bytesRecved_;                 // �Դ��ϴν���������ɻص������������˶����ֽ�
private:
	void trySend();
	void tryRecv();
	void errorOccurred();

	void setEventLoop(TcpEventLoop *eventLoop);
	TcpEventLoop* getEventLoop() { return eventLoop_; }

	static void onIocpCallback(const IocpTaskData& taskData, PVOID param);
	void onSendCallback(const IocpTaskData& taskData);
	void onRecvCallback(const IocpTaskData& taskData);
protected:
	virtual void doDisconnect();
public:
	TcpConnection(TcpServer *tcpServer, SOCKET socketHandle, const InetAddress& peerAddr);
	virtual ~TcpConnection();

	void postSendTask(char *buffer, int size, const CustomParams& params = EMPTY_PARAMS);
	void postRecvTask(PacketMeasurer *packetMeasurer, const CustomParams& params = EMPTY_PARAMS);

	int getServerIndex() const { return (int)tcpServer_->customData(); }
	int getServerPort() const { return tcpServer_->getLocalPort(); }
};

///////////////////////////////////////////////////////////////////////////////
// class IocpTaskData

class IocpTaskData
{
public:
	friend class IocpObject;
private:
	HANDLE iocpHandle_;
	HANDLE fileHandle_;
	IOCP_TASK_TYPE taskType_;
	UINT taskSeqNum_;
	CallbackDef<IOCP_CALLBACK_PROC> callback_;
	PVOID caller_;
	CustomParams params_;
	PVOID entireDataBuf_;
	int entireDataSize_;
	WSABUF wsaBuffer_;
	int bytesTrans_;
	int errorCode_;

public:
	IocpTaskData();

	HANDLE getIocpHandle() const { return iocpHandle_; }
	HANDLE getFileHandle() const { return fileHandle_; }
	IOCP_TASK_TYPE getTaskType() const { return taskType_; }
	UINT getTaskSeqNum() const { return taskSeqNum_; }
	const IOCP_CALLBACK_DEF& getCallback() const { return callback_; }
	PVOID getCaller() const { return caller_; }
	const CustomParams& getParams() const { return params_; }
	char* getEntireDataBuf() const { return (char*)entireDataBuf_; }
	int getEntireDataSize() const { return entireDataSize_; }
	char* getDataBuf() const { return (char*)wsaBuffer_.buf; }
	int getDataSize() const { return wsaBuffer_.len; }
	int getBytesTrans() const { return bytesTrans_; }
	int getErrorCode() const { return errorCode_; }
};

#pragma pack(1)
struct CIocpOverlappedData
{
	OVERLAPPED overlapped;
	IocpTaskData taskData;
};
#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class IocpBufferAllocator

class IocpBufferAllocator
{
private:
	int bufferSize_;
	PointerList items_;
	int usedCount_;
	CriticalSection lock_;
private:
	void clear();
public:
	IocpBufferAllocator(int bufferSize);
	~IocpBufferAllocator();

	PVOID allocBuffer();
	void returnBuffer(PVOID buffer);

	int getUsedCount() const { return usedCount_; }
};

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

class IocpPendingCounter
{
private:
	struct COUNT_DATA
	{
		int sendCount;
		int recvCount;
	};

	typedef std::map<PVOID, COUNT_DATA> ITEMS;   // <caller, COUNT_DATA>

	ITEMS items_;
	CriticalSection lock_;
public:
	IocpPendingCounter() {}
	virtual ~IocpPendingCounter() {}

	void inc(PVOID caller, IOCP_TASK_TYPE taskType);
	void dec(PVOID caller, IOCP_TASK_TYPE taskType);
	int get(PVOID caller);
	int get(IOCP_TASK_TYPE taskType);
};

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

class IocpObject
{
public:
	friend class AutoFinalizer;

private:
	static IocpBufferAllocator bufferAlloc_;
	static SeqNumberAlloc taskSeqAlloc_;
	static IocpPendingCounter pendingCounter_;

	HANDLE iocpHandle_;

private:
	void initialize();
	void finalize();
	void throwGeneralError();
	CIocpOverlappedData* createOverlappedData(IOCP_TASK_TYPE taskType,
		HANDLE fileHandle, PVOID buffer, int size, int offset,
		const IOCP_CALLBACK_DEF& callbackDef, PVOID caller,
		const CustomParams& params);
	void destroyOverlappedData(CIocpOverlappedData *ovDataPtr);
	void postError(int errorCode, CIocpOverlappedData *ovDataPtr);
	void invokeCallback(const IocpTaskData& taskData);

public:
	IocpObject();
	virtual ~IocpObject();

	bool associateHandle(SOCKET socketHandle);
	bool isComplete(PVOID caller);

	void work();
	void wakeup();

	void send(SOCKET socketHandle, PVOID buffer, int size, int offset,
		const IOCP_CALLBACK_DEF& callbackDef, PVOID caller, const CustomParams& params);
	void recv(SOCKET socketHandle, PVOID buffer, int size, int offset,
		const IOCP_CALLBACK_DEF& callbackDef, PVOID caller, const CustomParams& params);
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop - �¼�ѭ��

class TcpEventLoop : public BaseTcpEventLoop
{
private:
	IocpObject *iocpObject_;
protected:
	virtual void executeLoop(Thread *thread);
	virtual void wakeupLoop();
	virtual void registerConnection(TcpConnection *connection);
	virtual void unregisterConnection(TcpConnection *connection);
public:
	TcpEventLoop();
	virtual ~TcpEventLoop();

	IocpObject* getIocpObject() { return iocpObject_; }
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WIN32 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection - Proactorģ���µ�TCP����

class TcpConnection : public BaseTcpConnection
{
public:
	friend class MainTcpServer;
	friend class TcpEventLoop;
public:
	struct SEND_TASK
	{
		int bytes;
		CustomParams params;
	};

	struct RECV_TASK
	{
		PacketMeasurer *packetMeasurer;
		CustomParams params;
	};

	typedef deque<SEND_TASK> SEND_TASK_QUEUE;
	typedef deque<RECV_TASK> RECV_TASK_QUEUE;

private:
	TcpServer *tcpServer_;           // ���� TcpServer
	TcpEventLoop *eventLoop_;        // ���� TcpEventLoop
	IoBuffer sendBuffer_;            // ���ݷ��ͻ���
	IoBuffer recvBuffer_;            // ���ݽ��ջ���
	SEND_TASK_QUEUE sendTaskQueue_;  // �����������
	RECV_TASK_QUEUE recvTaskQueue_;  // �����������
	int bytesSent_;                  // �Դ��ϴη���������ɻص������������˶����ֽ�
	bool enableSend_;                // �Ƿ���ӿɷ����¼�
	bool enableRecv_;                // �Ƿ���ӿɽ����¼�
private:
	void setSendEnabled(bool enabled);
	void setRecvEnabled(bool enabled);

	void trySend();
	void tryRecv();
	void errorOccurred();

	void setEventLoop(TcpEventLoop *eventLoop);
	TcpEventLoop* getEventLoop() { return eventLoop_; }
protected:
	virtual void doDisconnect();
public:
	TcpConnection(TcpServer *tcpServer, SOCKET socketHandle, const InetAddress& peerAddr);
	virtual ~TcpConnection();

	void postSendTask(char *buffer, int size, const CustomParams& params = EMPTY_PARAMS);
	void postRecvTask(PacketMeasurer *packetMeasurer, const CustomParams& params = EMPTY_PARAMS);

	int getServerIndex() const { return (long)(tcpServer_->customData()); }
	int getServerPort() const { return tcpServer_->getLocalPort(); }
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

	typedef vector<struct epoll_event> EVENT_LIST;
	typedef int EVENT_PIPE[2];

	typedef void (*NOTIFY_EVENT_PROC)(void *param, TcpConnection *connection, EVENT_TYPE eventType);

private:
	int epollFd_;                     // EPoll ���ļ�������
	EVENT_LIST events_;               // ��� epoll_wait() ���ص��¼�
	EVENT_PIPE pipeFds_;              // ���ڻ��� epoll_wait() �Ĺܵ�
	CallbackDef<NOTIFY_EVENT_PROC> onNotifyEvent_;
private:
	void createEpoll();
	void destroyEpoll();
	void createPipe();
	void destroyPipe();

	void epollControl(int operation, void *param, int handle, bool enableSend, bool enableRecv);

	void processPipeEvent();
	void processEvents(int eventCount);
public:
	EpollObject();
	~EpollObject();

	void poll();
	void wakeup();

	void addConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
	void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
	void removeConnection(TcpConnection *connection);

	void setOnNotifyEventCallback(NOTIFY_EVENT_PROC proc, void *param = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop - �¼�ѭ��

class TcpEventLoop : public BaseTcpEventLoop
{
private:
	EpollObject *epollObject_;
private:
	static void onEpollNotifyEvent(void *param, TcpConnection *connection,
		EpollObject::EVENT_TYPE eventType);
protected:
	virtual void executeLoop(Thread *thread);
	virtual void wakeupLoop();
	virtual void registerConnection(TcpConnection *connection);
	virtual void unregisterConnection(TcpConnection *connection);
public:
	TcpEventLoop();
	virtual ~TcpEventLoop();

	void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class MainTcpServer - TCP����������

class MainTcpServer
{
private:
	typedef vector<TcpServer*> TCP_SERVER_LIST;
private:
	bool isActive_;
	TCP_SERVER_LIST tcpServerList_;
	TcpEventLoopList eventLoopList_;

private:
	void createTcpServerList();
	void destroyTcpServerList();
	void doOpen();
	void doClose();
private:
	static void onCreateConnection(void *param, TcpServer *tcpServer,
		SOCKET socketHandle, const InetAddress& peerAddr, BaseTcpConnection*& connection);
	static void onAcceptConnection(void *param, TcpServer *tcpServer,
		BaseTcpConnection *connection);
public:
	explicit MainTcpServer();
	virtual ~MainTcpServer();

	void open();
	void close();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_TCP_H_
