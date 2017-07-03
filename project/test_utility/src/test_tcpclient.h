#ifndef __TEST_TCPCLIENT_H__
#define __TEST_TCPCLIENT_H__

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <thread>
#include <memory>
#include "utility/clientsocket.h"
#include "utility/address.h"
#include "utility/state.h"
#include "../../delta/src/protocal.h"
using namespace std;
using namespace utility;

class test_tcpclient {
public:
    test_tcpclient()
        : m_tcpclient(nullptr)
        , m_rec_thread(nullptr)
    {
        //windows used
        qsocket::start();
    }

    virtual ~test_tcpclient()
    {
        m_state.none();
        m_rec_thread->join();
        if (m_tcpclient != nullptr) {
            delete m_tcpclient;
            m_tcpclient = nullptr;
        }

        //windows used
        qsocket::clean();
    }
public:
    void test()
    {
        m_tcpclient = new clientsocket();
        address addr;
        addr.host_name = "127.0.0.1";
        addr.port = "8889";
        m_tcpclient->connect_server(addr);


        //req contract
        common::MSG_HEADER msg_head;
        size_t header_size = sizeof(common::MSG_HEADER);
        msg_head.type = common::TYPE_MANAGERSERVER_INSTRUMENT_REQ;
        msg_head.data_size = sizeof(common::manager_server_instrument_req);

        common::manager_server_instrument_req instrument_req = { 0 };
        instrument_req.requestid = 1;
        strcpy(instrument_req.BrokerID, "9999");
        strcpy(instrument_req.InvestorID, "123456");

        string data;
        data.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));
        data.append(reinterpret_cast<char*>(&instrument_req), sizeof(common::manager_server_instrument_req));

        m_tcpclient->send_data(data);

        m_rec_thread = std::make_shared<std::thread>(std::bind(&test_tcpclient::run, this));
        while (1) {
            q_sleep(1000);
        }

    }

    void run()
    {
        m_state.ok();
        while (m_state.isok()) {
            string rsp;
            int len = m_tcpclient->receive_data(rsp);
            cout << len << endl;

            common::MSG_HEADER msg_head;
            memcpy(&msg_head, rsp.c_str(), sizeof(common::MSG_HEADER));
            common::manager_server_instrument_rsp instrument_rsp = { 0 };
            memcpy(&instrument_rsp, rsp.c_str() + sizeof(common::MSG_HEADER),
                sizeof(common::manager_server_instrument_rsp));

            cout << "rsp type:" << msg_head.type << endl;
            cout << "rsp instrument:" << instrument_rsp.instrumentid << endl;
        }
    }
private:
    clientsocket *m_tcpclient;

    utility::state m_state;
    std::shared_ptr<std::thread> m_rec_thread;
};

#endif
