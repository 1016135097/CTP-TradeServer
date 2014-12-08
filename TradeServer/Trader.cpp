#include "Trader.h"
#include "LoginCommand.h"
#include "ComfirmSettlementCommand.h"
#include "InsertOrderCommand.h"
#include "WithdrawOrderCommand.h"
#include "QueryFundCommand.h"
#include "QueryPositionCommand.h"
#include "BackgroundTrader.h"
#include "GVAR.h"
#include <qsqlquery.h>
#include <qdir.h>
#include <qvariant.h>
#include <qcoreapplication.h>
#include <set>
#include <vector>
#include <qdebug.h>
//#include <qsqlerror.h>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

using std::shared_ptr;
using std::make_shared;
using std::set;
using std::make_pair;
using std::vector;

Trader::Trader(shared_ptr<AccountID> id){
	this->id = id;
	qDebug() << "���ڳ�ʼ���˻�" << id->getInvestorID() << "��Ϣ";
	initOrders();
	initOrderRef();
	initOrderFilter();
	initReportFilter();
	readyToTrade();
	//ÿ5���Ӳ�ѯһ���ʽ�
	queryFundTimer = new QTimer(this);
	queryFundTimer->setSingleShot(false);
	connect(queryFundTimer, SIGNAL(timeout()), this, SLOT(updateFund()));
	queryFundTimer->start(5 * 60 * 1000);
}

//ͨ������ָ�����ɱ�����ִ��
void Trader::generateAndExecuteOrder(QByteArray *instruction, int volume, double price){
	auto order = make_shared<Order>();
	//�����ֶ�
	QString instructionId = instruction->mid(0, 15).trimmed();
	QString investorId = instruction->mid(15, 16).trimmed();
	QString strategyId = instruction->mid(31, 10).trimmed();
	QString instrumentId = instruction->mid(41, 6).trimmed();
	QString direction = instruction->mid(47, 1);
	QString open_close_flag = instruction->mid(48, 1);
	QString limit_flag = instruction->mid(49, 1);
	order->setInvestorId(investorId);
	order->setInstructionId(instructionId);
	order->setStrategyId(strategyId);
	order->setInstrumentId(instrumentId);
	order->setOriginalVolume(volume);
	order->setTradedVolume(0);
	order->setRestVolume(volume);
	order->setPrice(price);
	order->setDirection(direction.at(0).toLatin1());
	order->setOpenCloseFlag(open_close_flag.at(0).toLatin1());
	order->setSequenceId(-1);
	order->setOrderStatus('a');
	//if (open_close_flag == "1"){
	//	//���յ�ƽ���ֶ�ʱ�Ժ�Լ�����жϣ�������Ϻ��ڻ��ĺ�Լ���Զ�ƽ��
	//	auto info = BackgroundTrader::getInstance()->getInstruments();
	//	if (info[instrumentId]->getExchangeId() == "SHFE"){
	//		delete instruction;
	//		qDebug() << "����ƽ��";
	//		splitSHFEOrder(order, limit_flag);
	//		return;
	//	}
	//}
	//ִ��ָ��
	executeOrder(order, limit_flag);
	delete instruction;
}

//����
void Trader::cancleOrder(Order *order){
	//��ý���������
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	auto &info = instruments[order->getInstrumentId()];
	const QString &exchangeID = info->getExchangeId();
	//���ó�����Ϣ
	CThostFtdcInputOrderActionField *orderField = new CThostFtdcInputOrderActionField();
	strcpy(orderField->BrokerID, id->getBrokerID().toStdString().c_str());
	strcpy(orderField->InvestorID, id->getInvestorID().toStdString().c_str());
	//strcpy(orderField->ExchangeID, exchangeID.toStdString().c_str());
	strcpy(orderField->ExchangeID, "SHFE");	//���Ի�����ȫ����������
	strcpy(orderField->OrderSysID, order->getSystemId().toStdString().c_str());
	itoa(order->getOrderRef(), orderField->OrderRef, 10);
	orderField->ActionFlag = THOST_FTDC_AF_Delete;	//ɾ������ '0'
	shared_ptr<ApiCommand> command = make_shared<WithdrawOrderCommand>(api, orderField, requestID);
	commandQueue.addCommand(command);
}

