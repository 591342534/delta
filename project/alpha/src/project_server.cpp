/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: project_server.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "project_server.h"
#include "database/dbscope.h"
#include "base/pugixml.hpp"
#include "common.h"
#include "base/util.h"
using namespace database;

namespace serverframe {
    project_server::project_server()
        : m_deal_pro_(nullptr)
        , m_http_pro_(nullptr)
        , m_http_unit_sptr_(nullptr)
        , started_(false)
    {
    }

    project_server::~project_server()
    {
        stop();
    }


    int project_server::start(std::string &path)
    {
        int ret = NAUT_S_OK;
        LABEL_SCOPE_START;

        TRACE_SYSTEM(MODULE_NAME, "----------start alpha-----------");

        ret = load_config(path.c_str());
        CHECK_LABEL(ret, end);
        ret = db_pool_.init(params_.dbinfo_.dbparam, 5, params_.dbinfo_.dbtype);

        ret = start_internal();
        CHECK_LABEL(ret, end);

        LABEL_SCOPE_END;
    end:
        if (BSUCCEEDED(ret)) {
            started_ = true;
        }
        else {
            stop();
        }

        return ret;
    }

    int project_server::stop()
    {
        int ret = NAUT_S_OK;

        TRACE_SYSTEM(MODULE_NAME, "----------stop alpha-----------");

        started_ = false;
        ret = stop_internal();

        return ret;
    }

    int project_server::start_internal()
    {
        int ret = NAUT_S_OK;
        LABEL_SCOPE_START;

        /* start processors */
        m_deal_pro_ = new deals_processor();
        ret = m_deal_pro_->start("1");
        CHECK_LABEL(ret, end);

        for (int i = 0; i < params_.query_processor_thread_count_; i++) {
            query_processor* qp = new query_processor();
            m_query_pro_vec_.push_back(qp);
            string intstr("");
            intstr += ('0' + i);
            ret = qp->start(intstr.c_str());
            CHECK_LABEL(ret, end);
        }

        m_http_pro_ = std::make_shared<http_processor>();
        ret = m_http_pro_->start();
        CHECK_LABEL(ret, end);

        m_http_unit_sptr_ = std::make_shared<http_unit>();
        ret = m_http_unit_sptr_->start("1");
        CHECK_LABEL(ret, end);

        LABEL_SCOPE_END;
    end:
        return ret;
    }

    int project_server::stop_internal()
    {
        int ret = NAUT_S_OK;
        if (m_deal_pro_ != NULL) {
            m_deal_pro_->stop();
            delete m_deal_pro_;
            m_deal_pro_ = NULL;
        }

        for (size_t i = 0; i < m_query_pro_vec_.size(); i++) {
            if (m_query_pro_vec_[i] != NULL) {
                m_query_pro_vec_[i]->stop();
                delete m_query_pro_vec_[i];
            }
        }

        return ret;
    }

    int project_server::dispatch_message(atp_message& msg)
    {
        if (!started_) {
            ref_dictionary* rd = (ref_dictionary*)msg.param1;
            rd->release();
            return NAUT_S_OK;
        }

        switch (msg.type) {
        case ATP_MESSAGE_TYPE_TRADE_RSP:
        {
            if (m_deal_pro_ != NULL) {
                m_deal_pro_->post(msg);
            }
        }
            break;
        case ATP_MESSAGE_TYPE_QUERY_REQ:
        {
            if (m_query_pro_vec_.size() > 0) {
                ref_dictionary* rd = (ref_dictionary*)msg.param1;
                assert(rd != NULL);
                string account_key = (*(rd->get()))["broker"].string_value() + (*(rd->get()))["account"].string_value().c_str();
                int index = get_index(account_key.c_str(), (int)m_query_pro_vec_.size());
                if (m_query_pro_vec_[index] != NULL) {
                    m_query_pro_vec_[index]->post(msg);
                }
            }
        }
            break;
        default:
            TRACE_WARNING(MODULE_NAME, "unknown message type: %d", msg.type);
            break;
        }
        return NAUT_S_OK;
    }

    int project_server::get_index(const char* key, int bound)
    {
        return base::util::hash_key(key) % bound;
    }

    int project_server::load_config(const char* config_file)
    {
        int ret = NAUT_S_OK;

        LABEL_SCOPE_START;

        pugi::xml_document config;
        if (!config.load_file(config_file)) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIGFILE_INVALID,
                "xml %s load failed", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIGFILE_INVALID, end);
        }

        pugi::xml_node xdb = config.child("manager_server").child("db");
        pugi::xml_node xtableconfig = config.child("manager_server").child(
            "table_config");
        //pugi::xml_node xsystem = config.child("manager_server").child("system");
        pugi::xml_node xupdatetime = config.child("manager_server").child(
            "update_time");
        pugi::xml_node xserverinfo = config.child("manager_server").child(
            "server_info");

        std::string tmp;
        //db info
        tmp = xdb.child("type").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: type of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dbinfo_.dbtype = tmp;

        tmp = xdb.child("host").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: host of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dbinfo_.dbparam.host = tmp;

        tmp = xdb.child("port").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: port of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dbinfo_.dbparam.port = atoi(tmp.c_str());

        tmp = xdb.child("user").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: user of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dbinfo_.dbparam.user = tmp;

        tmp = xdb.child("pwd").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: pwd of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        base::util::encode(const_cast<char*>(tmp.c_str()));
        params_.dbinfo_.dbparam.password = tmp;

        tmp = xdb.child("dbname").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: dbname of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dbinfo_.dbparam.database_name = tmp;

        tmp = xdb.child("charset").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: charset of the db should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dbinfo_.dbparam.charset = tmp;
        params_.dbinfo_.dbparam.create_database_if_not_exists = false;
        params_.dbinfo_.dbparam.recreate_database_if_exists = false;

        //table config
        tmp = xtableconfig.child("instrument").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: instrument_table should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.instrument_table_str_ = tmp;

        tmp = xtableconfig.child("insmrgnrate").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: insmrgnrate_table should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.insmrgnrate_table_str_ = tmp;

        tmp = xtableconfig.child("dcepair").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: dcepair_table should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.dcepair_inscom_table_str_ = tmp;

        tmp = xtableconfig.child("calendar").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: calendar_table should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.calendar_table_str_ = tmp;

        tmp = xtableconfig.child("calcdeposit").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: calcdeposit_table should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.calcdeposit_table_str_ = tmp;

        //update_time
        tmp = xupdatetime.child("am_time").text().as_string();
        if (!tmp.empty()) {
            params_.update_data_time_ = tmp;
        }

        //serverinfo
        tmp = xserverinfo.child("ip").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: ip of server should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.serverinfo_.ip = tmp;

        tmp = xserverinfo.child("port").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(MODULE_NAME, MANAGER_E_CONFIG_INVALID,
                "manager_server config: port of server should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, MANAGER_E_CONFIG_INVALID,
                end);
        }
        params_.serverinfo_.port = atoi(tmp.c_str());

        LABEL_SCOPE_END;

    end: return ret;
        return 0;
    }
}
