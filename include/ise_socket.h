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
// ise_socket.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SOCKET_H_
#define _ISE_SOCKET_H_

#include "ise_options.h"

#ifdef ISE_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <string>
#endif

#include "ise_classes.h"
#include "ise_thread.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class Socket;
class UdpSocket;
class UdpClient;
class UdpServer;
class TcpSocket;
class BaseTcpConnection;
class TcpClient;
class TcpServer;
class ListenerThread;
class UdpListenerThread;
class UdpListenerThreadPool;
class TcpListenerThread;

///////////////////////////////////////////////////////////////////////////////
// ��������

#ifdef ISE_WIN32
const int SS_SD_RECV            = 0;
const int SS_SD_SEND            = 1;
const int SS_SD_BOTH            = 2;

const int SS_EINTR              = WSAEINTR;
const int SS_EBADF              = WSAEBADF;
const int SS_EACCES             = WSAEACCES;
const int SS_EFAULT             = WSAEFAULT;
const int SS_EINVAL             = WSAEINVAL;
const int SS_EMFILE             = WSAEMFILE;
const int SS_EWOULDBLOCK        = WSAEWOULDBLOCK;
const int SS_EINPROGRESS        = WSAEINPROGRESS;
const int SS_EALREADY           = WSAEALREADY;
const int SS_ENOTSOCK           = WSAENOTSOCK;
const int SS_EDESTADDRREQ       = WSAEDESTADDRREQ;
const int SS_EMSGSIZE           = WSAEMSGSIZE;
const int SS_EPROTOTYPE         = WSAEPROTOTYPE;
const int SS_ENOPROTOOPT        = WSAENOPROTOOPT;
const int SS_EPROTONOSUPPORT    = WSAEPROTONOSUPPORT;
const int SS_ESOCKTNOSUPPORT    = WSAESOCKTNOSUPPORT;
const int SS_EOPNOTSUPP         = WSAEOPNOTSUPP;
const int SS_EPFNOSUPPORT       = WSAEPFNOSUPPORT;
const int SS_EAFNOSUPPORT       = WSAEAFNOSUPPORT;
const int SS_EADDRINUSE         = WSAEADDRINUSE;
const int SS_EADDRNOTAVAIL      = WSAEADDRNOTAVAIL;
const int SS_ENETDOWN           = WSAENETDOWN;
const int SS_ENETUNREACH        = WSAENETUNREACH;
const int SS_ENETRESET          = WSAENETRESET;
const int SS_ECONNABORTED       = WSAECONNABORTED;
const int SS_ECONNRESET         = WSAECONNRESET;
const int SS_ENOBUFS            = WSAENOBUFS;
const int SS_EISCONN            = WSAEISCONN;
const int SS_ENOTCONN           = WSAENOTCONN;
const int SS_ESHUTDOWN          = WSAESHUTDOWN;
const int SS_ETOOMANYREFS       = WSAETOOMANYREFS;
const int SS_ETIMEDOUT          = WSAETIMEDOUT;
const int SS_ECONNREFUSED       = WSAECONNREFUSED;
const int SS_ELOOP              = WSAELOOP;
const int SS_ENAMETOOLONG       = WSAENAMETOOLONG;
const int SS_EHOSTDOWN          = WSAEHOSTDOWN;
const int SS_EHOSTUNREACH       = WSAEHOSTUNREACH;
const int SS_ENOTEMPTY          = WSAENOTEMPTY;
#endif

#ifdef ISE_LINUX
const int SS_SD_RECV            = SHUT_RD;
const int SS_SD_SEND            = SHUT_WR;
const int SS_SD_BOTH            = SHUT_RDWR;

const int SS_EINTR              = EINTR;
const int SS_EBADF              = EBADF;
const int SS_EACCES             = EACCES;
const int SS_EFAULT             = EFAULT;
const int SS_EINVAL             = EINVAL;
const int SS_EMFILE             = EMFILE;
const int SS_EWOULDBLOCK        = EWOULDBLOCK;
const int SS_EINPROGRESS        = EINPROGRESS;
const int SS_EALREADY           = EALREADY;
const int SS_ENOTSOCK           = ENOTSOCK;
const int SS_EDESTADDRREQ       = EDESTADDRREQ;
const int SS_EMSGSIZE           = EMSGSIZE;
const int SS_EPROTOTYPE         = EPROTOTYPE;
const int SS_ENOPROTOOPT        = ENOPROTOOPT;
const int SS_EPROTONOSUPPORT    = EPROTONOSUPPORT;
const int SS_ESOCKTNOSUPPORT    = ESOCKTNOSUPPORT;

