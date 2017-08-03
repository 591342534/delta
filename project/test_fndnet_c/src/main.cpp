#include <iostream>
#include <string>
#include "fndnet/ITCPClient.h"

using namespace std;

class client_notify : public IFndNetClientNotify
{
public:
    client_notify(){};
    ~client_notify(){};

    /*
    注意：由于线程时序的原因，有可能出现OnRawData，先于OnConnected被调用的情况，
    如有同步需求，请在回调函数内自行处理。
    */
    /// \brief 连接服务器回调函数，由nStatus指定连接是否成功。
    virtual void OnConnected(int nStatus)
    {
        cout << __FUNCTION__ << endl;
        cout << "status:" << nStatus << endl;
    }

    /// \brief 从服务器接收到数据回调(回调函数应复制数据后，立即返回，否则影响接收效率，有心跳方式阻塞时间不得超过15s，否则断线)
    /// \param pbData 数据指针(函数调用后即释放，勿超范围使用，勿在函数中删除)
    /// \param unDataSize 数据大小
    virtual void OnRawData(const unsigned char* pbData, unsigned int unDataSize)
    {
        cout << __FUNCTION__ << endl;
        string result;
        result.append((char*)(pbData), unDataSize);
        cout << result << endl;
    }

    /// \brief 从服务器接收数据异常回调
    virtual void OnServerDataError()
    {
        cout << __FUNCTION__ << endl;
    }

    /// \brief Socket连接断开回调
    virtual void OnDisconnected(int nStatus)
    {
        cout << __FUNCTION__ << endl;
    }
};

int main(int argc,char* argv[])
{
    client_notify notify;
    IFndNetClient* client = NetCreateClient(&notify, 2);
    client->ConnectServerWithHB("127.0.0.1", 9025);

    string msg = "hello world";
    client->SendDataToServerWithHB((const unsigned char*)msg.c_str(), msg.size());

    getchar();
	return 0;
}