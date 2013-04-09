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

class IseServerModule;
class IseServerModuleMgr;
class IseSvrModBusiness;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

// ������������
typedef vector<UINT> ACTION_CODE_ARRAY;

// UDP��������
struct UDP_GROUP_OPTIONS
{
	int queueCapacity;
	int minThreads;
	int maxThreads;
};

// TCP������������
struct TCP_SERVER_OPTIONS
{
	int port;
};

///////////////////////////////////////////////////////////////////////////////
// class IseServerModule - ������ģ�����

class IseServerModule
{
public:
	friend class IseServerModuleMgr;
private:
	int svrModIndex_;   // (0-based)
public:
	IseServerModule() : svrModIndex_(0) {}
	virtual ~IseServerModule() {}

	// ȡ�ø÷���ģ���е�UDP�������
	virtual int getUdpGroupCount() { return 0; }
	// ȡ�ø�ģ����ĳUDP������ӹܵĶ�������
	virtual void getUdpGroupActionCodes(int groupIndex, ACTION_CODE_ARRAY& list) {}
	// ȡ�ø�ģ����ĳUDP��������
	virtual void getUdpGroupOptions(int groupIndex, UDP_GROUP_OPTIONS& options) {}
	// ȡ�ø÷���ģ���е�TCP����������
	virtual int getTcpServerCount() { return 0; }
	// ȡ�ø�ģ����ĳTCP������������
	virtual void getTcpServerOptions(int serverIndex, TCP_SERVER_OPTIONS& options) {}

	// UDP���ݰ�����
	virtual void dispatchUdpPacket(UdpWorkerThread& workerThread, UdpPacket& ) {}

	// ������һ���µ�TCP����
	virtual void onTcpConnection(TcpConnection *connection) {}
	// TCP���Ӵ�����̷����˴��� (ISE����֮ɾ�������Ӷ���)
	virtual void onTcpError(TcpConnection *connection) {}
	// TCP�����ϵ�һ���������������
	virtual void onTcpRecvComplete(TcpConnection *connection, void *packetBuffer,
		int packetSize, const CustomParams& params) {}
	// TCP�����ϵ�һ���������������
	virtual void onTcpSendComplete(TcpConnection *connection, const CustomParams& params) {}

	// ���ش�ģ�����踨�������̵߳�����
	virtual int getAssistorThreadCount() { return 0; }
	// ���������߳�ִ��(assistorIndex: 0-based)
	virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex) {}
	// ϵͳ�ػ��߳�ִ�� (secondCount: 0-based)
	virtual void daemonThreadExecute(Thread& thread, int secondCount) {}

	// ��Ϣ����
	virtual void dispatchMessage(BaseSvrModMessage& Message) {}

	// ���ط���ģ�����
	int getSvrModIndex() const { return svrModIndex_; }
};

///////////////////////////////////////////////////////////////////////////////
// class IseServerModuleMgr - ����ģ�������

class IseServerModuleMgr
{
private:
	PointerList items_;        // ����ģ���б�( (IseServerModule*)[] )
public:
	IseServerModuleMgr();
	virtual ~IseServerModuleMgr();

	void initServerModuleList(const PointerList& list);
	void clearServerModuleList();

	inline int getCount() const { return items_.getCount(); }
	inline IseServerModule& getItems(int index) { return *(IseServerModule*)items_[index]; }
};

///////////////////////////////////////////////////////////////////////////////
// class IseSvrModBusiness - ֧�ַ���ģ���ISEҵ����

class IseSvrModBusiness : public IseBusiness
{
public:
	friend class IseServerModule;
protected:
	typedef hash_map<UINT, int> ACTION_CODE_MAP;      // <��������, UDP����>
	typedef hash_map<UINT, int> UDP_GROUP_INDEX_MAP;  // <ȫ��UDP����, ����ģ���>
	typedef hash_map<UINT, int> TCP_SERVER_INDEX_MAP; // <ȫ��TCP���������, ����ģ���>

	IseServerModuleMgr serverModuleMgr_;              // ����ģ�������
	ACTION_CODE_MAP actionCodeMap_;                   // <��������, UDP����> ӳ���
	UDP_GROUP_INDEX_MAP udpGroupIndexMap_;            // <ȫ��UDP����, ����ģ���> ӳ���
	TCP_SERVER_INDEX_MAP tcpServerIndexMap_;          // <ȫ��TCP���������, ����ģ���> ӳ���
private:
	int getUdpGroupCount();
	int getTcpServerCount();
	void initActionCodeMap();
	void initUdpGroupIndexMap();
	void initTcpServerIndexMap();
	void updateIseOptions();
protected:
	// UDP���ݰ����˺��� (����: true-��Ч��, false-��Ч��)
	virtual bool filterUdpPacket(void *packetBuffer, int packetSize) { return true; }
	// ȡ��UDP���ݰ��еĶ�������
	virtual UINT getUdpPacketActionCode(void *packetBuffer, int packetSize) { return 0; }
	// �������з���ģ��
	virtual void createServerModules(PointerList& svrModList) {}
public:
	IseSvrModBusiness() {}
	virtual ~IseSvrModBusiness() {}
public:
	virtual void initialize();
	virtual void finalize();

	virtual void classifyUdpPacket(void *packetBuffer, int packetSize, int& groupIndex);
	virtual void dispatchUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet);

	virtual void onTcpConnection(TcpConnection *connection);
	virtual void onTcpError(TcpConnection *connection);
	virtual void onTcpRecvComplete(TcpConnection *connection, void *packetBuffer,
		int packetSize, const CustomParams& params);
	virtual void onTcpSendComplete(TcpConnection *connection, const CustomParams& params);

	virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex);
	virtual void daemonThreadExecute(Thread& thread, int secondCount);
public:
	int getAssistorIndex(int serverModuleIndex, int localAssistorIndex);
	void dispatchMessage(BaseSvrModMessage& message);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SVRMOD_H_
