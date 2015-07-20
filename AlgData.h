
/*
存放所有的母单的列表，以及每个母单对应的所有子单的信息,行情数据。 一直存在
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

	//卖单，买单，撤单，//没有计算金额是否足够
	//void SellOrder(StrategyChildInfo child_info);

	//void BuyOrder(StrategyChildInfo child_info);

	//订阅行情和逐笔
	void InquireMaketData(QString stockid);

	//订阅行情
	void InquireHangQing(QString stock_id);

	//订阅逐笔
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

	//查询母单或者母单的所有子单
	void InquireParentOrders(QList<QString> list_parent_id);

	void ParentOrderReceived(QJsonObject obj);

	void ChildOrderReceived(QString parent,QJsonObject obj);

	//添加母单
	void AddParentInfo(StrategyParentInfo *info);

	//删除母单
	void DelParentInfo(StrategyParentInfo *info);

	//修改母单
	void ALterParentInfo(StrategyParentInfo *info);


	/* Level2历史行情*/
	void InquireHistoryLevel2(int day, QDateTime start_time,QDateTime end_time);

	//某个时间段Level2历史行情
	void ReqPastLevel2TimeQuantum(int day,QDateTime start_time,QDateTime end_time);

	//某个时间点之后Level2历史行情
	void ReqPastLevel2TimeAfter(int day,QDateTime date_time);

	//某个时间点的请求之后Level2历史行情
	void ReqPastLevel2TimeDot(int day,QDateTime date_time);


	/*分钟线*/
	void ReqHistoryPKLine(QString stockid,QDateTime start_time, QDateTime end_time);

	//某个时间段的分钟线
	void ReqPastPKLineQuantum(QString stockid,QDateTime start_time, QDateTime end_time);

	//某个时间点之后逐笔
	void ReqPastPKLineTimeAfter(int day, QDateTime date_time);

	//某个时间点的请求之后逐笔
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
	//接受到返回的子单，显示
	void SignalToChildWidget(QList<StrategyChildInfo>);
	
	//更新母单进度
	void updateParentData(StrategyParentInfo*);

	//更新子单的价量图
	void updateChildPriceVolume();

public slots:

	//void slotBuyOrder(int num, QByteArray arr);

	//void slotSellOrder(int num, QByteArray arr);

	//订单状态推送和查询
    void slotTradeLogOrder(int, QByteArray);

    void slot_Level2_Recive(int t, unsigned long kw,
	unsigned long sec, QByteArray levelobj);

	void slotParentOrderReceived(int aint,QByteArray arr);

	void slotAddParentInfo(int aint,QByteArray arr);

	void slotDelParentInfo(int aint, QByteArray arr);

	void slotAlterParentInfo(int aint, QByteArray arr);

	void slotCancelOrder(int aint, QByteArray arr);

	//某个时间段
	void slotPastLevel2TimeQuantum(int aint, QByteArray arr);

	//某个时间点之后
	void slotPastLevel2TimeAfter(int aint, QByteArray arr);

	//某个时间点的请求
	void slotPastLevel2TimeDot(int aint, QByteArray arr);

	//某个时间段的分钟线
	void slotPastPKLineQuantum(int aint, QByteArray arr);

	//某个时间之后的分钟线
	void slotPastPKLineTimeAfter(int aint, QByteArray arr);

	//某个时间点之后的分钟线
	void slotpastPKLineTimeDot(int aint, QByteArray arr);

	void slotGetServerTime(int aint, QByteArray arr);

public:
	/*********************母单列表******************************/
	// ???
	//键：母单id，值：母单信息,
	QMap<QString, StrategyParentInfo*> map_parent_info;

	//键：母单id，值：
	QMap<QString, AlgoOrder*> map_algorder;

	//键：母单id, 间隔分割后的时间点
	QMap<QString, QList<QDateTime> > time_region;

	//键：母单id, 值是下单时间
	QMap<QString, QList<QDateTime> > map_send_time;

	//键：母单id,值，区间个数
	QMap<QString, int> time_total_interval;

	//键：母单id,值:成交股数
	QMap<QString, int> map_exec_vol;

	//键：母单id,值：进度  从配置文件中读取的
	static QMap<QString, QList<QString> > map_read_parent;