const int SS_EOPNOTSUPP         = EOPNOTSUPP;
const int SS_EPFNOSUPPORT       = EPFNOSUPPORT;
const int SS_EAFNOSUPPORT       = EAFNOSUPPORT;
const int SS_EADDRINUSE         = EADDRINUSE;
const int SS_EADDRNOTAVAIL      = EADDRNOTAVAIL;
const int SS_ENETDOWN           = ENETDOWN;
const int SS_ENETUNREACH        = ENETUNREACH;
const int SS_ENETRESET          = ENETRESET;
const int SS_ECONNABORTED       = ECONNABORTED;
const int SS_ECONNRESET         = ECONNRESET;
const int SS_ENOBUFS            = ENOBUFS;
const int SS_EISCONN            = EISCONN;
const int SS_ENOTCONN           = ENOTCONN;
const int SS_ESHUTDOWN          = ESHUTDOWN;
const int SS_ETOOMANYREFS       = ETOOMANYREFS;
const int SS_ETIMEDOUT          = ETIMEDOUT;
const int SS_ECONNREFUSED       = ECONNREFUSED;
const int SS_ELOOP              = ELOOP;
const int SS_ENAMETOOLONG       = ENAMETOOLONG;
const int SS_EHOSTDOWN          = EHOSTDOWN;
const int SS_EHOSTUNREACH       = EHOSTUNREACH;
const int SS_ENOTEMPTY          = ENOTEMPTY;
#endif

///////////////////////////////////////////////////////////////////////////////
// ������Ϣ (ISE Socket Error Message)

const char* const SSEM_ERROR             = "Socket Error #%d: %s";
const char* const SSEM_SOCKETERROR       = "Socket error";
const char* const SSEM_TCPSENDTIMEOUT    = "TCP send timeout";
const char* const SSEM_TCPRECVTIMEOUT    = "TCP recv timeout";

const char* const SSEM_EINTR             = "Interrupted system call.";
const char* const SSEM_EBADF             = "Bad file number.";
const char* const SSEM_EACCES            = "Access denied.";
const char* const SSEM_EFAULT            = "Buffer fault.";
const char* const SSEM_EINVAL            = "Invalid argument.";
const char* const SSEM_EMFILE            = "Too many open files.";
const char* const SSEM_EWOULDBLOCK       = "Operation would block.";
const char* const SSEM_EINPROGRESS       = "Operation now in progress.";
const char* const SSEM_EALREADY          = "Operation already in progress.";
const char* const SSEM_ENOTSOCK          = "Socket operation on non-socket.";
const char* const SSEM_EDESTADDRREQ      = "Destination address required.";
const char* const SSEM_EMSGSIZE          = "Message too long.";
const char* const SSEM_EPROTOTYPE        = "Protocol wrong type for socket.";
const char* const SSEM_ENOPROTOOPT       = "Bad protocol option.";
const char* const SSEM_EPROTONOSUPPORT   = "Protocol not supported.";
const char* const SSEM_ESOCKTNOSUPPORT   = "Socket type not supported.";
const char* const SSEM_EOPNOTSUPP        = "Operation not supported on socket.";
const char* const SSEM_EPFNOSUPPORT      = "Protocol family not supported.";
const char* const SSEM_EAFNOSUPPORT      = "Address family not supported by protocol family.";
const char* const SSEM_EADDRINUSE        = "Address already in use.";
const char* const SSEM_EADDRNOTAVAIL     = "Cannot assign requested address.";
const char* const SSEM_ENETDOWN          = "Network is down.";
const char* const SSEM_ENETUNREACH       = "Network is unreachable.";
const char* const SSEM_ENETRESET         = "Net dropped connection or reset.";
const char* const SSEM_ECONNABORTED      = "Software caused connection abort.";
const char* const SSEM_ECONNRESET        = "Connection reset by peer.";
const char* const SSEM_ENOBUFS           = "No buffer space available.";
const char* const SSEM_EISCONN           = "Socket is already connected.";
const char* const SSEM_ENOTCONN          = "Socket is not connected.";
const char* const SSEM_ESHUTDOWN         = "Cannot send or receive after socket is closed.";
const char* const SSEM_ETOOMANYREFS      = "Too many references, cannot splice.";
const char* const SSEM_ETIMEDOUT         = "Connection timed out.";
const char* const SSEM_ECONNREFUSED      = "Connection refused.";
const char* const SSEM_ELOOP             = "Too many levels of symbolic links.";
const char* const SSEM_ENAMETOOLONG      = "File name too long.";
const char* const SSEM_EHOSTDOWN         = "Host is down.";
const char* const SSEM_EHOSTUNREACH      = "No route to host.";
const char* const SSEM_ENOTEMPTY         = "Directory not empty";

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

