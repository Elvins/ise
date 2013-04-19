///////////////////////////////////////////////////////////////////////////////

#include "echo.h"

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

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
    const char *msg = "Echo server stoped.";
    cout << msg << endl;
    logger().writeStr(msg);
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
            const char *msg = "Echo server started.";
            cout << endl << msg << endl;
            logger().writeStr(msg);
        }
        break;

    case SS_START_FAIL:
        {
            const char *msg = "Fail to start echo server.";
            cout << endl << msg << endl;
            logger().writeStr(msg);
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
void AppBusiness::onTcpConnect(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnect (%s)", connection->getPeerAddr().getDisplayStr().c_str());

    connection->recv(LINE_PACKET_SPLITTER);
}

//-----------------------------------------------------------------------------
// ����: �Ͽ���һ��TCP���� (ISE����֮ɾ�������Ӷ���)
//-----------------------------------------------------------------------------
void AppBusiness::onTcpDisconnect(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnect (%s)", connection->getPeerAddr().getDisplayStr().c_str());
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void AppBusiness::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
    int packetSize, const Context& context)
{
    logger().writeStr("onTcpRecvComplete");

    string msg((char*)packetBuffer, packetSize);
    msg = trimString(msg);
    if (msg == "bye")
        connection->disconnect();
    else
        connection->send((char*)packetBuffer, packetSize);

    logger().writeFmt("Received message: %s", msg.c_str());
}

//-----------------------------------------------------------------------------
// ����: TCP�����ϵ�һ���������������
//-----------------------------------------------------------------------------
void AppBusiness::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    logger().writeStr("onTcpSendComplete");
    connection->recv(LINE_PACKET_SPLITTER);
}