/****************************Api���׺���****************************************/
//��¼
void Trader::login(){
	CThostFtdcReqUserLoginField *loginField = new CThostFtdcReqUserLoginField();
	strcpy(loginField->BrokerID, id->getBrokerID().toStdString().c_str());
	strcpy(loginField->UserID, id->getInvestorID().toStdString().c_str());
	strcpy(loginField->Password, id->getPassword().toStdString().c_str());
	//��ָ��ŵ�����β��,�������ָ���ִ�з�������
	shared_ptr<ApiCommand> command = make_shared<LoginCommand>(api, loginField, requestID);
	commandQueue.addCommand(command);
}

//ȷ�ϼ���
void Trader::comfirmSettlement(){
	CThostFtdcSettlementInfoConfirmField *comfirmField = new CThostFtdcSettlementInfoConfirmField();
	strcpy(comfirmField->BrokerID, id->getBrokerID().toStdString().c_str());
	strcpy(comfirmField->InvestorID, id->getInvestorID().toStdString().c_str());
	shared_ptr<ApiCommand> command = make_shared<ComfirmSettlementCommand>(api, comfirmField, requestID);
	commandQueue.addCommand(command);
}

//��ѯ�ʽ�
void Trader::queryFund() {
	CThostFtdcQryTradingAccountField *accountField = new CThostFtdcQryTradingAccountField();
	strcpy(accountField->BrokerID, id->getBrokerID().toStdString().c_str());
	strcpy(accountField->InvestorID, id->getInvestorID().toStdString().c_str());
	shared_ptr<ApiCommand> command = make_shared<QueryFundCommand>(api, accountField, requestID);
	commandQueue.addCommand(command);
}

//��ѯ�ͻ�����ֲ����
void Trader::queryPosition(QString instrument) {
	CThostFtdcQryInvestorPositionField *accountField = new CThostFtdcQryInvestorPositionField();
	strcpy(accountField->BrokerID, id->getBrokerID().toStdString().c_str());
	strcpy(accountField->InvestorID, id->getInvestorID().toStdString().c_str());
	strcpy(accountField->InstrumentID, instrument.toStdString().c_str());
	shared_ptr<ApiCommand> command = make_shared<QueryPositionCommand>(api, accountField, requestID);
	commandQueue.addCommand(command);
}
/****************************Api���׺���****************************************/

/****************************Spi�ص�����****************************************/
///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
void Trader::OnFrontConnected(){
	login();
}

///��¼������Ӧ
void Trader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		comfirmSettlement();	//ȷ�Ͻ�����
	}
	else
	{
		qDebug() << pRspUserLogin->UserID << "��¼ʧ��, ������Ϣ:" << pRspInfo->ErrorID << QString::fromLocal8Bit(pRspInfo->ErrorMsg);
	}
}

//Ͷ���߽�����ȷ����Ӧ
void Trader::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		tradable = true;
		qDebug() << "�˻�:" << pSettlementInfoConfirm->InvestorID << "�Ѿ����Խ���";
	}
	else
	{
		qDebug() << "�˻�:" << pSettlementInfoConfirm->InvestorID << "ȷ�Ͻ���ʧ��";
	}
}

///����¼��������Ӧ(������ͨ��)
void Trader::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	qDebug() << "����¼��������Ӧ(������ͨ��)";
	qDebug() << "�������:" << pRspInfo->ErrorID << "������Ϣ:" << QString::fromLocal8Bit(pRspInfo->ErrorMsg);
}

///��������������Ӧ(������ͨ��)
void Trader::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	qDebug() << "��������������Ӧ(������ͨ��)";
	qDebug() << "�������:" << pRspInfo->ErrorID << "������Ϣ:" << QString::fromLocal8Bit(pRspInfo->ErrorMsg);
}

