
/*
������е�ĸ�����б��Լ�ÿ��ĸ����Ӧ�������ӵ�����Ϣ,�������ݡ� һֱ����
*/

#ifndef ALGDATA_H
#define ALGDATA_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>
#include <QSqlQuery>

#include "StrategyChildInfo.h"
#include "StrategyParentInfo.h"
#include "Level2.h"
#include "globaldef.h"
#include "sellInTime.h"
#include "KLineData.h"

struct LogEvent
{
	QString event;
	QString data1;
	QString data2;
	QString data3;
	QString data4;
	QString data5;
	QString parent_id;
	QString parent_volume;
	QString execute_price;
	QString price_type;
	QString execute_type;
	double bid_price1;
	int bid_vol1;
	double ask_price1;
	int ask_vol1;
	double last_price;
	QString execute_time;
};

class AlgoOrder;
class AlgData :public QObject
{
	Q_OBJECT
public:
	AlgData();

	~AlgData();
	
	static AlgData* inst()
	{
		static AlgData instance_;
		return &instance_;
	}
	
	void Init();

	//void ReadConfigFile();

	//void ReadParentInfo();
	void ReadHoliday();

	void DBSellInTime();

	void DbPKLine();

	void DealSendOrder(StrategyChildInfo child_info);

	//�������򵥣�������//û�м������Ƿ��㹻
	//void SellOrder(StrategyChildInfo child_info);

	//void BuyOrder(StrategyChildInfo child_info);

	//������������
	void InquireMaketData(QString stockid);

	//��������
	void InquireHangQing(QString stock_id);

	//�������
	void InquireSellInTime(QString stock_id);

	void SellInTimeReceive(QString code, unsigned long sec, QByteArray sellintime);

	void SavaSellInTimeToDB(QList<SellInTime> list_sell);

    void SetHangQing(QString code, QByteArray levelobj);


	void PovAlg(SellInTime sell_in);



	int CaculateTotalInterval(QDateTime start_date_time,
		QDateTime end_date_time,int ses_divid);

	QList<QDateTime> CaculateTimeRegion(QDateTime start_date_time, 
		QDateTime end_date_time, int ses_divid);

	QString GetMidType(QString stock_id);

	QString GetMarketType(QString stock_id);

	//��ѯĸ������ĸ���������ӵ�
	void InquireParentOrders(QList<QString> list_parent_id);

	void ParentOrderReceived(QJsonObject obj);

	void ChildOrderReceived(QString parent,QJsonObject obj);

	//���ĸ��
	void AddParentInfo(StrategyParentInfo *info);

	//ɾ��ĸ��
	void DelParentInfo(StrategyParentInfo *info);

	//�޸�ĸ��
	void ALterParentInfo(StrategyParentInfo *info);


	/* Level2��ʷ����*/
	void InquireHistoryLevel2(int day, QDateTime start_time,QDateTime end_time);

	//ĳ��ʱ���Level2��ʷ����
	void ReqPastLevel2TimeQuantum(int day,QDateTime start_time,QDateTime end_time);

	//ĳ��ʱ���֮��Level2��ʷ����
	void ReqPastLevel2TimeAfter(int day,QDateTime date_time);

	//ĳ��ʱ��������֮��Level2��ʷ����
	void ReqPastLevel2TimeDot(int day,QDateTime date_time);


	/*������*/
	void ReqHistoryPKLine(QString stockid,QDateTime start_time, QDateTime end_time);

	//ĳ��ʱ��εķ�����
	void ReqPastPKLineQuantum(QString stockid,QDateTime start_time, QDateTime end_time);

	//ĳ��ʱ���֮�����
	void ReqPastPKLineTimeAfter(int day, QDateTime date_time);

	//ĳ��ʱ��������֮�����
	void ReqPastPKLineTimeDot(int day, QDateTime date_time);


	void CancelOrder(StrategyChildInfo c_info);

	void ReqServerTime();

	QString GetPriceFromType(QString stockid,QString price_type);


	int GetEnumFromTypeName(QString directive);

	QString GetTyepNameFromEnum(int enum_d);

	QString GetDirectiveFromEnum(int enum_d);

	int GetEnumFromDirective(QString dir);

	QString GetSideFromType(int type);

	QString GetClassNameFromStrategy(QString strategy);

	bool createDB(const QString dbFilePath, const QString dbFileName);
	bool sqlQuery(const QString sql);
signals:
	//���ܵ����ص��ӵ�����ʾ
	void SignalToChildWidget(QList<StrategyChildInfo>);
	
	//����ĸ������
	void updateParentData(StrategyParentInfo*);

	//�����ӵ��ļ���ͼ
	void updateChildPriceVolume();

public slots:

	//void slotBuyOrder(int num, QByteArray arr);

	//void slotSellOrder(int num, QByteArray arr);

	//����״̬���ͺͲ�ѯ
    void slotTradeLogOrder(int, QByteArray);

    void slot_Level2_Recive(int t, unsigned long kw,
	unsigned long sec, QByteArray levelobj);

	void slotParentOrderReceived(int aint,QByteArray arr);

	void slotAddParentInfo(int aint,QByteArray arr);

	void slotDelParentInfo(int aint, QByteArray arr);

	void slotAlterParentInfo(int aint, QByteArray arr);

	void slotCancelOrder(int aint, QByteArray arr);

	//ĳ��ʱ���
	void slotPastLevel2TimeQuantum(int aint, QByteArray arr);

	//ĳ��ʱ���֮��
	void slotPastLevel2TimeAfter(int aint, QByteArray arr);

