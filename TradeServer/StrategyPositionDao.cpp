#include "StrategyPositionDao.h"
#include "BackgroundTrader.h"
#include "GVAR.h"
#include <qsqlquery.h>
#include <qvariant.h>
#include <qdebug.h>

//���ݳɽ��ر����²��Գֲ����
void StrategyPositionDao::updatePosition(CThostFtdcTradeField *pTrade, QString strategyId){
	//���뿪��
	if (pTrade->Direction == THOST_FTDC_D_Buy && pTrade->OffsetFlag == THOST_FTDC_OF_Open){
		QSqlQuery updateLong(DATABASE);
		updateLong.prepare("update strategy_position set long_position=long_position+:position,today_long_position=today_long_position+:today_position "
			" where investor_id=:investor and strategy_id=:strategy and instrument_id=:instrument ");
		updateLong.bindValue(":position", pTrade->Volume);
		updateLong.bindValue(":investor", pTrade->InvestorID);
		updateLong.bindValue(":strategy", strategyId);
		updateLong.bindValue(":instrument", pTrade->InstrumentID);
		//�ж��Ƿ������ڵĺ�Լ����¼���ղ�λ
		if (isSHFE(pTrade->InstrumentID)){
			updateLong.bindValue(":today_position", pTrade->Volume);
		}
		else
		{
			updateLong.bindValue(":today_position", 0);
		}
		updateLong.exec();
	}
	//����ƽ��
	if (pTrade->Direction == THOST_FTDC_D_Buy && ( pTrade->OffsetFlag == THOST_FTDC_OF_Close ||
		pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday || pTrade->OffsetFlag == THOST_FTDC_OF_CloseYesterday)){
		QSqlQuery updateLong(DATABASE);
		updateLong.prepare("update strategy_position set short_position=short_position-:position,today_long_position=today_long_position-:today_position "
			" where investor_id=:investor and strategy_id=:strategy and instrument_id=:instrument ");
		updateLong.bindValue(":position", pTrade->Volume);
		updateLong.bindValue(":investor", pTrade->InvestorID);
		updateLong.bindValue(":strategy", strategyId);
		updateLong.bindValue(":instrument", pTrade->InstrumentID);
		//�ж��Ƿ������ڵĺ�Լ����¼���ղ�λ
		if (isSHFE(pTrade->InstrumentID)){
			updateLong.bindValue(":today_position", pTrade->Volume);
		}
		else
		{
			updateLong.bindValue(":today_position", 0);
		}
		updateLong.exec();
	}
	//��������
	if (pTrade->Direction == THOST_FTDC_D_Sell && pTrade->OffsetFlag == THOST_FTDC_OF_Open){
		QSqlQuery updateShort(DATABASE);
		updateShort.prepare("update strategy_position set short_position=short_position+:position,today_short_position=today_short_position+:today_position "
			" where investor_id=:investor and strategy_id=:strategy and instrument_id=:instrument ");
		updateShort.bindValue(":position", pTrade->Volume);
		updateShort.bindValue(":investor", pTrade->InvestorID);
		updateShort.bindValue(":strategy", strategyId);
		updateShort.bindValue(":instrument", pTrade->InstrumentID);
		//�ж��Ƿ������ڵĺ�Լ����¼���ղ�λ
		if (isSHFE(pTrade->InstrumentID)){
			updateShort.bindValue(":today_position", pTrade->Volume);
		}
		else
		{
			updateShort.bindValue(":today_position", 0);
		}
		updateShort.exec();
	}
	//����ƽ��
	if (pTrade->Direction == THOST_FTDC_D_Sell && (pTrade->OffsetFlag == THOST_FTDC_OF_Close ||
		pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday || pTrade->OffsetFlag == THOST_FTDC_OF_CloseYesterday)){
		QSqlQuery updateShort(DATABASE);
		updateShort.prepare("update strategy_position set long_position=long_position-:position,today_short_position=today_short_position-:today_position "
			" where investor_id=:investor and strategy_id=:strategy and instrument_id=:instrument ");
		updateShort.bindValue(":position", pTrade->Volume);
		updateShort.bindValue(":investor", pTrade->InvestorID);
		updateShort.bindValue(":strategy", strategyId);
		updateShort.bindValue(":instrument", pTrade->InstrumentID);
		//�ж��Ƿ������ڵĺ�Լ����¼���ղ�λ
		if (isSHFE(pTrade->InstrumentID)){
			updateShort.bindValue(":today_position", pTrade->Volume);
		}
		else
		{
			updateShort.bindValue(":today_position", 0);
		}
		updateShort.exec();
	}
}

