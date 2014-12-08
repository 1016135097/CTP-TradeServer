#include "InstructionPort.h"

#include "qdebug.h"
#include "GVAR.h"
using std::lock_guard;
using std::mutex;

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

InstructionPort::InstructionPort(QObject *parent) :QObject(parent){
	server = new QTcpServer(this);
	connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
	if (server->listen(QHostAddress::LocalHost, 8888)){
		qDebug() << "ָ������˿ڿ������˿ں�Ϊ8888";
	}
}

void InstructionPort::newConnection(){
	socket = server->nextPendingConnection();
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyToRead()));
}

//��62���ֽڲŽ��ж�ȡ
void InstructionPort::readyToRead(){
	QDataStream socketStream(socket);
	if (socket->bytesAvailable() >= 62){
		char *str = new char[50];
		socketStream.readRawData(str, 50);
		QByteArray *instruction = new QByteArray(str);
		int volume;
		socketStream >> volume;
		double price;
		socketStream >> price;
		QString investorId = instruction->mid(15, 16).trimmed();
		auto &trader = TRADERS[investorId];
		trader->generateAndExecuteOrder(instruction, volume, price);
	}
}

//д�ؿͻ���ָ��ִ�н��
void InstructionPort::writeBackResult(QString result){
	lock_guard<mutex> guard(writeBackMutex);
	if (socket != nullptr){
		socket->write(result.toStdString().c_str());
		socket->flush();
	}
}