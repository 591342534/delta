/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: qry_instrument_handler.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "parse.h"
#include "qry_instrument_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"
#include "base/log_binder.h"
////////////////////////////////////////////////////////////////////////////////
using namespace common;
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
void qry_instrument_handler::on_request(Context& context)
{
    try
    {
        std::string message;
        unsigned int clientid;
        decode(context, message, clientid);

        manager_server_instrument_req data;
        memcpy(&data, message.c_str(), sizeof(manager_server_instrument_req));
        TRACE_INFO(MODULE_NAME, "qry_instrument_handler::on_request, clientid:%d "
                "requestid:%d", clientid, data.requestid);


        database::dbscope mysql_db_keepper(
                get_project_server().get_db_conn_pool());
        database::db_instance* mysql_dbconn = mysql_db_keepper.get_db_conn();
        //CHECK_IF_DBCONN_NULL(mysql_dbconn);

        char sql[1024] = { 0 };
        sprintf(sql, "select * from %s",
                get_project_server().get_process_param().
            instrument_table_str_.c_str());
        int affected_rows = 0;
        if (mysql_dbconn->_conn->query(sql)) {
            affected_rows = mysql_dbconn->_conn->affected_rows();

//            RSP_DATA_HEAD info;
//            info.status = MANAGER_S_OK;
//            info.num = affected_rows;
//            int data_size = sizeof(RSP_DATA_HEAD) + sizeof(manager_server_instrument_rsp) * info.num;

            MSG_HEADER msg_head;
            size_t header_size = sizeof(MSG_HEADER);
            msg_head.type = TYPE_MANAGERSERVER_INSTRUMENT_RSP;
            msg_head.data_size = sizeof(manager_server_instrument_rsp);
            size_t len = header_size + msg_head.data_size;

//            char *reply = new char[len];
//            memcpy(reply, &msg_head, sizeof(MSG_HEADER));
//            memcpy(reply + sizeof(MSG_HEADER), &info, sizeof(RSP_DATA_HEAD));

//            string reply;
//            reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));
//            reply.append(reinterpret_cast<char*>(&info), sizeof(common::RSP_DATA_HEAD));

            while (mysql_dbconn->_conn->fetch_row()) {
                string reply;
                reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));

                manager_server_instrument_rsp sp = {0};
                strcpy( sp.instrumentid, mysql_dbconn->_conn->get_string("instrumentid"));
                strcpy( sp.exchangeid, mysql_dbconn->_conn->get_string("exchangeid") );
                strcpy( sp.productid, mysql_dbconn->_conn->get_string("productid") );
                sp.productclass = mysql_dbconn->_conn->get_string("productclass")[0];
                sp.volumemultiple = mysql_dbconn->_conn->get_long("volumemultiple");
                sp.optionstype = mysql_dbconn->_conn->get_string("optionstype")[0];
                sp.strikeprice = mysql_dbconn->_conn->get_double("strikeprice");
                sp.IsLast = 0;

//                memcpy( reply + sizeof(MSG_HEADER) +sizeof(RSP_DATA_HEAD) +
//                        i*sizeof(manager_server_instrument_rsp),
//                       &sp, sizeof(manager_server_instrument_rsp) );
                reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_instrument_rsp));
                get_tcp_engine().get_tcpserver()->send_data(clientid, reply);

            }
            string reply;
            reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));
            manager_server_instrument_rsp sp = {0};
            sp.IsLast = 1;
            reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_instrument_rsp));
            get_tcp_engine().get_tcpserver()->send_data(clientid, reply);

            cout << "instrument counter: " << affected_rows << endl;
//            delete[] reply;
//            reply = NULL;
        } else {
            throw 404;
        }
    }
    catch (int error_code)
    {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_DB_FAILED, \
                "qry_instrument_handler failed to on_request, error:%d", error_code);
    }
}

////////////////////////////////////////////////////////////////////////////////
}// serverframe

