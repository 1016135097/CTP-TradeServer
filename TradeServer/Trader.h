#pragma once

#include "ThostFtdcTraderApi.h"
#include "AccountID.h"
#include "AccountFund.h"
#include <memory>
#include <map>
#include <qtimer.h>
#include <qstring.h>
#include "Order.h"
#include "OrderDao.h"
#include "CommandQueue.h"
#include "AccountPositionDao.h"
#include "FundDao.h"
#include "StrategyPositionDao.h"
#include "ReportDao.h"
#include <qbytearray.h>
#include <qobject.h>

class Trader :public QObject,public CThostFtdcTraderSpi{
	Q_OBJECT
public:
	Trader(std::shared_ptr<AccountID> id);

	bool isTradable(){ return tradable; };

	//ͨ������ָ�����ɱ�����ִ��
	void generateAndExecuteOrder(QByteArray *instruction, int volume, double price);

	//����
	void cancleOrder(Order *order);
private:
	/****************************Api���׺���****************************************/
	//��¼
	void login();
	//ȷ�Ͻ���
	void comfirmSettlement();

	//��ѯ
	void queryFund();										//��ѯ�ʽ�
	void queryPosition(QString instrument = "");			//��ѯ�ֲ���һ֧��Լ���������
	/****************************Api���׺���****************************************/

	/****************************Spi�ص�����****************************************/
	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	void OnFrontConnected() override;

	///��¼������Ӧ
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	//Ͷ���߽�����ȷ����Ӧ
	void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	///����¼��������Ӧ(������ͨ��)
	void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	///��������������Ӧ(������ͨ��)
	void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	///����֪ͨ
	void OnRtnOrder(CThostFtdcOrderField *pOrder) override;

	///�ɽ�֪ͨ
	void OnRtnTrade(CThostFtdcTradeField *pTrade) override;

	///�����ѯͶ���ֲ߳���Ӧ
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	///�����ѯ�ʽ��˻���Ӧ
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	/****************************Spi�ص�����****************************************/

	/****************************��������****************************************/
	//��ʼ����Ծ����(����)
	void initOrders();

	//��ʼ������������(����)
	void initOrderFilter();

	//��ʼ����󱨵�����(����)
	void initOrderRef();

	//��ʼ���ɽ��ر�������(����)
	void initReportFilter();

	//�뽻�����������ӣ�����׼�����׵�״̬(�Ǳ���)
	void readyToTrade();

	//������������
	void increaseRef(){ maxOrderRef++; }

	//���Ϻ��ڻ���ƽ�ֱ������Խ���ƽ�����
	void splitSHFEOrder(std::shared_ptr<Order> &order, QString limit_flag);

	//���������������ɽ�
	void executeOrder(std::shared_ptr<Order> order, QString limit_flag);

	//���ݽ���ָ����㽨�ֳɱ�
	double calculateOpenCost(const QString &instructionId);

	//����ƽ��ָ�����ƽ���ջ��ʽ�
	double calculateCloseRegain(const QString &instructionId);

	//�������׽���ʱ������map
	void cleanOrderMap(const QString &instructionId);

	//�ӳ����������µı���
	void generateOrderFromCanceledOrder(const std::shared_ptr<Order> &order);

	//����ĳ�����Ļر�
	void calculateOrder(std::shared_ptr<Order> order);
	/****************************��������****************************************/
private slots:
	//��ѯ�����ʽ����
	void updateFund();
private:
	//�˻���Ϣ
	std::shared_ptr<AccountID> id;
	//�ʽ����
	std::shared_ptr<AccountFund> fund;

	//���ص���󱨵�����
	int maxOrderRef;

	//δ����ı�������(ÿ��������orderRefΨһȷ��)
	std::map<int, std::shared_ptr<Order>> orders;
	//����DAO
	OrderDao orderDao;
	//�������ù�����
	std::set<int> orderFilter;
	//�û�-���Գֲ�DAO
	StrategyPositionDao strategyPositionDao;
	//�ɽ��ر�������
	std::set<QString> reportFilter;
	//�ɽ��ر�DAO
	ReportDao reportDao;

	//����api
	CThostFtdcTraderApi *api;
	//������
	int requestID = 0;
	//api�������
	CommandQueue commandQueue;

	//�Ƿ���Խ��н���
	bool tradable = false;

	//�û��ֲ�DAO
	AccountPositionDao accountPositionDao;
	//�û��ʽ�DAO
	FundDao fundDao;
	//��ѯ�ʽ�Ķ�ʱ��
	QTimer *queryFundTimer;

	//�����㱨�������ü���
	std::set<int> calculateOrders;
};