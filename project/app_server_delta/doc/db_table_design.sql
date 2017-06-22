Create Database If Not Exists future_risk Character Set UTF8;

use future_risk;

-- ----------------------------
-- Table structure for future_risk
-- ----------------------------

#DROP TABLE IF EXISTS t_instrument;
#CREATE TABLE t_instrument
#(
#  INSTRUMENTID varchar(31) NOT NULL COMMENT '合约代码1',
#  EXCHANGEID varchar(9) NOT NULL COMMENT '交易所代码2',
#  PRODUCTID varchar(31) COMMENT '产品代码5',
#	PRODUCTCLASS char(1) COMMENT '产品类型6',
#	VOLUMEMULTIPLE int COMMENT '合约数量乘数15',
#	EXPIREDATE char(9) COMMENT '到期日19',
#	OPTIONSTYPE char(1)  COMMENT '期权类型37',
#  STRIKEPRICE double COMMENT '执行价38',
#  PRIMARY KEY(INSTRUMENTID)
#)
#ENGINE=MyISAM 
#AUTO_INCREMENT=1 
#DEFAULT CHARSET=utf8 
#ROW_FORMAT=DYNAMIC
#;

DROP TABLE IF EXISTS t_instrument;
CREATE TABLE t_instrument
(
  instrumentid             CHAR(30) not null,
  exchangeid               CHAR(8) not null,
  instrumentname           CHAR(20) not null,
  exchangeinstid           CHAR(30) not null,
  productid                CHAR(30) not null,
  productclass             CHAR(1) not null,
  openyear                 DECIMAL(4) not null,
  openmonth                DECIMAL(2) not null,
  deliveryyear             DECIMAL(4) not null,
  deliverymonth            DECIMAL(2) not null,
  maxmarketordervolume     DECIMAL(10) not null,
  minmarketordervolume     DECIMAL(10) not null,
  maxlimitordervolume      DECIMAL(10) not null,
  minlimitordervolume      DECIMAL(10) not null,
  volumemultiple           DECIMAL(5) not null,
  pricetick                DECIMAL(12,3) not null,
  createdate               CHAR(8) not null,
  opendate                 CHAR(8) not null,
  expiredate               CHAR(8) not null,
  startdelivdate           CHAR(8),
  enddelivdate             CHAR(8),
  instlifephase            CHAR(1) not null,
  istrading                DECIMAL(1) not null,
  positiontype             CHAR(1) not null,
  positiondatetype         CHAR(1) not null,
  deliverymode             CHAR(1) not null,
  delivsmarginratebymoney  DECIMAL(19,8) not null,
  delivsmarginratebyvolume DECIMAL(19,8) not null,
  delivhmarginratebymoney  DECIMAL(19,8) not null,
  delivhmarginratebyvolume DECIMAL(19,8) not null,
  createdateexpr           varchar(1024),
  opendateexpr             varchar(1024),
  expiredateexpr           varchar(1024),
  startdelivdateexpr       varchar(1024),
  enddelivdateexpr         varchar(1024),
  baseprice                DECIMAL(12,3),
  optionstype              CHAR(1),
  strikeprice              DECIMAL(12,3),
  underlyinginstrid        CHAR(30),
  strikemode               CHAR(1),
  underlyingmultiple       DECIMAL(19,8),
  PRIMARY KEY(instrumentid,exchangeid)
)
ENGINE=MyISAM 
AUTO_INCREMENT=1 
DEFAULT CHARSET=utf8 
ROW_FORMAT=DYNAMIC
;

