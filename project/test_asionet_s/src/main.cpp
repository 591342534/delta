#include <iostream>
#include <string>
#include "fndnet/ITCPServer.h"

using namespace std;


IFndNetServer* server;

class client_notify : public IFndNetServerNotify
{
public:
    client_notify(){};
    ~client_notify(){};

    /*
    注意：由于线程时序的原因，有可能出现OnRawData，先于OnAccept被调用的情况，
    如有同步需求，请在回调函数内自行处理。
    */

    /// \brief 接受客户端连接回调
    /// \param ullClientID 客户端ID
    /// \param szClientIP 客户端IP
    /// \param usPort 客户器端口
    virtual void OnAccept(unsigned __int64 ullClientID, const char *szClientIP, unsigned short usPort)
    {
        cout << __FUNCTION__ << endl;
        cout << "ullClientID:" << ullClientID << endl;
    }

    /// \brief 从客户端接收到数据回调(回调函数应复制数据后，立即返回，否则影响接收效率，有心跳方式阻塞时间不得超过15s，否则断线)
    /// \param ullClientID 客户端ID
    /// \param pbData 数据指针(函数调用后即释放，勿超范围使用，勿在函数中删除)
    /// \param unDataSize 数据大小
    virtual void OnRawData(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize)
    {
        cout << __FUNCTION__ << endl;
        cout << "pbData:" << pbData << endl;

        string msg = "hello world";
        server->SendDataWithHB(ullClientID, (const unsigned char*)msg.c_str(), msg.size());
    }

    /// \brief 从客户端接收数据异常回调
    /// \param ullClientID 客户端ID
    virtual void OnClientDataError(unsigned __int64 ullClientID)
    {
        cout << __FUNCTION__ << endl;
    }

    /// \brief 断开客户端网络连接回调
    /// \param ullClientID 客户端ID
    virtual void OnClientDisconnect(unsigned __int64 ullClientID)
    {
        cout << __FUNCTION__ << endl;
    }
};



int main(int argc,char* argv[])
{
    client_notify notify;
    server = NetCreateServer(&notify, 2);
    server->StartListenWithHB(9025);

    getchar();
	return 0;
}