///����֪ͨ
void Trader::OnRtnOrder(CThostFtdcOrderField *pOrder){
	qDebug() << "�����ر�";
	qDebug()
		<< "���������:" << pOrder->ExchangeID
		<< "��Լ����:" << pOrder->InstrumentID
		<< "��������:" << pOrder->OrderRef
		<< "��������:" << pOrder->Direction
		<< "��Ͽ�ƽ��־:" << pOrder->CombOffsetFlag
		<< "�۸�:" << pOrder->LimitPrice
		<< "����:" << pOrder->VolumeTotalOriginal
		<< "��ɽ�����:" << pOrder->VolumeTraded
		<< "ʣ������:" << pOrder->VolumeTotal
		<< "������ţ��жϱ����Ƿ���Ч��:" << pOrder->OrderSysID
		<< "����״̬:" << pOrder->OrderStatus
		<< "��������:" << pOrder->InsertDate
		<< "���:" << pOrder->SequenceNo;
	int orderRef = atoi(pOrder->OrderRef);
	if (orderFilter.find(orderRef) == orderFilter.end()){	//��������
		auto &order = orders[orderRef];
		order->update(pOrder);
		if (order->getUpdateFlag()){		//�жϱ����Ƿ�����˸��£���Ϊupdate����˳���Ź���
			switch (order->getOrderStatus()){
				//����������ɡ���������Ҫ׼��������ã��ȳɽ��ر���ͽ��н���
			case 'f':
			case 'w':
				calculateOrders.insert(order->getOrderRef());
				orderDao.updateOrderTable(order);
				break;
				//����
			case 'c':
				generateOrderFromCanceledOrder(order);
				orderDao.updateOrderTable(order);
				break;
			default:
				break;
			}
		}
		order->recoverUpdateFlag();
	}
}

///�ɽ��ر�
void Trader::OnRtnTrade(CThostFtdcTradeField *pTrade){
	qDebug() << "�ɽ��ر�";
	QString tradedID(pTrade->TradeID);
	//����
	if (reportFilter.find(tradedID) == reportFilter.end()){
		auto order = orders[atoi(pTrade->OrderRef)];
		QString strategyId = order->getStrategyId();
		//���ݿ���ƽ���벻ͬ�ĳɽ��ر�����
		reportDao.updateReportTable(pTrade, strategyId);
		//���²��Գֲ����
		strategyPositionDao.updatePosition(pTrade, strategyId);
		//���¹�����
		reportFilter.insert(tradedID);
		//�����û��ֲ����
		queryPosition();
		//�����ʽ�
		queryFund();
		//�鿴�ûر��Ƿ���ĳ�����������ر�(��Ϊ�ɽ��ر����ڱ����ر�֮�����Ҫ������ñ���ȳɽ��ر�)
		if (calculateOrders.find(atoi(pTrade->OrderRef)) != calculateOrders.end()){
			auto order = orders[atoi(pTrade->OrderRef)];
			calculateOrder(order);
		}
	}
}

///�����ѯͶ���ֲ߳���Ӧ
void Trader::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	qDebug() << "�����ѯͶ����" << id->getInvestorID() << "�ֲ���Ӧ";
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		accountPositionDao.updatePosition(pInvestorPosition);
	}
}

///�����ѯ�ʽ��˻���Ӧ
void Trader::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	qDebug() << "�����ѯ�ʽ��˻�" << id->getInvestorID() << "��Ӧ";
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		fundDao.logFund(pTradingAccount);
		fundDao.updateFund(pTradingAccount);
	}
}
/****************************Spi�ص�����****************************************/

/****************************��������******************************************/
//��ѯ�����ʽ����
void Trader::updateFund(){
	queryFund();
}

//��ʼ����Ծ����(����)
void Trader::initOrders(){
	qDebug() << "���ڳ�ʼ���˻���Ծ������Ϣ...";
	const QString &investorId = id->getInvestorID();
	const QString &date = BackgroundTrader::getInstance()->getTradingDate();
	shared_ptr< set<shared_ptr<Order>> > results = orderDao.getActivedOrders(investorId, date);
	for (auto order : (*results)){
		int orderRef = order->getOrderRef();
		orders.insert(make_pair(orderRef, order));
	}
}

//��ʼ������������(����)
void Trader::initOrderFilter(){
	qDebug() << "���������˻�����������...";
	const QString &investorId = id->getInvestorID();
	const QString &date = BackgroundTrader::getInstance()->getTradingDate();
	orderDao.initOrderFilter(investorId, date, orderFilter);
}

//��ʼ����󱨵�����(����)
void Trader::initOrderRef(){
	qDebug() << "���ڶ�ȡ�˻���󱨵�������Ϣ...";
	const QString &investorId = id->getInvestorID();
	const QString &date = BackgroundTrader::getInstance()->getTradingDate();
	maxOrderRef = orderDao.getMaximumOrderRef(investorId, date);
	++maxOrderRef;		//�ܱ����ݿ��еĶ�һ
}

