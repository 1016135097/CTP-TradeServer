#include "ReportDao.h"
#include "GVAR.h"
#include "BackgroundTrader.h"
#include <qsqlquery.h>
#include <qvariant.h>
//#include <qsqlerror.h>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

//���ݳɽ��ر����»ر����
void ReportDao::updateReportTable(CThostFtdcTradeField *report, QString &strategyId){
	//���ֻر�
	if (report->OffsetFlag == THOST_FTDC_OF_Open){
		QSqlQuery insertOpen(DATABASE);
		insertOpen.prepare("insert into open_traded_report (trade_date,trade_id,trade_time,investor_id, "
			" strategy_id,system_id,order_ref,instrument_id,direction,volume,open_price,to_be_close ) values "
			" (:date,:trade_id,:time,:investor_id,:strategy_id,:system_id,:order_ref,:instrument_id,:direction, "
			"  :volume,:open_price,:to_be_close)");
		insertOpen.bindValue(":date", report->TradeDate);
		insertOpen.bindValue(":trade_id", report->TradeID);
		insertOpen.bindValue(":time", report->TradeTime);
		insertOpen.bindValue(":investor_id", report->InvestorID);
		insertOpen.bindValue(":strategy_id", strategyId);
		insertOpen.bindValue(":system_id", report->OrderSysID);
		insertOpen.bindValue(":order_ref", atoi(report->OrderRef));
		insertOpen.bindValue(":instrument_id", report->InstrumentID);
		QString direction;
		if (report->Direction == THOST_FTDC_D_Buy){
			direction = "b";
		}
		else
		{
			direction = "s";
		}
		insertOpen.bindValue(":direction", direction);
		insertOpen.bindValue(":volume", report->Volume);
		insertOpen.bindValue(":open_price", report->Price);
		insertOpen.bindValue(":to_be_close", report->Volume);
		insertOpen.exec();
	}
	//ƽ�ֻر�
	if (report->OffsetFlag == THOST_FTDC_OF_Close ||
		report->OffsetFlag == THOST_FTDC_OF_CloseToday || report->OffsetFlag == THOST_FTDC_OF_CloseYesterday){
		QSqlQuery insertClose(DATABASE);
		insertClose.prepare("insert into close_traded_report (trade_date,trade_id,trade_time,investor_id, "
			" strategy_id,system_id,order_ref,instrument_id,direction,volume,close_price,today_flag ) values "
			" (:date,:trade_id,:time,:investor_id,:strategy_id,:system_id,:order_ref,:instrument_id,:direction, "
			"  :volume,:close_price,:today_flag)");
		insertClose.bindValue(":date", report->TradeDate);
		insertClose.bindValue(":trade_id", report->TradeID);
		insertClose.bindValue(":time", report->TradeTime);
		insertClose.bindValue(":investor_id", report->InvestorID);
		insertClose.bindValue(":strategy_id", strategyId);
		insertClose.bindValue(":system_id", report->OrderSysID);
		insertClose.bindValue(":order_ref", atoi(report->OrderRef));
		insertClose.bindValue(":instrument_id", report->InstrumentID);
		QString direction;
		if (report->Direction == THOST_FTDC_D_Buy){
			direction = "b";
		}
		else
		{
			direction = "s";
		}
		insertClose.bindValue(":direction", direction);
		insertClose.bindValue(":volume", report->Volume);
		insertClose.bindValue(":close_price", report->Price);
		QString today_flag;
		if (report->OffsetFlag == THOST_FTDC_OF_CloseToday){
			today_flag = "t";
		}
		else{
			today_flag = "y";
		}
		insertClose.bindValue(":today_flag", today_flag);
		insertClose.exec();
		//���ɾ�̬Ȩ��
		generateStaticProfit(report, strategyId);
	}
}

