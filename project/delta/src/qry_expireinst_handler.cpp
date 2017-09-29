/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: qry_expireinst_handler.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "parse.h"
#include "qry_expireinst_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"
#include "base/log_binder.h"
#include "base/util.h"
////////////////////////////////////////////////////////////////////////////////
using namespace common;
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
void qry_expireinst_handler::on_request(Context& context)
{
    try {
        std::string message;
        unsigned int clientid;
        decode(context, message, clientid);

        manager_server_expireinst_req data;
        memcpy(&data, message.c_str(), sizeof(manager_server_expireinst_req));

        TRACE_INFO(MODULE_NAME, "qry_expireinst_handler::on_request, clientid:%d "
                "requestid:%d", clientid, data.requestid);

        database::dbscope mysql_db_keepper(
                get_project_server().get_db_conn_pool());
        database::db_instance* mysql_dbconn = mysql_db_keepper.get_db_conn();
        //CHECK_IF_DBCONN_NULL(mysql_dbconn);

        char sql[1024] = { 0 };
        int affected_rows = 0;
        MSG_HEADER msg_head;
        size_t header_size = sizeof(MSG_HEADER);
        msg_head.type = TYPE_MANAGERSERVER_EXPIREINST_RSP;
        msg_head.data_size = sizeof(manager_server_expireinst_rsp);
        size_t len = header_size + msg_head.data_size;
        //最后到期日合约
        sprintf(sql, "SELECT * FROM %s WHERE expiredate=CURDATE()",
                get_project_server().get_process_param().
                instrument_table_str_.c_str());

        if (mysql_dbconn->_conn->query(sql)) {
            affected_rows = mysql_dbconn->_conn->affected_rows();
            while (mysql_dbconn->_conn->fetch_row()) {
                string reply;
                reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));

                manager_server_expireinst_rsp sp = {0};
                strcpy( sp.instrumentid, mysql_dbconn->_conn->get_string("instrumentid"));
                strcpy( sp.exchangeid, mysql_dbconn->_conn->get_string("exchangeid") );
                sp.productclass = mysql_dbconn->_conn->get_string("productclass")[0];
                strcpy( sp.expiredate, mysql_dbconn->_conn->get_string("expiredate") );
                sp.flag = EXPIREINST_FLAG_ALL;
                sp.IsLast = 0;

                reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_expireinst_rsp));
                //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);

                }
            cout << "expireinst counter 0: " << affected_rows << endl;
        } else {
            throw 404;
            }


        //上期所最后到期日前五个交易日
        sprintf(sql, "SELECT * FROM %s WHERE exchangeid='SHFE' "
                "AND expiredate>CURDATE() AND expiredate <= (SELECT day FROM %s "
                "WHERE day>=CURDATE() AND tra=1 ORDER BY day ASC LIMIT 4,1)",
                get_project_server().get_process_param().
                instrument_table_str_.c_str(),
                get_project_server().get_process_param().
                calendar_table_str_.c_str());

        if (mysql_dbconn->_conn->query(sql)) {
            affected_rows = mysql_dbconn->_conn->affected_rows();
            while (mysql_dbconn->_conn->fetch_row()) {
                string reply;
                reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));

                manager_server_expireinst_rsp sp = {0};
                strcpy( sp.instrumentid, mysql_dbconn->_conn->get_string("instrumentid"));
                strcpy( sp.exchangeid, mysql_dbconn->_conn->get_string("exchangeid") );
                sp.productclass = mysql_dbconn->_conn->get_string("productclass")[0];
                strcpy( sp.expiredate, mysql_dbconn->_conn->get_string("expiredate") );
                sp.flag = EXPIREINST_FLAG_SHFE;
                sp.IsLast = 0;

                reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_expireinst_rsp));
                //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);

                }
            cout << "expireinst counter 1: " << affected_rows << endl;
        } else {
            throw 404;
            }

        //中金所交割月前一个交易日
        string local_date = base::util::local_date_string();
        for (auto pos = local_date.begin(); pos != local_date.end(); pos++) {
            if (*pos == '-') local_date.erase(pos);
        }
        database::dbscope mysql_db_keepper_cffex(
                get_project_server().get_db_conn_pool());
        database::db_instance* mysql_dbconn_cffex = mysql_db_keepper_cffex.get_db_conn();
        sprintf(sql, "SELECT * FROM %s WHERE exchangeid='CFFEX' AND expiredate>CURDATE() AND deliverymode='2'",
                get_project_server().get_process_param().
                instrument_table_str_.c_str());

        if (mysql_dbconn->_conn->query(sql)) {
            affected_rows = mysql_dbconn->_conn->affected_rows();
            int real_rsp_rows = 0;
            while (mysql_dbconn->_conn->fetch_row()) {
                int deliveryyear = mysql_dbconn->_conn->get_long("deliveryyear");
                int deliverymonth = mysql_dbconn->_conn->get_long("deliverymonth");
                char delivery[9] = {0};
                sprintf(delivery, "%4d%02d00", deliveryyear, deliverymonth);
                sprintf(sql, "SELECT day FROM %s WHERE day<'%s' AND tra=1 "
                        "ORDER BY day DESC LIMIT 1",
                        get_project_server().get_process_param().
                        calendar_table_str_.c_str(), delivery);
                if (mysql_dbconn_cffex->_conn->query(sql)
                        && mysql_dbconn_cffex->_conn->fetch_row()) {
                    string day = mysql_dbconn_cffex->_conn->get_string("day");
                    if (local_date >= day) {
                        real_rsp_rows++;
                        string reply;
                        reply.append(reinterpret_cast<char*>(&msg_head),
                                sizeof(common::MSG_HEADER));
                        manager_server_expireinst_rsp sp = { 0 };
                        strcpy(sp.instrumentid, mysql_dbconn->_conn->get_string("instrumentid"));
                        strcpy(sp.exchangeid, mysql_dbconn->_conn->get_string("exchangeid"));
                        sp.productclass = mysql_dbconn->_conn->get_string("productclass")[0];
                        strcpy(sp.expiredate, mysql_dbconn->_conn->get_string("expiredate"));
                        sp.flag = EXPIREINST_FLAG_CFFEX;
                        sp.IsLast = 0;

                        reply.append(reinterpret_cast<char*>(&sp),
                                sizeof(common::manager_server_expireinst_rsp));
                        //get_tcp_engine().get_tcpserver()->SendData(clientid,reply.c_str(), len);
                        }
                    }
                else {
                    throw 404;
                    }
                }
            cout << "expireinst counter 2: " << affected_rows << endl;
            cout << "expireinst counter 3: " << real_rsp_rows << endl;
        } else {
            throw 404;
            }

        string reply;
        reply.append(reinterpret_cast<char*>(&msg_head), sizeof(common::MSG_HEADER));
        manager_server_expireinst_rsp sp = {0};
        sp.IsLast = 1;
        reply.append(reinterpret_cast<char*>(&sp), sizeof(common::manager_server_expireinst_rsp));
        //get_tcp_engine().get_tcpserver()->SendData(clientid, reply.c_str(), len);
    }
    catch (int error_code) {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_DB_FAILED, \
                "qry_calendar_handler failed to on_request, error:%d", error_code);
    }
}

////////////////////////////////////////////////////////////////////////////////
}// serverframe

