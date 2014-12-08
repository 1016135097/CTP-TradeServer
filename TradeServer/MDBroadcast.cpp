#include "MDBroadcast.h"
#include <qdebug.h>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

using std::shared_ptr;
using std::make_shared;

shared_ptr<MDBroadcast> MDBroadcast::broadcast = nullptr;

shared_ptr<MDBroadcast> MDBroadcast::getInstance(){
	if (broadcast != nullptr){
		return broadcast;
	}
	else
	{
		broadcast = shared_ptr<MDBroadcast>(new MDBroadcast());
		return broadcast;
	}
}

MDBroadcast::~MDBroadcast(){
	if (api != nullptr){
		api->RegisterSpi(nullptr);
		api->Release();
		api = nullptr;
	}
}

void MDBroadcast::subscribeInstruments(std::vector<QString> &instruments){
	qDebug() << "���ڶ��ĺ�Լ:";
	int count = instruments.size();
	char* *allInstruments = new char*[count];
	for (int i = 0; i < count; i++){
		allInstruments[i] = new char[7];
		strcpy(allInstruments[i], instruments.at(i).toStdString().c_str());
		qDebug() << allInstruments[i];
	}
	api->SubscribeMarketData(allInstruments, count);
}

bool MDBroadcast::isReadyToSubscribe(){
	return readyToSubscribe;
}

void MDBroadcast::setChannel(shared_ptr<MDChannel> channel){
	this->channel = channel;
	connect(this, SIGNAL(broadcastData(CThostFtdcDepthMarketDataField *)), channel.get(), SLOT(writeToSocket(CThostFtdcDepthMarketDataField*)));
}

/******************************˽�и�������***********************************************/

MDBroadcast::MDBroadcast(){
	api = CThostFtdcMdApi::CreateFtdcMdApi("./spi_con/");
	api->RegisterSpi(this);
	api->RegisterFront("tcp://asp-sim2-md1.financial-trading-platform.com:26213");	 //ģ����
	//api->RegisterFront("tcp://180.169.75.19:41213");								//ʵ��
	api->Init();
}


/*****************************spi�ص�����\***********************************************/

//���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷���������
void MDBroadcast::OnFrontConnected() {
	//�����Ϸ�����֮���Զ������½
	qDebug() << "���������½���������...";
	CThostFtdcReqUserLoginField loginField;
	strcpy(loginField.BrokerID, "");
	strcpy(loginField.UserID, "");
	strcpy(loginField.Password, "");
	api->ReqUserLogin(&loginField, 0);
}

///��¼������Ӧ
void MDBroadcast::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
	int nRequestID, bool bIsLast) {
	if (pRspInfo->ErrorID == 0){
		qDebug() << "�ѵ�½��������������ڴ򿪱������������...";
		qDebug() << "����������鶩��...";
		readyToSubscribe = true;
	}
	else{
		qDebug() << "��¼ʧ��:" << pRspInfo->ErrorID << " " << pRspInfo->ErrorMsg;
		abort();
	}
}

//����ر���Ӧ
void MDBroadcast::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData){
	//qDebug() << "�յ�" << pDepthMarketData->InstrumentID << "����";
	emit broadcastData(pDepthMarketData);
	//qDebug() << pDepthMarketData->InstrumentID[0];
	//qDebug() << pDepthMarketData->InstrumentID[1];
	//qDebug() << pDepthMarketData->InstrumentID[2];
	//qDebug() << pDepthMarketData->InstrumentID[3];
	//qDebug() << pDepthMarketData->InstrumentID[4];
	//qDebug() << pDepthMarketData->InstrumentID[5];
}