//�����ƽ�ֻر����ڸ�����ر����֮�����ɾ�̬Ȩ��
void ReportDao::generateStaticProfit(CThostFtdcTradeField *report, QString &strategyId){
	int volume = report->Volume;
	while (volume > 0){
		QString openDate;
		QString openTradeId;
		double openPrice;
		int openVolume;
		QSqlQuery selectOpen(DATABASE);
		if (report->OffsetFlag == THOST_FTDC_OF_CloseToday){
			//ƽ���
			selectOpen.prepare("select trade_date,trade_id,open_price,volume from open_traded_report "
				" where trade_date=:date and investor_id=:id and strategy_id=:strategy_id and instrument_id=:instrument_id and "
				" to_be_close!=0 order by trade_time limit 1");
			selectOpen.bindValue(":date", report->TradeDate);
		}
		else{
			//ƽ�ɲ�
			QSqlQuery selectOpen(DATABASE);
			selectOpen.prepare("select trade_date,trade_id,open_price,volume from open_traded_report "
				" where investor_id=:id and strategy_id=:strategy_id and instrument_id=:instrument_id and "
				" to_be_close!=0 order by trade_time limit 1");		//sql����в��޶�����
		}
		selectOpen.bindValue(":id", report->InvestorID);
		selectOpen.bindValue(":strategy_id", strategyId);
		selectOpen.bindValue(":instrument_id", report->InstrumentID);
		selectOpen.exec();
		//qDebug() << "111:" << selectOpen.lastError().databaseText();
		selectOpen.next();
		openDate = selectOpen.value("trade_date").toString();
		openTradeId = selectOpen.value("trade_id").toString();
		openPrice = selectOpen.value("open_price").toDouble();
		openVolume = selectOpen.value("volume").toInt();

		//����������ƽ����
		if (openVolume >= volume){
			// ���¿��ֻر��е�to_be_close�ֶ�
			QSqlQuery updateOpen(DATABASE);
			updateOpen.prepare("update open_traded_report set to_be_close=to_be_close-:volume where "
				" trade_date=:date and trade_id=:id");
			updateOpen.bindValue(":volume", volume);
			updateOpen.bindValue(":date", report->TradeDate);
			updateOpen.bindValue(":id", openTradeId);
			updateOpen.exec();
			//qDebug() << "222:" << updateOpen.lastError().databaseText();
			//���ɾ�̬Ȩ���¼
			QSqlQuery closeProfit(DATABASE);
			closeProfit.prepare("insert into close_profit (close_date,close_id,open_date,open_id,close_price, "
				" open_price,volume,profit) values (:close_date,:close_id,:open_date,:open_id,:close_price, "
				" :open_price,:volume,:profit)");
			closeProfit.bindValue(":close_date", report->TradeDate);
			closeProfit.bindValue(":close_id", report->TradeID);
			closeProfit.bindValue(":open_date", openDate);
			closeProfit.bindValue(":open_id", openTradeId);
			closeProfit.bindValue(":close_price", report->Price);
			closeProfit.bindValue(":open_price", openPrice);
			closeProfit.bindValue(":volume", volume);
			double profit = 0;
			if (report->OffsetFlag == THOST_FTDC_OF_CloseToday){
				profit = calculateStaticProfit_today(report->InstrumentID, openPrice,
					report->Price, volume, report->Direction);
			}
			else{
				profit = calculateStaticProfit(report->InstrumentID, openPrice,
					report->Price, volume, report->Direction);
			}
			closeProfit.bindValue(":profit", profit);
			closeProfit.exec();
			//qDebug() << "333:" << closeProfit.lastError().databaseText();
			return;		//ֱ�ӷ��ؼ���
		}
		else{
			//������С��ƽ����
			// ���¿��ֻر��е�to_be_close�ֶ�
			QSqlQuery updateOpen(DATABASE);
			updateOpen.prepare("update set to_be_close=0 where "
				" trade_date=:date,trade_id=:id");		//to_be_close�ֶ�����
			updateOpen.bindValue(":date", report->TradeDate);
			updateOpen.bindValue(":id", openTradeId);
			updateOpen.exec();
			//���ɾ�̬Ȩ���¼
			QSqlQuery closeProfit(DATABASE);
			closeProfit.prepare("insert into close_profit (close_date,close_id,open_date,open_id,close_price, "
				" open_price,volume,profit) values (:close_date,:close_id,:open_date,:open_id,:close_price, "
				" :open_price,:volume,:profit)");
			closeProfit.bindValue(":close_date", report->TradeDate);
			closeProfit.bindValue(":close_id", report->TradeID);
			closeProfit.bindValue(":open_date", openDate);
			closeProfit.bindValue(":open_id", openTradeId);
			closeProfit.bindValue(":close_price", report->Price);
			closeProfit.bindValue(":open_price", openPrice);
			closeProfit.bindValue(":volume", openVolume);	//��Ҳ��ɿ�����
			double profit = 0;
			if (report->OffsetFlag == THOST_FTDC_OF_CloseToday){
				profit = calculateStaticProfit_today(report->InstrumentID, openPrice,
					report->Price, openVolume, report->Direction);
			}
			else{
				profit = calculateStaticProfit(report->InstrumentID, openPrice,
					report->Price, openVolume, report->Direction);
			}
			closeProfit.bindValue(":profit", profit);
			closeProfit.exec();
			//qDebug() << "444:" << closeProfit.lastError().databaseText();
			volume -= openVolume;
		}
	}
}

