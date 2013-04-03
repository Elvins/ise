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
// �ļ�����: ise_socket.cpp
// ��������: ����������
///////////////////////////////////////////////////////////////////////////////

#include "ise_socket.h"
#include "ise_sysutils.h"
#include "ise_exceptions.h"

#ifdef ISE_COMPILER_VC
#pragma comment(lib, "ws2_32.lib")
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// �����

static int s_nNetworkInitCount = 0;

//-----------------------------------------------------------------------------
// ����: �����ʼ�� (��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void NetworkInitialize()
{
	s_nNetworkInitCount++;
	if (s_nNetworkInitCount > 1) return;

#ifdef ISE_WIN32
	WSAData Wsd;
	if (WSAStartup(MAKEWORD(2, 2), &Wsd) != 0)
	{
		s_nNetworkInitCount--;
		IseThrowSocketLastError();
	}
#endif
}

//-----------------------------------------------------------------------------
// ����: ���������
//-----------------------------------------------------------------------------
void NetworkFinalize()
{
	if (s_nNetworkInitCount > 0)
		s_nNetworkInitCount--;
	if (s_nNetworkInitCount != 0) return;

#ifdef ISE_WIN32
	WSACleanup();
#endif
}

//-----------------------------------------------------------------------------

bool IsNetworkInited()
{
	return (s_nNetworkInitCount > 0);
}

//-----------------------------------------------------------------------------

void EnsureNetworkInited()
{
	if (!IsNetworkInited())
		NetworkInitialize();
}

