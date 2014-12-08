#include "BackgroundTrader.h"
#include "ThostFtdcTraderApi.h"
#include "GVAR.h"
#include <qdebug.h>
#include <qsqlquery.h>
#include <qvariant.h>
#include <qstringlist.h>
#include <qdir.h>
#include <vector>
#include <thread>
#include <chrono>

using std::shared_ptr;
using std::make_shared;
using std::make_pair;
using std::map;

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

shared_ptr<BackgroundTrader> BackgroundTrader::bgTrader = nullptr;

std::shared_ptr<BackgroundTrader> BackgroundTrader::getInstance(){
	if (bgTrader != nullptr){
		return bgTrader;
	}
	else{
		bgTrader = shared_ptr<BackgroundTrader>(new BackgroundTrader());
		return bgTrader;
	}
}

BackgroundTrader::~BackgroundTrader(){
	if (api != nullptr){
		api->RegisterSpi(nullptr);
		api->Release();
		api = nullptr;
	}
}

const QString & BackgroundTrader::getTradingDate() const{
	return tradingDate;
}

const map<QString, shared_ptr<InstrumentInfo>> & BackgroundTrader::getInstruments() const{
	return interestedInstruments;
}

/***************************˽�и�������**********************************************************/

//���캯��
BackgroundTrader::BackgroundTrader(){
	accountID = make_shared<AccountID>();
	//�����ݿ��ж�ȡ��̨�˻�����Ϣ:��̨�˺�Ϊʵ���˺�83601689
	QSqlQuery query(DATABASE);
	QString initialize_AccountID_SQL = "select * from account where investor_id = 83601689";
	if (query.exec(initialize_AccountID_SQL) && query.next()){
		QString investor_id = query.value("investor_id").toString();
		QString password = query.value("password").toString();
		QString broker_id = query.value("broker_id").toString();
		QString front_address = query.value("front_address").toString();
		accountID->setInvestorID(investor_id);
		accountID->setPassword(password);
		accountID->setBrokerID(broker_id);
		accountID->setFrontAddress(front_address);
	}
	//�Ѻ�̨�˺ŵ��ļ���������һ���ļ�����
	QString dirName = "user/background";
	QDir conFileDir(dirName);
	if (!conFileDir.exists()){
		conFileDir.mkpath(".");
	}
	api = CThostFtdcTraderApi::CreateFtdcTraderApi((dirName + "/").toStdString().c_str());
	api->RegisterSpi(this);
	//���Ĺ�������˽������ע��ǰ�û�����ʼ��
	api->SubscribePublicTopic(THOST_TERT_RESTART);
	api->SubscribePrivateTopic(THOST_TERT_RESTART);
	char *frontAddress = new char[100];
	strcpy(frontAddress, accountID->getFrontAddress().toStdString().c_str());
	api->RegisterFront(frontAddress);
	api->Init();
	while (true){
		if (loginFlag){
			init();
			break;
		}
	}
}

//��ʼ�����󣬻�ý������ڡ�������Ҫ�ĺ�Լ��Ϣ
void BackgroundTrader::init(){
	//�����ݿ��ж�ȡ���Ը���Ȥ�����к�Լ����ʼ����Щ��Լ��Ϣ����������д�����ݿ�ĺ�Լ��Ϣ��
	QSqlQuery query(DATABASE);
	QString read_all_interested_instruments = "select interested_instruments from strategy";
	query.exec(read_all_interested_instruments);
	while (query.next()){
		QString rawString = query.value(0).toString();
		initInterestedInstruments(rawString);
	}
	supplementInstrumentInfo();
	tradingDate = QString(api->GetTradingDay());
}