//������վ�̬Ȩ��
double ReportDao::calculateStaticProfit_today(QString instrumentID, double openPrice,
	double closePrice, int volume, char closeDirection){
	//'0'��,'1'��
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	auto info = instruments[instrumentID];
	int multiplier = info->getMultiplier();
	double openCommission = info->getOpenCommission();
	double totalOpenCommission = 0;
	if (openCommission < 1){
		//������=����*��Լ����*����*�ٷֱ�
		totalOpenCommission = openPrice*multiplier*volume*openCommission;
	}
	else{
		//������=����*ÿ��Ǯ��
		totalOpenCommission = volume*openCommission;
	}
	double closeCommission = info->getCloseTodayCommission();	//��ȡ����ƽ�ֽ����
	double totalCloseCommission = 0;
	if (closeCommission < 1){
		totalCloseCommission = closePrice*multiplier*volume*closeCommission;
	}
	else{
		totalCloseCommission = volume*closeCommission;
	}
	double totalCommission = totalCloseCommission + totalOpenCommission;
	//��������仯ӯ��
	//ƽ��ͷ
	double profit = (closePrice - openPrice)*volume*multiplier;
	//ƽ��ͷ
	if (closeDirection == '0'){
		profit = -profit;
	}
	//���ս��
	profit = profit - totalCloseCommission;
	qDebug() << "volume:" << volume;
	qDebug() << "close price:" << closePrice;
	qDebug() << "open price:" << openPrice;
	qDebug() << "multipler:" << multiplier;
	qDebug() << "commission��" << totalCloseCommission;
	qDebug() << "---profit---:" << profit;
	return profit;
}

//���㾲̬Ȩ��
double ReportDao::calculateStaticProfit(QString instrumentID, double openPrice,
	double closePrice, int volume, char closeDirection){
	//'0'��,'1'��
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	auto info = instruments[instrumentID];
	int multiplier = info->getMultiplier();
	double openCommission = info->getOpenCommission();
	double totalOpenCommission = 0;
	if (openCommission < 1){
		//������=����*��Լ����*����*�ٷֱ�
		totalOpenCommission = openPrice*multiplier*volume*openCommission;
	}
	else{
		//������=����*ÿ��Ǯ��
		totalOpenCommission = volume*openCommission;
	}
	double closeCommission = info->getCloseCommission();	//��ȡ����ƽ�ֽ����
	double totalCloseCommission = 0;
	if (closeCommission < 1){
		totalCloseCommission = closePrice*multiplier*volume*closeCommission;
	}
	else{
		totalCloseCommission = volume*closeCommission;
	}
	double totalCommission = totalCloseCommission + totalOpenCommission;
	//��������仯ӯ��
	double profit = (closePrice - openPrice)*volume*multiplier;
	if (closeDirection == '0'){
		profit = -profit;
	}
	//���ս��
	profit -= totalCloseCommission;
	return profit;
}