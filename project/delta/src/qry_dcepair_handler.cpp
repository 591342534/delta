/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: qry_dcepair_handler.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "parse.h"
#include "qry_dcepair_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"
#include "base/log_binder.h"
////////////////////////////////////////////////////////////////////////////////
using namespace common;
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
void qry_dcepair_handler::on_request(Context& context)
{
    try {
        std::string message;
        unsigned int clientid;
        decode(context, message, clientid);

        manager_server_dcepair_req data;
        memcpy(&data, message.c_str(), sizeof(manager_server_dcepair_req));

        TRACE_INFO(MODULE_NAME, "qry_dcepair_handler::on_request, clientid:%d "
                "requestid:%d", clientid, data.requestid);

        database::dbscope mysql_db_keepper(
                get_project_server().get_db_conn_pool());
        database::db_instance* mysql_dbconn = mysql_db_keepper.get_db_conn();
        //CHECK_IF_DBCONN_NULL(mysql_dbconn);

        char sql[1024] = { 0 };
        sprintf(sql, "select * from %s",
                get_project_server().get_process_param().
            dcepair_inscom_table_str_.c_str());
        int affected_rows = 0;
        if (mysql_dbconn->_conn->query(sql)) {
            affected_rows = mysql_dbconn->_conn->affected_rows();

            MSG_HEADER msg_head;
            size_t header_size = sizeof(MSG_HEADER);
            msg_head.type = TYPE_MANAGERSERVER_DCEPAIR_RSP;
            msg_head.data_size = sizeof(manager_server_dcepair_rsp);
            size_t len = header_size + msg_head.data_size;

            while (mysql_dbconn->_conn->fetch_row()) {
                string reply;
                reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));

                manager_server_dcepair_rsp sp = {0};
                strcpy( sp.contractcode, mysql_dbconn->_conn->get_string("contractcode") );
                strcpy( sp.leftcont, mysql_dbconn->_conn->get_string("leftcont") );
                strcpy( sp.rightcont, mysql_dbconn->_conn->get_string("rightcont") );
                strcpy( sp.productcode, mysql_dbconn->_conn->get_string("productcode") );
                strcpy( sp.leftprodcode, mysql_dbconn->_conn->get_string("leftprodcode") );
                strcpy( sp.rightprodcode, mysql_dbconn->_conn->get_string("rightprodcode") );
                strcpy( sp.productkind, mysql_dbconn->_conn->get_string("productkind") );
                strcpy( sp.exchangecode, mysql_dbconn->_conn->get_string("exchangecode") );
                sp.permargin = mysql_dbconn->_conn->get_double("permargin");
                sp.margindir = mysql_dbconn->_conn->get_string("margindir")[0];
                sp.mainseq = mysql_dbconn->_conn->get_long("mainseq");
                sp.groupseq = mysql_dbconn->_conn->get_long("groupseq");
                sp.contractseq = mysql_dbconn->_conn->get_long("contractseq");
                sp.groupid = mysql_dbconn->_conn->get_long("groupid");
                sp.IsLast = 0;

                reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_dcepair_rsp));
                //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);

                }
            string reply;
            reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));
            manager_server_dcepair_rsp sp = {0};
            sp.IsLast = 1;
            reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_dcepair_rsp));
            //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);

            cout << "dcepair counter: " << affected_rows << endl;
        } else {
            throw 404;
            }
    }
    catch (int error_code) {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_DB_FAILED, \
                "qry_dcepair_handler failed to on_request, error:%d", error_code);
    }
}

////////////////////////////////////////////////////////////////////////////////
}// serverframe

