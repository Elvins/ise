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
// ise_svrmod.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SVRMOD_H_
#define _ISE_SVRMOD_H_

#include "ise.h"
#include "ise_application.h"
#include "ise_svrmod_msgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class CIseServerModule;
class CIseServerModuleMgr;
class CIseSvrModBusiness;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

// ������������
typedef vector<UINT> ACTION_CODE_ARRAY;

// UDP��������
struct UDP_GROUP_OPTIONS
{
	int nQueueCapacity;
	int nMinThreads;
	int nMaxThreads;
};

// TCP������������
struct TCP_SERVER_OPTIONS
{
	int nPort;
};

///////////////////////////////////////////////////////////////////////////////
// class CIseServerModule - ������ģ�����

class CIseServerModule
{
public:
	friend class CIseServerModuleMgr;
private:
	int m_nSvrModIndex;   // (0-based)
public:
	CIseServerModule() : m_nSvrModIndex(0) {}
	virtual ~CIseServerModule() {}

	// ȡ�ø÷���ģ���е�UDP�������
	virtual int GetUdpGroupCount() { return 0; }
	// ȡ�ø�ģ����ĳUDP������ӹܵĶ�������
	virtual void GetUdpGroupActionCodes(int nGroupIndex, ACTION_CODE_ARRAY& List) {}
	// ȡ�ø�ģ����ĳUDP��������
	virtual void GetUdpGroupOptions(int nGroupIndex, UDP_GROUP_OPTIONS& Options) {}
	// ȡ�ø÷���ģ���е�TCP����������
	virtual int GetTcpServerCount() { return 0; }
	// ȡ�ø�ģ����ĳTCP������������
	virtual void GetTcpServerOptions(int nServerIndex, TCP_SERVER_OPTIONS& Options) {}

	// UDP���ݰ�����
	virtual void DispatchUdpPacket(CUdpWorkerThread& WorkerThread, CUdpPacket& ) {}

	// ������һ���µ�TCP����
	virtual void OnTcpConnection(CTcpConnection *pConnection) {}
	// TCP���Ӵ�����̷����˴��� (ISE����֮ɾ�������Ӷ���)
	virtual void OnTcpError(CTcpConnection *pConnection) {}
	// TCP�����ϵ�һ���������������
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params) {}
	// TCP�����ϵ�һ���������������
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params) {}

	// ���ش�ģ�����踨�������̵߳�����
	virtual int GetAssistorThreadCount() { return 0; }
	// ���������߳�ִ��(nAssistorIndex: 0-based)
	virtual void AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex) {}
	// ϵͳ�ػ��߳�ִ�� (nSecondCount: 0-based)
	virtual void DaemonThreadExecute(CThread& Thread, int nSecondCount) {}

	// ��Ϣ����
	virtual void DispatchMessage(CBaseSvrModMessage& Message) {}

	// ���ط���ģ�����
	int GetSvrModIndex() const { return m_nSvrModIndex; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseServerModuleMgr - ����ģ�������

class CIseServerModuleMgr
{
private:
	CList m_Items;        // ����ģ���б�( (CIseServerModule*)[] )
public:
	CIseServerModuleMgr();
	virtual ~CIseServerModuleMgr();

	void InitServerModuleList(const CList& List);
	void ClearServerModuleList();

	inline int GetCount() const { return m_Items.GetCount(); }
	inline CIseServerModule& GetItems(int nIndex) { return *(CIseServerModule*)m_Items[nIndex]; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseSvrModBusiness - ֧�ַ���ģ���ISEҵ����

class CIseSvrModBusiness : public CIseBusiness
{
public:
	friend class CIseServerModule;
protected:
	typedef hash_map<UINT, int> ACTION_CODE_MAP;      // <��������, UDP����>
	typedef hash_map<UINT, int> UDP_GROUP_INDEX_MAP;  // <ȫ��UDP����, ����ģ���>
	typedef hash_map<UINT, int> TCP_SERVER_INDEX_MAP; // <ȫ��TCP���������, ����ģ���>

	CIseServerModuleMgr m_ServerModuleMgr;            // ����ģ�������
	ACTION_CODE_MAP m_ActionCodeMap;                  // <��������, UDP����> ӳ���
	UDP_GROUP_INDEX_MAP m_UdpGroupIndexMap;           // <ȫ��UDP����, ����ģ���> ӳ���
	TCP_SERVER_INDEX_MAP m_TcpServerIndexMap;         // <ȫ��TCP���������, ����ģ���> ӳ���
private:
	int GetUdpGroupCount();
	int GetTcpServerCount();
	void InitActionCodeMap();
	void InitUdpGroupIndexMap();
	void InitTcpServerIndexMap();
	void UpdateIseOptions();
protected:
	// UDP���ݰ����˺��� (����: true-��Ч��, false-��Ч��)
	virtual bool FilterUdpPacket(void *pPacketBuffer, int nPacketSize) { return true; }
	// ȡ��UDP���ݰ��еĶ�������
	virtual UINT GetUdpPacketActionCode(void *pPacketBuffer, int nPacketSize) { return 0; }
	// �������з���ģ��
	virtual void CreateServerModules(CList& SvrModList) {}
public:
	CIseSvrModBusiness() {}
	virtual ~CIseSvrModBusiness() {}
public:
	virtual void Initialize();
	virtual void Finalize();

	virtual void ClassifyUdpPacket(void *pPacketBuffer, int nPacketSize, int& nGroupIndex);
	virtual void DispatchUdpPacket(CUdpWorkerThread& WorkerThread, int nGroupIndex, CUdpPacket& Packet);

	virtual void OnTcpConnection(CTcpConnection *pConnection);
	virtual void OnTcpError(CTcpConnection *pConnection);
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params);
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params);

	virtual void AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex);
	virtual void DaemonThreadExecute(CThread& Thread, int nSecondCount);
public:
	int GetAssistorIndex(int nServerModuleIndex, int nLocalAssistorIndex);
	void DispatchMessage(CBaseSvrModMessage& Message);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SVRMOD_H_
