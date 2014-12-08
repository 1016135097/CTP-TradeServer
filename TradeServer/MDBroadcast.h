#pragma once

#include "ThostFtdcMdApi.h"
#include "MDChannel.h"
#include <memory>
#include <vector>

class MDBroadcast :public QObject, public CThostFtdcMdSpi{
	Q_OBJECT
public:
	static std::shared_ptr<MDBroadcast> getInstance();
	~MDBroadcast();

	void subscribeInstruments(std::vector<QString> &instruments);
	void setChannel(std::shared_ptr<MDChannel> channel);
	bool isReadyToSubscribe();
signals:
	void broadcastData(CThostFtdcDepthMarketDataField *pDepthMarketData);
private:
	//���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷���������
	void OnFrontConnected() override;

	///��¼������Ӧ
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, 
		int nRequestID, bool bIsLast) override;

	//����ر���Ӧ
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;
private:
	MDBroadcast();
	MDBroadcast(const MDBroadcast &) = delete;
	MDBroadcast & operator=(const MDBroadcast &) = delete;
private:
	//ȫ�ֵ�������
	static std::shared_ptr<MDBroadcast>  broadcast;

	CThostFtdcMdApi *api;
	std::shared_ptr<MDChannel> channel = nullptr;
	bool readyToSubscribe = false;
}; 