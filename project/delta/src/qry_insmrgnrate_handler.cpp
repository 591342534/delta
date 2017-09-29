/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: qry_insmrgnrate_handler.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "parse.h"
#include "qry_insmrgnrate_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"
#include "base/log_binder.h"
////////////////////////////////////////////////////////////////////////////////
using namespace common;
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
void qry_insmrgnrate_handler::on_request(Context& context)
{
    try {
        std::string message;
        unsigned int clientid;
        decode(context, message, clientid);

        manager_server_instrument_req data;
        memcpy(&data, message.c_str(), sizeof(manager_server_instrument_req));

        TRACE_INFO(MODULE_NAME, "qry_insmrgnrate_handler::on_request, clientid:%d "
                "requestid:%d", clientid, data.requestid);

        database::dbscope mysql_db_keepper(
                get_project_server().get_db_conn_pool());
        database::db_instance* mysql_dbconn = mysql_db_keepper.get_db_conn();
        //CHECK_IF_DBCONN_NULL(mysql_dbconn);

        char sql[1024] = { 0 };
        sprintf(sql, "select * from %s",
                get_project_server().get_process_param().
            insmrgnrate_table_str_.c_str());
        int affected_rows = 0;
        if (mysql_dbconn->_conn->query(sql)) {
            affected_rows = mysql_dbconn->_conn->affected_rows();

            MSG_HEADER msg_head;
            size_t header_size = sizeof(MSG_HEADER);
            msg_head.type = TYPE_MANAGERSERVER_INSMRGNRATE_RSP;
            msg_head.data_size = sizeof(manager_server_insmrgnrate_rsp);
            size_t len = header_size + msg_head.data_size;

            while (mysql_dbconn->_conn->fetch_row()) {
                string reply;
                reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));

                manager_server_insmrgnrate_rsp sp = {0};
                strcpy( sp.tradingday, mysql_dbconn->_conn->get_string("tradingday"));
                strcpy( sp.contractcode, mysql_dbconn->_conn->get_string("contractcode") );
                strcpy( sp.exchangecode, mysql_dbconn->_conn->get_string("exchangecode") );
                sp.hedgeflag = mysql_dbconn->_conn->get_string("hedgeflag")[0];
                sp.direction = mysql_dbconn->_conn->get_string("direction")[0];
                sp.marginrate = mysql_dbconn->_conn->get_double("marginrate");
                sp.segmrgn = mysql_dbconn->_conn->get_double("segmrgn");
                sp.IsLast = 0;

                reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_insmrgnrate_rsp));
                //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);

                }
            string reply;
            reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));
            manager_server_insmrgnrate_rsp sp = {0};
            sp.IsLast = 1;
            reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_insmrgnrate_rsp));
            //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);

            cout << "insmrgnrate counter: " << affected_rows << endl;
        } else {
            throw 404;
            }
    }
    catch (int error_code) {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_DB_FAILED, \
                "qry_insmrgnrate_handler failed to on_request, error:%d", error_code);
    }
}

////////////////////////////////////////////////////////////////////////////////
}// serverframe

