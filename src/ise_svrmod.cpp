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
// �ļ�����: ise_svrmod.cpp
// ��������: ����ģ��֧��
///////////////////////////////////////////////////////////////////////////////

#include "ise_svrmod.h"
#include "ise_scheduler.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classs CIseServerModuleMgr

CIseServerModuleMgr::CIseServerModuleMgr()
{
	// nothing
}

CIseServerModuleMgr::~CIseServerModuleMgr()
{
	ClearServerModuleList();
}

//-----------------------------------------------------------------------------
// ����: ��ʼ������ģ���б�
//-----------------------------------------------------------------------------
void CIseServerModuleMgr::InitServerModuleList(const CList& List)
{
	m_Items.Clear();
	for (int i = 0; i < List.GetCount(); i++)
	{
		CIseServerModule *pModule = (CIseServerModule*)List[i];
		pModule->m_nSvrModIndex = i;
		m_Items.Add(pModule);
	}
}

//-----------------------------------------------------------------------------
// ����: ������з���ģ��
//-----------------------------------------------------------------------------
void CIseServerModuleMgr::ClearServerModuleList()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CIseServerModule*)m_Items[i];
	m_Items.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// class CIseSvrModBusiness

//-----------------------------------------------------------------------------
// ����: ȡ��ȫ��UDP�������
//-----------------------------------------------------------------------------
int CIseSvrModBusiness::GetUdpGroupCount()
{
	int nResult = 0;

	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(i);
		nResult += ServerModule.GetUdpGroupCount();
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ȡ��ȫ��TCP����������
//-----------------------------------------------------------------------------
int CIseSvrModBusiness::GetTcpServerCount()
{
	int nResult = 0;

	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(i);
		nResult += ServerModule.GetTcpServerCount();
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� m_ActionCodeMap
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::InitActionCodeMap()
{
	int nGlobalGroupIndex = 0;
	m_ActionCodeMap.clear();

	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nGroupIndex = 0; nGroupIndex < ServerModule.GetUdpGroupCount(); nGroupIndex++)
		{
			ACTION_CODE_ARRAY ActionCodes;
			ServerModule.GetUdpGroupActionCodes(nGroupIndex, ActionCodes);

			for (UINT i = 0; i < ActionCodes.size(); i++)
				m_ActionCodeMap[ActionCodes[i]] = nGlobalGroupIndex;

			nGlobalGroupIndex++;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� m_UdpGroupIndexMap
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::InitUdpGroupIndexMap()
{
	int nGlobalGroupIndex = 0;
	m_UdpGroupIndexMap.clear();

	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nGroupIndex = 0; nGroupIndex < ServerModule.GetUdpGroupCount(); nGroupIndex++)
		{
			m_UdpGroupIndexMap[nGlobalGroupIndex] = nModIndex;
			nGlobalGroupIndex++;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� m_TcpServerIndexMap
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::InitTcpServerIndexMap()
{
	int nGlobalServerIndex = 0;
	m_TcpServerIndexMap.clear();

	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nServerIndex = 0; nServerIndex < ServerModule.GetTcpServerCount(); nServerIndex++)
		{
			m_TcpServerIndexMap[nGlobalServerIndex] = nModIndex;
			nGlobalServerIndex++;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ����ISE����
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::UpdateIseOptions()
{
	CIseOptions& IseOpt = IseApplication.GetIseOptions();

	// ����UDP��������������
	IseOpt.SetUdpRequestGroupCount(GetUdpGroupCount());
	// ����TCP��������������
	IseOpt.SetTcpServerCount(GetTcpServerCount());

	// UDP��������������
	int nGlobalGroupIndex = 0;
	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nGroupIndex = 0; nGroupIndex < ServerModule.GetUdpGroupCount(); nGroupIndex++)
		{
			UDP_GROUP_OPTIONS GrpOpt;
			memset(&GrpOpt, 0, sizeof(GrpOpt));
			ServerModule.GetUdpGroupOptions(nGroupIndex, GrpOpt);
			IseOpt.SetUdpRequestQueueCapacity(nGlobalGroupIndex, GrpOpt.nQueueCapacity);
			IseOpt.SetUdpWorkerThreadCount(nGlobalGroupIndex, GrpOpt.nMinThreads, GrpOpt.nMaxThreads);
			nGlobalGroupIndex++;
		}
	}

	// TCP�������������
	int nGlobalServerIndex = 0;
	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nServerIndex = 0; nServerIndex < ServerModule.GetTcpServerCount(); nServerIndex++)
		{
			TCP_SERVER_OPTIONS SvrOpt;
			memset(&SvrOpt, 0, sizeof(SvrOpt));
			ServerModule.GetTcpServerOptions(nServerIndex, SvrOpt);
			IseOpt.SetTcpServerPort(nGlobalServerIndex, SvrOpt.nPort);
			nGlobalServerIndex++;
		}
	}

	// ���ø��������̵߳�����
	int nAssistorCount = 0;
	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
		nAssistorCount += m_ServerModuleMgr.GetItems(i).GetAssistorThreadCount();
	IseOpt.SetAssistorThreadCount(nAssistorCount);
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� (ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::Initialize()
{
	CIseBusiness::Initialize();

	// �������з���ģ��
	CList SvrModList;
	CreateServerModules(SvrModList);
	m_ServerModuleMgr.InitServerModuleList(SvrModList);

	// ��ʼ������ӳ���
	InitActionCodeMap();
	InitUdpGroupIndexMap();
	InitTcpServerIndexMap();

	// ����ISE����
	UpdateIseOptions();
}

//-----------------------------------------------------------------------------
// ����: ������ (���۳�ʼ���Ƿ����쳣������ʱ����ִ��)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::Finalize()
{
	try { m_ServerModuleMgr.ClearServerModuleList(); } catch (...) {}

	CIseBusiness::Finalize();
}

//-----------------------------------------------------------------------------
// ����: UDP���ݰ�����
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::ClassifyUdpPacket(void *pPacketBuffer, int nPacketSize, int& nGroupIndex)
{
	nGroupIndex = -1;
	if (nPacketSize <= 0 || !FilterUdpPacket(pPacketBuffer, nPacketSize)) return;

	UINT nActionCode = GetUdpPacketActionCode(pPacketBuffer, nPacketSize);
	ACTION_CODE_MAP::iterator iter = m_ActionCodeMap.find(nActionCode);
	if (iter != m_ActionCodeMap.end())
	{
		nGroupIndex = iter->second;
	}
}

//-----------------------------------------------------------------------------
// ����: UDP���ݰ�����
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::DispatchUdpPacket(CUdpWorkerThread& WorkerThread,
	int nGroupIndex, CUdpPacket& Packet)
{
	UDP_GROUP_INDEX_MAP::iterator iter = m_UdpGroupIndexMap.find(nGroupIndex);
	if (iter != m_UdpGroupIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).DispatchUdpPacket(WorkerThread, Packet);
	}
}

//-----------------------------------------------------------------------------
// ����: ������һ���µ�TCP����
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpConnection(CTcpConnection *pConnection)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpConnection(pConnection);
	}
}

