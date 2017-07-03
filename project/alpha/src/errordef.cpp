#include "errordef.h"

namespace serverframe
{

const char *errno_transfer(long nerrno)
{
    switch (nerrno)
    {
        case 0:
            return "success";

        //FIELD_ERROR
        case ERROR_FIELD_ACCOUNT:
        	return "field account not true";
        case ERROR_FIELD_EXCHANGE_CODE:
        	return "field exchange_code not true";
        case ERROR_FIELD_DATE:
        	return "field date not true";
        case ERROR_FIELD_SWITCH:
        	return "field switch note true";
        case ERROR_FIELD_OPT:
        	return "field opt not true";
        case ERROR_FIELD_FEE_TYPE:
        	return "field fee_type not true";
        case ERROR_FIELD_CURRENCY_CODE:
        	return "field currency_code not true";
        case ERROR_FIELD_BASE_CURRENCY_CODE:
        	return "field base_currency_code not true";
        case ERROR_FIELD_PARAM_CODE:
        	return "field param_code not true";
        case ERROR_FIELD_PARAM_TYPE:
        	return "field param_type not true";
        case ERROR_FIELD_SECURITIES_TYPE:
        	return "field securities_type not true";
        case ERROR_FIELD_SECURITIES_CODE:
        	return "field securities_code not true";
        case ERROR_FIELD_HOLIDAY:
        	return "field holiday not true";
        case ERROR_FIELD_PRODUCT_TYPE:
        	return "field product_type not true";
        case ERROR_FIELD_STOP_LOSS_RATE:
        	return "field stop_loss_rate not true";
        case ERROR_FIELD_STOP_WIN_RATE:
        	return "field stop_win_rate not true";
        case ERROR_FIELD_LOAN_RATE:
        	return "field loan_rate not true";
        case ERROR_FIELD_SYSTEM_NAME:
        	return "field system_name not true";
        case ERROR_FIELD_MODULE_NAME:
        	return "field module_name not true";
        case ERROR_FIELD_ERROR_CODE:
        	return "field error_code not true";
        case ERROR_FIELD_ENTRUST_TYPE:
        	return "field entrust_type not ture";
        case ERROR_FIELD_CONTRACT_ID:
        	return "field contract_id not true";
        case ERROR_FIELD_TRADE_FLAG:
        	return "field trade_flag not true";
        case ERROR_FIELD_BROKE_DAY:
        	return "field broke_day not true";
        case ERROR_FIELD_OPERATOR:
        	return "field operator not true";
        case ERROR_FIELD_SERVER_NAME:
        	return "field server_name not true";
        case ERROR_FIELD_EXT_ORDER_NO:
        	return "field ext_order_no not true";
        case ERROR_FIELD_OBJECT:
        	return "field object not true";
        case ERROR_FIELD_SQL:
        	return "field sql not true";

        //BUSI_ERROR
		case ERROR_BUSI_DB_NULL_RECORD:
			return "no record";

		//SYSTEM_ERROR
		case ERROR_SYSTEM_UNAVAILABLE:
			return "sytem unavailable";
		case ERROR_SYSTEM_TIMEOUT:
			return "system timeout";
		case ERROR_SYSTEM_DB_CONTORL:
			return "system db_contorl";
		case ERROR_SYSTEM_DB_NULL_RECORD:
			return "system db_null_record";
		case ERROR_SYSTEM_DB_AFFECTED_ROWS:
			return "system db_affected_rows";
		case ERROR_SYSTEM_HTTP_POST_ILLEGAL_JSON:
			return "system http_post_illegal_json";
		case ERROR_SYSTEM_HTTP_POST_ILLEGAL_DATA_LEN:
			return "system http_post_illegal_data_len";
		case ERROR_SYSTEM_QUOTE:
			return "system quote";

        //policy error
        case ERROR_POLICY_CONFIGFILE_INVALID:
        	return "policy config file invalid";
        case ERROR_POLICY_CONFIG_INVALID:
        	return "policy config invalid";
        case ERROR_POLICY_DB_CONNECT_INVALID:
        	return "policy db connect invalid";
        case ERROR_POLICY_DB_INVALID:
        	return "policy db invalid";
        case ERROR_POLICY_FRONT_SENDER_EXCEPTION:
        	return "policy front send exception";
        case ERROR_POLICY_ORDER_QUEUE_NOT_EMPTY:
        	return "policy order queue not empty";
        case ERROR_POLICY_LIM_PROSSOOR_NULL:
        	return "policy limit processor is null";
        case ERROR_POLICY_EXCEP_PROSSOOR_NULL:
        	return "policy onreceive exception processor is null";
        case ERROR_POLICY_NEW_EXCEPTION:
        	return "policy new is null";
        case ERROR_POLICY_MARKET_EXCEPTION:
        	return "policy market exception";
        case ERROR_POLICY_UNFREEZE_TOO_MUCH:
        	return "policy unfeeze too much cash or stock not enough";
        case ERROR_POLICY_REDO_TOO_MUCH:
        	return "policy exception order redo too much";
        case ERROR_POLICY_ORDER_TYPE_EXCEPTION:
        	return "policy unkown order type";
        	
        	//risk error
        case ERROR_RISK_CONFIGFILE_INVALID:
        	return "risk config file invalid";
        case ERROR_RISK_CONFIG_INVALID:
        	return "risk config invalid";
        case ERROR_RISK_DB_CONNECT_INVALID:
        	return "risk db connect invalid";
        case ERROR_RISK_DB_INVALID:
        	return "risk db invalid";
        case ERROR_RISK_FRONTSENDER_EXCEPTION:
        	return "risk front sender exception";
        case ERROR_RISK_CONTRACTID_EXIST:
        	return "risk cur contract_id is in risk queue";
        case ERROR_RISK_CONTRACTID_EXCEPTION:
        	return "risk cur contract_id exception";
        case ERROR_RISK_MARKET_EXCEPTION:
        	return "risk market exception";
        case ERROR_RISK_NEW_EXCEPTION:
        	return "risk new exception";
        case ERROR_RISK_RISK_ORDER_REMOVE:
        	return "risk risk contract_id is already removed";
        case ERROR_RISK_NOT_TRADE_TIME:
        	return "risk cur is ont trade_time";
        case ERROR_RISK_HQ_EXCEPTION:
        	return "risk cur hq exception";
        case ERROR_RISK_FORCE_SELL_EXCEPTION:
        	return "risk force sell exception";

        default:
            return "not define error";
    }
}

}
