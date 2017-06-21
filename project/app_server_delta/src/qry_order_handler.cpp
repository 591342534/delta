#include "qry_order_handler.h"
#include "parse.h"
////////////////////////////////////////////////////////////////////////////////
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
void QryOrderHandler::on_request(context& context)
{
    try 
    {
        std::string message;
        unsigned int clientid;
        decode(context, message, clientid);
        std::cout << clientid << std::endl;

        manager_server_instrument_req data;
        memcpy(&data, message.c_str(), sizeof(manager_server_instrument_req));
        std::cout << data.UniqSequenceNo << std::endl;
        std::cout << data.BrokerID << std::endl;
        std::cout << data.InvestorID << std::endl;
        return;
    }
    catch (...)
    {
        
    }
}

////////////////////////////////////////////////////////////////////////////////
}// serverframe