//-----------------------------------------------------------------------------
// ����: ȡ�����Ĵ������
//-----------------------------------------------------------------------------
int IseSocketGetLastError()
{
#ifdef ISE_WIN32
	return WSAGetLastError();
#endif
#ifdef ISE_LINUX
	return errno;
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ش�����Ϣ
//-----------------------------------------------------------------------------
string IseSocketGetErrorMsg(int nError)
{
	string strResult;
	const char *p = "";

	switch (nError)
	{
	case SS_EINTR:              p = SSEM_EINTR;                break;
	case SS_EBADF:              p = SSEM_EBADF;                break;
	case SS_EACCES:             p = SSEM_EACCES;               break;
	case SS_EFAULT:             p = SSEM_EFAULT;               break;
	case SS_EINVAL:             p = SSEM_EINVAL;               break;
	case SS_EMFILE:             p = SSEM_EMFILE;               break;

	case SS_EWOULDBLOCK:        p = SSEM_EWOULDBLOCK;          break;
	case SS_EINPROGRESS:        p = SSEM_EINPROGRESS;          break;
	case SS_EALREADY:           p = SSEM_EALREADY;             break;
	case SS_ENOTSOCK:           p = SSEM_ENOTSOCK;             break;
	case SS_EDESTADDRREQ:       p = SSEM_EDESTADDRREQ;         break;
	case SS_EMSGSIZE:           p = SSEM_EMSGSIZE;             break;
	case SS_EPROTOTYPE:         p = SSEM_EPROTOTYPE;           break;
	case SS_ENOPROTOOPT:        p = SSEM_ENOPROTOOPT;          break;
	case SS_EPROTONOSUPPORT:    p = SSEM_EPROTONOSUPPORT;      break;
	case SS_ESOCKTNOSUPPORT:    p = SSEM_ESOCKTNOSUPPORT;      break;
	case SS_EOPNOTSUPP:         p = SSEM_EOPNOTSUPP;           break;
	case SS_EPFNOSUPPORT:       p = SSEM_EPFNOSUPPORT;         break;
	case SS_EAFNOSUPPORT:       p = SSEM_EAFNOSUPPORT;         break;
	case SS_EADDRINUSE:         p = SSEM_EADDRINUSE;           break;
	case SS_EADDRNOTAVAIL:      p = SSEM_EADDRNOTAVAIL;        break;
	case SS_ENETDOWN:           p = SSEM_ENETDOWN;             break;
	case SS_ENETUNREACH:        p = SSEM_ENETUNREACH;          break;
	case SS_ENETRESET:          p = SSEM_ENETRESET;            break;
	case SS_ECONNABORTED:       p = SSEM_ECONNABORTED;         break;
	case SS_ECONNRESET:         p = SSEM_ECONNRESET;           break;
	case SS_ENOBUFS:            p = SSEM_ENOBUFS;              break;
	case SS_EISCONN:            p = SSEM_EISCONN;              break;
	case SS_ENOTCONN:           p = SSEM_ENOTCONN;             break;
	case SS_ESHUTDOWN:          p = SSEM_ESHUTDOWN;            break;
	case SS_ETOOMANYREFS:       p = SSEM_ETOOMANYREFS;         break;
	case SS_ETIMEDOUT:          p = SSEM_ETIMEDOUT;            break;
	case SS_ECONNREFUSED:       p = SSEM_ECONNREFUSED;         break;
	case SS_ELOOP:              p = SSEM_ELOOP;                break;
	case SS_ENAMETOOLONG:       p = SSEM_ENAMETOOLONG;         break;
	case SS_EHOSTDOWN:          p = SSEM_EHOSTDOWN;            break;
	case SS_EHOSTUNREACH:       p = SSEM_EHOSTUNREACH;         break;
	case SS_ENOTEMPTY:          p = SSEM_ENOTEMPTY;            break;
	}

	strResult = FormatString(SSEM_ERROR, nError, p);
	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ȡ��������Ķ�Ӧ��Ϣ
//-----------------------------------------------------------------------------
string IseSocketGetLastErrMsg()
{
	return IseSocketGetErrorMsg(IseSocketGetLastError());
}

//-----------------------------------------------------------------------------
// ����: �ر��׽���
//-----------------------------------------------------------------------------
void IseCloseSocket(SOCKET nHandle)
{
#ifdef ISE_WIN32
	closesocket(nHandle);
#endif
#ifdef ISE_LINUX
	close(nHandle);
#endif
}

//-----------------------------------------------------------------------------
// ����: ����IP(�����ֽ�˳��) -> ����IP
//-----------------------------------------------------------------------------
string IpToString(UINT nIp)
{
#pragma pack(1)
	union CIpUnion
	{
		UINT nValue;
		struct
		{
			unsigned char ch1;  // nValue������ֽ�
			unsigned char ch2;
			unsigned char ch3;
			unsigned char ch4;
		} Bytes;
	} IpUnion;
#pragma pack()
	char strString[64];

	IpUnion.nValue = nIp;
	sprintf(strString, "%u.%u.%u.%u", IpUnion.Bytes.ch4, IpUnion.Bytes.ch3,
		IpUnion.Bytes.ch2, IpUnion.Bytes.ch1);
	return &strString[0];
}

//-----------------------------------------------------------------------------
// ����: ����IP -> ����IP(�����ֽ�˳��)
//-----------------------------------------------------------------------------
UINT StringToIp(const string& strString)
{
#pragma pack(1)
	union CIpUnion
	{
		UINT nValue;
		struct
		{
			unsigned char ch1;
			unsigned char ch2;
			unsigned char ch3;
			unsigned char ch4;
		} Bytes;
	} IpUnion;
#pragma pack()
	IntegerArray IntList;

	SplitStringToInt(strString, '.', IntList);
	if (IntList.size() == 4)
	{
		IpUnion.Bytes.ch1 = IntList[3];
		IpUnion.Bytes.ch2 = IntList[2];
		IpUnion.Bytes.ch3 = IntList[1];
		IpUnion.Bytes.ch4 = IntList[0];
		return IpUnion.nValue;
	}
	else
		return 0;
}

//-----------------------------------------------------------------------------
// ����: ��� SockAddr �ṹ
//-----------------------------------------------------------------------------
void GetSocketAddr(SockAddr& SockAddr, UINT nIpHostValue, int nPort)
{
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = htonl(nIpHostValue);
	SockAddr.sin_port = htons(nPort);
}

//-----------------------------------------------------------------------------
// ����: ȡ�ÿ��ж˿ں�
// ����:
//   nProto      - ����Э��(UDP,TCP)
//   nStartPort  - ��ʼ�˿ں�
//   nCheckTimes - ������
// ����:
//   ���ж˿ں� (��ʧ���򷵻� 0)
//-----------------------------------------------------------------------------
int GetFreePort(NET_PROTO_TYPE nProto, int nStartPort, int nCheckTimes)
{
	int i, nResult = 0;
	bool bSuccess;
	SockAddr Addr;

	ise::NetworkInitialize();
	struct CAutoFinalizer {
		~CAutoFinalizer() { ise::NetworkFinalize(); }
	} AutoFinalizer;

	SOCKET s = socket(PF_INET, (nProto == NPT_UDP? SOCK_DGRAM : SOCK_STREAM), IPPROTO_IP);
	if (s == INVALID_SOCKET) return 0;

	bSuccess = false;
	for (i = 0; i < nCheckTimes; i++)
	{
		nResult = nStartPort + i;
		GetSocketAddr(Addr, ntohl(INADDR_ANY), nResult);
		if (bind(s, (struct sockaddr*)&Addr, sizeof(Addr)) != -1)
		{
			bSuccess = true;
			break;
		}
	}

	IseCloseSocket(s);
	if (!bSuccess) nResult = 0;
	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ȡ�ñ���IP�б�
//-----------------------------------------------------------------------------
void GetLocalIpList(StringArray& IpList)
{
#ifdef ISE_WIN32
	char sHostName[250];
	hostent *pHostEnt;
	in_addr **addr_ptr;

	IpList.clear();
	gethostname(sHostName, sizeof(sHostName));
	pHostEnt = gethostbyname(sHostName);
	if (pHostEnt)
	{
		addr_ptr = (in_addr**)(pHostEnt->h_addr_list);
		int i = 0;
		while (addr_ptr[i])
		{
			UINT nIp = ntohl( *(UINT*)(addr_ptr[i]) );
			IpList.push_back(IpToString(nIp));
			i++;
		}
	}
#endif
#ifdef ISE_LINUX
	const int MAX_INTERFACES = 16;
	int nFd, nIntfCount;
	struct ifreq buf[MAX_INTERFACES];
	struct ifconf ifc;

	IpList.clear();
	if ((nFd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t) buf;
		if (!ioctl(nFd, SIOCGIFCONF, (char*)&ifc))
		{
			nIntfCount = ifc.ifc_len / sizeof(struct ifreq);
			for (int i = 0; i < nIntfCount; i++)
			{
				ioctl(nFd, SIOCGIFADDR, (char*)&buf[i]);
				UINT nIp = ((struct sockaddr_in*)(&buf[i].ifr_addr))->sin_addr.s_addr;
				IpList.push_back(IpToString(ntohl(nIp)));
			}
		}
		close(nFd);
	}
#endif
}

//-----------------------------------------------------------------------------
// ����: ȡ�ñ���IP
//-----------------------------------------------------------------------------
string GetLocalIp()
{
	StringArray IpList;
	string strResult;

	GetLocalIpList(IpList);
	if (!IpList.empty())
	{
		if (IpList.size() == 1)
			strResult = IpList[0];
		else
		{
			for (UINT i = 0; i < IpList.size(); i++)
				if (IpList[i] != "127.0.0.1")
				{
					strResult = IpList[i];
					break;
				}

			if (strResult.length() == 0)
				strResult = IpList[0];
		}
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ������ַ -> IP��ַ
// ��ע: ��ʧ�ܣ��򷵻ؿ��ַ�����
//-----------------------------------------------------------------------------
string LookupHostAddr(const string& strHost)
{
	string strResult = "";

	struct hostent* pHost = gethostbyname(strHost.c_str());
	if (pHost != NULL)
		strResult = IpToString(ntohl(((struct in_addr *)pHost->h_addr)->s_addr));

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ȡ���Ĵ����벢�׳��쳣
//-----------------------------------------------------------------------------
void IseThrowSocketLastError()
{
	IseThrowSocketException(IseSocketGetLastErrMsg().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// class CSocket

CSocket::CSocket() :
	m_bActive(false),
	m_nHandle(INVALID_SOCKET),
	m_nDomain(PF_INET),
	m_nType(SOCK_STREAM),
	m_nProtocol(IPPROTO_IP),
	m_bBlockMode(true)
{
	// nothing
}

//-----------------------------------------------------------------------------

CSocket::~CSocket()
{
	Close();
}

//-----------------------------------------------------------------------------

void CSocket::DoSetBlockMode(SOCKET nHandle, bool bValue)
{
#ifdef ISE_WIN32
	UINT nNotBlock = (bValue? 0 : 1);
	if (ioctlsocket(nHandle, FIONBIO, (u_long*)&nNotBlock) < 0)
		IseThrowSocketLastError();
#endif
#ifdef ISE_LINUX
	int nFlag = fcntl(nHandle, F_GETFL);

	if (bValue)
		nFlag &= ~O_NONBLOCK;
	else
		nFlag |= O_NONBLOCK;

	if (fcntl(nHandle, F_SETFL, nFlag) < 0)
		IseThrowSocketLastError();
#endif
}

//-----------------------------------------------------------------------------

void CSocket::DoClose()
{
	shutdown(m_nHandle, SS_SD_BOTH);
	IseCloseSocket(m_nHandle);
	m_nHandle = INVALID_SOCKET;
	m_bActive = false;
}

//-----------------------------------------------------------------------------

void CSocket::SetActive(bool bValue)
{
	if (bValue != m_bActive)
	{
		if (bValue) Open();
		else Close();
	}
}

//-----------------------------------------------------------------------------

void CSocket::SetDomain(int nValue)
{
	if (nValue != m_nDomain)
	{
		if (GetActive()) Close();
		m_nDomain = nValue;
	}
}

//-----------------------------------------------------------------------------

void CSocket::SetType(int nValue)
{
	if (nValue != m_nType)
	{
		if (GetActive()) Close();
		m_nType = nValue;
	}
}

//-----------------------------------------------------------------------------

void CSocket::SetProtocol(int nValue)
{
	if (nValue != m_nProtocol)
	{
		if (GetActive()) Close();
		m_nProtocol = nValue;
	}
}

//-----------------------------------------------------------------------------

void CSocket::SetBlockMode(bool bValue)
{
	// �˴���Ӧ�� bValue != m_bBlockMode ���жϣ���Ϊ�ڲ�ͬ��ƽ̨�£�
	// �׽���������ʽ��ȱʡֵ��һ����
	if (m_bActive)
		DoSetBlockMode(m_nHandle, bValue);
	m_bBlockMode = bValue;
}

//-----------------------------------------------------------------------------

void CSocket::SetHandle(SOCKET nValue)
{
	if (nValue != m_nHandle)
	{
		if (GetActive()) Close();
		m_nHandle = nValue;
		if (m_nHandle != INVALID_SOCKET)
			m_bActive = true;
	}
}

//-----------------------------------------------------------------------------
// ����: ���׽���
//-----------------------------------------------------------------------------
void CSocket::Bind(int nPort)
{
	SockAddr Addr;
	int nValue = 1;

	GetSocketAddr(Addr, ntohl(INADDR_ANY), nPort);

	// ǿ�����°󶨣��������������ص�Ӱ��
	setsockopt(m_nHandle, SOL_SOCKET, SO_REUSEADDR, (char*)&nValue, sizeof(int));
	// ���׽���
	if (bind(m_nHandle, (struct sockaddr*)&Addr, sizeof(Addr)) < 0)
		IseThrowSocketLastError();
}

//-----------------------------------------------------------------------------
// ����: ���׽���
//-----------------------------------------------------------------------------
void CSocket::Open()
{
	if (!m_bActive)
	{
		try
		{
			SOCKET nHandle;
			nHandle = socket(m_nDomain, m_nType, m_nProtocol);
			if (nHandle == INVALID_SOCKET)
				IseThrowSocketLastError();
			m_bActive = (nHandle != INVALID_SOCKET);
			if (m_bActive)
			{
				m_nHandle = nHandle;
				SetBlockMode(m_bBlockMode);
			}
		}
		catch (CSocketException&)
		{
			DoClose();
			throw;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: �ر��׽���
//-----------------------------------------------------------------------------
void CSocket::Close()
{
	if (m_bActive) DoClose();
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpSocket

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
int CUdpSocket::RecvBuffer(void *pBuffer, int nSize)
{
	CPeerAddress PeerAddr;
	return RecvBuffer(pBuffer, nSize, PeerAddr);
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
int CUdpSocket::RecvBuffer(void *pBuffer, int nSize, CPeerAddress& PeerAddr)
{
	SockAddr Addr;
	int nBytes;
	socklen_t nSockLen = sizeof(Addr);

	memset(&Addr, 0, sizeof(Addr));
	nBytes = recvfrom(m_nHandle, (char*)pBuffer, nSize, 0, (struct sockaddr*)&Addr, &nSockLen);

	if (nBytes > 0)
	{
		PeerAddr.nIp = ntohl(Addr.sin_addr.s_addr);
		PeerAddr.nPort = ntohs(Addr.sin_port);
	}

	return nBytes;
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
int CUdpSocket::SendBuffer(void *pBuffer, int nSize, const CPeerAddress& PeerAddr, int nSendTimes)
{
	int nResult = 0;
	SockAddr Addr;
	socklen_t nSockLen = sizeof(Addr);

	GetSocketAddr(Addr, PeerAddr.nIp, PeerAddr.nPort);

	for (int i = 0; i < nSendTimes; i++)
		nResult = sendto(m_nHandle, (char*)pBuffer, nSize, 0, (struct sockaddr*)&Addr, nSockLen);

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ���׽���
//-----------------------------------------------------------------------------
void CUdpSocket::Open()
{
	CSocket::Open();

#ifdef ISE_WIN32
	if (m_bActive)
	{
		// Windows�£����յ�ICMP��("ICMP port unreachable")��recvfrom������-1��
		// ����Ϊ WSAECONNRESET(10054)��������ķ������ø���Ϊ��

		#define IOC_VENDOR        0x18000000
		#define _WSAIOW(x,y)      (IOC_IN|(x)|(y))
		#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)

		DWORD dwBytesReturned = 0;
		BOOL bNewBehavior = FALSE;
		::WSAIoctl(GetHandle(), SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
			NULL, 0, &dwBytesReturned, NULL, NULL);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpServer

CUdpServer::CUdpServer() :
	m_nLocalPort(0),
	m_pListenerThreadPool(NULL)
{
	m_pListenerThreadPool = new CUdpListenerThreadPool(this);
	SetListenerThreadCount(1);
}

//-----------------------------------------------------------------------------

CUdpServer::~CUdpServer()
{
	delete m_pListenerThreadPool;
}

//-----------------------------------------------------------------------------
// ����: �յ����ݰ�
//-----------------------------------------------------------------------------
void CUdpServer::DataReceived(void *pPacketBuffer, int nPacketSize, const CPeerAddress& PeerAddr)
{
	if (m_OnRecvData.pProc)
		m_OnRecvData.pProc(m_OnRecvData.pParam, pPacketBuffer, nPacketSize, PeerAddr);
}

//-----------------------------------------------------------------------------
// ����: ���������߳�
//-----------------------------------------------------------------------------
void CUdpServer::StartListenerThreads()
{
	m_pListenerThreadPool->StartThreads();
}

//-----------------------------------------------------------------------------
// ����: ֹͣ�����߳�
//-----------------------------------------------------------------------------
void CUdpServer::StopListenerThreads()
{
	m_pListenerThreadPool->StopThreads();
}

//-----------------------------------------------------------------------------
// ����: ���ü����˿�
//-----------------------------------------------------------------------------
void CUdpServer::SetLocalPort(int nValue)
{
	if (nValue != m_nLocalPort)
	{
		if (GetActive()) Close();
		m_nLocalPort = nValue;
	}
}

//-----------------------------------------------------------------------------
// ����: ���� UDP ������
//-----------------------------------------------------------------------------
void CUdpServer::Open()
{
	try
	{
		if (!m_bActive)
		{
			CUdpSocket::Open();
			if (m_bActive)
			{
				Bind(m_nLocalPort);
				StartListenerThreads();
			}
		}
	}
	catch (CSocketException&)
	{
		Close();
		throw;
	}
}

//-----------------------------------------------------------------------------
// ����: �ر� UDP ������
//-----------------------------------------------------------------------------
void CUdpServer::Close()
{
	if (GetActive())
	{
		StopListenerThreads();
		CUdpSocket::Close();
	}
}

//-----------------------------------------------------------------------------
// ����: ȡ�ü����̵߳�����
//-----------------------------------------------------------------------------
int CUdpServer::GetListenerThreadCount() const
{
	return m_pListenerThreadPool->GetMaxThreadCount();
}

//-----------------------------------------------------------------------------
// ����: ���ü����̵߳�����
//-----------------------------------------------------------------------------
void CUdpServer::SetListenerThreadCount(int nValue)
{
	if (nValue < 1) nValue = 1;
	m_pListenerThreadPool->SetMaxThreadCount(nValue);
}

//-----------------------------------------------------------------------------
// ����: ���á��յ����ݰ����Ļص�
//-----------------------------------------------------------------------------
void CUdpServer::SetOnRecvDataCallBack(UDPSVR_ON_RECV_DATA_PROC pProc, void *pParam)
{
	m_OnRecvData.pProc = pProc;
	m_OnRecvData.pParam = pParam;
}

///////////////////////////////////////////////////////////////////////////////
// class CBaseTcpConnection

CBaseTcpConnection::CBaseTcpConnection()
{
	m_Socket.SetBlockMode(false);
}

//-----------------------------------------------------------------------------

CBaseTcpConnection::CBaseTcpConnection(SOCKET nSocketHandle, const CPeerAddress& PeerAddr)
{
	m_Socket.SetHandle(nSocketHandle);
	m_Socket.SetBlockMode(false);
	m_PeerAddr = PeerAddr;
}

//-----------------------------------------------------------------------------
// ����: ��������
//   nTimeOutMSecs - ָ����ʱʱ��(����)��������ָ��ʱ����δ������ȫ���������˳�������
//                   �� nTimeOutMSecs Ϊ -1�����ʾ�����г�ʱ��⡣
// ����:
//   < 0    - δ�����κ����ݣ��ҷ������ݹ��̷����˴���
//   >= 0   - ʵ�ʷ������ֽ�����
// ��ע:
//   1. �����׳��쳣��
//   2. ������Ϊ��ʱ�����أ�����ֵ����С��0��
//   3. �˴����÷������׽���ģʽ���Ա��ܼ�ʱ�˳���
//-----------------------------------------------------------------------------
int CBaseTcpConnection::DoSyncSendBuffer(void *pBuffer, int nSize, int nTimeOutMSecs)
{
	const int SELECT_WAIT_MSEC = 250;    // ÿ�εȴ�ʱ�� (����)

	int nResult = -1;
	bool bError = false;
	fd_set fds;
	struct timeval tv;
	SOCKET nSocketHandle = m_Socket.GetHandle();
	int n, r, nRemainSize, nIndex;
	UINT nStartTime, nElapsedMSecs;

	if (nSize <= 0 || !m_Socket.GetActive())
		return nResult;

	nRemainSize = nSize;
	nIndex = 0;
	nStartTime = GetCurTicks();

	while (m_Socket.GetActive() && nRemainSize > 0)
	try
	{
		tv.tv_sec = 0;
		tv.tv_usec = (nTimeOutMSecs? SELECT_WAIT_MSEC * 1000 : 0);

		FD_ZERO(&fds);
		FD_SET((UINT)nSocketHandle, &fds);

		r = select(nSocketHandle + 1, NULL, &fds, NULL, &tv);
		if (r < 0)
		{
			if (IseSocketGetLastError() != SS_EINTR)
			{
				bError = true;    // error
				break;
			}
		}

		if (r > 0 && m_Socket.GetActive() && FD_ISSET(nSocketHandle, &fds))
		{
			n = send(nSocketHandle, &((char*)pBuffer)[nIndex], nRemainSize, 0);
			if (n <= 0)
			{
				int nError = IseSocketGetLastError();
				if ((n == 0) || (nError != SS_EWOULDBLOCK && nError != SS_EINTR))
				{
					bError = true;    // error
					Disconnect();
					break;
				}
				else
					n = 0;
			}

			nIndex += n;
			nRemainSize -= n;
		}

		// �����Ҫ��ʱ���
		if (nTimeOutMSecs >= 0 && nRemainSize > 0)
		{
			nElapsedMSecs = GetTickDiff(nStartTime, GetCurTicks());
			if (nElapsedMSecs >= (UINT)nTimeOutMSecs)
				break;
		}
	}
	catch (...)
	{
		bError = true;
		break;
	}

	if (nIndex > 0)
		nResult = nIndex;
	else if (bError)
		nResult = -1;
	else
		nResult = 0;

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ��������
// ����:
//   nTimeOutMSecs - ָ����ʱʱ��(����)��������ָ��ʱ����δ������ȫ���������˳�������
//                   �� nTimeOutMSecs Ϊ -1�����ʾ�����г�ʱ��⡣
// ����:
//   < 0    - δ���յ��κ����ݣ��ҽ������ݹ��̷����˴���
//   >= 0   - ʵ�ʽ��յ����ֽ�����
// ��ע:
//   1. �����׳��쳣��
//   2. ������Ϊ��ʱ�����أ�����ֵ����С��0��
//   3. �˴����÷������׽���ģʽ���Ա��ܼ�ʱ�˳���
//-----------------------------------------------------------------------------
int CBaseTcpConnection::DoSyncRecvBuffer(void *pBuffer, int nSize, int nTimeOutMSecs)
{
	const int SELECT_WAIT_MSEC = 250;    // ÿ�εȴ�ʱ�� (����)

	int nResult = -1;
	bool bError = false;
	fd_set fds;
	struct timeval tv;
	SOCKET nSocketHandle = m_Socket.GetHandle();
	int n, r, nRemainSize, nIndex;
	UINT nStartTime, nElapsedMSecs;

	if (nSize <= 0 || !m_Socket.GetActive())
		return nResult;

	nRemainSize = nSize;
	nIndex = 0;
	nStartTime = GetCurTicks();

	while (m_Socket.GetActive() && nRemainSize > 0)
	try
	{
		tv.tv_sec = 0;
		tv.tv_usec = (nTimeOutMSecs? SELECT_WAIT_MSEC * 1000 : 0);

		FD_ZERO(&fds);
		FD_SET((UINT)nSocketHandle, &fds);

		r = select(nSocketHandle + 1, &fds, NULL, NULL, &tv);
		if (r < 0)
		{
			if (IseSocketGetLastError() != SS_EINTR)
			{
				bError = true;    // error
				break;
			}
		}

		if (r > 0 && m_Socket.GetActive() && FD_ISSET(nSocketHandle, &fds))
		{
			n = recv(nSocketHandle, &((char*)pBuffer)[nIndex], nRemainSize, 0);
			if (n <= 0)
			{
				int nError = IseSocketGetLastError();
				if ((n == 0) || (nError != SS_EWOULDBLOCK && nError != SS_EINTR))
				{
					bError = true;    // error
					Disconnect();
					break;
				}
				else
					n = 0;
			}

			nIndex += n;
			nRemainSize -= n;
		}

		// �����Ҫ��ʱ���
		if (nTimeOutMSecs >= 0 && nRemainSize > 0)
		{
			nElapsedMSecs = GetTickDiff(nStartTime, GetCurTicks());
			if (nElapsedMSecs >= (UINT)nTimeOutMSecs)
				break;
		}
	}
	catch (...)
	{
		bError = true;
		break;
	}

	if (nIndex > 0)
		nResult = nIndex;
	else if (bError)
		nResult = -1;
	else
		nResult = 0;

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �������� (������)
// ����:
//   < 0    - δ�����κ����ݣ��ҷ������ݹ��̷����˴���
//   >= 0   - ʵ�ʷ������ֽ�����
// ��ע:
//   �����׳��쳣��
//-----------------------------------------------------------------------------
int CBaseTcpConnection::DoAsyncSendBuffer(void *pBuffer, int nSize)
{
	int nResult = -1;
	try
	{
		nResult = send(m_Socket.GetHandle(), (char*)pBuffer, nSize, 0);
		if (nResult <= 0)
		{
			int nError = IseSocketGetLastError();
			if ((nResult == 0) || (nError != SS_EWOULDBLOCK && nError != SS_EINTR))
			{
				Disconnect();    // error
				nResult = -1;
			}
			else
				nResult = 0;
		}
	}
	catch (...)
	{}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �������� (������)
// ����:
//   < 0    - δ���յ��κ����ݣ��ҽ������ݹ��̷����˴���
//   >= 0   - ʵ�ʽ��յ����ֽ�����
// ��ע:
//   �����׳��쳣��
//-----------------------------------------------------------------------------
int CBaseTcpConnection::DoAsyncRecvBuffer(void *pBuffer, int nSize)
{
	int nResult = -1;
	try
	{
		nResult = recv(m_Socket.GetHandle(), (char*)pBuffer, nSize, 0);
		if (nResult <= 0)
		{
			int nError = IseSocketGetLastError();
			if ((nResult == 0) || (nError != SS_EWOULDBLOCK && nError != SS_EINTR))
			{
				Disconnect();    // error
				nResult = -1;
			}
			else
				nResult = 0;
		}
	}
	catch (...)
	{}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ��������
//   bSyncMode     - �Ƿ���ͬ����ʽ����
//   nTimeOutMSecs - ָ����ʱʱ��(����)��������ָ��ʱ����δ������ȫ���������˳�������
//                   �� nTimeOutMSecs Ϊ -1�����ʾ�����г�ʱ��⡣
// ����:
//   < 0    - δ�����κ����ݣ��ҷ������ݹ��̷����˴���
//   >= 0   - ʵ�ʷ������ֽ�����
// ��ע:
//   1. �����׳��쳣��
//   2. ������Ϊ��ʱ�����أ�����ֵ����С��0��
//-----------------------------------------------------------------------------
int CBaseTcpConnection::SendBuffer(void *pBuffer, int nSize, bool bSyncMode, int nTimeOutMSecs)
{
	int nResult = nSize;

	if (bSyncMode)
		nResult = DoSyncSendBuffer(pBuffer, nSize, nTimeOutMSecs);
	else
		nResult = DoAsyncSendBuffer(pBuffer, nSize);

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ��������
//   bSyncMode     - �Ƿ���ͬ����ʽ����
//   nTimeOutMSecs - ָ����ʱʱ��(����)��������ָ��ʱ����δ������ȫ���������˳�������
//                   �� nTimeOutMSecs Ϊ -1�����ʾ�����г�ʱ��⡣
// ����:
//   < 0    - δ���յ��κ����ݣ��ҽ������ݹ��̷����˴���
//   >= 0   - ʵ�ʽ��յ����ֽ�����
// ��ע:
//   1. �����׳��쳣��
//   2. ������Ϊ��ʱ�����أ�����ֵ����С��0��
//-----------------------------------------------------------------------------
int CBaseTcpConnection::RecvBuffer(void *pBuffer, int nSize, bool bSyncMode, int nTimeOutMSecs)
{
	int nResult = nSize;

	if (bSyncMode)
		nResult = DoSyncRecvBuffer(pBuffer, nSize, nTimeOutMSecs);
	else
		nResult = DoAsyncRecvBuffer(pBuffer, nSize);

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �Ͽ�����
//-----------------------------------------------------------------------------
void CBaseTcpConnection::DoDisconnect()
{
	m_Socket.Close();
}

//-----------------------------------------------------------------------------
// ����: ���ص�ǰ�Ƿ�Ϊ����״̬
//-----------------------------------------------------------------------------
bool CBaseTcpConnection::IsConnected() const
{
	return m_Socket.GetActive();
}

//-----------------------------------------------------------------------------
// ����: �Ͽ�����
//-----------------------------------------------------------------------------
void CBaseTcpConnection::Disconnect()
{
	if (IsConnected())
		DoDisconnect();
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpClient

//-----------------------------------------------------------------------------
// ����: ����TCP�������� (����ʽ)
// ��ע: ������ʧ�ܣ����׳��쳣��
//-----------------------------------------------------------------------------
void CTcpClient::Connect(const string& strIp, int nPort)
{
	if (IsConnected()) Disconnect();

	try
	{
		m_Socket.Open();
		if (m_Socket.GetActive())
		{
			SockAddr Addr;

			GetSocketAddr(Addr, StringToIp(strIp), nPort);

			bool bOldBlockMode = m_Socket.GetBlockMode();
			m_Socket.SetBlockMode(true);

			if (connect(m_Socket.GetHandle(), (struct sockaddr*)&Addr, sizeof(Addr)) < 0)
				IseThrowSocketLastError();

			m_Socket.SetBlockMode(bOldBlockMode);
			m_PeerAddr = CPeerAddress(ntohl(Addr.sin_addr.s_addr), nPort);
		}
	}
	catch (CSocketException&)
	{
		m_Socket.Close();
		throw;
	}
}

//-----------------------------------------------------------------------------
// ����: ����TCP�������� (������ʽ)
// ����:
//   nTimeOutMSecs - ���ȴ��ĺ�������Ϊ-1��ʾ���ȴ�
// ����:
//   ACS_CONNECTING - ��δ������ϣ�����δ��������
//   ACS_CONNECTED  - �����ѽ����ɹ�
//   ACS_FAILED     - ���ӹ����з����˴��󣬵�������ʧ��
// ��ע:
//   �����쳣��
//-----------------------------------------------------------------------------
int CTcpClient::AsyncConnect(const string& strIp, int nPort, int nTimeOutMSecs)
{
	int nResult = ACS_CONNECTING;

	if (IsConnected()) Disconnect();
	try
	{
		m_Socket.Open();
		if (m_Socket.GetActive())
		{
			SockAddr Addr;
			int r;

			GetSocketAddr(Addr, StringToIp(strIp), nPort);
			m_Socket.SetBlockMode(false);
			r = connect(m_Socket.GetHandle(), (struct sockaddr*)&Addr, sizeof(Addr));
			if (r == 0)
				nResult = ACS_CONNECTED;
#ifdef ISE_WIN32
			else if (IseSocketGetLastError() != SS_EWOULDBLOCK)
#endif
#ifdef ISE_LINUX
			else if (IseSocketGetLastError() != SS_EINPROGRESS)
#endif
				nResult = ACS_FAILED;

			m_PeerAddr = CPeerAddress(ntohl(Addr.sin_addr.s_addr), nPort);
		}
	}
	catch (...)
	{
		m_Socket.Close();
		nResult = ACS_FAILED;
	}

	if (nResult == ACS_CONNECTING)
		nResult = CheckAsyncConnectState(nTimeOutMSecs);

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ����첽���ӵ�״̬
// ����:
//   nTimeOutMSecs - ���ȴ��ĺ�������Ϊ-1��ʾ���ȴ�
// ����:
//   ACS_CONNECTING - ��δ������ϣ�����δ��������
//   ACS_CONNECTED  - �����ѽ����ɹ�
//   ACS_FAILED     - ���ӹ����з����˴��󣬵�������ʧ��
// ��ע:
//   �����쳣��
//-----------------------------------------------------------------------------
int CTcpClient::CheckAsyncConnectState(int nTimeOutMSecs)
{
	if (!m_Socket.GetActive()) return ACS_FAILED;

	const int WAIT_STEP = 100;   // ms
	int nResult = ACS_CONNECTING;
	SOCKET nHandle = GetSocket().GetHandle();
	fd_set rset, wset;
	struct timeval tv;
	int ms = 0;

	nTimeOutMSecs = max(nTimeOutMSecs, -1);

	while (true)
	{
		tv.tv_sec = 0;
		tv.tv_usec = (nTimeOutMSecs? WAIT_STEP * 1000 : 0);
		FD_ZERO(&rset);
		FD_SET((DWORD)nHandle, &rset);
		wset = rset;

		int r = select(nHandle + 1, &rset, &wset, NULL, &tv);
		if (r > 0 && (FD_ISSET(nHandle, &rset) || FD_ISSET(nHandle, &wset)))
		{
			socklen_t nErrLen = sizeof(int);
			int nError = 0;
			// If error occurs
			if (getsockopt(nHandle, SOL_SOCKET, SO_ERROR, (char*)&nError, &nErrLen) < 0 || nError)
				nResult = ACS_FAILED;
			else
				nResult = ACS_CONNECTED;
		}
		else if (r < 0)
		{
			if (IseSocketGetLastError() != SS_EINTR)
				nResult = ACS_FAILED;
		}

		if (nResult != ACS_CONNECTING)
			break;

		// Check timeout
		if (nTimeOutMSecs != -1)
		{
			ms += WAIT_STEP;
			if (ms >= nTimeOutMSecs)
				break;
		}
	}

	return nResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpServer

CTcpServer::CTcpServer() :
	m_nLocalPort(0),
	m_pListenerThread(NULL)
{
	// nothing
}

//-----------------------------------------------------------------------------

CTcpServer::~CTcpServer()
{
	Close();
}

//-----------------------------------------------------------------------------
// ����: �������Ӷ���
//-----------------------------------------------------------------------------
CBaseTcpConnection* CTcpServer::CreateConnection(SOCKET nSocketHandle, const CPeerAddress& PeerAddr)
{
	CBaseTcpConnection *pResult = NULL;

	if (m_OnCreateConn.pProc)
		m_OnCreateConn.pProc(m_OnCreateConn.pParam, this, nSocketHandle, PeerAddr, pResult);

	if (pResult == NULL)
		pResult = new CBaseTcpConnection(nSocketHandle, PeerAddr);

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: �յ����� (ע: pConnection �ǶѶ�����ʹ�����ͷ�)
//-----------------------------------------------------------------------------
void CTcpServer::AcceptConnection(CBaseTcpConnection *pConnection)
{
	if (m_OnAcceptConn.pProc)
		m_OnAcceptConn.pProc(m_OnAcceptConn.pParam, this, pConnection);
	else
		delete pConnection;
}

//-----------------------------------------------------------------------------
// ����: ���������߳�
//-----------------------------------------------------------------------------
void CTcpServer::StartListenerThread()
{
	if (!m_pListenerThread)
	{
		m_pListenerThread = new CTcpListenerThread(this);
		m_pListenerThread->Run();
	}
}

//-----------------------------------------------------------------------------
// ����: ֹͣ�����߳�
//-----------------------------------------------------------------------------
void CTcpServer::StopListenerThread()
{
	if (m_pListenerThread)
	{
		m_pListenerThread->Terminate();
		m_pListenerThread->WaitFor();
		delete m_pListenerThread;
		m_pListenerThread = NULL;
	}
}

//-----------------------------------------------------------------------------
// ����: ����TCP������
//-----------------------------------------------------------------------------
void CTcpServer::Open()
{
	try
	{
		if (!GetActive())
		{
			m_Socket.Open();
			m_Socket.Bind(m_nLocalPort);
			if (listen(m_Socket.GetHandle(), LISTEN_QUEUE_SIZE) < 0)
				IseThrowSocketLastError();
			StartListenerThread();
		}
	}
	catch (CSocketException&)
	{
		Close();
		throw;
	}
}

//-----------------------------------------------------------------------------
// ����: �ر�TCP������
//-----------------------------------------------------------------------------
void CTcpServer::Close()
{
	if (GetActive())
	{
		StopListenerThread();
		m_Socket.Close();
	}
}

//-----------------------------------------------------------------------------
// ����: ����/�ر�TCP������
//-----------------------------------------------------------------------------
void CTcpServer::SetActive(bool bValue)
{
	if (GetActive() != bValue)
	{
		if (bValue) Open();
		else Close();
	}
}

//-----------------------------------------------------------------------------
// ����: ����TCP�����������˿�
//-----------------------------------------------------------------------------
void CTcpServer::SetLocalPort(int nValue)
{
	if (nValue != m_nLocalPort)
	{
		if (GetActive()) Close();
		m_nLocalPort = nValue;
	}
}

//-----------------------------------------------------------------------------
// ����: ���á����������ӡ��Ļص�
//-----------------------------------------------------------------------------
void CTcpServer::SetOnCreateConnCallBack(TCPSVR_ON_CREATE_CONN_PROC pProc, void *pParam)
{
	m_OnCreateConn.pProc = pProc;
	m_OnCreateConn.pParam = pParam;
}

//-----------------------------------------------------------------------------
// ����: ���á��յ����ӡ��Ļص�
//-----------------------------------------------------------------------------
void CTcpServer::SetOnAcceptConnCallBack(TCPSVR_ON_ACCEPT_CONN_PROC pProc, void *pParam)
{
	m_OnAcceptConn.pProc = pProc;
	m_OnAcceptConn.pParam = pParam;
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpListenerThread

CUdpListenerThread::CUdpListenerThread(CUdpListenerThreadPool *pThreadPool, int nIndex) :
	m_pThreadPool(pThreadPool),
	m_nIndex(nIndex)
{
	SetFreeOnTerminate(true);
	m_pUdpServer = &(pThreadPool->GetUdpServer());
	m_pThreadPool->RegisterThread(this);
}

//-----------------------------------------------------------------------------

CUdpListenerThread::~CUdpListenerThread()
{
	m_pThreadPool->UnregisterThread(this);
}

//-----------------------------------------------------------------------------
// ����: UDP��������������
//-----------------------------------------------------------------------------
void CUdpListenerThread::Execute()
{
	const int MAX_UDP_BUFFER_SIZE = 8192;   // UDP���ݰ�����ֽ���
	const int SELECT_WAIT_MSEC    = 100;    // ÿ�εȴ�ʱ�� (����)

	fd_set fds;
	struct timeval tv;
	SOCKET nSocketHandle = m_pUdpServer->GetHandle();
	CBuffer PacketBuffer(MAX_UDP_BUFFER_SIZE);
	CPeerAddress PeerAddr;
	int r, n;

	while (!GetTerminated() && m_pUdpServer->GetActive())
	try
	{
		// �趨ÿ�εȴ�ʱ��
		tv.tv_sec = 0;
		tv.tv_usec = SELECT_WAIT_MSEC * 1000;

		FD_ZERO(&fds);
		FD_SET((UINT)nSocketHandle, &fds);

		r = select(nSocketHandle + 1, &fds, NULL, NULL, &tv);

		if (r > 0 && m_pUdpServer->GetActive() && FD_ISSET(nSocketHandle, &fds))
		{
			n = m_pUdpServer->RecvBuffer(PacketBuffer.Data(), MAX_UDP_BUFFER_SIZE, PeerAddr);
			if (n > 0)
			{
				m_pUdpServer->DataReceived(PacketBuffer.Data(), n, PeerAddr);
			}
		}
		else if (r < 0)
		{
			int nError = IseSocketGetLastError();
			if (nError != SS_EINTR && nError != SS_EINPROGRESS)
				break;  // error
		}
	}
	catch (CException&)
	{}
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpListenerThreadPool

CUdpListenerThreadPool::CUdpListenerThreadPool(CUdpServer *pUdpServer) :
	m_pUdpServer(pUdpServer),
	m_nMaxThreadCount(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

CUdpListenerThreadPool::~CUdpListenerThreadPool()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void CUdpListenerThreadPool::RegisterThread(CUdpListenerThread *pThread)
{
	m_ThreadList.Add(pThread);
}

//-----------------------------------------------------------------------------
// ����: ע���߳�
//-----------------------------------------------------------------------------
void CUdpListenerThreadPool::UnregisterThread(CUdpListenerThread *pThread)
{
	m_ThreadList.Remove(pThread);
}

//-----------------------------------------------------------------------------
// ����: �����������߳�
//-----------------------------------------------------------------------------
void CUdpListenerThreadPool::StartThreads()
{
	for (int i = 0; i < m_nMaxThreadCount; i++)
	{
		CUdpListenerThread *pThread;
		pThread = new CUdpListenerThread(this, i);
		pThread->Run();
	}
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ���ȴ������߳��˳�
//-----------------------------------------------------------------------------
void CUdpListenerThreadPool::StopThreads()
{
	const int MAX_WAIT_FOR_SECS = 5;
	m_ThreadList.WaitForAllThreads(MAX_WAIT_FOR_SECS);
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpListenerThread

CTcpListenerThread::CTcpListenerThread(CTcpServer *pTcpServer) :
	m_pTcpServer(pTcpServer)
{
	SetFreeOnTerminate(false);
}

//-----------------------------------------------------------------------------
// ����: TCP��������������
//-----------------------------------------------------------------------------
void CTcpListenerThread::Execute()
{
	const int SELECT_WAIT_MSEC = 100;    // ÿ�εȴ�ʱ�� (����)

	fd_set fds;
	struct timeval tv;
	SockAddr Addr;
	socklen_t nSockLen = sizeof(Addr);
	SOCKET nSocketHandle = m_pTcpServer->GetSocket().GetHandle();
	CPeerAddress PeerAddr;
	SOCKET nAcceptHandle;
	int r;

	while (!GetTerminated() && m_pTcpServer->GetActive())
	try
	{
		// �趨ÿ�εȴ�ʱ��
		tv.tv_sec = 0;
		tv.tv_usec = SELECT_WAIT_MSEC * 1000;

		FD_ZERO(&fds);
		FD_SET((UINT)nSocketHandle, &fds);

		r = select(nSocketHandle + 1, &fds, NULL, NULL, &tv);

		if (r > 0 && m_pTcpServer->GetActive() && FD_ISSET(nSocketHandle, &fds))
		{
			nAcceptHandle = accept(nSocketHandle, (struct sockaddr*)&Addr, &nSockLen);
			if (nAcceptHandle != INVALID_SOCKET)
			{
				PeerAddr = CPeerAddress(ntohl(Addr.sin_addr.s_addr), ntohs(Addr.sin_port));
				CBaseTcpConnection *pConnection = m_pTcpServer->CreateConnection(nAcceptHandle, PeerAddr);
				m_pTcpServer->AcceptConnection(pConnection);
			}
		}
		else if (r < 0)
		{
			int nError = IseSocketGetLastError();
			if (nError != SS_EINTR && nError != SS_EINPROGRESS)
				break;  // error
		}
	}
	catch (CException&)
	{}
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
