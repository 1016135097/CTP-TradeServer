#pragma once

#include <qstring.h>
#include <qdatetime.h>

//��Լ��Ϣ��id�����֡���������š���󽻸��ա���֤���ʡ���Լ��������֤���ʡ������ѡ���С�䶯��λ�������Ƿ���Խ���
class InstrumentInfo{
public:
	InstrumentInfo() = default;
	InstrumentInfo(const InstrumentInfo&) = delete;
	InstrumentInfo& operator=(const InstrumentInfo&) = delete;
	void setId(const QString &id);
	void setName(const QString &name);
	void setExchangeId(const QString &exchangeId);
	void setDeadline(const QDate &deadline);
	void setMarginRate(const double &marginRate);
	void setMutiplier(const int &multiplier);
	void setOpenCommission(const double &commission);
	void setCloseCommission(const double &commission);
	void setCloseTodayCommission(const double &commission);
	void setMinimumUnit(const double &minimumUnit);
	void setTradable(const bool &tradable);

	const QString & getId() const;
	const QString & getName() const;
	const QString & getExchangeId() const;
	const QDate & getDeadline() const;
	const double & getMarginRate() const;
	const int & getMultiplier() const;
	const double & getOpenCommission() const;
	const double & getCloseCommission() const;
	const double & getCloseTodayCommission() const;
	const double & getMinimumUnit() const;
	const bool & isTradable() const;

	//Ϊ�˷���set�����У����� < �����
	bool operator<(const InstrumentInfo &i);
private:
	QString id;
	QString name;
	QString exchangeId;
	QDate deadline;
	double marginRate;
	int multiplier;
	double openCommission;
	double closeCommission;
	double closeTodayCommission;
	double minimumUnit;
	bool tradable;
};