// 	//键：母单id,值：vwap
// 	QMap<QString, AlgVwap*> map_vwap;

	//键：母单id,值：twap
	//QMap<QString, AlgTwap*> map_twap;

	//键：母单id,值：pov
	//QMap<QString, AlgPov*> map_pov;

	//pov.  //键：parentid,值：stockid
//	QMap<QString, QString> pov_parentid_stockid;

	//testOne 
	//QMap<QString, QString> testone_parentid_stockid;

	//键：母单id,值：TestOne
	//QMap<QString, TestOne*> map_test_one;

	//键： ，值：stockid
	//QMap<AlgoOrder*, QString> map_algorder_level2;

	///键： ，值：stockid
	//QMap<AlgoOrder*, QString> map_algorder_sellin;




	/*******************************子单*******************************************/

	//键：母单id,子单id
	QMap<QString, QMap<QString, StrategyChildInfo> > child_from_parentid;

	//键，母单 id,下单序号，子单的clientid
	QMap<QString, QMap<int, QString> > map_child_sequence; 

	//键：子单id,值，母单id
	QMap<QString, QString> parent_from_child;

	//键：母单  
	QMap<QString, QVector<double> > map_y_market_price;//最新价格
	QMap<QString, QVector<double> > map_y_market_avg_price;//市场均价
	QMap<QString, QVector<double> > map_y_exec_price;//成交价
	QMap<QString, QVector<double> > map_y_exec_avg_price;//成交均价
	QMap<QString, QVector<double> > map_y_limit_price;//限价

	QMap<QString, QVector<double> > map_y_market_volume;//市场成交量
	QMap<QString, QVector<double> > map_y_algorithm_volume;//算法下单量
	QMap<QString, QVector<double> > map_y_actual_volume;//实际成交量
	

	/***************************************历史数据********************************/	

	//键：股票id ,值是时间间隔内的成交量
	QMap<QString, QVector<int> > volume;

	//键，股票id,日期，值:分钟线
	QMap<QString, QMap<QDate, QList<KLineData>> > history_kline;


	/********************************实时行情数据***********************************8*/

	//键：股票id,值，true表示订阅了，就不在订阅
	QMap<QString, bool> is_sub_sellin;

	QMap<QString, bool> is_sub_level2;

 	//键：股票id,时间，值：价格，逐笔
 	//QMap<QString, QMap<QTime, double> > map_sell_in_price;
	
	//键：股票id,时间，值：成交量，保存逐笔成交量 
	//QMap<QString, QMap<QTime, int> > map_sell_in_volume;

	//行情数据
	QMap<QString, Level2Obj> map_level2;


	//逐笔
	QSqlDatabase sell_intime_db;
	//历史分钟线数据
	QSqlDatabase pk_line_db;
	QList<SellInTime> list_sellin_;
	int sell_in_time_count;
	QDateTime server_time;
	QList<QDate> holiday_list;

	int i = 0;//ceshi

private:
	static QMap<algorithm::Directive, QString> exchageDirMap;/*< 交易方向 */
	/*@breif 初始化静态变量 myMap */
	static QMap<algorithm::Directive, QString> initExchageDirMap();
	QSqlDatabase m_sqliteDB;	/**< 用户存储用户操作的一些log数据库 */

public:
	/*@breif 通过key获取value */
	static QString getExchageDirVaule(algorithm::Directive key)
	{
		return exchageDirMap.value(key, QStringLiteral("普通买入"));
	}
	/*@breif 通过value获取key */
	static algorithm::Directive getExchageDirKey(QString str)
	{
		return exchageDirMap.key(str, algorithm::Directive::GENERAL_BUY);
	}

private:
	void initSqliteDataBase();
};

#endif //ALGDATA_H