	//ĳ��ʱ��������
	void slotPastLevel2TimeDot(int aint, QByteArray arr);

	//ĳ��ʱ��εķ�����
	void slotPastPKLineQuantum(int aint, QByteArray arr);

	//ĳ��ʱ��֮��ķ�����
	void slotPastPKLineTimeAfter(int aint, QByteArray arr);

	//ĳ��ʱ���֮��ķ�����
	void slotpastPKLineTimeDot(int aint, QByteArray arr);

	void slotGetServerTime(int aint, QByteArray arr);

public:
	/*********************ĸ���б�******************************/
	// ???
	//����ĸ��id��ֵ��ĸ����Ϣ,
	QMap<QString, StrategyParentInfo*> map_parent_info;

	//����ĸ��id��ֵ��
	QMap<QString, AlgoOrder*> map_algorder;

	//����ĸ��id, ����ָ���ʱ���
	QMap<QString, QList<QDateTime> > time_region;

	//����ĸ��id, ֵ���µ�ʱ��
	QMap<QString, QList<QDateTime> > map_send_time;

	//����ĸ��id,ֵ���������
	QMap<QString, int> time_total_interval;

	//����ĸ��id,ֵ:�ɽ�����
	QMap<QString, int> map_exec_vol;

	//����ĸ��id,ֵ������  �������ļ��ж�ȡ��
	static QMap<QString, QList<QString> > map_read_parent;

// 	//����ĸ��id,ֵ��vwap
// 	QMap<QString, AlgVwap*> map_vwap;

	//����ĸ��id,ֵ��twap
	//QMap<QString, AlgTwap*> map_twap;

	//����ĸ��id,ֵ��pov
	//QMap<QString, AlgPov*> map_pov;

	//pov.  //����parentid,ֵ��stockid
//	QMap<QString, QString> pov_parentid_stockid;

	//testOne 
	//QMap<QString, QString> testone_parentid_stockid;

	//����ĸ��id,ֵ��TestOne
	//QMap<QString, TestOne*> map_test_one;

	//���� ��ֵ��stockid
	//QMap<AlgoOrder*, QString> map_algorder_level2;

	///���� ��ֵ��stockid
	//QMap<AlgoOrder*, QString> map_algorder_sellin;




	/*******************************�ӵ�*******************************************/

	//����ĸ��id,�ӵ�id
	QMap<QString, QMap<QString, StrategyChildInfo> > child_from_parentid;

	//����ĸ�� id,�µ���ţ��ӵ���clientid
	QMap<QString, QMap<int, QString> > map_child_sequence; 

	//�����ӵ�id,ֵ��ĸ��id
	QMap<QString, QString> parent_from_child;

	//����ĸ��  
	QMap<QString, QVector<double> > map_y_market_price;//���¼۸�
	QMap<QString, QVector<double> > map_y_market_avg_price;//�г�����
	QMap<QString, QVector<double> > map_y_exec_price;//�ɽ���
	QMap<QString, QVector<double> > map_y_exec_avg_price;//�ɽ�����
	QMap<QString, QVector<double> > map_y_limit_price;//�޼�

	QMap<QString, QVector<double> > map_y_market_volume;//�г��ɽ���
	QMap<QString, QVector<double> > map_y_algorithm_volume;//�㷨�µ���
	QMap<QString, QVector<double> > map_y_actual_volume;//ʵ�ʳɽ���
	

	/***************************************��ʷ����********************************/	

	//������Ʊid ,ֵ��ʱ�����ڵĳɽ���
	QMap<QString, QVector<int> > volume;

	//������Ʊid,���ڣ�ֵ:������
	QMap<QString, QMap<QDate, QList<KLineData>> > history_kline;


	/********************************ʵʱ��������***********************************8*/

	//������Ʊid,ֵ��true��ʾ�����ˣ��Ͳ��ڶ���
	QMap<QString, bool> is_sub_sellin;

	QMap<QString, bool> is_sub_level2;

 	//������Ʊid,ʱ�䣬ֵ���۸����
 	//QMap<QString, QMap<QTime, double> > map_sell_in_price;
	
	//������Ʊid,ʱ�䣬ֵ���ɽ�����������ʳɽ��� 
	//QMap<QString, QMap<QTime, int> > map_sell_in_volume;

	//��������
	QMap<QString, Level2Obj> map_level2;


	//���
	QSqlDatabase sell_intime_db;
	//��ʷ����������
	QSqlDatabase pk_line_db;
	QList<SellInTime> list_sellin_;
	int sell_in_time_count;
	QDateTime server_time;
	QList<QDate> holiday_list;

	int i = 0;//ceshi

private:
	static QMap<algorithm::Directive, QString> exchageDirMap;/*< ���׷��� */
	/*@breif ��ʼ����̬���� myMap */
	static QMap<algorithm::Directive, QString> initExchageDirMap();
	QSqlDatabase m_sqliteDB;	/**< �û��洢�û�������һЩlog���ݿ� */

public:
	/*@breif ͨ��key��ȡvalue */
	static QString getExchageDirVaule(algorithm::Directive key)
	{
		return exchageDirMap.value(key, QStringLiteral("��ͨ����"));
	}
	/*@breif ͨ��value��ȡkey */
	static algorithm::Directive getExchageDirKey(QString str)
	{
		return exchageDirMap.key(str, algorithm::Directive::GENERAL_BUY);
	}

private:
	void initSqliteDataBase();
};

#endif //ALGDATA_H