//��ʼ���ɽ��ر�������(����)
void Trader::initReportFilter(){
	qDebug() << "���������˻��ɽ��ر�������...";
	const QString &investorId = id->getInvestorID();
	const QString &date = BackgroundTrader::getInstance()->getTradingDate();
	//��ѯƽ�ֳɽ��ر�
	QSqlQuery queryClose(DATABASE);
	queryClose.prepare(" select trade_id from close_traded_report where investor_id=:id and trade_date=:date ");
	queryClose.bindValue(":id", investorId);
	queryClose.bindValue(":date", date);
	queryClose.exec();
	while (queryClose.next()){
		QString &trade_id = queryClose.value(0).toString();
		reportFilter.insert(trade_id);
	}
	//��ѯ���ֳɽ��ر�
	QSqlQuery queryOpen(DATABASE);
	queryOpen.prepare(" select * from open_traded_report where investor_id=:id and trade_date=:date ");
	queryOpen.bindValue(":id", investorId);
	queryOpen.bindValue(":date", date);
	queryOpen.exec();
	while (queryOpen.next()){
		QString &trade_id = queryOpen.value("trade_id").toString();
		reportFilter.insert(trade_id);
	}
}

//�뽻�����������ӣ�����׼�����׵�״̬(�Ǳ���)
void Trader::readyToTrade(){
	qDebug() << "�˻�������½...";
	//��ctp��ͨѶ�ļ����ڲ�ͬ�ĸ����ͻ����ļ�����
	QString dirName = "user/" + id->getInvestorID() + "/con_Files";
	QDir conFileDir(dirName);
	if (!conFileDir.exists()){
		conFileDir.mkpath(".");
	}
	api = CThostFtdcTraderApi::CreateFtdcTraderApi((dirName + "/").toStdString().c_str());
	api->RegisterSpi(this);
	//���Ĺ�������˽����
	api->SubscribePublicTopic(THOST_TERT_RESTART);
	api->SubscribePrivateTopic(THOST_TERT_RESTART);
	//ע��ǰ�û�
	char *frontAddress = new char[100];
	strcpy(frontAddress, id->getFrontAddress().toStdString().c_str());
	api->RegisterFront(frontAddress);
	//�����������
	commandQueue.start();
	api->Init();
}

//���Ϻ��ڻ���ƽ�ֱ������Խ���ƽ�����
void Trader::splitSHFEOrder(std::shared_ptr<Order> &order, QString limit_flag){
	const QString &investorId = order->getInvestorId();
	const QString &strategyId = order->getStrategyId();
	const QString &instrumentId = order->getInstructionId();
	char direction = order->getDirection();
	int todayPosition = strategyPositionDao.getTodayPosition(investorId, strategyId, instrumentId, direction);
	//�����Ҫƽ�ֵ�����С�ڽ��ճֲ���������ֱ���޸�ԭ�б���
	if (order->getOriginalVolume() <= todayPosition){
		order->setOpenCloseFlag('3');
		executeOrder(order, limit_flag);
	}
	//����ѽ���ȫ��ƽ����ƽ�ɲ�
	else{
		int oldPosition = order->getOriginalVolume() - todayPosition;
		//ƽ����
		order->setOriginalVolume(todayPosition);
		order->setOpenCloseFlag('3');
		executeOrder(order, limit_flag);
		//ƽ�ɲ�
		auto oldOrder = make_shared<Order>();
		oldOrder->setInstructionId(order->getInstructionId());
		oldOrder->setInvestorId(order->getInvestorId());
		oldOrder->setStrategyId(order->getStrategyId());
		oldOrder->setInstrumentId(order->getInstrumentId());
		oldOrder->setOriginalVolume(oldPosition);
		oldOrder->setPrice(order->getPrice());
		oldOrder->setDirection(order->getDirection());
		oldOrder->setOpenCloseFlag('4');
		executeOrder(oldOrder, limit_flag);
	}
}

