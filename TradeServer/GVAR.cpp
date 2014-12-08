#include "GVAR.h"
#include "StrategyPositionDao.h"
#include "BackgroundTrader.h"
#include "MDBroadcast.h"
#include "AccountID.h"
#include "Trader.h"
#include <qthread.h>
#include <qfile.h>
#include <qsqlquery.h>
#include <qdebug.h>
#include <memory>

using std::make_shared;
using std::shared_ptr;
using std::map;


#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

QSqlDatabase DATABASE;

map<QString, shared_ptr<Trader>> TRADERS;

InstructionPort *instructionPort = nullptr;

//�������ݿ�
void init::connectToDatabase(){
	DATABASE = QSqlDatabase::addDatabase("QMYSQL");
	DATABASE.setHostName("127.0.0.1");
	DATABASE.setDatabaseName("trade_server");
	DATABASE.setUserName("root");
	DATABASE.setPassword("");
	if (!DATABASE.open()){
		qDebug() << "���ݿ����Ӵ���";
		abort();
	}
	qDebug() << "���ݿ����ӳɹ�!";
}

//���º�Լ��Ϣ���������鲢�򿪶˿�
void init::initBroadcast(){
	//���Դ洢���ݿ����¼���ĺ�Լ����
	std::vector<QString> toBeSubscribeInstru;
	//��ʼ����̨�˺�
	auto instruments = BackgroundTrader::getInstance()->getInstruments();
	qDebug() << "����" << instruments.size() << "����Լ";
	for (auto &item : instruments){
		auto &s = item.second;
		qDebug() << s->getName() << "��Լ����:" << s->getMarginRate() << "��С�䶯��λ:" << s->getMinimumUnit() << "������:"
			<< s->getCloseCommission() << s->getOpenCommission() << s->getCloseTodayCommission();
		toBeSubscribeInstru.push_back(item.first);
	}
	qDebug() << "������:" << BackgroundTrader::getInstance()->getTradingDate();
	//��ʼ������㲥
	auto broacast = MDBroadcast::getInstance();
	while (true){
		if (broacast->isReadyToSubscribe()){
			broacast->subscribeInstruments(toBeSubscribeInstru);
			break;
		}
	}
	//Ϊ����㲥ע��ͨ��
	QThread *channelThread = new QThread();
	auto channel = MDChannel::getInstance();
	channel->moveToThread(channelThread);
	channelThread->start();
	broacast->setChannel(channel);
}

//��ʼ���˻�-���Գֱֲ�
void init::initStrategyPosition(){
	StrategyPositionDao::synStrategyPosition();
	StrategyPositionDao::refreshDaily();
}

//��ʼ�����׽����˻�
void init::initAccounts(){
	QFile iniFile("user/acount.ini");
	if (!iniFile.open(QIODevice::ReadOnly | QIODevice::Text)){
		qDebug() << "cannot find front.ini";
		abort();
	}
	QTextStream in(&iniFile);
	QString investorID;
	while (!in.atEnd()){
		investorID = in.readLine();
		//�����˻�ID�ӱ��ж�ȡ�������� 
		QSqlQuery query(DATABASE);
		query.prepare("select password,broker_id,front_address from account where investor_id=:id ");
		query.bindValue(":id", investorID);
		query.exec();
		query.next();
		if (query.isNull(0)){
			qDebug() << "���ݿ���û���˻�:" << investorID << "��Ϣ�����������ļ������ݿ�";
			abort();
		}
		QString &password = query.value("password").toString();
		QString &broker_id = query.value("broker_id").toString();
		QString &front_address = query.value("front_address").toString();
		auto accountID = make_shared<AccountID>();
		accountID->setInvestorID(investorID);
		accountID->setPassword(password);
		accountID->setFrontAddress(front_address);
		accountID->setBrokerID(broker_id);
		auto trader = make_shared<Trader>(accountID);
		//���˻�����ȫ�ֱ�����
		TRADERS.insert(std::make_pair(investorID, trader));
	}
	iniFile.close();
}

//��ʼ��ָ��˿�
void init::initInstructionPort(){
	instructionPort = new InstructionPort();
}