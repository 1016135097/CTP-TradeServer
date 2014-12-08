#pragma once
#include <qsqldatabase.h>
#include <map>
#include <qstring.h>
#include <memory>
#include "Trader.h"
#include "InstructionPort.h"

extern QSqlDatabase DATABASE;

extern std::map<QString, std::shared_ptr<Trader>> TRADERS;

extern InstructionPort *instructionPort;

//��������
namespace init{
	//�������ݿ�
	void connectToDatabase();

	//���º�Լ��Ϣ���������鲢�򿪶˿�
	void initBroadcast();

	//��ʼ���˻�-���Գֱֲ�
	void initStrategyPosition();

	//��ʼ�������˻�
	void initAccounts();

	//��ʼ��ָ��˿�
	void initInstructionPort();
}