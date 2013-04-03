///////////////////////////////////////////////////////////////////////////////

#include "echo.h"

CSseBusiness* CreateSseBusinessObject()
{
	return new CAppBusiness();
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� (ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CAppBusiness::Initialize()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ������ (���۳�ʼ���Ƿ����쳣������ʱ����ִ��)
//-----------------------------------------------------------------------------
void CAppBusiness::Finalize()
{
	const char *pMsg = "Echo server stoped.";
	cout << pMsg << endl;
	Logger().WriteStr(pMsg);
}

//-----------------------------------------------------------------------------
// ����: ��������״̬
//-----------------------------------------------------------------------------
void CAppBusiness::DoStartupState(STARTUP_STATE nState)
{
	switch (nState)
	{
	case SS_AFTER_START:
		{
			const char *pMsg = "Echo server started.";
			cout << endl << pMsg << endl;
			Logger().WriteStr(pMsg);
		}
		break;

	case SS_START_FAIL:
		{
			const char *pMsg = "Fail to start echo server.";
			cout << endl << pMsg << endl;
			Logger().WriteStr(pMsg);
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ��SSE������Ϣ
//-----------------------------------------------------------------------------
void CAppBusiness::InitSseOptions(CSseOptions& SseOpt)
{
	SseOpt.SetLogFileName(GetAppSubPath("log") + "echo-log.txt", true);
	SseOpt.SetIsDaemon(true);
	SseOpt.SetAllowMultiInstance(false);

	// ���÷���������
	SseOpt.SetServerType(ST_TCP);
	// ����TCP������������
	SseOpt.SetTcpServerCount(1);
	// ����TCP����˿ں�
	SseOpt.SetTcpServerPort(0, 12345);
	// ����TCP�¼�ѭ���ĸ���
	SseOpt.SetTcpEventLoopCount(3);
}

//-----------------------------------------------------------------------------
// ����: ������һ���µ�TCP����
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpConnection(CTcpConnection *pConnection)
{
	Logger().WriteStr("OnTcpConnection");
	pConnection->PostRecvTask(&CLinePacketMeasurer::Instance());
}

//-----------------------------------------------------------------------------
// ����: TCP���Ӵ�����̷����˴��� (SSE����֮ɾ�������Ӷ���)
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpError(CTcpConnection *pConnection)
{
	Logger().WriteStr("OnTcpError");
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
	int nPacketSize, const CCustomParams& Params)
{
	Logger().WriteStr("OnTcpRecvComplete");

	string strMsg((char*)pPacketBuffer, nPacketSize);
	strMsg = TrimString(strMsg);
	if (strMsg == "bye")
		pConnection->Disconnect();
	else
		pConnection->PostSendTask((char*)pPacketBuffer, nPacketSize);

	Logger().WriteFmt("Received message: %s", strMsg.c_str());
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params)
{
	Logger().WriteStr("OnTcpSendComplete");
	pConnection->PostRecvTask(&CLinePacketMeasurer::Instance());
}
