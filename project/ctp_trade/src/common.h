/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: common.h
 Version: 1.0
 Date: 2017.08.08

 History:
 eric     2017.08.08   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CTPTRADE_COMMON_H__
#define __NAUT_CTPTRADE_COMMON_H__

namespace ctp
{

enum CTPTRADE_ERROR
{
	NAUT_AT_S_OK = 0,

	NAUT_AT_E_CONFIGFILE_INVALID = 30000,					/* 配置文件找不到或者不合法 */
	NAUT_AT_E_CONFIG_INVALID = 30001,						/* 配置不合法 */

	NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED = 30010,		/* 连接配置数据库失败 */
	NAUT_AT_E_INIT_DATABASE_CONN_FAILED = 30011,			/* 初始化数据库连接失败 */

	NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED = 30012,			/* 请求服务器配置失败 */
	NAUT_AT_E_SERVER_CONFIG_NOT_EXIST = 30013,				/* 该服务对应配置不存在 */
	NAUT_AT_E_SERVER_CONFIG_INVALID = 30014,				/* 该服务对应配置不合法 */

	NAUT_AT_E_QUERY_ACCOUNT_CONFIG_FAILED = 30015,			/* 请求账户配置失败 */
	NAUT_AT_E_ACCOUNT_CONFIG_NOT_EXIT = 30016,				/* 该服务对应账户配置不存在 */
	NAUT_AT_E_ACCOUNT_CONFIG_INVALID = 30017,				/* 该服务对应账户配置不合法 */

	NAUT_AT_E_ACCOUNTS_DUPLICATED = 30020,					/* 账户重复 */
	NAUT_AT_E_ACCOUNT_MISMATCHED = 30021,					/* 账户不匹配 */

	NAUT_AT_E_CONNECT_MQ_FAILED = 30021,					/* 连接到消息中间件失败 */
	NAUT_AT_E_MQ_SUBSCRIBE_FAILED = 30032,					/* 订阅消息中间件失败 */
	NAUT_AT_E_MQ_DISCONNECTED = 30033,						/* 与消息中间件的连接中断 */
	NAUT_AT_E_MQ_PUBLISH_MESSAGE_FAILED = 30034,			/* 发布消息到消息中间件失败 */

	NAUT_AT_E_MQ_MESSAGE_INVALID = 30100,					/* 消息中间件的消息不合法 */

	NAUT_AT_E_CTPTRADE_GET_INSTANCE_FAILED = 30200,			/* 生成交易服务实例失败 */
	NAUT_AT_E_CTPTRADE_INIT_FAILED = 30201,					/* 交易服务实例初始化失败 */
	NAUT_AT_E_CTPTRADE_REGISTER_CALLBACK_FAILED = 30202,		/* 交易服务实例注册回调失败 */
	NAUT_AT_E_CTPTRADE_REGISTER_FRONT_FAILED = 30203,		/* 交易服务实例连接服务失败 */
	NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED = 30204,		/* 交易服务实例发送交易信息失败 */
	NAUT_AT_E_CTPTRADE_NOT_CONNECTED = 30205,				/* 交易服务实例连接中断 */
	NAUT_AT_E_CTPTRADE_ACCOUNT_NOT_MATCHED = 30206,			/* 交易服务实例中账户不匹配 */
	NAUT_AT_E_CTPTRADE_LOGIN_FAILED = 30207,				/* 交易服务登录失败 */

	NAUT_AT_E_ORDER_EXPIRED = 30300,						/* 订单过期 */
	NAUT_AT_E_ORDER_EXISTED = 30301,						/* 委托订单已存在（拒绝同订单号的重复订单） */
	NAUT_AT_E_ORDER_NOT_EXIST = 30302,						/* 委托订单不存在（比如查询目标订单，但实际不存在） */
	NAUT_AT_E_EXCUTE_DB_QUERY_FAILED = 30303,				/* 执行数据库查询失败 */
	NAUT_AT_E_EXCUTE_DB_FAILED = 30304,						/* 执行数据库操作失败 */
	NAUT_AT_E_ENTRUST_CHECK_ERROR = 30305,					/* 委托检查失败（如订单不存在或订单重复） */
	NAUT_AT_E_DEAL_EXISTED = 30306,							/* 成交已存在 */
	NAUT_AT_E_SYSTEMNO_NOT_EXIST = 30307,					/* 订单系统号不存在 */
	NAUT_AT_E_ORDER_HAS_BEEN_DEALED = 30308,				/* 目标订单已成交 */
	NAUT_AT_E_ORDERID_SHOULD_SPECIFIED = 30309,				/* 订单号必须指定（对于委托查询等业务） */
	NAUT_AT_E_ENTRUSTNO_NOT_EXIST = 30310,					/* 订单委托号不存在 */
	NAUT_AT_E_ORDER_HAS_BEEN_WITHDRAWED = 30311,			/* 目标订单已撤单 */
	NAUT_AT_E_ORDER_IS_FAILED = 30312,						/* 目标订单已失败 */
	NAUT_AT_E_ACCOUNT_NOT_EXIST = 30313,					/* 帐号不存在 */
	NAUT_AT_E_SQP_CONNECT_FAILED = 30314,					/* 特别查询连接帐号失败 */
	NAUT_AT_E_SQL_WAIT_RESPONSE_TIMEOUT = 30315,			/* 特别查询等待回报超时 */
	NAUT_AT_E_LOCALNO_NOT_EXIST = 30316,					/* 本地号不存在 */
	NAUT_AT_E_UNKNOWN_TRADE_CMD = 30317,					/* 未知交易命令 */
	NAUT_AT_E_SEND_TRADEINFO_FAILED = 30318,				/* 发送交易信息失败 */

