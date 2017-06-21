#include "server.h"

#include "default_handler.h"
#include "qry_order_handler.h"
#include "project_server.h"
#include "protocal.h"
namespace serverframe
{
////////////////////////////////////////////////////////////////////////////////
void server::run(const size_t thread_num)
{
    // 注册请求处理
    std::cout << "server starting" << std::endl;

    m_hub.register_handle(std::bind(&server::init, this, std::placeholders::_1));

    // 开启处理者线程
    m_hub.run(thread_num);

    // tcp连接到消息总线
    tcp_engine::create_instance();
    int port = project_server::get_instance()->get_process_param().serverinfo_.port;
    tcp_engine::get_instance()->init("127.0.0.1", port, m_tcp_notify);
    
}


////////////////////////////////////////////////////////////////////////////////
void server::join()
{
    m_hub.join();
}


////////////////////////////////////////////////////////////////////////////////
void server::stop()
{
    m_hub.stop();
    tcp_engine::destory_instance();
}


////////////////////////////////////////////////////////////////////////////////
void server::init(message_dispather& dispatcher)
{
    // 默认处理请求.
    dispatcher.route_default<DefaultHandler>();

    // 查询订单委托。
    dispatcher.route<QryOrderHandler>(TYPE_MANAGERSERVER_INSTRUMENT_REQ);
}


////////////////////////////////////////////////////////////////////////////////
}// ns::serverframe
