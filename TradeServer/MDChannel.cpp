#include "MDChannel.h"
#include <qbytearray.h>
#include <qdebug.h>
#include <qthread.h>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

using std::shared_ptr;
using std::make_shared;

shared_ptr<MDChannel> MDChannel::channel = nullptr;

shared_ptr<MDChannel> MDChannel::getInstance(){
	if (channel != nullptr){
		return channel;
	}
	else{
		channel = shared_ptr<MDChannel>(new MDChannel());
		return channel;
	}
}

MDChannel::MDChannel(){
	server = new QTcpServer(this);
	connect(server, SIGNAL(newConnection()), this, SLOT(setSocket()));
	if (server->listen(QHostAddress::LocalHost, 9999)){
		qDebug() << "�ѿ����������������,�˿ں�Ϊ:" << 9999;
	}
	else{
		qDebug() << "�޷������������";
		abort();
	}
}

//�ڻ����������֮����socket��д������
void MDChannel::writeToSocket(CThostFtdcDepthMarketDataField *data){
	if (socket != nullptr){
		qDebug() << "д��socket";
		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
		out << data->InstrumentID;
		if (data->InstrumentID[5] == 0){
			out << ' ';
		}
		socket->write(block);
		socket->flush();
	}
}

//�����µ�����ʱ��socket�Զ�ָ�����µ�����
void MDChannel::setSocket(){
	if (socket != nullptr){
		socket->close();
	}
	socket = server->nextPendingConnection();
}