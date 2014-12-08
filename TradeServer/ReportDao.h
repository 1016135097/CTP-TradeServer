#pragma once

#include "ThostFtdcUserApiStruct.h"
#include <qstring.h>

class ReportDao{
public:
	//���ݳɽ��ر����»ر����
	void updateReportTable(CThostFtdcTradeField *report,QString &strategyId);
private:
	//�����ƽ�ֻر����ڸ�����ر����֮�����ɾ�̬Ȩ��
	void generateStaticProfit(CThostFtdcTradeField *report, QString &strategyId);
	//���㾲̬Ȩ��
	double calculateStaticProfit_today(QString instrumentID, double openPrice, double closePrice, 
		int volume,char closeDirection);
	double calculateStaticProfit(QString instrumentID, double openPrice, double closePrice, 
		int volume,char closeDirection);
};