///////////////////////////////////////////////////////////////////////////////

#include "echo.h"

IseBusiness* createIseBusinessObject()
{
	return new AppBusiness();
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� (ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void AppBusiness::initialize()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ������ (���۳�ʼ���Ƿ����쳣������ʱ����ִ��)
//-----------------------------------------------------------------------------
void AppBusiness::finalize()
{
	const char *pMsg = "Echo server stoped.";
	cout << pMsg << endl;
	logger().writeStr(pMsg);
}

//-----------------------------------------------------------------------------
// ����: ��������״̬
//-----------------------------------------------------------------------------
void AppBusiness::doStartupState(STARTUP_STATE state)
{
	switch (state)
	{
	case SS_AFTER_START:
		{
			const char *pMsg = "Echo server started.";
			cout << endl << pMsg << endl;
			logger().writeStr(pMsg);
		}
		break;

	case SS_START_FAIL:
		{
			const char *pMsg = "Fail to start echo server.";
			cout << endl << pMsg << endl;
			logger().writeStr(pMsg);
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ��SSE������Ϣ
//-----------------------------------------------------------------------------
void AppBusiness::initIseOptions(IseOptions& options)
{
	options.setLogFileName(getAppSubPath("log") + "echo-log.txt", true);
	options.setIsDaemon(true);
	options.setAllowMultiInstance(false);

	// ���÷���������
	options.setServerType(ST_TCP);
	// ����TCP������������
	options.setTcpServerCount(1);
	// ����TCP����˿ں�
	options.setTcpServerPort(0, 12345);
	// ����TCP�¼�ѭ���ĸ���
	options.setTcpEventLoopCount(3);
}

//-----------------------------------------------------------------------------
// ����: ������һ���µ�TCP����
//-----------------------------------------------------------------------------
void AppBusiness::onTcpConnection(TcpConnection *connection)
{
	logger().writeStr("OnTcpConnection");
	connection->postRecvTask(&LinePacketMeasurer::instance());
}

//-----------------------------------------------------------------------------
// ����: TCP���Ӵ�����̷����˴��� (SSE����֮ɾ�������Ӷ���)
//-----------------------------------------------------------------------------
void AppBusiness::onTcpError(TcpConnection *connection)
{
	logger().writeStr("OnTcpError");
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void AppBusiness::onTcpRecvComplete(TcpConnection *connection, void *packetBuffer,
	int packetSize, const CustomParams& params)
{
	logger().writeStr("OnTcpRecvComplete");

	string strMsg((char*)packetBuffer, packetSize);
	strMsg = trimString(strMsg);
	if (strMsg == "bye")
		connection->disconnect();
	else
		connection->postSendTask((char*)packetBuffer, packetSize);

	logger().writeFmt("Received message: %s", strMsg.c_str());
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void AppBusiness::onTcpSendComplete(TcpConnection *connection, const CustomParams& params)
{
	logger().writeStr("OnTcpSendComplete");
	connection->postRecvTask(&LinePacketMeasurer::instance());
}
