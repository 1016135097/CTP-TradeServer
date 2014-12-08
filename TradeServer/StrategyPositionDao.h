#pragma once

#include "ThostFtdcUserApiStruct.h"
#include <qstring.h>

class StrategyPositionDao{
public:
	//���ݳɽ��ر����²��Գֲ����
	void updatePosition(CThostFtdcTradeField *pTrade, QString strategyId);

	//�����˻����Թ�ϵ��ͬ�����Գֱֲ�
	//1.�˻������µĲ���֮������¼
	//2.�˻�����Խ����ϵ��ɾ����¼
	static void synStrategyPosition();

	//�������µĽ���������ʱ��ˢ�²��Գֱֲ��е�ʱ���¼���ѽ��ճֲ�����
	static void refreshDaily();

	//�����˺š����Ժͺ�Լ���ظú�Լ�Ľ��ճֲ�����
	int getTodayPosition(const QString &investorId, const QString &strategyId, const QString &instrumentId, char direction);
	
private:
	bool isSHFE(QString instrumentID);
};