//��"���Ա�"�����������и���Ȥ�ĺ�Լ
void BackgroundTrader::initInterestedInstruments(QString &rawString){
	if (rawString.contains(";")){
		//����Լ��������;�ű�ʾ�ò��ԶԶ����Լ����Ȥ�����Ҫ��һ����ֵõ����к�Լ����
		auto instrumentIDs = rawString.split(";");
		for (auto &instrumentID : instrumentIDs){
			auto instrumentInfo = make_shared<InstrumentInfo>();
			auto trimmedID = instrumentID.trimmed();
			instrumentInfo->setId(trimmedID);
			interestedInstruments.insert(make_pair(trimmedID, instrumentInfo));
		}
	}
	else
	{
		auto instrumentInfo = make_shared<InstrumentInfo>();
		auto trimmedID = rawString.trimmed();
		instrumentInfo->setId(trimmedID);
		interestedInstruments.insert(make_pair(trimmedID, instrumentInfo));
	}
}

//�������к�Լ����Ϣ������������ݿ�û�������������ѯ
void BackgroundTrader::supplementInstrumentInfo(){
	QSqlQuery query(DATABASE);
	query.prepare("select * from instrument_info where id = :id");
	for (auto &item : interestedInstruments){
		query.bindValue(":id", item.first);
		query.exec();
		if (query.next()){
			auto &info = item.second;
			info->setName(query.value("name").toString());
			info->setExchangeId(query.value("exchange_id").toString());
			info->setDeadline(query.value("deadline").toDate());
			info->setMarginRate(query.value("margin_rate").toDouble());
			info->setMutiplier(query.value("multiplier").toInt());
			info->setMinimumUnit(query.value("minimum_unit").toDouble());
			double oc = query.value("oc").toDouble();
			double oc_rate = query.value("oc_rate").toDouble();
			info->setOpenCommission(oc > oc_rate ? oc : oc_rate);	//�������ʺ�������ѡ��һ������Ч�ģ���Ч�������ݿ�����0
			double cc = query.value("cc").toDouble();
			double cc_rate = query.value("cc_rate").toDouble();
			info->setCloseCommission(cc > cc_rate ? cc : cc_rate);
			double today_cc = query.value("today_cc").toDouble();
			double today_cc_rate = query.value("today_cc_rate").toDouble();
			info->setCloseTodayCommission(today_cc > today_cc_rate ? today_cc : today_cc_rate);
		}
		else{
			newInstruments.push(item.first);	//���º�Լ��������Ӻ�Լ
		}
	}
	//�������ݿ���֮ǰû��������ϵĺ�Լ��������Ͳ�ѯ�����������в�ѯ���ڷ�����д�����ݿⲢ����ö���
	while (!newInstruments.empty() && readyForNext()){
		auto &instruName = newInstruments.front();
		finishBasicQuery = false;
		finishCommissionQuery = false;
		qDebug() << "�������������ѯ:" << instruName << "��Ϣ...";
		//��ѯ��Լ������Ϣ
		CThostFtdcQryInstrumentField *a = new CThostFtdcQryInstrumentField();
		strcpy(a->InstrumentID, instruName.toStdString().c_str());
		strcpy(a->ExchangeID, "");
		api->ReqQryInstrument(a, getRequestID());
		std::this_thread::sleep_for(std::chrono::seconds(1));	//�������ƣ�ÿ�β�ѯ���1��
		//��ѯ��Լ������
		CThostFtdcQryInstrumentCommissionRateField *b = new CThostFtdcQryInstrumentCommissionRateField();
		strcpy(b->BrokerID, accountID->getBrokerID().toStdString().c_str());
		strcpy(b->InvestorID, accountID->getInvestorID().toStdString().c_str());
		strcpy(b->InstrumentID, instruName.toStdString().c_str());
		api->ReqQryInstrumentCommissionRate(b, getRequestID());
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

//�ж�newInstruments�������Ƿ����������һ��
bool BackgroundTrader::readyForNext(){
	return (finishBasicQuery && finishCommissionQuery);
}

int BackgroundTrader::getRequestID(){
	requestID++;
	return requestID;
}

void BackgroundTrader::errorInstrumentID(char *id){
	if (id == nullptr){
		qDebug() << "�����Լ����:" << newInstruments.front() << "���������ݿ�Ĳ�����Ϣ";
		abort();
	}
}

/***************************Spi�¼��ص�����*******************************************************/

///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
void BackgroundTrader::OnFrontConnected(){
	//���ӵ�������֮���Զ���½,���˻��������������
	CThostFtdcReqUserLoginField *loginField = new CThostFtdcReqUserLoginField();
	strcpy(loginField->BrokerID, accountID->getBrokerID().toStdString().c_str());
	strcpy(loginField->UserID, accountID->getInvestorID().toStdString().c_str());
	strcpy(loginField->Password, accountID->getPassword().toStdString().c_str());
	api->ReqUserLogin(loginField, getRequestID());
}

///��¼������Ӧ
void BackgroundTrader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		loginFlag = true;
		qDebug() << "�������Ϸ����������б��س�ʼ��...";
	}
	else{
		qDebug() << "��̨�˻���¼ʧ��!";
		abort();
	}
}

