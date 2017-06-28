drop procedure if exists p_insmrgnrate_update;
CREATE PROCEDURE `p_insmrgnrate_update`()
BEGIN
DECLARE count_num INT DEFAULT 0;
SET SQL_SAFE_UPDATES = 0;

update t_insmrgnrate set tradingday=date_add(CURDATE(), interval 1 day)+0 where tradingday=
(SELECT a.tradingday FROM (SELECT tradingday from t_insmrgnrate GROUP BY tradingday ORDER BY tradingday desc LIMIT 0,1) a);

update t_insmrgnrate set tradingday=CURDATE()+0 where tradingday=
(SELECT a.tradingday FROM (SELECT tradingday from t_insmrgnrate GROUP BY tradingday ORDER BY tradingday desc LIMIT 1,1) a);

update t_insmrgnrate set tradingday=date_sub(CURDATE(), interval 1 day)+0 where tradingday=
(SELECT a.tradingday FROM (SELECT tradingday from t_insmrgnrate GROUP BY tradingday ORDER BY tradingday desc LIMIT 2,1) a);

END;

DROP EVENT IF EXISTS event_insmrgnrate_update;
CREATE EVENT `event_insmrgnrate_update`
ON SCHEDULE EVERY 1 DAY STARTS '2017-06-08 08:00:00'
ON COMPLETION PRESERVE
ENABLE
DO
call  p_insmrgnrate_update;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

drop procedure if exists p_backup_cash_deposit;
CREATE PROCEDURE `p_backup_cash_deposit`()
BEGIN
DECLARE count_num INT DEFAULT 0;
SET SQL_SAFE_UPDATES = 0;

DELETE from t_cash_deposit_yesterday;
SELECT count(*) into count_num FROM t_cash_deposit;
INSERT INTO t_cash_deposit_yesterday (
  `id`,
  `calc_time`,
  `customer`,
  `cffex`,
  `czce`,
  `dce`,
  `shfe`,
  `total`
) SELECT 
  `id`,
  `calc_time`,
  `customer`,
  `cffex`,
  `czce`,
  `dce`,
  `shfe`,
  `total`
 FROM t_cash_deposit;
IF ROW_COUNT() = count_num THEN
    TRUNCATE t_cash_deposit;
END IF;

END;

DROP EVENT IF EXISTS event_backup_cash_deposit;
CREATE EVENT `event_backup_cash_deposit`
ON SCHEDULE EVERY 1 DAY STARTS '2017-06-08 08:00:00'
ON COMPLETION PRESERVE
ENABLE
DO
call  p_backup_cash_deposit;