	NAUT_AT_E_OPEN_PROGRESS_LOG_FILE_FAILED = 30401,		/* 打开进度日志文件失败 */
	NAUT_AT_E_PROGRESS_INFO_CRC_MISMATCHED = 30402,			/* 进度信息CRC校验失败 */
};

const char* const AT_TRACE_TAG = "ctp_trade";

/* mq topic defines */
const char* const MQ_TOPIC_TRADE = "trade";
const char* const MQ_TOPIC_QUERY = "query";
const char* const MQ_TOPIC_RSP_TRADE = "rsp_trade";
const char* const MQ_TOPIC_RSP_QUERY = "rsp_query";

/* atp message command defines */
const char* const ATPM_CMD_ERROR = "cmd_error";
const char* const ATPM_CMD_ENTRUST = "entrust";
const char* const ATPM_CMD_ENTRUST_RESULT = "entrust_result";
const char* const ATPM_CMD_ENTRUST_RESPONSE = "entrust_response";
const char* const ATPM_CMD_ENTRUST_CAPITAL_RESPONSE = "entrust_capital_response";
const char* const ATPM_CMD_ORDER_STATUS_RESPONSE = "order_status_response";
const char* const ATPM_CMD_DEAL = "deal";
const char* const ATPM_CMD_WITHDRAW = "withdraw";
const char* const ATPM_CMD_WITHDRAW_RESULT = "withdraw_result";
const char* const ATPM_CMD_WITHDRAW_RESPONSE = "withdraw_response";
const char* const ATPM_CMD_SYSTEMNO_RESPONSE = "systemno_response";
const char* const ATPM_CMD_QRY_ACCOUNT = "qry_account";
const char* const ATPM_CMD_RSP_ACCOUNT = "rsp_account";
const char* const ATPM_CMD_QRY_DEAL = "qry_deal";
const char* const ATPM_CMD_RSP_DEAL = "rsp_deal";
const char* const ATPM_CMD_QRY_DEAL_ALL = "qry_deal_all";
const char* const ATPM_CMD_RSP_DEAL_ALL = "rsp_deal_all";
const char* const ATPM_CMD_QRY_CHECKER_ENTRUST = "qry_checker_entrust";
const char* const ATPM_CMD_RSP_CHECKER_ENTRUST = "rsp_checker_entrust";
const char* const ATPM_CMD_QRY_CHECKER_DEAL = "qry_checker_deal";
const char* const ATPM_CMD_RSP_CHECKER_DEAL = "rsp_checker_deal";
const char* const ATPM_CMD_QRY_OPENINTER = "qry_openinter";
const char* const ATPM_CMD_RSP_OPENINTER = "rsp_openinter";
const char* const ATPM_CMD_QRY_ENTRUST = "qry_entrust";
const char* const ATPM_CMD_RSP_ENTRUST = "rsp_entrust";

/* internal field name defines */
#define AT_ENTRUSTID 				"entrustid" 		/* 本地委托编号（自增）*/
#define AT_ORDERID					"orderid"		 	/* 内部订单号 */
#define AT_ENTRUST_NO				"entrustno" 		/* 券商委托号 */
#define AT_LOCAL_NO					"localno" 			/* 本地标识号，用户生成 */
#define AT_SYSTEM_NO			 	"systemno"		 	/* 系统编号，由交易系统生成 */
#define AT_EXCODE					"excode"			/* 交易所编号 */
#define AT_BROKER					"broker"			/* 经纪商 */
#define AT_USERID					"userid"			/* 客户帐号 */
#define AT_ACCOUNT					"account"			/* 资金帐号 */
#define AT_DIRECTION				"direction"			/* 买卖方向（1 买/多 2 卖/空）*/
#define AT_ENTRUST_TYPE				"entrust_type"		/* 委托类别（1 开仓 2 平仓）*/
#define AT_CODE						"code"				/* 合约代码 */
#define AT_ENTRUST_AMOUNT			"entrust_amount"	/* 买卖数量 */
#define AT_ENTRUST_PRICE			"entrust_price"		/* 委托价格 */
#define AT_PRICE_TYPE				"price_type"		/* 价格类别（1 limit order 2 market order 3 限价止损 4 市价止损）*/
#define AT_ENTRUST_DATE 			"entrust_date"		/* 委托日期 */
#define AT_ENTRUST_TIME				"entrust_time"		/* 委托时间 */
#define AT_ORDER_TYPE				"order_type" 		/* 订单类别（1：普通订单；2：自动止盈止损订单）*/
#define AT_REPORT_TIME				"report_time" 		/* 结果返回时间 */
#define AT_ERROR_RESULT 			"error_result"		/* 错误码 */
#define AT_ERROR_MSG				"error_msg"			/* 错误信息 */
#define AT_ORDER_STATUS				"order_status"		/* 订单状态 */
#define AT_ORDER_TIME				"order_time" 		/* 订单时间 */
#define AT_ORDER_VALID_TIME 		"order_valid_time" 	/* 订单失效时间 */
#define AT_ORDER_TIME_T				"order_time_t" 		/* 订单时间 */
#define AT_ORDER_VALID_TIME_T		"order_valid_time_t"/* 订单失效时间 */
#define AT_FLAG						"flag"				/* 标志位 */

#define QUERY_WAIT_TIMEOUT 120000  						/* 120 seconds */

/* params code */
#define PARAM_CODE_CHECKER_WAIT_TIMEOUT 20000			/* order checker query wait timeout */

/* ctp trade system error codes */
enum AT_CTP_ERROR_CODE
{
	AT_ERROR_CTP_NONE = 0,  /* CTP:正确 */
	AT_ERROR_CTP_INVALID_DATA_SYNC_STATUS = 1,  /* CTP:不在已同步状态 */
	AT_ERROR_CTP_INCONSISTENT_INFORMATION = 2,  /* CTP:会话信息不一致 */
	AT_ERROR_CTP_INVALID_LOGIN = 3,  /* CTP:不合法的登录 */
	AT_ERROR_CTP_USER_NOT_ACTIVE = 4,  /* CTP:用户不活跃 */
	AT_ERROR_CTP_DUPLICATE_LOGIN = 5,  /* CTP:重复的登录 */
	AT_ERROR_CTP_NOT_LOGIN_YET = 6,  /* CTP:还没有登录 */
	AT_ERROR_CTP_NOT_INITED = 7,  /* CTP:还没有初始化 */
	AT_ERROR_CTP_FRONT_NOT_ACTIVE = 8,  /* CTP:前置不活跃 */
	AT_ERROR_CTP_NO_PRIVILEGE = 9,  /* CTP:无此权限 */
	AT_ERROR_CTP_CHANGE_OTHER_PASSWORD = 10,  /* CTP:修改别人的口令 */
	AT_ERROR_CTP_USER_NOT_FOUND = 11,  /* CTP:找不到该用户 */
	AT_ERROR_CTP_BROKER_NOT_FOUND = 12,  /* CTP:找不到该经纪公司 */
	AT_ERROR_CTP_INVESTOR_NOT_FOUND = 13,  /* CTP:找不到投资者 */
	AT_ERROR_CTP_OLD_PASSWORD_MISMATCH = 14,  /* CTP:原口令不匹配 */
	AT_ERROR_CTP_BAD_FIELD = 15,  /* CTP:报单字段有误 */
	AT_ERROR_CTP_INSTRUMENT_NOT_FOUND = 16,  /* CTP:找不到合约 */
	AT_ERROR_CTP_INSTRUMENT_NOT_TRADING = 17,  /* CTP:合约不能交易 */
	AT_ERROR_CTP_NOT_EXCHANGE_PARTICIPANT = 18,  /* CTP:经纪公司不是交易所的会员 */
	AT_ERROR_CTP_INVESTOR_NOT_ACTIVE = 19,  /* CTP:投资者不活跃 */
	AT_ERROR_CTP_NOT_EXCHANGE_CLIENT = 20,  /* CTP:投资者未在交易所开户 */
	AT_ERROR_CTP_NO_VALID_TRADER_AVAILABLE = 21,  /* CTP:该交易席位未连接到交易所 */
	AT_ERROR_CTP_DUPLICATE_ORDER_REF = 22,  /* CTP:报单错误：不允许重复报单 */
	AT_ERROR_CTP_BAD_ORDER_ACTION_FIELD = 23,  /* CTP:错误的报单操作字段 */
	AT_ERROR_CTP_DUPLICATE_ORDER_ACTION_REF = 24,  /* CTP:撤单已报送，不允许重复撤单 */
	AT_ERROR_CTP_ORDER_NOT_FOUND = 25,  /* CTP:撤单找不到相应报单 */
	AT_ERROR_CTP_INSUITABLE_ORDER_STATUS = 26,  /* CTP:报单已全成交或已撤销，不能再撤 */
	AT_ERROR_CTP_UNSUPPORTED_FUNCTION = 27,  /* CTP:不支持的功能 */
	AT_ERROR_CTP_NO_TRADING_RIGHT = 28,  /* CTP:没有报单交易权限 */
	AT_ERROR_CTP_CLOSE_ONLY = 29,  /* CTP:只能平仓 */
	AT_ERROR_CTP_OVER_CLOSE_POSITION = 30,  /* CTP:平仓量超过持仓量 */
	AT_ERROR_CTP_INSUFFICIENT_MONEY = 31,  /* CTP:资金不足 */
	AT_ERROR_CTP_DUPLICATE_PK = 32,  /* CTP:主键重复 */
	AT_ERROR_CTP_CANNOT_FIND_PK = 33,  /* CTP:找不到主键 */
	AT_ERROR_CTP_CAN_NOT_INACTIVE_BROKER = 34,  /* CTP:设置经纪公司不活跃状态失败 */
	AT_ERROR_CTP_BROKER_SYNCHRONIZING = 35,  /* CTP:经纪公司正在同步 */
	AT_ERROR_CTP_BROKER_SYNCHRONIZED = 36,  /* CTP:经纪公司已同步 */
	AT_ERROR_CTP_SHORT_SELL = 37,  /* CTP:现货交易不能卖空 */
	AT_ERROR_CTP_INVALID_SETTLEMENT_REF = 38,  /* CTP:不合法的结算引用 */
	AT_ERROR_CTP_CFFEX_NETWORK_ERROR = 39,  /* CTP:交易所网络连接失败 */
	AT_ERROR_CTP_CFFEX_OVER_REQUEST = 40,  /* CTP:交易所未处理请求超过许可数 */
	AT_ERROR_CTP_CFFEX_OVER_REQUEST_PER_SECOND = 41,  /* CTP:交易所每秒发送请求数超过许可数 */
	AT_ERROR_CTP_SETTLEMENT_INFO_NOT_CONFIRMED = 42,  /* CTP:结算结果未确认 */
	AT_ERROR_CTP_DEPOSIT_NOT_FOUND = 43,  /* CTP:没有对应的入金记录 */
	AT_ERROR_CTP_EXCHANG_TRADING = 44,  /* CTP:交易所已经进入连续交易状态 */
	AT_ERROR_CTP_PARKEDORDER_NOT_FOUND = 45,  /* CTP:找不到预埋（撤单）单 */
	AT_ERROR_CTP_PARKEDORDER_HASSENDED = 46,  /* CTP:预埋（撤单）单已经发送 */
	AT_ERROR_CTP_PARKEDORDER_HASDELETE = 47,  /* CTP:预埋（撤单）单已经删除 */
	AT_ERROR_CTP_INVALID_INVESTORIDORPASSWORD = 48,  /* CTP:无效的投资者或者密码 */
	AT_ERROR_CTP_INVALID_LOGIN_IPADDRESS = 49,  /* CTP:不合法的登录IP地址 */
	AT_ERROR_CTP_OVER_CLOSETODAY_POSITION = 50,  /* CTP:平今仓位不足 */
	AT_ERROR_CTP_OVER_CLOSEYESTERDAY_POSITION = 51,  /* CTP:平昨仓位不足 */
	AT_ERROR_CTP_BROKER_NOT_ENOUGH_CONDORDER = 52,  /* CTP:经纪公司没有足够可用的条件单数量 */
	AT_ERROR_CTP_INVESTOR_NOT_ENOUGH_CONDORDER = 53,  /* CTP:投资者没有足够可用的条件单数量 */
	AT_ERROR_CTP_BROKER_NOT_SUPPORT_CONDORDER = 54,  /* CTP:经纪WITHDRAW_公司不支持条件单 */
	AT_ERROR_CTP_RESEND_ORDER_BROKERINVESTOR_NOTMATCH = 55,  /* CTP:重发未知单经济公司/投资者不匹配 */
	AT_ERROR_CTP_SYC_OTP_FAILED = 56,  /* CTP:同步动态令牌失败 */
	AT_ERROR_CTP_OTP_MISMATCH = 57,  /* CTP:动态令牌校验错误 */
	AT_ERROR_CTP_OTPPARAM_NOT_FOUND = 58,  /* CTP:找不到动态令牌配置信息 */
	AT_ERROR_CTP_UNSUPPORTED_OTPTYPE = 59,  /* CTP:不支持的动态令牌类型 */
	AT_ERROR_CTP_SINGLEUSERSESSION_EXCEED_LIMIT = 60,  /* CTP:用户在线会话超出上限 */
	AT_ERROR_CTP_EXCHANGE_UNSUPPORTED_ARBITRAGE = 61,  /* CTP:该交易所不支持套利类型报单 */
	AT_ERROR_CTP_NO_CONDITIONAL_ORDER_RIGHT = 62,  /* CTP:没有条件单交易权限 */
	AT_ERROR_CTP_AUTH_FAILED = 63,  /* CTP:客户端认证失败 */
	AT_ERROR_CTP_NOT_AUTHENT = 64,  /* CTP:客户端未认证 */
	AT_ERROR_CTP_SWAPORDER_UNSUPPORTED = 65,  /* CTP:该合约不支持互换类型报单 */
	AT_ERROR_CTP_OPTIONS_ONLY_SUPPORT_SPEC = 66,  /* CTP:该期权合约只支持投机类型报单 */
	AT_ERROR_CTP_DUPLICATE_EXECORDER_REF = 67,  /* CTP:执行宣告错误，不允许重复执行 */
	AT_ERROR_CTP_RESEND_EXECORDER_BROKERINVESTOR_NOTMATCH = 68,  /* CTP:重发未知执行宣告经纪公司/投资者不匹配 */
	AT_ERROR_CTP_EXECORDER_NOTOPTIONS = 69,  /* CTP:只有期权合约可执行 */
	AT_ERROR_CTP_OPTIONS_NOT_SUPPORT_EXEC = 70,  /* CTP:该期权合约不支持执行 */
	AT_ERROR_CTP_BAD_EXECORDER_ACTION_FIELD = 71,  /* CTP:执行宣告字段有误 */
	AT_ERROR_CTP_DUPLICATE_EXECORDER_ACTION_REF = 72,  /* CTP:执行宣告撤单已报送，不允许重复撤单 */
	AT_ERROR_CTP_EXECORDER_NOT_FOUND = 73,  /* CTP:执行宣告撤单找不到相应执行宣告 */
	AT_ERROR_CTP_OVER_EXECUTE_POSITION = 74,  /* CTP:执行仓位不足 */
	AT_ERROR_CTP_LOGIN_FORBIDDEN = 75,  /* CTP:连续登录失败次数超限，登录被禁止 */
	AT_ERROR_CTP_INVALID_TRANSFER_AGENT = 76,  /* CTP:非法银期代理关系 */
	AT_ERROR_CTP_NO_FOUND_FUNCTION = 77,  /* CTP:无此功能 */
	AT_ERROR_CTP_SEND_EXCHANGEORDER_FAILED = 78,  /* CTP:发送报单失败 */
	AT_ERROR_CTP_SEND_EXCHANGEORDERACTION_FAILED = 79,  /* CTP:发送报单操作失败 */
	AT_ERROR_CTP_PRICETYPE_NOTSUPPORT_BYEXCHANGE = 80,  /* CTP:交易所不支持的价格类型 */
	AT_ERROR_CTP_BAD_EXECUTE_TYPE = 81,  /* CTP:错误的执行类型 */
	AT_ERROR_CTP_BAD_OPTION_INSTR = 82,  /* CTP:无效的组合合约 */
	AT_ERROR_CTP_INSTR_NOTSUPPORT_FORQUOTE = 83,  /* CTP:该合约不支持询价 */
	AT_ERROR_CTP_RESEND_QUOTE_BROKERINVESTOR_NOTMATCH = 84,  /* CTP:重发未知报价经纪公司/投资者不匹配 */
	AT_ERROR_CTP_INSTR_NOTSUPPORT_QUOTE = 85,  /* CTP:该合约不支持报价 */
	AT_ERROR_CTP_QUOTE_NOT_FOUND = 86,  /* CTP:报价撤单找不到相应报价 */
	AT_ERROR_CTP_OPTIONS_NOT_SUPPORT_ABANDON = 87,  /* CTP:该期权合约不支持放弃执行 */
	AT_ERROR_CTP_COMBOPTIONS_SUPPORT_IOC_ONLY = 88,  /* CTP:该组合期权合约只支持IOC */
	AT_ERROR_CTP_OPEN_FILE_FAILED = 89,  /* CTP:打开文件失败 */
	AT_ERROR_CTP_NEED_RETRY = 90,  /* CTP：查询未就绪，请稍后重试 */
	AT_ERROR_CTP_EXCHANGE_RTNERROR = 91,  /* CTP：交易所返回的错误 */
	AT_ERROR_CTP_QUOTE_DERIVEDORDER_ACTIONERROR = 92,  /* CTP:报价衍生单要等待交易所返回才能撤单 */
	AT_ERROR_CTP_INSTRUMENTMAP_NOT_FOUND = 93,  /* CTP:找不到组合合约映射 */
	AT_ERROR_CTP_NO_TRADING_RIGHT_IN_SEPC_DR = 101,  /* CTP:用户在本系统没有报单权限 */
	AT_ERROR_CTP_NO_DR_NO = 102,  /* CTP:系统缺少灾备标示号 */
	AT_ERROR_CTP_SEND_INSTITUTION_CODE_ERROR = 1000,  /* CTP:银期转账：发送机构代码错误 */
	AT_ERROR_CTP_NO_GET_PLATFORM_SN = 1001,  /* CTP:银期转账：取平台流水号错误 */
	AT_ERROR_CTP_ILLEGAL_TRANSFER_BANK = 1002,  /* CTP:银期转账：不合法的转账银行 */
	AT_ERROR_CTP_ALREADY_OPEN_ACCOUNT = 1003,  /* CTP:银期转账：已经开户 */
	AT_ERROR_CTP_NOT_OPEN_ACCOUNT = 1004,  /* CTP:银期转账：未开户 */
	AT_ERROR_CTP_PROCESSING = 1005,  /* CTP:银期转账：处理中 */
	AT_ERROR_CTP_OVERTIME = 1006,  /* CTP:银期转账：交易超时 */
	AT_ERROR_CTP_RECORD_NOT_FOUND = 1007,  /* CTP:银期转账：找不到记录 */
	AT_ERROR_CTP_NO_FOUND_REVERSAL_ORIGINAL_TRANSACTION = 1008,  /* CTP:银期转账：找不到被冲正的原始交易 */
	AT_ERROR_CTP_CONNECT_HOST_FAILED = 1009,  /* CTP:银期转账：连接主机失败 */
	AT_ERROR_CTP_SEND_FAILED = 1010,  /* CTP:银期转账：发送失败 */
	AT_ERROR_CTP_LATE_RESPONSE = 1011,  /* CTP:银期转账：迟到应答 */
	AT_ERROR_CTP_REVERSAL_BANKID_NOT_MATCH = 1012,  /* CTP:银期转账：冲正交易银行代码错误 */
	AT_ERROR_CTP_REVERSAL_BANKACCOUNT_NOT_MATCH = 1013,  /* CTP:银期转账：冲正交易银行账户错误 */
	AT_ERROR_CTP_REVERSAL_BROKERID_NOT_MATCH = 1014,  /* CTP:银期转账：冲正交易经纪公司代码错误 */
	AT_ERROR_CTP_REVERSAL_ACCOUNTID_NOT_MATCH = 1015,  /* CTP:银期转账：冲正交易资金账户错误 */
	AT_ERROR_CTP_REVERSAL_AMOUNT_NOT_MATCH = 1016,  /* CTP:银期转账：冲正交易交易金额错误 */
	AT_ERROR_CTP_DB_OPERATION_FAILED = 1017,  /* CTP:银期转账：数据库操作错误 */
	AT_ERROR_CTP_SEND_ASP_FAILURE = 1018,  /* CTP:银期转账：发送到交易系统失败 */
	AT_ERROR_CTP_NOT_SIGNIN = 1019,  /* CTP:银期转账：没有签到 */
	AT_ERROR_CTP_ALREADY_SIGNIN = 1020,  /* CTP:银期转账：已经签到 */
	AT_ERROR_CTP_AMOUNT_OR_TIMES_OVER = 1021,  /* CTP:银期转账：金额或次数超限 */
	AT_ERROR_CTP_NOT_IN_TRANSFER_TIME = 1022,  /* CTP:银期转账：这一时间段不能转账 */
	AT_ERROR_CTP_BANK_SERVER_ERROR = 1023,  /* 银行主机错 */
	AT_ERROR_CTP_BANK_SERIAL_IS_REPEALED = 1024,  /* CTP:银期转账：银行已经冲正 */
	AT_ERROR_CTP_BANK_SERIAL_NOT_EXIST = 1025,  /* CTP:银期转账：银行流水不存在 */
	AT_ERROR_CTP_NOT_ORGAN_MAP = 1026,  /* CTP:银期转账：机构没有签约 */
	AT_ERROR_CTP_EXIST_TRANSFER = 1027,  /* CTP:银期转账：存在转账，不能销户 */
	AT_ERROR_CTP_BANK_FORBID_REVERSAL = 1028,  /* CTP:银期转账：银行不支持冲正 */
	AT_ERROR_CTP_DUP_BANK_SERIAL = 1029,  /* CTP:银期转账：重复的银行流水 */
	AT_ERROR_CTP_FBT_SYSTEM_BUSY = 1030,  /* CTP:银期转账：转账系统忙，稍后再试 */
	AT_ERROR_CTP_MACKEY_SYNCING = 1031,  /* CTP:银期转账：MAC密钥正在同步 */
	AT_ERROR_CTP_ACCOUNTID_ALREADY_REGISTER = 1032,  /* CTP:银期转账：资金账户已经登记 */
	AT_ERROR_CTP_BANKACCOUNT_ALREADY_REGISTER = 1033,  /* CTP:银期转账：银行账户已经登记 */
	AT_ERROR_CTP_DUP_BANK_SERIAL_REDO_OK = 1034,  /* CTP:银期转账：重复的银行流水,重发成功 */
	AT_ERROR_CTP_CURRENCYID_NOT_SUPPORTED = 1035,  /* CTP:银期转账：该币种代码不支持 */
	AT_ERROR_CTP_INVALID_MAC = 1036,  /* CTP:银期转账：MAC值验证失败 */
	AT_ERROR_CTP_NOT_SUPPORT_SECAGENT_BY_BANK = 1037,  /* CTP:银期转账：不支持银行端发起的二级代理商转账和查询 */
	AT_ERROR_CTP_PINKEY_SYNCING = 1038,  /* CTP:银期转账：PIN密钥正在同步 */
	AT_ERROR_CTP_SECAGENT_QUERY_BY_CCB = 1039,  /* CTP:银期转账：建行发起的二级代理商查询 */
	AT_ERROR_CTP_NO_VALID_BANKOFFER_AVAILABLE = 2000,  /* CTP:该报盘未连接到银行 */
	AT_ERROR_CTP_PASSWORD_MISMATCH = 2001,  /* CTP:资金密码错误 */
	AT_ERROR_CTP_DUPLATION_BANK_SERIAL = 2004,  /* CTP:银行流水号重复 */
	AT_ERROR_CTP_DUPLATION_OFFER_SERIAL = 2005,  /* CTP:报盘流水号重复 */
	AT_ERROR_CTP_SERIAL_NOT_EXSIT = 2006,  /* CTP:被冲正流水不存在(冲正交易) */
	AT_ERROR_CTP_SERIAL_IS_REPEALED = 2007,  /* CTP:原流水已冲正(冲正交易) */
	AT_ERROR_CTP_SERIAL_MISMATCH = 2008,  /* CTP:与原流水信息不符(冲正交易) */
	AT_ERROR_CTP_IdentifiedCardNo_MISMATCH = 2009,  /* CTP:证件号码或类型错误 */
	AT_ERROR_CTP_ACCOUNT_NOT_FUND = 2011,  /* CTP:资金账户不存在 */
	AT_ERROR_CTP_ACCOUNT_NOT_ACTIVE = 2012,  /* CTP:资金账户已经销户 */
	AT_ERROR_CTP_NOT_ALLOW_REPEAL_BYMANUAL = 2013,  /* CTP:该交易不能执行手工冲正 */
	AT_ERROR_CTP_AMOUNT_OUTOFTHEWAY = 2014,  /* CTP:转帐金额错误 */
	AT_ERROR_CTP_EXCHANGERATE_NOT_FOUND = 2015,  /* CTP:找不到汇率 */
	AT_ERROR_CTP_WAITING_OFFER_RSP = 999999,  /* CTP:等待银期报盘处理结果 */
	AT_ERROR_CTP_FBE_NO_GET_PLATFORM_SN = 3001,  /* CTP:银期换汇：取平台流水号错误 */
	AT_ERROR_CTP_FBE_ILLEGAL_TRANSFER_BANK = 3002,  /* CTP:银期换汇：不合法的转账银行 */
	AT_ERROR_CTP_FBE_PROCESSING = 3005,  /* CTP:银期换汇：处理中 */
	AT_ERROR_CTP_FBE_OVERTIME = 3006,  /* CTP:银期换汇：交易超时 */
	AT_ERROR_CTP_FBE_RECORD_NOT_FOUND = 3007,  /* CTP:银期换汇：找不到记录 */
	AT_ERROR_CTP_FBE_CONNECT_HOST_FAILED = 3009,  /* CTP:银期换汇：连接主机失败 */
	AT_ERROR_CTP_FBE_SEND_FAILED = 3010,  /* CTP:银期换汇：发送失败 */
	AT_ERROR_CTP_FBE_LATE_RESPONSE = 3011,  /* CTP:银期换汇：迟到应答 */
	AT_ERROR_CTP_FBE_DB_OPERATION_FAILED = 3017,  /* CTP:银期换汇：数据库操作错误 */
	AT_ERROR_CTP_FBE_NOT_SIGNIN = 3019,  /* CTP:银期换汇：没有签到 */
	AT_ERROR_CTP_FBE_ALREADY_SIGNIN = 3020,  /* CTP:银期换汇：已经签到 */
	AT_ERROR_CTP_FBE_AMOUNT_OR_TIMES_OVER = 3021,  /* CTP:银期换汇：金额或次数超限 */
	AT_ERROR_CTP_FBE_NOT_IN_TRANSFER_TIME = 3022,  /* CTP:银期换汇：这一时间段不能换汇 */
	AT_ERROR_CTP_FBE_BANK_SERVER_ERROR = 3023,  /* CTP:银期换汇：银行主机错 */
	AT_ERROR_CTP_FBE_NOT_ORGAN_MAP = 3026,  /* CTP:银期换汇：机构没有签约 */
	AT_ERROR_CTP_FBE_SYSTEM_BUSY = 3030,  /* CTP:银期换汇：换汇系统忙，稍后再试 */
	AT_ERROR_CTP_FBE_CURRENCYID_NOT_SUPPORTED = 3035,  /* CTP:银期换汇：该币种代码不支持 */
	AT_ERROR_CTP_FBE_WRONG_BANK_ACCOUNT = 3036,  /* CTP:银期换汇：银行帐号不正确 */
	AT_ERROR_CTP_FBE_BANK_ACCOUNT_NO_FUNDS = 3037,  /* CTP:银期换汇：银行帐户余额不足 */
	AT_ERROR_CTP_FBE_DUP_CERT_NO = 3038,  /* CTP:银期换汇：凭证号重复 */
};


/* a50 trade system error codes */
enum AT_ERROR_CODE
{
	AT_ERROR_NONE = 0,
	AT_ERROR_TRADER = 300000,							/* 交易服务返回错误 */
	AT_ERROR_ORDERID_NOT_EXIST = 300001,					/* 指定订单不存在（比如查询） */
	AT_ERROR_DEAL_NOT_EXIST = 300002,					/* 指定订单的成交不存在 */
	AT_ERROR_ORDER_EXISTED = 300003,						/* 订单已存在（比如重复订单） */
	AT_ERROR_ORDER_EXPIRED = 300004,						/* 订单已失效（超时） */
	AT_ERROR_DATABASE_FAILED = 300005,					/* 数据操作失败 */
	AT_ERROR_SYSTEMNO_NOT_EXIST = 300006,				/* 订单系统号不存在 */
	AT_ERROR_ORDER_HAS_BEEN_DEALED = 300007,				/* 订单已成交（比如撤单指令） */
	AT_ERROR_ORDERID_SHOULD_SPECIFIED = 300008,			/* 订单号必须指定 */
	AT_ERROR_WITHDRAW_FAILED = 300009,					/* 撤单失败 */
	AT_ERROR_ENTRUSTNO_NOT_EXIST = 300010,				/* 委托号不存在 */
	AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED = 300011,		/* 订单已被撤单 */
	AT_ERROR_ORDER_IS_FAILED = 300012,					/* 订单失败 */
	AT_ERROR_CONNECT_SERVER_FAILED = 300014,			/* 连接服务失败 */
	AT_ERROR_QUERY_TIMEOUT = 300015,					/* 查询已超时 */
	AT_ERROR_ACCOUNT_NOT_EXIST = 300016,				/* 指定资金帐号不存在 */
	AT_ERROR_SEND_TRADEINFO_FAILED = 300017,			/* 发送交易信息失败 */
    AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED_OR_IS_FAILED = 300018,            /* 订单已被撤单 or 订单失败 */
	AT_ERROR_UNSPECIFIED = 300099,						/* 未指定错误 */
	AT_ERROR_FUNCTION_NOT_EXIST = 300101,        /* 该函数功能不支持 */
	AT_ERROR_WITHDRAW_FROM_EXCODE = 300102,        /* 可能是委托单被退回做撤单处理比如超过每秒发送委托数量或者委托参数不对 */
	AT_ERROR_SEND_TRADE_INFO_FAILED = 300103,        /* 交易服务实例连接服务失败或者发送消息失败 */
};


#define CHECK_IF_DBCONN_NULL(dbconn) \
if (dbconn == NULL) { \
TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, \
"failed to get db conn "); \
return NAUT_AT_E_INIT_DATABASE_CONN_FAILED; \
} \

#define DBCONN_TRAN_START(dbconn) \
if (!dbconn->_conn->begin_transaction()) { \
TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, \
"failed to start transaction"); \
return NAUT_AT_E_INIT_DATABASE_CONN_FAILED; \
} \

#define DBCONN_TRAN_COMMIT(dbconn) \
if (!dbconn->_conn->commit()) { \
TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, \
"failed to commit transaction"); \
return NAUT_AT_E_INIT_DATABASE_CONN_FAILED; \
} \

#define DBCONN_TRAN_ROLLBACK(dbconn) \
if (!dbconn->_conn->rollback()) { \
TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, \
"failed to rollback transaction"); \
return NAUT_AT_E_INIT_DATABASE_CONN_FAILED; \
} \


int get_response_error_code(int ret, const char* server_error = NULL);
std::string get_response_error_msg(int error_code, const char* server_error = NULL);

}

#endif //__NAUT_CTPTRADE_COMMON_H__
