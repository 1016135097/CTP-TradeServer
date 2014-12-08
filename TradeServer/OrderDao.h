#pragma once

#include "Order.h"
//#include <mutex>
#include <set>
#include <memory>

class OrderDao{
public:
	//���Ͷ���ߵ��ջ�Ծ�ı�����ϵͳ��ʼ��ʱ����
	std::shared_ptr<std::set<std::shared_ptr<Order>>> getActivedOrders(const QString &id,const QString &date);
	//���Ͷ����ĳ�յ���󱨵����ã�ϵͳ��ʼ��ʱ����
	int getMaximumOrderRef(const QString &id, const QString &date);
	//��ʼ�����˱�����ż���
	void initOrderFilter(const QString &id, const QString &date, std::set<int> &orderFilter);
	//����ϵͳ�еı�����,�������������룬�������
	void updateOrderTable(const std::shared_ptr<Order> &order);
};