///�����ѯ��Լ��Ӧ
void BackgroundTrader::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	errorInstrumentID(pInstrument->InstrumentID);
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		QSqlQuery query(DATABASE);
		query.prepare("insert into instrument_info (id,name,exchange_id,deadline,margin_rate,multiplier,minimum_unit) "
			" values (:id,:name,:exchange_id,:deadline,:margin_rate,:multiplier,:minimum_unit)");
		query.bindValue(":id", pInstrument->InstrumentID);
		query.bindValue(":name", QString::fromLocal8Bit(pInstrument->InstrumentName));
		query.bindValue(":exchange_id", pInstrument->ExchangeID);
		query.bindValue(":deadline", pInstrument->ExpireDate);
		query.bindValue(":margin_rate", pInstrument->LongMarginRatio);
		query.bindValue(":multiplier", pInstrument->VolumeMultiple);
		query.bindValue(":minimum_unit", pInstrument->PriceTick);
		query.exec();
		//д���ڴ������
		auto &info = interestedInstruments[QString(pInstrument->InstrumentID)];
		info->setName(QString::fromLocal8Bit(pInstrument->InstrumentName));
		info->setExchangeId(QString(pInstrument->ExchangeID));
		info->setDeadline(QDate::fromString(QString(pInstrument->ExpireDate), "yyyyMMdd"));
		info->setMarginRate(pInstrument->LongMarginRatio);
		info->setMutiplier(pInstrument->VolumeMultiple);
		info->setMinimumUnit(pInstrument->PriceTick);
		finishBasicQuery = true;
		if (readyForNext()){
			newInstruments.pop();
		}
	}
}

///�����ѯ��Լ����������Ӧ
void BackgroundTrader::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	errorInstrumentID(pInstrumentCommissionRate->InstrumentID);
	if (pRspInfo == nullptr || pRspInfo->ErrorID == 0){
		QSqlQuery query(DATABASE);
		query.prepare("update instrument_info set oc=:oc , oc_rate=:oc_rate , cc=:cc , cc_rate=:cc_rate ," 
			" today_cc=:today_cc , today_cc_rate=:today_cc_rate where id=:id ");
		query.bindValue(":id", newInstruments.front());
		double &oc = pInstrumentCommissionRate->OpenRatioByVolume;
		query.bindValue(":oc", oc);
		double &oc_rate = pInstrumentCommissionRate->OpenRatioByMoney;
		query.bindValue(":oc_rate", oc_rate);
		double &cc = pInstrumentCommissionRate->OpenRatioByVolume;
		query.bindValue(":cc", cc);
		double &cc_rate = pInstrumentCommissionRate->CloseRatioByMoney;
		query.bindValue(":cc_rate", cc_rate);
		double &today_cc = pInstrumentCommissionRate->CloseTodayRatioByVolume;
		query.bindValue(":today_cc", today_cc);
		double &today_cc_rate = pInstrumentCommissionRate->CloseTodayRatioByMoney;
		query.bindValue(":today_cc_rate", today_cc_rate);
		query.exec();
		//д���ڴ������
		auto &info = interestedInstruments[newInstruments.front()];
		info->setOpenCommission(oc > oc_rate ? oc : oc_rate);
		info->setCloseCommission(cc > cc_rate ? cc : cc_rate);
		info->setCloseTodayCommission(today_cc > today_cc_rate ? today_cc : today_cc_rate);
		finishCommissionQuery = true;
		if (readyForNext()){
			newInstruments.pop();
		}
	}
}