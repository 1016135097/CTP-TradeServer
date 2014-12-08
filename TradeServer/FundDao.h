#pragma once

#include <qstring.h>
#include "ThostFtdcUserApiStruct.h"

class FundDao{
public:
	FundDao() = default;
	//��¼ÿ���ʽ�仯
	void logFund(CThostFtdcTradingAccountField *fund);
	//��¼���µ��ʽ����
	void updateFund(CThostFtdcTradingAccountField *fund);
	//���²��ԵĿ����ʽ�
	void updateStrategyFund(QString investorId, QString strategyId, double money);
};