//�������������������ź�
void Trader::executeOrder(std::shared_ptr<Order> order, QString limit_flag){
	CThostFtdcInputOrderField *orderField = new CThostFtdcInputOrderField();
	strcpy(orderField->BrokerID, id->getBrokerID().toStdString().c_str());
	strcpy(orderField->InvestorID, id->getInvestorID().toStdString().c_str());
	strcpy(orderField->InstrumentID, order->getInstrumentId().toStdString().c_str());
	itoa(maxOrderRef, orderField->OrderRef, 10);
	order->setOrderRef(maxOrderRef);
	orders.insert(make_pair(maxOrderRef, order));			//�ѱ������뵽map��
	orderDao.updateOrderTable(order);						//�ѱ����������ݿ���
	increaseRef();	//����orderRef
	if (limit_flag == "1"){
		orderField->OrderPriceType = THOST_FTDC_OPT_AnyPrice;		//�м�
		orderField->LimitPrice = 0;									//�۸�
	}
	else{
		orderField->OrderPriceType = THOST_FTDC_OPT_LimitPrice;		//�޼�
		orderField->LimitPrice = order->getPrice();
	}
	if (order->getDirection() == 'b'){
		orderField->Direction = THOST_FTDC_D_Buy;					//�� 
	}
	else{
		orderField->Direction = THOST_FTDC_D_Sell;					//��
	}
	if (order->getOpenCloseFlag() == '0'){
		orderField->CombOffsetFlag[0] = THOST_FTDC_OF_Open;				//����
	}
	else{
		orderField->CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;		//���Ի�������ƽ��
	}
	//if (order->getOpenCloseFlag() == '1'){
	//	orderField->CombOffsetFlag[0] = THOST_FTDC_OF_Close;				//ƽ��
	//}
	//if (order->getOpenCloseFlag() == '3'){
	//	orderField->CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;		//ƽ��
	//}
	//if (order->getOpenCloseFlag() == '4'){
	//	orderField->CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;	//ƽ��
	//}
	orderField->VolumeTotalOriginal = order->getOriginalVolume();		//����
	//�����ǹ̶����ֶ�
	orderField->CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;		//Ͷ�� 
	orderField->TimeCondition = THOST_FTDC_TC_GFD;				//������Ч '3'
	orderField->VolumeCondition = THOST_FTDC_VC_AV;				//�κ����� '1'
	orderField->MinVolume = 1;
	orderField->ContingentCondition = THOST_FTDC_CC_Immediately;	//��������'1'
	orderField->ForceCloseReason = THOST_FTDC_FCC_NotForceClose;	//��ǿƽ '0'
	orderField->IsAutoSuspend = 0;
	orderField->UserForceClose = 0;
	shared_ptr<ApiCommand> command = make_shared<InsertOrderCommand>(api, orderField, requestID);
	commandQueue.addCommand(command);
}

//���ݽ���ָ����㽨�ֳɱ�
double Trader::calculateOpenCost(const QString &instructionId){
	auto &tradingDate = BackgroundTrader::getInstance()->getTradingDate();
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	//��ú�Լ
	QString instrumentId;
	QSqlQuery getInstrumentId;
	getInstrumentId.prepare("select instrument_id from orders where instruction_id=:instruction_id and order_date=:order_date limit 1");
	getInstrumentId.bindValue(":instruction_id", instructionId);
	getInstrumentId.bindValue(":order_date", tradingDate);
	getInstrumentId.exec();
	while (getInstrumentId.next()){
		instrumentId = getInstrumentId.value(0).toString();
	}
	auto instrument = instruments[instrumentId];
	//���system_id
	QSqlQuery getSysId;
	getSysId.prepare("select system_id from orders where instruction_id=:instruction_id and order_date=:order_date ");
	getSysId.bindValue(":instruction_id", instructionId);
	getSysId.bindValue(":order_date", tradingDate);
	getSysId.exec();
	vector<QString> sysIds;
	while (getSysId.next()){
		sysIds.push_back(getSysId.value(0).toString());
	}
	if (sysIds.empty()){
		//�������Ϊ�գ�˵����ָ��ִ��ʧ�ܣ�û��ִ�н��֣���˳ɱ�Ϊ0
		return 0;
	}
	//�ӳɽ��ر����л��ÿ�ο��ֵ��������۸�Ȼ�����ɱ�
	double cost = 0;
	double marginRate = instrument->getMarginRate();
	double openCommission = instrument->getOpenCommission();
	int multiplier = instrument->getMultiplier();
	for (auto &sysid : sysIds){
		QSqlQuery getVolumeAndPrice;
		getVolumeAndPrice.prepare("select volume,open_price from open_traded_report where system_id=:system_id and "
			" trade_date=:trade_date ");
		getVolumeAndPrice.bindValue(":system_id", sysid);
		getVolumeAndPrice.bindValue(":trade_date", tradingDate);
		getVolumeAndPrice.exec();
		while (getVolumeAndPrice.next()){
			int volume = getVolumeAndPrice.value("volume").toInt();
			double openPrice = getVolumeAndPrice.value("open_price").toDouble();
			if (openCommission < 1){
				cost += openPrice*marginRate*multiplier*volume + openPrice*multiplier*volume*openCommission;
			}
			else{
				cost += openPrice*marginRate*multiplier*volume + volume*openCommission;
			}
		}
	}
	return cost;
}

