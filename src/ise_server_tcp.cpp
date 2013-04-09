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
// class DelimiterPacketMeasurer

bool DelimiterPacketMeasurer::isCompletePacket(const char *data, int bytes, int& packetSize)
{
	for (int i = 0; i < bytes; i++)
	{
		if (data[i] == delimiter_)
		{
			packetSize = i + 1;
			return true;
		}
	}

	return false;
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
void IoBuffer::append(const char *data, int bytes)
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

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopThread

TcpEventLoopThread::TcpEventLoopThread(BaseTcpEventLoop& eventLoop) :
	eventLoop_(eventLoop)
{
	setFreeOnTerminate(false);
}

//-----------------------------------------------------------------------------

void TcpEventLoopThread::execute()
{
	eventLoop_.executeLoop(this);
}

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpEventLoop

BaseTcpEventLoop::BaseTcpEventLoop() :
	thread_(NULL),
	acceptedConnList_(true, false)
{
	// nothing
}

BaseTcpEventLoop::~BaseTcpEventLoop()
{
	stop(false, true);
}

//-----------------------------------------------------------------------------
// ����: ������TcpServer�²���������
//-----------------------------------------------------------------------------
void BaseTcpEventLoop::processAcceptedConnList()
{
	TcpConnection *connection;
	while ((connection = acceptedConnList_.extract(0)))
		iseBusiness->onTcpConnection(connection);
}

//-----------------------------------------------------------------------------
// ����: ���������߳�
//-----------------------------------------------------------------------------
void BaseTcpEventLoop::start()
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
void BaseTcpEventLoop::stop(bool force, bool waitFor)
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
bool BaseTcpEventLoop::isRunning()
{
	return (thread_ != NULL && thread_->isRunning());
}

//-----------------------------------------------------------------------------
// ����: ��ָ������ע�ᵽ�� eventLoop ��
//-----------------------------------------------------------------------------
void BaseTcpEventLoop::addConnection(TcpConnection *connection)
{
	registerConnection(connection);
	acceptedConnList_.add(connection);
	wakeupLoop();
}

//-----------------------------------------------------------------------------
// ����: ��ָ�����ӴӴ� eventLoop ��ע��
//-----------------------------------------------------------------------------
void BaseTcpEventLoop::removeConnection(TcpConnection *connection)
{
	unregisterConnection(connection);
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList

TcpEventLoopList::TcpEventLoopList(int nLoopCount) :
	items_(false, true)
{
	setCount(nLoopCount);
}

TcpEventLoopList::~TcpEventLoopList()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ���� eventLoop �ĸ���
//-----------------------------------------------------------------------------
void TcpEventLoopList::setCount(int count)
{
	count = ensureRange(count, 1, (int)MAX_LOOP_COUNT);

	for (int i = 0; i < count; i++)
		items_.add(new TcpEventLoop());
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

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WIN32

///////////////////////////////////////////////////////////////////////////////

TcpConnection::TcpConnection(TcpServer *tcpServer,
	SOCKET socketHandle, const InetAddress& peerAddr) :
		BaseTcpConnection(socketHandle, peerAddr),
		tcpServer_(tcpServer),
		eventLoop_(NULL),
		isSending_(false),
		isRecving_(false),
		bytesSent_(0),
		bytesRecved_(0)
{
	// nothing
}

TcpConnection::~TcpConnection()
{
	setEventLoop(NULL);
}

//-----------------------------------------------------------------------------

void TcpConnection::trySend()
{
	if (isSending_) return;

	int readableBytes = sendBuffer_.getReadableBytes();
	if (readableBytes > 0)
	{
		const int MAX_SEND_SIZE = 1024*32;

		const char *buffer = sendBuffer_.peek();
		int sendSize = ise::min(readableBytes, MAX_SEND_SIZE);

		isSending_ = true;
		eventLoop_->getIocpObject()->send(getSocket().getHandle(), 
			(PVOID)buffer, sendSize, 0,
			IOCP_CALLBACK_DEF(onIocpCallback, this), this, EMPTY_PARAMS);
	}
}

//-----------------------------------------------------------------------------

void TcpConnection::tryRecv()
{
	if (isRecving_) return;

	if (!recvTaskQueue_.empty())
	{
		const int MAX_RECV_SIZE = 1024*8;

		isRecving_ = true;
		recvBuffer_.append(MAX_RECV_SIZE);
		const char *buffer = recvBuffer_.peek() + bytesRecved_;

		eventLoop_->getIocpObject()->recv(getSocket().getHandle(), 
			(PVOID)buffer, MAX_RECV_SIZE, 0,
			IOCP_CALLBACK_DEF(onIocpCallback, this), this, EMPTY_PARAMS);
	}
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
void TcpConnection::errorOccurred()
{
	setEventLoop(NULL);
	iseBusiness->onTcpError(this);
	delete this;
}

//-----------------------------------------------------------------------------
// ����: ���ô����Ӵ������ĸ� eventLoop
//-----------------------------------------------------------------------------
void TcpConnection::setEventLoop(TcpEventLoop *eventLoop)
{
	if (eventLoop_)
	{
		eventLoop_->removeConnection(this);
		eventLoop_ = NULL;
	}

	if (eventLoop)
	{
		eventLoop_ = eventLoop;
		eventLoop->addConnection(this);
	}
}

//-----------------------------------------------------------------------------

void TcpConnection::onIocpCallback(const IocpTaskData& taskData, PVOID param)
{
	TcpConnection *thisObj = (TcpConnection*)param;

	if (taskData.getErrorCode() == 0)
	{
		switch (taskData.getTaskType())
		{
		case ITT_SEND:
			thisObj->onSendCallback(taskData);
			break;
		case ITT_RECV:
			thisObj->onRecvCallback(taskData);
			break;
		}
	}
	else
	{
		thisObj->errorOccurred();
	}
}

//-----------------------------------------------------------------------------

void TcpConnection::onSendCallback(const IocpTaskData& taskData)
{
	ISE_ASSERT(taskData.getErrorCode() == 0);

	if (taskData.getBytesTrans() < taskData.getDataSize())
	{
		eventLoop_->getIocpObject()->send(
			(SOCKET)taskData.getFileHandle(),
			taskData.getEntireDataBuf(),
			taskData.getEntireDataSize(),
			taskData.getDataBuf() - taskData.getEntireDataBuf() + taskData.getBytesTrans(),
			taskData.getCallback(), taskData.getCaller(), taskData.getParams());
	}
	else
	{
		isSending_ = false;
		sendBuffer_.retrieve(taskData.getEntireDataSize());
	}

	bytesSent_ += taskData.getBytesTrans();

	while (!sendTaskQueue_.empty())
	{
		const SEND_TASK& task = sendTaskQueue_.front();
		if (bytesSent_ >= task.bytes)
		{
			bytesSent_ -= task.bytes;
			iseBusiness->onTcpSendComplete(this, task.params);
			sendTaskQueue_.pop_front();
		}
		else
			break;
	}

	if (!sendTaskQueue_.empty())
		trySend();
}

//-----------------------------------------------------------------------------

void TcpConnection::onRecvCallback(const IocpTaskData& taskData)
{
	ISE_ASSERT(taskData.getErrorCode() == 0);

	if (taskData.getBytesTrans() < taskData.getDataSize())
	{
		eventLoop_->getIocpObject()->recv(
			(SOCKET)taskData.getFileHandle(),
			taskData.getEntireDataBuf(),
			taskData.getEntireDataSize(),
			taskData.getDataBuf() - taskData.getEntireDataBuf() + taskData.getBytesTrans(),
			taskData.getCallback(), taskData.getCaller(), taskData.getParams());
	}
	else
	{
		isRecving_ = false;
	}

	bytesRecved_ += taskData.getBytesTrans();

	while (!recvTaskQueue_.empty())
	{
		RECV_TASK& task = recvTaskQueue_.front();
		const char *buffer = recvBuffer_.peek();
		int packetSize = 0;

		if (bytesRecved_ > 0 &&
			task.packetMeasurer->isCompletePacket(buffer, bytesRecved_, packetSize) &&
			packetSize > 0)
		{
			bytesRecved_ -= packetSize;
			iseBusiness->onTcpRecvComplete(this, (void*)buffer, packetSize, task.params);
			recvTaskQueue_.pop_front();
			recvBuffer_.retrieve(packetSize);
		}
		else
			break;
	}

	if (!recvTaskQueue_.empty())
		tryRecv();
}

//-----------------------------------------------------------------------------
// ����: �Ͽ�����
//-----------------------------------------------------------------------------
void TcpConnection::doDisconnect()
{
	setEventLoop(NULL);
	BaseTcpConnection::doDisconnect();
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void TcpConnection::postSendTask(char *buffer, int size, const CustomParams& params)
{
	if (!buffer || size <= 0) return;

	sendBuffer_.append(buffer, size);

	SEND_TASK task;
	task.bytes = size;
	task.params = params;

	sendTaskQueue_.push_back(task);

	trySend();
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void TcpConnection::postRecvTask(PacketMeasurer *packetMeasurer, const CustomParams& params)
{
	if (!packetMeasurer) return;

	RECV_TASK task;
	task.packetMeasurer = packetMeasurer;
	task.params = params;

	recvTaskQueue_.push_back(task);

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

void IocpBufferAllocator::clear()
{
	AutoLocker locker(lock_);

	for (int i = 0; i < items_.getCount(); i++)
		delete[] (char*)items_[i];
	items_.clear();
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

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

void IocpPendingCounter::inc(PVOID caller, IOCP_TASK_TYPE taskType)
{
	AutoLocker locker(lock_);

	ITEMS::iterator iter = items_.find(caller);
	if (iter == items_.end())
	{
		COUNT_DATA Data = {0, 0};
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

	ITEMS::iterator iter = items_.find(caller);
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

	ITEMS::iterator iter = items_.find(caller);
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
		for (ITEMS::iterator iter = items_.begin(); iter != items_.end(); ++iter)
			result += iter->second.sendCount;
	}
	else if (taskType == ITT_RECV)
	{
		for (ITEMS::iterator iter = items_.begin(); iter != items_.end(); ++iter)
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
	const IOCP_CALLBACK_DEF& callbackDef, PVOID caller, const CustomParams& params)
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
	result->taskData.callback_ = callbackDef;
	result->taskData.caller_ = caller;
	result->taskData.params_ = params;
	result->taskData.entireDataBuf_ = buffer;
	result->taskData.entireDataSize_ = size;
	result->taskData.wsaBuffer_.buf = (char*)buffer + offset;
	result->taskData.wsaBuffer_.len = size - offset;

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
	const IOCP_CALLBACK_DEF& callbackDef = taskData.getCallback();
	if (callbackDef.proc != NULL)
		callbackDef.proc(taskData, callbackDef.param);
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
	} autoFinalizer(*this, overlappedPtr);

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
	const IOCP_CALLBACK_DEF& callbackDef, PVOID caller, const CustomParams& params)
{
	CIocpOverlappedData *ovDataPtr;
	IocpTaskData *taskPtr;
	DWORD numberOfBytesSent;

	pendingCounter_.inc(caller, ITT_SEND);

	ovDataPtr = createOverlappedData(ITT_SEND, (HANDLE)socketHandle, buffer, size,
		offset, callbackDef, caller, params);
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
	const IOCP_CALLBACK_DEF& callbackDef, PVOID caller, const CustomParams& params)
{
	CIocpOverlappedData *ovDataPtr;
	IocpTaskData *taskPtr;
	DWORD nNumberOfBytesRecvd, flags = 0;

	pendingCounter_.inc(caller, ITT_RECV);

	ovDataPtr = createOverlappedData(ITT_RECV, (HANDLE)socketHandle, buffer, size,
		offset, callbackDef, caller, params);
	taskPtr = &(ovDataPtr->taskData);

	if (::WSARecv(socketHandle, &taskPtr->wsaBuffer_, 1, &nNumberOfBytesRecvd, &flags,
		(LPWSAOVERLAPPED)ovDataPtr, NULL) == SOCKET_ERROR)
	{
		if (GetLastError() != ERROR_IO_PENDING)
			postError(GetLastError(), ovDataPtr);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop

TcpEventLoop::TcpEventLoop()
{
	iocpObject_ = new IocpObject();
}

TcpEventLoop::~TcpEventLoop()
{
	stop(false, true);
	delete iocpObject_;
}

//-----------------------------------------------------------------------------
// ����: ִ���¼�ѭ��
//-----------------------------------------------------------------------------
void TcpEventLoop::executeLoop(Thread *thread)
{
	while (!thread->isTerminated())
	{
		iocpObject_->work();
		processAcceptedConnList();
	}
}

//-----------------------------------------------------------------------------
// ����: �����¼�ѭ���е���������
//-----------------------------------------------------------------------------
void TcpEventLoop::wakeupLoop()
{
	iocpObject_->wakeup();
}

//-----------------------------------------------------------------------------
// ����: ��������ע�ᵽ�¼�ѭ����
//-----------------------------------------------------------------------------
void TcpEventLoop::registerConnection(TcpConnection *connection)
{
	iocpObject_->associateHandle(connection->getSocket().getHandle());
}

//-----------------------------------------------------------------------------
// ����: ���¼�ѭ����ע������
//-----------------------------------------------------------------------------
void TcpEventLoop::unregisterConnection(TcpConnection *connection)
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WIN32 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection

TcpConnection::TcpConnection(TcpServer *tcpServer,
	SOCKET socketHandle, const InetAddress& peerAddr) :
		BaseTcpConnection(socketHandle, peerAddr),
		tcpServer_(tcpServer),
		eventLoop_(NULL),
		bytesSent_(0),
		enableSend_(false),
		enableRecv_(false)
{
	// nothing
}

TcpConnection::~TcpConnection()
{
	setEventLoop(NULL);
}

//-----------------------------------------------------------------------------
// ����: ���á��Ƿ���ӿɷ����¼���
//-----------------------------------------------------------------------------
void TcpConnection::setSendEnabled(bool enabled)
{
	ISE_ASSERT(eventLoop_ != NULL);

	enableSend_ = enabled;
	eventLoop_->updateConnection(this, enableSend_, enableRecv_);
}

//-----------------------------------------------------------------------------
// ����: ���á��Ƿ���ӿɽ����¼���
//-----------------------------------------------------------------------------
void TcpConnection::setRecvEnabled(bool enabled)
{
	ISE_ASSERT(eventLoop_ != NULL);

	enableRecv_ = enabled;
	eventLoop_->updateConnection(this, enableSend_, enableRecv_);
}

//-----------------------------------------------------------------------------
// ����: �����ɷ��͡��¼�����ʱ�����Է�������
//-----------------------------------------------------------------------------
void TcpConnection::trySend()
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
			const SEND_TASK& task = sendTaskQueue_.front();
			if (bytesSent_ >= task.bytes)
			{
				bytesSent_ -= task.bytes;
				iseBusiness->onTcpSendComplete(this, task.params);
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
void TcpConnection::tryRecv()
{
	if (recvTaskQueue_.empty())
	{
		setRecvEnabled(false);
		return;
	}

	const int BUFFER_SIZE = 1024*64;
	char dataBuf[BUFFER_SIZE];

	int nBytesRecved = recvBuffer(dataBuf, BUFFER_SIZE, false);
	if (nBytesRecved < 0)
	{
		errorOccurred();
		return;
	}

	if (nBytesRecved > 0)
		recvBuffer_.append(dataBuf, nBytesRecved);

	while (!recvTaskQueue_.empty())
	{
		RECV_TASK& task = recvTaskQueue_.front();
		const char *buffer = recvBuffer_.peek();
		int readableBytes = recvBuffer_.getReadableBytes();
		int packetSize = 0;

		if (readableBytes > 0 &&
			task.packetMeasurer->isCompletePacket(buffer, readableBytes, packetSize) &&
			packetSize > 0)
		{
			iseBusiness->onTcpRecvComplete(this, (void*)buffer, packetSize, task.params);
			recvTaskQueue_.pop_front();
			recvBuffer_.retrieve(packetSize);
		}
		else
			break;
	}
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
void TcpConnection::errorOccurred()
{
	setEventLoop(NULL);
	iseBusiness->onTcpError(this);
	delete this;
}

//-----------------------------------------------------------------------------
// ����: ���ô����Ӵ������ĸ� eventLoop
//-----------------------------------------------------------------------------
void TcpConnection::setEventLoop(TcpEventLoop *eventLoop)
{
	if (eventLoop_)
	{
		eventLoop_->removeConnection(this);
		eventLoop_ = NULL;
	}

	if (eventLoop)
	{
		eventLoop_ = eventLoop;
		eventLoop->addConnection(this);
	}
}

//-----------------------------------------------------------------------------
// ����: �Ͽ�����
//-----------------------------------------------------------------------------
void TcpConnection::doDisconnect()
{
	setEventLoop(NULL);
	BaseTcpConnection::doDisconnect();
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void TcpConnection::postSendTask(char *buffer, int size, const CustomParams& params)
{
	if (!buffer || size <= 0) return;

	sendBuffer_.append(buffer, size);

	SEND_TASK task;
	task.bytes = size;
	task.params = params;

	sendTaskQueue_.push_back(task);

	if (!enableSend_)
		setSendEnabled(true);
}

//-----------------------------------------------------------------------------
// ����: �ύһ����������
//-----------------------------------------------------------------------------
void TcpConnection::postRecvTask(PacketMeasurer *packetMeasurer, const CustomParams& params)
{
	if (!packetMeasurer) return;

	RECV_TASK task;
	task.packetMeasurer = packetMeasurer;
	task.params = params;

	recvTaskQueue_.push_back(task);

	if (!enableRecv_)
		setRecvEnabled(true);
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

			if (ev.events & (EPOLLERR | EPOLLHUP))
				eventType = ET_ERROR;
			else if (ev.events & (EPOLLIN | EPOLLPRI))
				eventType = ET_ALLOW_RECV;
			else if (ev.events & EPOLLOUT)
				eventType = ET_ALLOW_SEND;

			if (eventType != ET_NONE && onNotifyEvent_.proc)
				onNotifyEvent_.proc(onNotifyEvent_.param, connection, eventType);
		}
	}
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
void EpollObject::setOnNotifyEventCallback(NOTIFY_EVENT_PROC proc, void *param)
{
	onNotifyEvent_.proc = proc;
	onNotifyEvent_.param = param;
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop

TcpEventLoop::TcpEventLoop()
{
	epollObject_ = new EpollObject();
	epollObject_->setOnNotifyEventCallback(onEpollNotifyEvent, this);
}

TcpEventLoop::~TcpEventLoop()
{
	stop(false, true);
	delete epollObject_;
}

//-----------------------------------------------------------------------------
// ����: EPoll �¼��ص�
//-----------------------------------------------------------------------------
void TcpEventLoop::onEpollNotifyEvent(void *param, TcpConnection *connection,
	EpollObject::EVENT_TYPE eventType)
{
	if (eventType == EpollObject::ET_ALLOW_SEND)
		connection->trySend();
	else if (eventType == EpollObject::ET_ALLOW_RECV)
		connection->tryRecv();
	else if (eventType == EpollObject::ET_ERROR)
		connection->errorOccurred();
}

//-----------------------------------------------------------------------------
// ����: ִ���¼�ѭ��
//-----------------------------------------------------------------------------
void TcpEventLoop::executeLoop(Thread *thread)
{
	while (!thread->isTerminated())
	{
		epollObject_->poll();
		processAcceptedConnList();
	}
}

//-----------------------------------------------------------------------------
// ����: �����¼�ѭ���е���������
//-----------------------------------------------------------------------------
void TcpEventLoop::wakeupLoop()
{
	epollObject_->wakeup();
}

//-----------------------------------------------------------------------------
// ����: ��������ע�ᵽ�¼�ѭ����
//-----------------------------------------------------------------------------
void TcpEventLoop::registerConnection(TcpConnection *connection)
{
	epollObject_->addConnection(connection, false, false);
}

//-----------------------------------------------------------------------------
// ����: ���¼�ѭ����ע������
//-----------------------------------------------------------------------------
void TcpEventLoop::unregisterConnection(TcpConnection *connection)
{
	epollObject_->removeConnection(connection);
}

//-----------------------------------------------------------------------------
// ����: ���´� eventLoop �е�ָ�����ӵ�����
//-----------------------------------------------------------------------------
void TcpEventLoop::updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv)
{
	epollObject_->updateConnection(connection, enableSend, enableRecv);
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class MainTcpServer

MainTcpServer::MainTcpServer() :
	eventLoopList_(iseApplication.getIseOptions().getTcpEventLoopCount()),
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
// ����: ����TCP������
//-----------------------------------------------------------------------------
void MainTcpServer::createTcpServerList()
{
	int serverCount = iseApplication.getIseOptions().getTcpServerCount();
	ISE_ASSERT(serverCount >= 0);

	tcpServerList_.resize(serverCount);
	for (int i = 0; i < serverCount; i++)
	{
		TcpServer *tcpServer = new TcpServer();
		tcpServer->customData() = (PVOID)i;

		tcpServerList_[i] = tcpServer;

		tcpServer->setOnCreateConnCallback(onCreateConnection, this);
		tcpServer->setOnAcceptConnCallback(onAcceptConnection, this);

		tcpServer->setLocalPort(iseApplication.getIseOptions().getTcpServerPort(i));
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
// ����: �������Ӷ���
//-----------------------------------------------------------------------------
void MainTcpServer::onCreateConnection(void *param, TcpServer *tcpServer,
	SOCKET socketHandle, const InetAddress& peerAddr, BaseTcpConnection*& connection)
{
	connection = new TcpConnection(tcpServer, socketHandle, peerAddr);
}

//-----------------------------------------------------------------------------
// ����: �յ��µ�����
// ע��:
//   �˻ص���TCP�����������߳�(TcpListenerThread)��ִ�С�
//   Ϊ�˱���TCP�������ļ����̳߳�Ϊϵͳ��ƿ�����ʲ�Ӧ�ڴ˼����߳��д���
//   iseBusiness->OnTcpConnection() �ص�����ȷ�������ǣ����²���������֪ͨ��
//   TcpEventLoop�������¼�ѭ���߳�(TcpEventLoopThread)�����ص���
//   ����������һ���ô��ǣ�����ͬһ��TCP���ӣ�iseBusiness->OnTcpXXX() ϵ�лص�
//   ����ͬһ���߳���ִ�С�
//-----------------------------------------------------------------------------
void MainTcpServer::onAcceptConnection(void *param, TcpServer *tcpServer,
	BaseTcpConnection *connection)
{
	MainTcpServer *thisObj = (MainTcpServer*)param;

	// round-robin
	static int index = 0;
	TcpEventLoop *eventLoop = (TcpEventLoop*)thisObj->eventLoopList_[index];
	index = (index >= thisObj->eventLoopList_.getCount() - 1 ? 0 : index + 1);

	((TcpConnection*)connection)->setEventLoop(eventLoop);
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

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
