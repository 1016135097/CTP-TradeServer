#pragma once

#include "ThostFtdcTraderApi.h"
#include "InstrumentInfo.h"
#include "AccountID.h"
#include "ThostFtdcTraderApi.h"
#include <qobject.h>
#include <memory>
#include <qstring.h>
#include <map>
#include <queue>

//�ý����˻���Ҫ��������ctp��ȡ�����ա����׺�Լ����Ϣ������������������
//����ģʽ
class BackgroundTrader :public CThostFtdcTraderSpi{
public:
	static std::shared_ptr<BackgroundTrader> getInstance();
	~BackgroundTrader();
	const QString & getTradingDate() const;
	const std::map<QString, std::shared_ptr<InstrumentInfo>> & getInstruments() const;
private:
	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	void OnFrontConnected() override;

	///��¼������Ӧ
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	///�����ѯ��Լ����������Ӧ
	void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	///�����ѯ��Լ��Ӧ
	void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

private:
	BackgroundTrader();
	BackgroundTrader(const BackgroundTrader &) = delete;
	BackgroundTrader & operator=(const BackgroundTrader &) = delete;
	
	void init();
	void initInterestedInstruments(QString &rawString);
	void supplementInstrumentInfo();
	int getRequestID();
	bool readyForNext();
	void errorInstrumentID(char *id);
private:
	//����ȫ�ֱ���
	static std::shared_ptr<BackgroundTrader> bgTrader ;

	std::shared_ptr<AccountID> accountID;
	CThostFtdcTraderApi *api;
	QString tradingDate;

	//������в��Ը���Ȥ�ĺ�Լ
	std::map<QString,std::shared_ptr<InstrumentInfo>> interestedInstruments;

	bool loginFlag = false;
	int requestID = 0;

	//�º�Լ�����Լ�������
	std::queue<QString> newInstruments;
	bool finishBasicQuery = true;
	bool finishCommissionQuery = true;
};