//����ƽ��ָ�����ƽ���ջ��ʽ�
double Trader::calculateCloseRegain(const QString &instructionId){
	auto &tradingDate = BackgroundTrader::getInstance()->getTradingDate();
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	//��ú�Լ
	QString instrumentId;
	QSqlQuery getInstrumentId;
	getInstrumentId.prepare("select instrument_id from orders where instruction_id=:instruction_id and order_date=:order_date limit 1");
	getInstrumentId.bindValue(":instruction_id", instructionId);
	getInstrumentId.bindValue(":order_date", tradingDate);
	getInstrumentId.exec();
	while (getInstrumentId.next()){
		instrumentId = getInstrumentId.value(0).toString();
	}
	auto instrument = instruments[instrumentId];
	//���system_id
	QSqlQuery getSysId;
	getSysId.prepare("select system_id from orders where instruction_id=:instruction_id and order_date=:order_date ");
	getSysId.bindValue(":instruction_id", instructionId);
	getSysId.bindValue(":order_date", tradingDate);
	getSysId.exec();
	vector<QString> sysIds;
	while (getSysId.next()){
		sysIds.push_back(getSysId.value(0).toString());
	}
	if (sysIds.empty()){
		//�������Ϊ�գ�˵����ָ��ִ��ʧ�ܣ�û��ִ��ƽ�֣�����ջ�Ϊ0
		return 0;
	}
	//�ӳɽ��ر����л��ÿ��ƽ�ֵ��������۸�Ȼ���ջ�
	double regain = 0;
	double marginRate = instrument->getMarginRate();
	double openCommission = instrument->getOpenCommission();
	int multiplier = instrument->getMultiplier();
	for (auto &sysid : sysIds){
		QSqlQuery getVolumeAndPrice;
		getVolumeAndPrice.prepare("select volume,close_price,today_flag from close_traded_report where system_id=:system_id and "
			" trade_date=:trade_date ");
		getVolumeAndPrice.bindValue(":system_id", sysid);
		getVolumeAndPrice.bindValue(":trade_date", tradingDate);
		getVolumeAndPrice.exec();
		while (getVolumeAndPrice.next()){
			int volume = getVolumeAndPrice.value("volume").toInt();
			double closePrice = getVolumeAndPrice.value("close_price").toDouble();
			char todayFlag = getVolumeAndPrice.value("today_flag").toString().at(0).toLatin1();
			//�������ƽ����
			if (todayFlag == 'y'){
				if (openCommission < 1){
					regain += closePrice*marginRate*multiplier*volume - closePrice*multiplier*volume*openCommission;
				}
				else{
					regain += closePrice*marginRate*multiplier*volume - volume*openCommission;
				}
			}
			else{
				//�������ƽ���,��������
				if (openCommission < 1){
					regain += closePrice*marginRate*multiplier*volume;
				}
				else{
					regain += closePrice*marginRate*multiplier*volume;
				}
			}
		}
	}
	return regain;
}

void Trader::cleanOrderMap(const QString &instructionId){
	auto &tradingDate = BackgroundTrader::getInstance()->getTradingDate();
	QSqlQuery getOrderRefs;
	getOrderRefs.prepare(" select order_ref from orders where investor_id = :investor_id and order_date = :order_date "
		" and instruciton_id = :instruction_id ");
	getOrderRefs.bindValue(":investor_id", id->getInvestorID());
	getOrderRefs.bindValue(":order_date", tradingDate);
	getOrderRefs.bindValue(":instruction_id", instructionId);
	getOrderRefs.exec();
	vector<int> refs;
	while (getOrderRefs.next()){
		refs.push_back(getOrderRefs.value(0).toInt());
	}
	for (auto &ref : refs){
		orders.erase(ref);
		orderFilter.insert(ref);
	}
}

