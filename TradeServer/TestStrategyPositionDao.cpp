#include "TestStrategyPositionDao.h"
#include "StrategyPositionDao.h"
#include "GVAR.h"

#include <qtest.h>
#include <memory>
using std::shared_ptr;
using std::make_shared;

void TestStrategyPositionDao::testBuyOpen(){
	//�������ݿ�
	init::connectToDatabase();

	//���º�Լ��Ϣ���������鲢�򿪶˿�
	init::initBroadcast();

	//��ʼ���˻�-���Գֱֲ�
	init::initStrategyPosition();

	//��ʼ�������˻�
	init::initAccounts();

	//��ʼ��ָ��˿�
	init::initInstructionPort();

	shared_ptr<StrategyPositionDao> dao = make_shared<StrategyPositionDao>();

	CThostFtdcTradeField *pTrade = new CThostFtdcTradeField();
	pTrade->Direction = THOST_FTDC_D_Buy;
	pTrade->OffsetFlag = THOST_FTDC_OF_Open;
	strcpy(pTrade->InvestorID, "00000004");
	pTrade->Volume = 100;
	strcpy(pTrade->InstrumentID, "IF1409");

	QString strategyId = "001";
	dao->updatePosition(pTrade, strategyId);
}

//QTEST_MAIN(TestStrategyPositionDao)