#ifdef ISE_WIN32
typedef int socklen_t;
#endif
#ifdef ISE_LINUX
typedef int SOCKET;
#define INVALID_SOCKET ((SOCKET)(-1))
#endif

typedef struct sockaddr_in SockAddr;

// ����Э������(UDP|TCP)
enum NET_PROTO_TYPE
{
	NPT_UDP,        // UDP
	NPT_TCP         // TCP
};

// DTP Э������(TCP|UTP)
enum DTP_PROTO_TYPE
{
	DPT_TCP,        // TCP
	DPT_UTP         // UTP (UDP Transfer Protocol)
};

// �첽���ӵ�״̬
enum ASYNC_CONNECT_STATE
{
	ACS_NONE,       // ��δ��������
	ACS_CONNECTING, // ��δ������ϣ�����δ��������
	ACS_CONNECTED,  // �����ѽ����ɹ�
	ACS_FAILED      // ���ӹ����з����˴��󣬵�������ʧ��
};

#pragma pack(1)     // 1�ֽڶ���

// ��ַ��Ϣ
struct InetAddress
{
	UINT nIp;       // IP (�����ֽ�˳��)
	int port;      // �˿�

	InetAddress() : nIp(0), port(0) {}
	InetAddress(UINT _nIp, int _nPort)
		{ nIp = _nIp;  port = _nPort; }
	bool operator == (const InetAddress& rhs) const
		{ return (nIp == rhs.nIp && port == rhs.port); }
	bool operator != (const InetAddress& rhs) const
		{ return !((*this) == rhs); }
};

#pragma pack()

// �ص���������
typedef void (*UDPSVR_ON_RECV_DATA_PROC)(void *param, void *packetBuffer,
	int packetSize, const InetAddress& peerAddr);
typedef void (*TCPSVR_ON_CREATE_CONN_PROC)(void *param, TcpServer *tcpServer,
	SOCKET socketHandle, const InetAddress& peerAddr, BaseTcpConnection*& connection);
typedef void (*TCPSVR_ON_ACCEPT_CONN_PROC)(void *param, TcpServer *tcpServer,
	BaseTcpConnection *connection);

///////////////////////////////////////////////////////////////////////////////
// �����

// �����ʼ��/������
void networkInitialize();
void networkFinalize();
bool isNetworkInited();
void ensureNetworkInited();

// �����ƽ̨����ĺ���
int iseSocketGetLastError();
string iseSocketGetErrorMsg(int errorCode);
string iseSocketGetLastErrMsg();
void iseCloseSocket(SOCKET handle);

// �����
string ipToString(UINT ip);
UINT stringToIp(const string& str);
void getSocketAddr(SockAddr& sockAddr, UINT ipHostValue, int port);
int getFreePort(NET_PROTO_TYPE proto, int startPort, int checkTimes);
void getLocalIpList(StringArray& ipList);
string getLocalIp();
string lookupHostAddr(const string& host);
void iseThrowSocketLastError();

///////////////////////////////////////////////////////////////////////////////
// class Socket - �׽�����

class Socket
{
public:
	friend class TcpServer;

protected:
	bool isActive_;     // �׽����Ƿ�׼������
	SOCKET handle_;     // �׽��־��
	int domain_;        // �׽��ֵ�Э����� (PF_UNIX, PF_INET, PF_INET6, PF_IPX, ...)
	int type_;          // �׽������ͣ�����ָ�� (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_RDM, SOCK_SEQPACKET)
	int protocol_;      // �׽�������Э�飬��Ϊ0 (IPPROTO_IP, IPPROTO_UDP, IPPROTO_TCP, ...)
	bool isBlockMode_;  // �Ƿ�Ϊ����ģʽ (ȱʡΪ����ģʽ)

private:
	void doSetBlockMode(SOCKET handle, bool value);
	void doClose();

protected:
	void setActive(bool value);
	void setDomain(int value);
	void setType(int value);
	void setProtocol(int value);

	void bind(int port);
public:
	Socket();
	virtual ~Socket();

	virtual void open();
	virtual void close();

	bool isActive() const { return isActive_; }
	SOCKET getHandle() const { return handle_; }
	bool isBlockMode() const { return isBlockMode_; }
	void setBlockMode(bool value);
	void setHandle(SOCKET value);
};

///////////////////////////////////////////////////////////////////////////////
// class UdpSocket - UDP �׽�����

class UdpSocket : public Socket
{
public:
	UdpSocket()
	{
		type_ = SOCK_DGRAM;
		protocol_ = IPPROTO_UDP;
		isBlockMode_ = true;
	}

	int recvBuffer(void *buffer, int size);
	int recvBuffer(void *buffer, int size, InetAddress& peerAddr);
	int sendBuffer(void *buffer, int size, const InetAddress& peerAddr, int sendTimes = 1);

	virtual void open();
};

///////////////////////////////////////////////////////////////////////////////
// class UdpClient - UDP Client ��

class UdpClient : public UdpSocket
{
public:
	UdpClient() { open(); }
};

///////////////////////////////////////////////////////////////////////////////
// class UdpServer - UDP Server ��

class UdpServer : public UdpSocket
{
public:
	friend class UdpListenerThread;
private:
	int localPort_;
	UdpListenerThreadPool *listenerThreadPool_;
	CallbackDef<UDPSVR_ON_RECV_DATA_PROC> onRecvData_;
	PVOID customData_;
private:
	void dataReceived(void *packetBuffer, int packetSize, const InetAddress& peerAddr);
protected:
	virtual void startListenerThreads();
	virtual void stopListenerThreads();
public:
	UdpServer();
	virtual ~UdpServer();

	virtual void open();
	virtual void close();

	int getLocalPort() const { return localPort_; }
	void setLocalPort(int value);

	int getListenerThreadCount() const;
	void setListenerThreadCount(int value);

	PVOID& customData() { return customData_; }