//�����˻����Թ�ϵ��ͬ�����Գֱֲ�
//1.�˻������µĲ���֮������¼
//2.�˻�����Խ����ϵ��ɾ����¼
void StrategyPositionDao::synStrategyPosition(){
	//1.�˻������µĲ���֮������¼
	QSqlQuery accountStrategy(DATABASE);
	accountStrategy.exec("select investor_id,strategy_id from account_strategy");
	while (accountStrategy.next()){
		QString investor_id = accountStrategy.value("investor_id").toString();
		QString strategy_id = accountStrategy.value("strategy_id").toString();
		//��strategy_position�м���Ƿ������Ӧ���˻�-���Բ�λ
		QSqlQuery exist;
		exist.prepare("select investor_id from strategy_position where investor_id=:investor_id and strategy_id=:strategy_id");
		exist.exec();
		exist.next();	//������¼����
		if (exist.isNull("investor_id")){
			//��������ڣ����ò�����ʲô����Ȥ�ĺ�Լ��Ȼ��ֱ�Ϊ��Щ��Լ�����˻�-���Ժ�Լ��λ
			QSqlQuery strategyInstruments;
			strategyInstruments.prepare("select interested_instruments from strategy where id=:id ");
			strategyInstruments.bindValue(":id", strategy_id);
			strategyInstruments.exec();
			strategyInstruments.next();
			QString rawInstruments = strategyInstruments.value(0).toString();
			auto instruments = rawInstruments.trimmed().split(";");
			for (auto &instrument : instruments){
				QSqlQuery insert;
				insert.prepare("insert into strategy_position (investor_id,strategy_id,instrument_id,today) "
					" values (:investor_id,:strategy_id,:instrument_id,:today) ");
				insert.bindValue(":investor_id", investor_id);
				insert.bindValue(":strategy_id", strategy_id);
				insert.bindValue(":instrument_id", instrument);
				insert.bindValue(":today", BackgroundTrader::getInstance()->getTradingDate());
				insert.exec();
			}
		}
	}
	//2.�˻�����Խ����ϵ��ɾ����¼
	QSqlQuery strategyPositions(DATABASE);
	strategyPositions.exec("select investor_id,strategy_id from strategy_position");
	while (strategyPositions.next()){
		QString investor_id = strategyPositions.value("investor_id").toString();
		QString strategy_id = strategyPositions.value("strategy_id").toString();
		//��account_strategy���м���Ƿ������Ӧ���˻�-���Թ�ϵ
		QSqlQuery exist;
		exist.prepare("select investor_id from account_strategy where investor_id=:investor_id and strategy_id=:strategy_id");
		exist.bindValue(":investor_id", investor_id);
		exist.bindValue(":strategy_id", strategy_id);
		exist.exec();
		exist.next();
		if (exist.isNull("investor_id")){
			QSqlQuery deleteSQL;
			deleteSQL.prepare("delete from strategy_position where investor_id=:investor_id and strategy_id=:strategy_id ");
			deleteSQL.bindValue(":investor_id", investor_id);
			deleteSQL.bindValue(":strategy_id", strategy_id);
			deleteSQL.exec();
		}
	}
}

//�������µĽ���������ʱ��ˢ�²��Գֱֲ��е�ʱ���¼���ѽ��ճֲ�����
void StrategyPositionDao::refreshDaily(){
	const QString &today = BackgroundTrader::getInstance()->getTradingDate();
	QSqlQuery refresh(DATABASE);
	refresh.prepare("update strategy_position set today_long_position=0,today_short_position=0 "
		" where today!=:today");
	refresh.bindValue(":today", today);
	refresh.exec();
	QSqlQuery setToday(DATABASE);
	setToday.prepare("update strategy_position set today=:today");
	setToday.bindValue(":today", today);
	setToday.exec();
}

bool StrategyPositionDao::isSHFE(QString instrumentID){
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	auto &instrumentInfo = instruments[instrumentID];
	if (instrumentInfo->getExchangeId() == "SHFE"){
		return true;
	}
	else
	{
		return false;
	}
}

int StrategyPositionDao::getTodayPosition(const QString &investorId, const QString &strategyId,
	const QString &instrumentId, char direction){
	QSqlQuery query;
	query.prepare("select today_long_position,today_short_position from strategy_position "
		" where investor_id=:investor_id and strategy_id=:strategy_id and instrument_id=:instrument_id ");
	query.bindValue(":investor_id", investorId);
	query.bindValue(":strategy_id", strategyId);
	query.bindValue(":instrument_id", instrumentId);
	query.exec();
	query.next();
	if (direction == 'b'){
		return query.value("today_long_position").toInt();
	}
	else
	{
		return query.value("today_short_position").toInt();
	}
}