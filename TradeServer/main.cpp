//#define TEST

#ifndef TEST
#include <QCoreApplication>
#include <qdebug.h>
#include <vector>
#include <qthread.h>
#include "BackgroundTrader.h"
#include "MDBroadcast.h"
#include "GVAR.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#pragma comment(lib,"thostmduserapi.lib")
#pragma comment(lib,"thosttraderapi.lib")

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

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

	return a.exec();
}

#endif