//-----------------------------------------------------------------------------
// ����: TCP���Ӵ�����̷����˴��� (ISE����֮ɾ�������Ӷ���)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpError(CTcpConnection *pConnection)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpError(pConnection);
	}
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
	int nPacketSize, const CCustomParams& Params)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpRecvComplete(pConnection,
			pPacketBuffer, nPacketSize, Params);
	}
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpSendComplete(pConnection, Params);
	}
}

//-----------------------------------------------------------------------------
// ����: ���������߳�ִ��(nAssistorIndex: 0-based)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex)
{
	int nIndex1, nIndex2 = 0;

	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		nIndex1 = nIndex2;
		nIndex2 += m_ServerModuleMgr.GetItems(i).GetAssistorThreadCount();

		if (nAssistorIndex >= nIndex1 && nAssistorIndex < nIndex2)
		{
			m_ServerModuleMgr.GetItems(i).AssistorThreadExecute(AssistorThread, nAssistorIndex - nIndex1);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ϵͳ�ػ��߳�ִ�� (nSecondCount: 0-based)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::DaemonThreadExecute(CThread& Thread, int nSecondCount)
{
	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
		m_ServerModuleMgr.GetItems(i).DaemonThreadExecute(Thread, nSecondCount);
}

//-----------------------------------------------------------------------------
// ����: ���ݷ���ģ����ź�ģ���ڵľֲ������߳���ţ�ȡ�ô˸����̵߳�ȫ�����
//-----------------------------------------------------------------------------
int CIseSvrModBusiness::GetAssistorIndex(int nServerModuleIndex, int nLocalAssistorIndex)
{
	int nResult = -1;

	if (nServerModuleIndex >= 0 && nServerModuleIndex < m_ServerModuleMgr.GetCount())
	{
		nResult = 0;
		for (int i = 0; i < nServerModuleIndex; i++)
			nResult += m_ServerModuleMgr.GetItems(i).GetAssistorThreadCount();
		nResult += nLocalAssistorIndex;
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ������Ϣ������ģ��
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::DispatchMessage(CBaseSvrModMessage& Message)
{
	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		if (Message.bHandled) break;
		m_ServerModuleMgr.GetItems(i).DispatchMessage(Message);
	}
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