	void setOnRecvDataCallback(UDPSVR_ON_RECV_DATA_PROC proc, void *param = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// class TcpSocket - TCP �׽�����

class TcpSocket : public Socket
{
public:
	TcpSocket()
	{
		type_ = SOCK_STREAM;
		protocol_ = IPPROTO_TCP;
		isBlockMode_ = false;
	}
};

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpConnection - TCP Connection ����

class BaseTcpConnection
{
protected:
	TcpSocket socket_;
	InetAddress peerAddr_;
	PVOID customData_;
private:
	int doSyncSendBuffer(void *buffer, int size, int timeoutMSecs = -1);
	int doSyncRecvBuffer(void *buffer, int size, int timeoutMSecs = -1);
	int doAsyncSendBuffer(void *buffer, int size);
	int doAsyncRecvBuffer(void *buffer, int size);
protected:
	int sendBuffer(void *buffer, int size, bool syncMode = false, int timeoutMSecs = -1);
	int recvBuffer(void *buffer, int size, bool syncMode = false, int timeoutMSecs = -1);
protected:
	virtual void doDisconnect();
public:
	BaseTcpConnection();
	BaseTcpConnection(SOCKET socketHandle, const InetAddress& peerAddr);
	virtual ~BaseTcpConnection() {}

	virtual bool isConnected() const;
	void disconnect();

	const TcpSocket& getSocket() const { return socket_; }
	const InetAddress& getPeerAddr() const { return peerAddr_; }
	PVOID& customData() { return customData_; }
};

///////////////////////////////////////////////////////////////////////////////
// class TcpClient - TCP Client ��

class TcpClient : public BaseTcpConnection
{
public:
	// ����ʽ����
	void connect(const string& ip, int port);
	// �첽(������ʽ)���� (���� enum ASYNC_CONNECT_STATE)
	int asyncConnect(const string& ip, int port, int timeoutMSecs = -1);
	// ����첽���ӵ�״̬ (���� enum ASYNC_CONNECT_STATE)
	int checkAsyncConnectState(int timeoutMSecs = -1);

	using BaseTcpConnection::sendBuffer;
	using BaseTcpConnection::recvBuffer;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpServer - TCP Server ��

class TcpServer
{
public:
	friend class TcpListenerThread;
public:
	enum { LISTEN_QUEUE_SIZE = 30 };   // TCP�������г���
private:
	TcpSocket socket_;
	int localPort_;
	TcpListenerThread *listenerThread_;
	CallbackDef<TCPSVR_ON_CREATE_CONN_PROC> onCreateConn_;
	CallbackDef<TCPSVR_ON_ACCEPT_CONN_PROC> onAcceptConn_;
	PVOID customData_;
private:
	BaseTcpConnection* createConnection(SOCKET socketHandle, const InetAddress& peerAddr);
	void acceptConnection(BaseTcpConnection *connection);
protected:
	virtual void startListenerThread();
	virtual void stopListenerThread();
public:
	TcpServer();
	virtual ~TcpServer();

	virtual void open();
	virtual void close();

	bool isActive() const { return socket_.isActive(); }
	void setActive(bool value);

	int getLocalPort() const { return localPort_; }
	void setLocalPort(int value);

	const TcpSocket& getSocket() const { return socket_; }
	PVOID& customData() { return customData_; }

	void setOnCreateConnCallback(TCPSVR_ON_CREATE_CONN_PROC proc, void *param = NULL);
	void setOnAcceptConnCallback(TCPSVR_ON_ACCEPT_CONN_PROC proc, void *param = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// class ListenerThread - �����߳���

class ListenerThread : public Thread
{
protected:
	virtual void execute() {}
public:
	ListenerThread()
	{
#ifdef ISE_WIN32
		setPriority(THREAD_PRI_HIGHEST);
#endif
#ifdef ISE_LINUX
		setPolicy(THREAD_POL_RR);
		setPriority(THREAD_PRI_HIGH);
#endif
	}
	virtual ~ListenerThread() {}
};

///////////////////////////////////////////////////////////////////////////////
// class UdpListenerThread - UDP�����������߳���

class UdpListenerThread : public ListenerThread
{
private:
	UdpListenerThreadPool *threadPool_;  // �����̳߳�
	UdpServer *udpServer_;               // ����UDP������
	int index_;                          // �߳��ڳ��е�������(0-based)
protected:
	virtual void execute();
public:
	explicit UdpListenerThread(UdpListenerThreadPool *threadPool, int index);
	virtual ~UdpListenerThread();
};

///////////////////////////////////////////////////////////////////////////////
// class UdpListenerThreadPool - UDP�����������̳߳���

class UdpListenerThreadPool
{
private:
	UdpServer *udpServer_;                // ����UDP������
	ThreadList threadList_;               // �߳��б�
	int maxThreadCount_;                  // ��������߳�����
public:
	explicit UdpListenerThreadPool(UdpServer *udpServer);
	virtual ~UdpListenerThreadPool();

	void registerThread(UdpListenerThread *thread);
	void unregisterThread(UdpListenerThread *thread);

	void startThreads();
	void stopThreads();

	int getMaxThreadCount() const { return maxThreadCount_; }
	void setMaxThreadCount(int value) { maxThreadCount_ = value; }

	// ��������UDP������
	UdpServer& getUdpServer() { return *udpServer_; }
};

///////////////////////////////////////////////////////////////////////////////
// class TcpListenerThread - TCP�����������߳���

class TcpListenerThread : public ListenerThread
{
private:
	TcpServer *tcpServer_;
protected:
	virtual void execute();
public:
	explicit TcpListenerThread(TcpServer *tcpServer);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SOCKET_H_