CREATE TABLE `t_product` (
`productid`  varchar(31) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL DEFAULT '' COMMENT '产品代码' ,
`productname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '产品名称' ,
`exchangeid`  varchar(9) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '交易所代码' ,
`productclass`  char(1) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '产品类型' ,
`volumemultiple`  int(11) NULL DEFAULT NULL COMMENT '合约数量乘数' ,
PRIMARY KEY (`productid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
CHECKSUM=0
ROW_FORMAT=DYNAMIC
DELAY_KEY_WRITE=0
;


create table t_calendar
(
  day CHAR(8) not null,
  dat decimal(1) not null,
  wrk decimal(1) not null,
  tra decimal(1) not null,
  sun decimal(1) not null,
  mon decimal(1) not null,
  tue decimal(1) not null,
  wed decimal(1) not null,
  thu decimal(1) not null,
  fri decimal(1) not null,
  sat decimal(1) not null,
  str decimal(1) not null,
  tal decimal(1) not null,
  spr decimal(1) not null,
  PRIMARY KEY (`day`)
)
ENGINE=MyISAM 
AUTO_INCREMENT=1 
DEFAULT CHARSET=utf8 
ROW_FORMAT=DYNAMIC
;

CREATE TABLE `t_exchmarginrate` (
`brokerid`  char(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`exchangeid`  char(8) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`instrumentid`  char(30) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`begindateexpr`  varchar(1024) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`slongmarginratiobymoney`  decimal(19,8) NOT NULL ,
`slongmarginratiobyvolume`  decimal(19,6) NOT NULL ,
`sshortmarginratiobymoney`  decimal(19,8) NOT NULL ,
`sshortmarginratiobyvolume`  decimal(19,6) NOT NULL ,
`hlongmarginratiobymoney`  decimal(19,8) NOT NULL ,
`hlongmarginratiobyvolume`  decimal(19,6) NOT NULL ,
`hshortmarginratiobymoney`  decimal(19,8) NOT NULL ,
`hshortmarginratiobyvolume`  decimal(19,6) NOT NULL,
  PRIMARY KEY (`brokerid`,`exchangeid`,`instrumentid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
CHECKSUM=0
ROW_FORMAT=DYNAMIC
DELAY_KEY_WRITE=0
;

CREATE TABLE `z_mrgnratetmpl` (
`tmplname`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`productcode`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`contractcode`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`exchangecode`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`phase`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`hedgeflag`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`direction`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`zipbegindate`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`marginexpr`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`marginrate`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`remark`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`status`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`zipbeginid`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`zipenddate`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`zipendid`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`ziplog`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
CHECKSUM=0
ROW_FORMAT=DYNAMIC
DELAY_KEY_WRITE=0
;

CREATE TABLE `t_insmrgnrate` (
`tradingday`  varchar(8) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`contractcode`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`exchangecode`  varchar(8) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`hedgeflag`  char(1) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`direction`  char(1) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`marginrate`  decimal(19,8) NULL DEFAULT NULL ,
`segmrgn`  decimal(19,8) NULL DEFAULT NULL ,
`tmplname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`marginexpr`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL ,
`ismanual`  char(1) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`remark`  varchar(1024) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`tradingday`,`contractcode`,`hedgeflag`,`direction`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
CHECKSUM=0
ROW_FORMAT=DYNAMIC
DELAY_KEY_WRITE=0
;


DROP TABLE IF EXISTS t_dcepair_inscom;
CREATE TABLE t_dcepair_inscom
(
  comkind       varchar(50) not null,
  comname       varchar(50) not null,
  contractcode  varchar(50) not null,
  leftcont      varchar(50),
  rightcont     varchar(50),
  productcode   varchar(50) not null,
  leftprodcode  varchar(50) not null,
  rightprodcode varchar(50) not null,
  productkind   varchar(10),
  exchangecode  varchar(8) not null,
  permargin     decimal(20,4),
  margindir     char(1),
  mainseq       decimal(10),
  groupseq      decimal(10),
  contractseq   decimal(10),
  groupid       decimal(10),
  remark        varchar(1024),
  PRIMARY KEY(contractcode)
)
ENGINE=MyISAM 
AUTO_INCREMENT=1 
DEFAULT CHARSET=utf8 
ROW_FORMAT=DYNAMIC
;

CREATE TABLE `t_cash_deposit` (
`id`  int(11) NOT NULL AUTO_INCREMENT COMMENT '自增列ID' ,
`calc_time`  datetime NOT NULL DEFAULT '1900-01-01 00:00:00' COMMENT '计算时间',
`customer`  varchar(15) NOT NULL ,
`cffex`  decimal(12,4) NOT NULL ,
`czce`  decimal(12,4) NOT NULL ,
`dce`  decimal(12,4) NOT NULL ,
`shfe`  decimal(12,4) NOT NULL ,
`total`  decimal(12,4) NOT NULL ,
  PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
CHECKSUM=0
ROW_FORMAT=DYNAMIC
DELAY_KEY_WRITE=0
;

CREATE TABLE `t_cash_deposit_yesterday` (
`id`  int(11) NOT NULL AUTO_INCREMENT COMMENT '自增列ID' ,
`calc_time`  datetime NOT NULL DEFAULT '1900-01-01 00:00:00' COMMENT '计算时间',
`customer`  varchar(15) NOT NULL ,
`cffex`  decimal(12,4) NOT NULL ,
`czce`  decimal(12,4) NOT NULL ,
`dce`  decimal(12,4) NOT NULL ,
`shfe`  decimal(12,4) NOT NULL ,
`total`  decimal(12,4) NOT NULL ,
  PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
CHECKSUM=0
ROW_FORMAT=DYNAMIC
DELAY_KEY_WRITE=0
;