//�ӳ����������µı���
void Trader::generateOrderFromCanceledOrder(const shared_ptr<Order> &order){
	shared_ptr<Order> newOrder = make_shared<Order>();
	newOrder->moveToThread(QCoreApplication::instance()->thread());
	newOrder->setInvestorId(order->getInvestorId());
	newOrder->setStrategyId(order->getStrategyId());
	newOrder->setInstructionId(order->getInstructionId());
	newOrder->setInstrumentId(order->getInstrumentId());
	newOrder->setDirection(order->getDirection());
	newOrder->setOpenCloseFlag(order->getOpenCloseFlag());
	double price = order->getPrice();
	//��øú�Լ����С�۸�䶯��λ���������������ӣ�����������ۼ�С
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	auto &info = instruments[order->getInstrumentId()];
	double minPrice = info->getMinimumUnit();
	if (order->getDirection() == 'b'){
		price += minPrice;
	}
	else{
		price -= minPrice;
	}
	newOrder->setPrice(price);
	newOrder->setOriginalVolume(order->getRestVolume());
	newOrder->setTradedVolume(0);
	newOrder->setRestVolume(newOrder->getOriginalVolume());
	newOrder->setOpenCloseFlag(order->getOpenCloseFlag());
	newOrder->setOrderStatus('a');
	if (newOrder->getOpenCloseFlag() == '1'){
		//���յ�ƽ���ֶ�ʱ�Ժ�Լ�����жϣ�������Ϻ��ڻ��ĺ�Լ���Զ�ƽ��
		if (info->getExchangeId() == "SHFE"){
			splitSHFEOrder(newOrder, "0");			//�޼۵���־0
			return;
		}
	}
	//ִ��ָ��
	executeOrder(newOrder, "0");
}

//����ĳ�����Ļر�
void Trader::calculateOrder(std::shared_ptr<Order> order){
	switch (order->getOrderStatus()){
	case 'f':
		if (order->getOpenCloseFlag() == '0'){
			double cost = -calculateOpenCost(order->getInstructionId());
			QString report = order->getInstructionId() + "," + QString::number(cost);
			qDebug() << "ָ��" << order->getInstructionId() << "�Ŀ��ַ��ã�" << cost;
			instructionPort->writeBackResult(report);
			//���²����ʽ�
			QString investorId = order->getInvestorId();
			QString strategyId = order->getStrategyId();
			fundDao.updateStrategyFund(investorId, strategyId, cost);
		}
		else{
			double regain = calculateCloseRegain(order->getInstructionId());
			QString report = order->getInstructionId() + "," + QString::number(regain);
			qDebug() << "ָ��" << order->getInstructionId() << "��ƽ���ջأ�" << regain;
			instructionPort->writeBackResult(report);
			QString investorId = order->getInvestorId();
			QString strategyId = order->getStrategyId();
			fundDao.updateStrategyFund(investorId, strategyId, regain);
		}
		cleanOrderMap(order->getInstructionId());
		calculateOrders.erase(order->getOrderRef());		//�Ӵ����㼯�����Ƴ�����
		break;
		//��������
	case 'w':
		if (order->getOpenCloseFlag() == '0'){
			double cost = -calculateOpenCost(order->getInstructionId());
			QString report = order->getInstructionId() + "," + QString::number(cost);
			qDebug() << "ָ��" << order->getInstructionId() << "�Ŀ��ַ��ã�" << cost;
			instructionPort->writeBackResult(report);
			//���²����ʽ�
			QString investorId = order->getInvestorId();
			QString strategyId = order->getStrategyId();
			fundDao.updateStrategyFund(investorId, strategyId, cost);
		}
		else{
			double regain = calculateCloseRegain(order->getInstructionId());
			QString report = order->getInstructionId() + "," + QString::number(regain);
			qDebug() << "ָ��" << order->getInstructionId() << "��ƽ���ջأ�" << regain;
			instructionPort->writeBackResult(report);
			QString investorId = order->getInvestorId();
			QString strategyId = order->getStrategyId();
			fundDao.updateStrategyFund(investorId, strategyId, regain);
			calculateOrders.erase(order->getOrderRef());		//�Ӵ����㼯�����Ƴ�����
		}
		cleanOrderMap(order->getInstructionId());
		break;
	default:
		break;
	}
}
/****************************��������******************************************/