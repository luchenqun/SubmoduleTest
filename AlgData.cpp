#include "AlgData.h"
#include "algorithm_module.h"
#include "StrategyChildWidget.h"
#include "StrategyWidget.h"
#include "globaldef.h"
#include "sellInTime.h"
#include "time.h"
#include "algorithm_module.h"
#include "AlgorithmConfig.h"
#include "include/constdef.h"

#include <QJsonDocument>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlDriver>
#include <QSettings>
#include <QTextCodec>

QMap<QString, QList<QString> > AlgData::map_read_parent = QMap<QString, QList<QString> >();

AlgData::AlgData()
{
	//KNetpackSubscribeHead(algorithm::SocketPacketHead::BUY_ORDER_RESP, this, SLOT(slotBuyOrder(int, QByteArray)));
	//KNetpackSubscribeHead(algorithm::SocketPacketHead::SELL_ORDER_RESP, this, SLOT(slotSellOrder(int, QByteArray)));
	//KNetpackSubscribeHead(algorithm::SocketPacketHead::ORDER_LOG_RESP, this, SLOT(slotTradeLogOrder(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::INQUIRE_PARENT_ORDER_RESP, this, SLOT(slotParentOrderReceived(int, QByteArray)));
	//KNetpackSubscribeHead(algorithm::INQUIRE_HISTORY_RESP, this, SLOT(slotHistoryReceived(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::ADD_PARENT_ORDER_RESP, this, SLOT(slotAddParentInfo(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::DEL_PARENT_ORDER_RESP, this, SLOT(slotDelParentInfo(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::ALTER_PARENT_ORDER_RESP, this, SLOT(slotAlterParentInfo(int, QByteArray)));

	KNetpackSubscribeHead(algorithm::SocketPacketHead::CANCEL_ORDER_RESP, this, SLOT(slotCancelOrder(int, QByteArray)));

	KNetpackSubscribeHead(algorithm::SocketPacketHead::HISTORY_TIMEQUANTUM_RESP, this, SLOT(slotPastLevel2TimeQuantum(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::HISTORY_TIMEAFTER_RESP, this, SLOT(slotPastLevel2TimeAfter(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::HISTORY_TIMEDOT_RESP, this, SLOT(slotPastLevel2TimeDot(int, QByteArray)));
	KNetpackSubscribeHead(algorithm::SocketPacketHead::GETSERVERTIME_RESP, this, SLOT(slotGetServerTime(int, QByteArray)));

	//KNetpackSubscribeHead(atrade::TRADELOG_RESP, this, SLOT(slotTradeLogOrder(int, QByteArray)));
	//历史分钟线数据
	//KNetpackSubscribeHead(algorithm::HISTORY_PKLLINE_QUANTUM_RESP, this, SLOT(slotPastPKLineQuantum(int, QByteArray)));
	//KNetpackSubscribeHead(algorithm::HISTORY_PKLINE_TIMEAFTER_RESP, this, SLOT(slotPastPKLineTimeAfter(int, QByteArray)));
	//KNetpackSubscribeHead(algorithm::HISTORY_PKLINE_TIMEDOT_RESP, this, SLOT(slotpastPKLineTimeDot(int, QByteArray)));

	DBSellInTime();

	DbPKLine();

	ReadHoliday();

	initSqliteDataBase();

	//sell_in_time_count = 0;

}

void AlgData::DBSellInTime()
{
	QString str_name = QDir::currentPath() + "/AlgData/" + "AlgSellInTime.db";
	if (!QFile::exists(str_name))
	{
		sell_intime_db = QSqlDatabase::addDatabase("QSQLITE", "sellintime");
		QString aa = sell_intime_db.connectionName();

		sell_intime_db.setDatabaseName(str_name);
		if (!sell_intime_db.open())
		{
			qDebug() << sell_intime_db.lastError().text();
		}

		QSqlQuery query(sell_intime_db);
		bool success = query.exec("create table realtimesell(count int primary key,stockid text,timesell text,inttime int,price text,volume int)");
		if (success)
			qDebug() << QObject::tr("create sucsess!\n");
		else
			qDebug() << QObject::tr("create fill!\n");

		sell_in_time_count = 0;
	}
	else
	{
		sell_intime_db = QSqlDatabase::addDatabase("QSQLITE", "sellintime");
		sell_intime_db.setDatabaseName(str_name);
		if (!sell_intime_db.open())
		{
			qDebug() << sell_intime_db.lastError().text();
		}
		QSqlQuery query(sell_intime_db);

		QSettings setting("AlgConfig.ini", QSettings::IniFormat);
		setting.setIniCodec(QTextCodec::codecForName("GBK"));
		setting.beginGroup("DB");
		DbUpdateTimeInfo::date_sell_in_ = setting.value("SellInTime", QString()).toString();
		setting.endGroup();

		if (!DbUpdateTimeInfo::date_sell_in_.isEmpty())
		{
			QDate date_sell_in = QDate::fromString(DbUpdateTimeInfo::date_sell_in_, "yyyy-MM-dd");
			QDate time_cur = QDate::currentDate();
			//if (date_sell_in<server_time.date())
			if (date_sell_in < time_cur)
			{
				query.prepare("delete from realtimesell");
				if (!query.exec())
				{
					qDebug() << "delete filled";
				}

				//保存时间
				setting.beginGroup("DB");
				QString str_sell = time_cur.toString("yyyy-MM-dd");
				setting.setValue("SellInTime", str_sell);
				setting.endGroup();

				sell_in_time_count = 0;
			}
			else
			{
				bool success = query.exec("select * from realtimesell");
				if (!success)
				{
					qDebug() << "filled";
				}
				QSqlDatabase defaultDB = QSqlDatabase::database();
				if (defaultDB.driver()->hasFeature(QSqlDriver::QuerySize))
				{
					sell_in_time_count = query.size();
				}
				else
				{
					query.last();
					if (query.at() < 0)
					{
						sell_in_time_count = 0;
					}
					else
					{
						sell_in_time_count = query.at() + 1;
					}
				}
			}
		}
		else
		{
			//保存时间
			QDate time_cur = QDate::currentDate();
			setting.beginGroup("DB");
			QString str_sell = time_cur.toString("yyyy-MM-dd");
			setting.setValue("SellInTime", str_sell);
			setting.endGroup();

			query.prepare("delete from realtimesell");
			if (!query.exec())
			{
				qDebug() << "delete filled";
			}
			sell_in_time_count = 0;
		}
	}
}

void AlgData::DbPKLine()
{
	QString str_name = QDir::currentPath() + "/AlgData/" + "AlgHisPKLine.db";
	if (!QFile::exists(str_name))
	{
		pk_line_db = QSqlDatabase::addDatabase("QSQLITE", "pkline");

		pk_line_db.setDatabaseName(str_name);
		if (!pk_line_db.open())
		{
			qDebug() << pk_line_db.lastError().text();
		}

		QSqlQuery query(pk_line_db);

		QString str_temp = QString("create table hispkline(stockid text,datepk text,timepk text,")
			+ QString("sestime int,preclose text, open text, close text ,high text,low text,volume int,turnover text)");

		bool success = query.exec(str_temp);
		if (success)
			qDebug() << QObject::tr("create  hispkline sucsess!\n");
		else
			qDebug() << QObject::tr("create  hispkline fill!\n");


		QString str_date = QString("create table pkdatestock(stockid text,datepk text)");

		bool success_date = query.exec(str_date);
		if (success_date)
			qDebug() << QObject::tr("create pkdatestock sucsess!\n");
		else
			qDebug() << QObject::tr("create pkdatestock fill!\n");

	}
	else
	{
		pk_line_db = QSqlDatabase::addDatabase("QSQLITE", "pkline");

		QString aa = pk_line_db.connectionName();

		pk_line_db.setDatabaseName(str_name);
		if (!pk_line_db.open())
		{
			qDebug() << "APK" << pk_line_db.lastError().text();
		}
	}
}

void AlgData::ReadHoliday()
{
	QString  file_path = QDir::currentPath() + "/AlgData/" + "holiday.txt";
	QFile holiday_file(file_path);
	if (holiday_file.open(QIODevice::ReadOnly))
	{
		QTextStream in(&holiday_file);
		while (!in.atEnd())
		{
			QDate date_temp = QDate::fromString(in.readLine(), "yyyy/MM/dd");
			holiday_list.push_back(date_temp);
		}

		holiday_file.close();
	}
}

AlgData::~AlgData()
{
	QList<QString> list_info = map_parent_info.keys();
	for (int i = 0; i < list_info.count(); i++)
	{
		delete map_parent_info.value(list_info.at(i));
		map_parent_info[list_info.at(i)] = nullptr;
	}

	sell_intime_db.close();

	if (m_sqliteDB.isOpen())
	{
		m_sqliteDB.close();
	}
}

void AlgData::Init()
{

}

int AlgData::CaculateTotalInterval(QDateTime start_date_time, QDateTime end_date_time, int ses_divid)
{
	QTime time_eleven = QTime::fromString("11:30:00", "hh:mm:ss");
	QTime time_thirteen = QTime::fromString("13:00:00", "hh:mm:ss");
	QTime time_fifth = QTime::fromString("15:00:00", "hh:mm:ss");

	int total_interval = 0;
	if (start_date_time.time() < time_eleven)
	{
		int inter_one = 0;
		int inter_two = 0;
		if (end_date_time.time() <= time_eleven)
		{
			qint64 t_temp = start_date_time.time().secsTo(end_date_time.time());
			inter_one = t_temp / (ses_divid);//下限
			inter_two = 0;
		}
		else
		{
			qint64 t_temp = start_date_time.time().secsTo(time_eleven);
			inter_one = t_temp / (ses_divid);//下限

			t_temp = time_thirteen.secsTo(end_date_time.time());
			inter_two = t_temp / (ses_divid);
		}
		total_interval = inter_one + inter_two;
	}
	else if (start_date_time.time() >= time_eleven && start_date_time.time() < time_thirteen)
	{
		qint64 ses_interval = time_thirteen.secsTo(end_date_time.time());
		total_interval = ses_interval / (ses_divid);//取下限
	}
	else if (start_date_time.time() >= time_thirteen && start_date_time.time() < time_fifth)
	{
		qint64 ses_interval = start_date_time.secsTo(end_date_time);

		total_interval = ses_interval / (ses_divid);//取下限
	}
	return total_interval;
}

QList<QDateTime> AlgData::CaculateTimeRegion(QDateTime start_date_time,
	QDateTime end_date_time, int ses_divid)
{
	QTime time_eleven = QTime::fromString("11:30:00", "hh:mm:ss");
	QTime time_thirteen = QTime::fromString("13:00:00", "hh:mm:ss");
	QTime time_fifth = QTime::fromString("15:00:00", "hh:mm:ss");

	QList<QDateTime> list_time;
	//int add_ses = info->divid_period.toDouble() * 60;
	if (start_date_time.time() <= time_eleven)
	{
		//结束时间在11:30之前或者在13:00之后，
		if (end_date_time.time() <= time_eleven)
		{
			list_time.push_back(start_date_time);
			QTime time_temp = start_date_time.time();

			time_temp = time_temp.addSecs(ses_divid);
			while (time_temp <= end_date_time.time())
			{
				QDateTime date_time_temp;
				date_time_temp.setDate(start_date_time.date());
				date_time_temp.setTime(time_temp);
				list_time.push_back(date_time_temp);
				time_temp = time_temp.addSecs(ses_divid);
			}
		}
		else
		{
			list_time.push_back(start_date_time);
			QTime time_temp = start_date_time.time();

			time_temp = time_temp.addSecs(ses_divid);
			QDateTime date_temp;
			date_temp.setDate(start_date_time.date());
			while (time_temp <= time_eleven)
			{
				date_temp.setTime(time_temp);
				list_time.push_back(date_temp);
				time_temp = time_temp.addSecs(ses_divid);
			}

			time_temp = time_thirteen;
			date_temp.setTime(time_temp);
			list_time.push_back(date_temp);

			time_temp = time_temp.addSecs(ses_divid);
			while (time_temp <= end_date_time.time())
			{
				date_temp.setTime(time_temp);
				list_time.push_back(date_temp);
				time_temp = time_temp.addSecs(ses_divid);
			}
		}
	}
	else
	{
		QDateTime date_temp;
		date_temp.setDate(start_date_time.date());
		list_time.push_back(start_date_time);
		QTime temp = start_date_time.time();
		temp = temp.addSecs(ses_divid);
		while (temp <= end_date_time.time())
		{
			date_temp.setTime(temp);
			list_time.push_back(date_temp);
			temp = temp.addSecs(ses_divid);
		}
	}
	return list_time;
}



//母子单查询添加和删除
void AlgData::InquireParentOrders(QList<QString> list_parent_id)
{
	////////////////////////////////////////////////////////////////////////////////////////////ceshi
	/* QJsonObject json_oms;
	QJsonArray json_arr;
	json_arr.append("1308530001");
	json_arr.append("1338030001");
	json_oms.insert("parentorderid", json_arr);
	QJsonDocument json_doc;
	json_doc.setObject(json_oms);

	QByteArray arr = json_doc.toJson(QJsonDocument::Compact);

	twd::AsyncPostRequest(30, 101, algorithm::INQUIRE_PARENT_ORDER_RESP, arr);*/
	/////////////////////////////////////////////////////////////////////////////////////////////////cehsi

	QJsonObject json_oms;
	QJsonArray json_arr;

	if (list_parent_id.count() == 1)
	{
		json_arr.append(list_parent_id.at(0));
	}
	else if (list_parent_id.count() > 1)
	{
		for (int i = 0; i < list_parent_id.count(); i++)
		{
			json_arr.append(list_parent_id.at(i));
		}
	}
	json_oms.insert("parentorderid", json_arr);

	QJsonDocument json_doc;
	json_doc.setObject(json_oms);

	QByteArray arr = json_doc.toJson(QJsonDocument::Compact);

	twd::AsyncPostRequest(30, 101, algorithm::SocketPacketHead::INQUIRE_PARENT_ORDER_RESP, arr);
}

void AlgData::slotParentOrderReceived(int aint, QByteArray arr)
{
	QJsonObject json_oms;
	QJsonObject json_obj;
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	json_obj = json_doc.object();
	json_oms = json_obj.value("oms").toObject();

	QJsonArray arr_prent = json_oms.value("parentorderid").toArray();
	QJsonArray arr_ogs = json_obj.value("ogs").toArray();

	if (arr_prent.count() == 0 && arr_ogs.count() != 0)
	{
		ParentOrderReceived(json_obj);
	}
	else if (arr_prent.count() == 1 && arr_ogs.count() != 0)
	{
		QString str_parent = arr_prent.at(0).toString();
		ChildOrderReceived(str_parent, json_obj);
	}
	// 	else
	// 	{
	// 		qDebug() << arr;
	// 	}
}

void AlgData::ParentOrderReceived(QJsonObject obj)
{
	//算法在前台，重新登录之后，需要重新编辑之后再继续执行
	QJsonArray arr_temp = obj.value("ogs").toArray();

	QJsonObject obj_parent;

	//StrategyChildInfo child_info;
	//QJsonObject obj_child;
	QMap<QString, StrategyChildInfo> map_child_info;

	for (int i = 0; i < arr_temp.count(); i++)
	{
		obj_parent = arr_temp.at(i).toObject();
		StrategyParentInfo *info = new StrategyParentInfo;

		info->comd_type = obj_parent.value("commandtype").toString();
		info->comd_num = obj_parent.value("commandnum").toString();

		info->parent_id = obj_parent.value("parentorderid").toString();
		info->strategy_type = obj_parent.value("strategytype").toString();
		info->stock_id = obj_parent.value("code").toString();
		info->stock_name = KGetLocalStockCode()->value(info->stock_id);
		info->side = obj_parent.value("direction").toString();
		info->order_type = obj_parent.value("ordertype").toString();
		QString str_vol = obj_parent.value("volume").toString();
		int int_volume = str_vol.toDouble();
		info->volume = QString::number(int_volume);
		info->price_type = obj_parent.value("pricetype").toString();
		//info->action_type = obj_parent.value("actiontype").toString();

		if (info->price_type == QStringLiteral("市价"))
		{
			info->price = QString::number(0);
		}
		else
		{
			QString str_price = obj_parent.value("price").toString();
			double dou_price = str_price.toDouble();
			info->price = QString("%1").arg(dou_price, 0, 'f', 2);
		}

		info->execute_type = obj_parent.value("executetype").toString();
		info->start_time = obj_parent.value("starttime").toString();
		info->end_time = obj_parent.value("endtime").toString();
		info->divid_period = obj_parent.value("period").toString();
		info->history_day = obj_parent.value("historyday").toString();

		info->max_vol = obj_parent.value("maxvol").toString();
		info->participation_ratio = obj_parent.value("participation").toString();
		//info->initiative = obj_parent.value("initiative").toString();

		//double dou_rogress = obj_parent.value("progress").toDouble();
		//info->progress = QString("%1").arg(dou_rogress, 0, 'f', 2);
		QList<QString> list_parent = map_read_parent.value(info->parent_id);
		if (list_parent.count() == 6)
		{
			info->exec_vol = list_parent.at(1);
			info->progress = list_parent.at(2);
			info->exec_avg_price = list_parent.at(3);
			info->order_type = list_parent.at(4);
			info->initiative = list_parent.at(5);
		}
		info->status = QStringLiteral("终止");
		//info->timer = new QTimer;
		//info->timer = nullptr;

		info->float_region = obj_parent.value("floatregion").toString();
		info->split_percent = obj_parent.value("splitpercent").toString();
		info->order_stay_time = obj_parent.value("orderstaytime").toString();
		info->uper_price = obj_parent.value("uperprice").toString();
		info->scan_region = obj_parent.value("scanregion").toString();
		info->constant_price = obj_parent.value("constantprice").toString();
		info->mid_price = obj_parent.value("midprice").toString();
		info->ladder_def = obj_parent.value("ladderdef").toString();
		info->ascend_percent = obj_parent.value("ascendpercent").toString();
		info->descend_percent = obj_parent.value("descendpercent").toString();

		map_parent_info.insert(info->parent_id, info);

		if (StrategyWidget::GetInstance() != nullptr)
		{
			StrategyWidget::GetInstance()->addToTable(info);
		}

		/*	int sum_child_vol = 0;
		double sum_price = 0;

		QJsonArray arr_child = obj_parent.value("p").toArray();
		for (int j = 0; j < arr_child.count(); j++)
		{
		obj_child = arr_child.at(j).toObject();
		child_info.client_id = obj_child.value("clientorderid").toString();
		child_info.stock_id = obj_child.value("code").toString();
		child_info.stock_name = KGetLocalStockCode->value(child_info.stock_id, QString(""));
		child_info.side = obj_child.value("direction").toString();
		child_info.send_price = obj_child.value("price").toString();
		child_info.executed_price = obj_child.value("executedprice").toString();
		child_info.send_volume = obj_child.value("volume").toString();
		child_info.executed_volume = obj_child.value("executedvolume").toString();
		child_info.commission_time = obj_child.value("commissiontime").toString();
		child_info.status = obj_child.value("status").toString();
		child_info.msg = obj_child.value("msg").toString();
		child_info.sysorderid = obj_child.value("sysorderid").toString();

		map_child_info.insert(child_info.client_id, child_info);
		sum_child_vol += child_info.executed_volume.toInt();

		if (child_info.status.toInt() == algorithm::FILLED
		|| child_info.status.toInt() == algorithm::PART_EXECUTED_WAIT_CANCEL
		|| child_info.status.toInt() == algorithm::PART_CANCELED
		|| child_info.status.toInt() == algorithm::PART_FILLED)
		{
		sum_price += child_info.executed_price.toDouble() * child_info.executed_volume.toInt();
		}
		}
		child_from_parentid.insert(info->parent_id, map_child_info);

		double dou_price = sum_price / sum_child_vol;
		info->exec_avg_price = QString("%1").arg(dou_price, 0, 'f', 2);
		double dou_progress = (sum_child_vol * 100) / info->volume.toDouble();

		if (dou_progress < 0.01)
		{
		dou_progress = 0.01;
		}

		info->progress = QString("%1").arg(dou_progress, 0, 'f', 2);
		//map_parent_info.insert(info->parent_id, info);
		*/
		//info->timer->start(100);
		//connect(info->timer, SIGNAL(timeout()), info, SLOT(slotTimer()));
	}
}

void AlgData::ChildOrderReceived(QString parent, QJsonObject obj)
{
	if (parent.isEmpty() || !map_parent_info.contains(parent))
		return;

	QJsonArray child_array = obj.value("ogs").toArray();
	StrategyChildInfo child_info;

	QJsonObject child_obj;
	QMap<QString, StrategyChildInfo> map_child = child_from_parentid.value(parent);
	QList<StrategyChildInfo> list_child;

	StrategyParentInfo *parent_info = map_parent_info.value(parent);
	double sum_price = 0;
	int sum_vol = 0;

	for (int i = 0; i < child_array.count(); i++)
	{
		child_obj = child_array.at(i).toObject();
		child_info.parent_id = parent;
		child_info.client_id = child_obj.value("clientorderid").toString();
		if (child_info.parent_id.isEmpty() || child_info.client_id.isEmpty())
		{
			continue;
		}

		child_info.stock_id = child_obj.value("cid").toString();
		child_info.stock_name = KGetLocalStockCode()->value(child_info.stock_id);
		QString str_dire = child_obj.value("direction").toString();
		if (str_dire == "buy")
		{
			child_info.side = QStringLiteral("买");
		}
		else if (str_dire == "sell")
		{
			child_info.side = QStringLiteral("卖");
		}

		child_info.order_type_ = map_parent_info.value(parent)->order_type;
		double dou_price = child_obj.value("price").toString().toDouble();
		child_info.send_price = QString("%1").arg(dou_price, 0, 'f', 2);

		double dou_deal_price = child_obj.value("dealprice").toString().toDouble();
		child_info.executed_price = QString("%1").arg(dou_deal_price, 0, 'f', 2);

		int int_send_vol = int(child_obj.value("volume").toString().toDouble());
		child_info.send_volume = QString::number(int_send_vol);

		int int_exec_vol = int(child_obj.value("dealvolume").toString().toDouble());
		child_info.executed_volume = QString::number(int_exec_vol);

		QDateTime date_time = QDateTime::fromString(child_obj.value("ordertime").toString(), "yyyy-MM-dd hh:mm:ss");
		//child_info.commission_time = date_time.time().toString("hh:mm:ss");
		child_info.trading_datetime = date_time.time().toString("hh:mm:ss");

		child_info.status = child_obj.value("orderstatus").toString();
		child_info.sysorderid = child_obj.value("borderid").toString();//sysorderid

		double dou_pro = child_info.executed_volume.toDouble() / map_parent_info.value(parent)->volume.toDouble();
		child_info.proportion = QString("%1").arg(dou_pro * 100, 0, 'f', 2) + "%";

		//child_info.msg = child_obj.value("msg").toString();

		map_child.insert(child_info.client_id, child_info);
		list_child.push_back(child_info);

		sum_vol += child_info.executed_volume.toInt();
		if (child_info.status.toInt() == algorithm::OrderStatus::FILLED
			|| child_info.status.toInt() == algorithm::OrderStatus::PART_EXECUTED_WAIT_CANCEL
			|| child_info.status.toInt() == algorithm::OrderStatus::PART_CANCELED
			|| child_info.status.toInt() == algorithm::OrderStatus::PART_FILLED)
		{
			sum_price += child_info.executed_price.toDouble() * child_info.executed_volume.toInt();
		}
	}

	child_from_parentid.insert(parent, map_child);

	StrategyChildWidget *child_widget_vol =
		StrategyChildWidget::widgets.value(parent, nullptr);
	if (child_widget_vol != nullptr)
	{
		child_widget_vol->setChildTable(list_child);
	}

	double dou_price = 0.0;

	if (sum_vol != 0)
	{
		dou_price = sum_price / sum_vol;
		parent_info->exec_avg_price = QString("%1").arg(dou_price, 0, 'f', 2);

		double dou_progress = (sum_vol * 100) / parent_info->volume.toDouble();
		if (dou_progress < 0.01)
		{
			dou_progress = 0.01;
		}
		parent_info->progress = QString("%1").arg(dou_progress, 0, 'f', 2);
	}
	else
	{
		parent_info->exec_avg_price = QString::number(0.00);
		parent_info->progress = QString::number(0.00);
	}

	parent_info->exec_vol = QString::number(sum_vol);

	//map_parent_info.insert(parent_info->parent_id, parent_info);

	//更新母单		
	if (StrategyWidget::GetInstance() != nullptr)
	{
		connect(this, SIGNAL(updateParentData(StrategyParentInfo*)),
			StrategyWidget::GetInstance(), SLOT(updateParentInfo(StrategyParentInfo*)));
		emit updateParentData(parent_info);
	}
}

void AlgData::AddParentInfo(StrategyParentInfo *info)
{
	QJsonObject json_oms;
	json_oms.insert("commandtype", info->comd_type);
	json_oms.insert("commandnum", info->comd_num);
	json_oms.insert("parentorderid", info->parent_id);
	json_oms.insert("code", info->stock_id);
	json_oms.insert("direction", info->side);
	json_oms.insert("volume", info->volume);
	json_oms.insert("strategytype", info->strategy_type);
	json_oms.insert("pricetype", info->price_type);
	json_oms.insert("actiontype", "");
	json_oms.insert("price", info->price);

	json_oms.insert("starttime", info->start_time);
	json_oms.insert("endtime", info->end_time);
	json_oms.insert("period", info->divid_period);
	json_oms.insert("executetype", info->execute_type);
	json_oms.insert("historyday", info->history_day);
	json_oms.insert("maxvol", info->max_vol);
	json_oms.insert("participation", info->participation_ratio);
	json_oms.insert("progress", 0.00);
	json_oms.insert("state", info->status);
	json_oms.insert("msg", info->msg);

	json_oms.insert("floatregion", info->float_region);
	json_oms.insert("splitpercent", info->split_percent);
	json_oms.insert("orderstaytime", info->order_stay_time);
	json_oms.insert("uperprice", info->uper_price);
	json_oms.insert("scanregion", info->scan_region);
	json_oms.insert("constantprice", info->constant_price);
	json_oms.insert("midprice", info->mid_price);
	json_oms.insert("ladderdef", info->ladder_def);
	json_oms.insert("ascendpercent", info->ascend_percent);
	json_oms.insert("descendpercent", info->descend_percent);
	json_oms.insert("ordertype", info->order_type);
	json_oms.insert("initiative", info->initiative);

	QJsonDocument json_doc;
	json_doc.setObject(json_oms);

	QByteArray arr = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequest(30, 106, algorithm::SocketPacketHead::ADD_PARENT_ORDER_RESP, arr);
}

void AlgData::slotAddParentInfo(int aint, QByteArray arr)
{
	QJsonObject json_obj;
	QJsonParseError json_error;

	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);
	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	json_obj = json_doc.object();

	QJsonObject json_ogs = json_obj.value("ogs").toObject();
	int str_flag = json_ogs.value("flag").toInt();
	QString parent_id = json_obj.value("oms").toObject().value("parentorderid").toString();

	if (parent_id.isEmpty())
		return;

	StrategyParentInfo *parent_info = map_parent_info.value(parent_id);
	if (str_flag == 0)
	{
		//添加成功到数据库 
		parent_info->msg = QStringLiteral("添加成功");
	}
	else
	{
		parent_info->msg = QStringLiteral("添加失败");
		parent_info->status = QStringLiteral("未执行");
	}

	//更新
	if (StrategyWidget::GetInstance() != nullptr)
	{
		StrategyWidget::GetInstance()->updateParentInfo(parent_info);
	}
}

void AlgData::DelParentInfo(StrategyParentInfo *info)
{
	QJsonObject json_oms;
	json_oms.insert("parentorderid", info->parent_id);

	QJsonDocument json_doc;
	json_doc.setObject(json_oms);

	QByteArray arr = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequest(30, 111, algorithm::SocketPacketHead::DEL_PARENT_ORDER_RESP, arr);
}

void AlgData::slotDelParentInfo(int aint, QByteArray arr)
{
	QJsonObject json_obj;
	QJsonParseError json_error;

	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);
	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	json_obj = json_doc.object();
	int flag = json_obj.value("ogs").toObject().value("flag").toInt();
	QString str_parent = json_obj.value("oms").toObject().value("parentorderid").toString();

	if (flag == 0)//删除成功
	{
		if (StrategyWidget::GetInstance() != nullptr)
		{
			StrategyWidget::GetInstance()->DelParentSuccess(str_parent);
		}

		map_parent_info.remove(str_parent);
		time_region.remove(str_parent);
		map_send_time.remove(str_parent);
		time_total_interval.remove(str_parent);
		map_exec_vol.remove(str_parent);
		map_read_parent.remove(str_parent);

		child_from_parentid.remove(str_parent);

		QList<QString> list_str;
		QList<QString> list_keys = parent_from_child.keys();
		for (int i = 0; i < list_keys.count(); i++)
		{
			if (parent_from_child.value(list_keys.at(i)) == str_parent)
			{
				list_str.push_back(list_keys.at(i));
			}
		}
		for (int m = 0; m < list_str.count(); m++)
		{
			parent_from_child.remove(list_str.at(m));
		}
		map_child_sequence.remove(str_parent);

		map_y_market_price.remove(str_parent);
		map_y_market_avg_price.remove(str_parent);
		map_y_exec_price.remove(str_parent);
		map_y_limit_price.remove(str_parent);

		map_y_market_volume.remove(str_parent);
		map_y_algorithm_volume.remove(str_parent);
		map_y_actual_volume.remove(str_parent);
	}
	else
	{
		StrategyParentInfo *parent_info = map_parent_info.value(str_parent, nullptr);
		if (parent_info != nullptr)
		{
			parent_info->status = QStringLiteral("终止");
			parent_info->msg = QStringLiteral("删除失败");
			//更新
			if (StrategyWidget::GetInstance() != nullptr)
			{
				StrategyWidget::GetInstance()->updateParentInfo(parent_info);
			}
		}
	}
}

void AlgData::ALterParentInfo(StrategyParentInfo *info)
{
	QJsonObject json_oms;
	json_oms.insert("commandtype", info->comd_type);
	json_oms.insert("commandnum", info->comd_num);
	json_oms.insert("parentorderid", info->parent_id);
	json_oms.insert("code", info->stock_id);
	json_oms.insert("direction", info->side);
	json_oms.insert("volume", info->volume);
	json_oms.insert("strategytype", info->strategy_type);
	json_oms.insert("pricetype", info->price_type);
	json_oms.insert("actiontype", "");
	json_oms.insert("price", info->price);

	json_oms.insert("starttime", info->start_time);
	json_oms.insert("endtime", info->end_time);
	json_oms.insert("period", info->divid_period);
	json_oms.insert("executetype", info->execute_type);
	json_oms.insert("historyday", info->history_day);
	json_oms.insert("maxvol", info->max_vol);
	json_oms.insert("participation", info->participation_ratio);
	json_oms.insert("state", info->status);
	json_oms.insert("msg", info->msg);

	json_oms.insert("floatregion", info->float_region);
	json_oms.insert("splitpercent", info->split_percent);
	json_oms.insert("orderstaytime", info->order_stay_time);
	json_oms.insert("uperprice", info->uper_price);
	json_oms.insert("scanregion", info->scan_region);
	json_oms.insert("constantprice", info->constant_price);
	json_oms.insert("midprice", info->mid_price);
	json_oms.insert("ladderdef", info->ladder_def);
	json_oms.insert("ascendpercent", info->ascend_percent);
	json_oms.insert("descendpercent", info->descend_percent);
	json_oms.insert("ordertype", info->order_type);
	json_oms.insert("initiative", info->initiative);

	QJsonDocument json_doc;
	json_doc.setObject(json_oms);

	QByteArray arr = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequest(30, 116, algorithm::SocketPacketHead::ALTER_PARENT_ORDER_RESP, arr);
}

void AlgData::slotAlterParentInfo(int aint, QByteArray arr)
{
	QJsonParseError json_error;

	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);
	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();
	QJsonObject json_oms = json_obj.value("oms").toObject();
	QJsonObject json_ogs = json_obj.value("ogs").toObject();

	int flag = json_ogs.value("flag").toInt();
	QString parent_id = json_oms.value("parentorderid").toString();
	StrategyParentInfo *new_parent_info = map_parent_info.value(parent_id);

	if (flag == 0)//修改成功
	{
		new_parent_info->msg = QStringLiteral("修改成功");

		// 		new_parent_info->comd_type = json_oms.value("commandtype").toString();
		// 		new_parent_info->comd_num = json_oms.value("commandnum").toString();
		// 		new_parent_info->parent_id = json_oms.value("parentorderid").toString();
		// 		new_parent_info->strategy_type = json_oms.value("strategytype").toString();
		// 		new_parent_info->stock_id = json_oms.value("code").toString();
		//      new_parent_info->stock_name = KGetLocal;;
		// 		new_parent_info->side = json_oms.value("direction").toString();
		// 		QString str_vol = json_oms.value("volume").toString();
		// 		int int_volume = str_vol.toDouble();
		// 		new_parent_info->volume = QString::number(int_volume);
		// 		new_parent_info->price_type = json_oms.value("pricetype").toString();
		//new_parent_info->action_type = obj_parent.value("actiontype").toString();
		// 		if (new_parent_info->price_type == QStringLiteral("市价"))
		// 		{
		// 			new_parent_info->price = QString::number(0);
		// 		}
		// 		else
		// 		{
		// 			QString str_price = json_oms.value("price").toString();
		// 			double dou_price = str_price.toDouble();
		// 			new_parent_info->price = QString("%1").arg(dou_price, 0, 'f', 2);
		// 		}
		// 		//new_parent_info->execute_type = obj_parent.value("executetype").toString();
		// 		new_parent_info->start_time = json_oms.value("starttime").toString();
		// 		new_parent_info->end_time = json_oms.value("endtime").toString();
		// 		new_parent_info->divid_period = json_oms.value("period").toString();
		// 		new_parent_info->history_day = json_oms.value("historyday").toString();
		// 		new_parent_info->max_proportion = json_oms.value("maxproportion").toString();
		// 		new_parent_info->participation_ratio = json_oms.value("participation").toString();
	}
	else
	{
		new_parent_info->msg = QStringLiteral("修改失败");
	}

	if (StrategyWidget::GetInstance() != nullptr)
	{
		StrategyWidget::GetInstance()->updateParentInfo(new_parent_info);
	}
}



//查询历史Level2
void AlgData::InquireHistoryLevel2(int day, QDateTime start_time, QDateTime end_time)
{
	//判断本地数据库中数据，只请求缺少的部分

	//ReqPastLevel2TimeQuantum(day, start_time, end_time);
	//ReqPastLevel2TimeAfter(day, start_time);
	ReqPastLevel2TimeDot(day, start_time);
}

void AlgData::ReqPastLevel2TimeQuantum(int day, QDateTime start_time, QDateTime end_time)
{
	//{head,QueryType,DataType,BeginDate,BeginTime,EndDate,EndTime,Stock_id,Market_id}
	QJsonObject obj;
	obj.insert("head", algorithm::SocketPacketHead::HISTORY_TIMEQUANTUM_RESP);
	int query = 1;
	obj.insert("QueryType", query);
	int data_type = 83;
	obj.insert("DataType", data_type);
	int begin_date = 20150206;
	obj.insert("BeginDate", begin_date);
	int begin_time = 93000000;
	obj.insert("BeginTime", begin_time);
	int end_date = 20150206;
	obj.insert("EndDate", end_date);
	int int_end_time = 113000000;
	obj.insert("EndTime", int_end_time);
	int stock_id = 600446;
	obj.insert("Stock_id", stock_id);
	QString str_market = "SH";
	obj.insert("Market_id", str_market);

	QJsonDocument json_doc;
	json_doc.setObject(obj);

	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequestToHangQingServer(40, 81, algorithm::SocketPacketHead::HISTORY_TIMEQUANTUM_RESP, arr_send);
}

void AlgData::slotPastLevel2TimeQuantum(int aint, QByteArray arr)
{
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();

	// 	QString str_path = QString("D:\TWD\twd\bin\level.txt");
	// 	QFile level_file(str_path);
	// 
	// 	if (level_file.open(QIODevice::WriteOnly))
	// 	{
	// 		QTextStream out;
	// 		out << arr << "\n";
	// 	}
	// 	level_file.close();
	//QString str_head = json_obj.value("head")

}

void AlgData::ReqPastLevel2TimeAfter(int day, QDateTime date_time)
{
	//{head,QueryType,DateType,Date,Time,Stock_id,Market_id}
	QJsonObject obj;
	obj.insert("head", algorithm::SocketPacketHead::HISTORY_TIMEAFTER_RESP);
	int query = 2;
	obj.insert("QueryType", query);
	int data_type = 83;
	obj.insert("DataType", data_type);

	int begin_date = 20150213;
	obj.insert("Date", begin_date);
	int begin_time = 140000000;
	obj.insert("Time", begin_time);

	int stock_id = 600446;
	obj.insert("Stock_id", stock_id);
	QString str_market = "SH";
	obj.insert("Market_id", str_market);

	QJsonDocument json_doc;
	json_doc.setObject(obj);

	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequestToHangQingServer(40, 91, algorithm::SocketPacketHead::HISTORY_TIMEQUANTUM_RESP, arr_send);
}

void AlgData::slotPastLevel2TimeAfter(int aint, QByteArray arr)
{
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();
}

void AlgData::ReqPastLevel2TimeDot(int day, QDateTime date_time)
{
	//{head, QueryType, DateType, Date, Time, Stock_id, Market_id}
	QJsonObject obj;
	obj.insert("head", algorithm::SocketPacketHead::HISTORY_TIMEDOT_RESP);
	int query = 3;
	obj.insert("QueryType", query);
	int data_type = 83;
	obj.insert("DataType", data_type);

	int begin_date = 20150213;
	obj.insert("Date", begin_date);
	int begin_time = 140006605;
	obj.insert("Time", begin_time);

	int stock_id = 600446;
	obj.insert("Stock_id", stock_id);
	QString str_market = "SH";
	obj.insert("Market_id", str_market);

	QJsonDocument json_doc;
	json_doc.setObject(obj);

	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequestToHangQingServer(40, 91, algorithm::SocketPacketHead::HISTORY_TIMEQUANTUM_RESP, arr_send);
}

void AlgData::slotPastLevel2TimeDot(int aint, QByteArray arr)
{
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();
}


//查询历史分钟线数据
void AlgData::ReqHistoryPKLine(QString stockid, QDateTime start_time, QDateTime end_time)
{
	//判断本地数据库中数据，只请求缺少的部分
	//ReqPastPKLineQuantum(day, start_time, end_time);
	ReqPastPKLineQuantum(stockid, start_time, end_time);
}

void AlgData::ReqPastPKLineQuantum(QString stockid, QDateTime start_time, QDateTime end_time)
{
	//{Seqno, Head, QueryType, DataType, BeginDate, BeginTime, EndDate, EndTime, Stock_id, Market_id}
	QJsonObject obj;
	obj.insert("Seqno", 100);
	obj.insert("head", algorithm::SocketPacketHead::HISTORY_PKLLINE_QUANTUM_RESP);
	int query = 1;
	obj.insert("QueryType", query);
	int data_type = 51;
	obj.insert("DataType", data_type);

	//QString str_start = start_time.date().toString("yyyyMMdd");
	//obj.insert("BeginDate",str_start.toInt());
	//QString str_start_time = start_time.time().toString("hhmmsszzz");
	//obj.insert("BeginTime", str_start_time.toInt());
	//QString str_end = end_time.date().toString("yyyyMMdd");
	//obj.insert("EndDate", str_end.toInt());
	//QString str_end_time = start_time.time().toString("hhmmsszzz");
	//obj.insert("EndTime", str_end_time.toInt());
	//obj.insert("Stock_id", stockid);
	//QString market = GetMarketType(stockid);
	//obj.insert("Market_id", market);


	int begin_date = 20150306;
	obj.insert("BeginDate", begin_date);
	int begin_time = 93000000;
	obj.insert("BeginTime", begin_time);
	int end_date = 20150306;
	obj.insert("EndDate", end_date);
	int int_end_time = 150000000;
	obj.insert("EndTime", int_end_time);
	int stock_id = 600446;
	obj.insert("Stock_id", stock_id);
	QString str_market = "SH";
	obj.insert("Market_id", str_market);


	QJsonDocument json_doc;
	json_doc.setObject(obj);
	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequestToHangQingServer(40, 81, algorithm::SocketPacketHead::HISTORY_PKLLINE_QUANTUM_RESP, arr_send);

}

void AlgData::slotPastPKLineQuantum(int aint, QByteArray arr)
{
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);
	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();
	QJsonArray json_array = json_obj.value("SbeData").toArray();

	QJsonObject json_temp;
	QString str_stockid;
	QTime time_nine = QTime::fromString("09:30:00", "hh:mm:ss");
	for (int i = 0; i < json_array.count(); i++)
	{
		json_temp = json_array.at(i).toObject();

		KLineData kl_data;
		if (json_temp.value("Code").toString().isEmpty())
		{
			continue;
		}
		kl_data.stockid = json_temp.value("Code").toString().split(".").first();
		str_stockid = kl_data.stockid;

		QString str_aa = json_temp.value("Date").toString();
		QDate date_temp = QDate::fromString(json_temp.value("Date").toString(), "yyyyMMdd");
		QString zz1 = date_temp.toString("yyyyMMdd");
		kl_data.date = date_temp;

		QString str_cc = json_temp.value("Time").toString();
		QTime time_temp = QTime::fromString(json_temp.value("Time").toString(), "hhmmsszzz");
		QString zz2 = time_temp.toString("hh:mm:ss");
		kl_data.time = QTime::fromString(time_temp.toString("hh:mm:ss"), "hh:mm:ss");

		int ses_diff = (kl_data.time.hour() - time_nine.hour()) * 60 * 60
			+ (kl_data.time.minute() - time_nine.minute()) * 60
			+ (kl_data.time.second() - time_nine.second());
		kl_data.sestime = ses_diff;

		QString str_pre_close = json_temp.value("PreClose").toString();
		double dou_pre_close = str_pre_close.toDouble() / 10000;
		kl_data.pre_close = QString("%1").arg(dou_pre_close, 0, 'f', 2);

		QString str_open = json_temp.value("Open").toString();
		double dou_open = str_open.toDouble() / 10000;
		kl_data.open = QString("%1").arg(dou_open, 0, 'f', 2);

		QString str_close = json_temp.value("Close").toString();
		double dou_close = str_close.toDouble() / 10000;
		kl_data.close = QString("%1").arg(dou_close, 0, 'f', 2);

		QString str_high = json_temp.value("High").toString();
		double dou_high = str_high.toDouble() / 10000;
		kl_data.high = QString("%1").arg(dou_high, 0, 'f', 2);

		QString str_low = json_temp.value("Low").toString();
		double dou_low = str_low.toDouble() / 10000;
		kl_data.low = QString("%1").arg(dou_low, 0, 'f', 2);

		kl_data.volume = json_temp.value("Volume").toString().toInt();

		QString str_turn_over = json_temp.value("Turnover").toString();
		double dou_turn = str_turn_over.toDouble() / 10000;
		kl_data.turn_over = QString("%1").arg(dou_turn, 0, 'f', 2);

		QList<KLineData> list_kline = history_kline.value(kl_data.stockid).value(kl_data.date);
		list_kline.push_back(kl_data);

		QMap<QDate, QList<KLineData>> map_kl = history_kline.value(kl_data.stockid);
		map_kl.insert(kl_data.date, list_kline);

		history_kline.insert(kl_data.stockid, map_kl);
	}

	int int_last = json_obj.value("IsLast").toString().toInt();
	if (int_last == 89)
	{
		QSqlQuery query(pk_line_db);
		query.prepare("insert into hispkline(stockid,datepk,timepk,sestime,preclose,open,close,high,low,volume,turnover)"
			"values (:stockid, :datepk,:timepk,:sestime,:preclose,:open,:close,:high,:low,:volume,:turnover)");
		pk_line_db.transaction();

		QMap<QDate, QList<KLineData>> map_kl_temp = history_kline.value(str_stockid);
		QMap<QDate, QList<KLineData>>::const_iterator begin = map_kl_temp.begin();
		QMap<QDate, QList<KLineData>>::const_iterator end = map_kl_temp.end();

		while (begin != end)
		{
			QList<KLineData> list_kl_temp = begin.value();
			for (int j = 0; j < list_kl_temp.count(); j++)
			{
				KLineData line_data = list_kl_temp.at(j);
				query.bindValue(":stockid", line_data.stockid);
				QString str_date = line_data.date.toString("yyyy-MM-dd");
				query.bindValue(":datepk", str_date);
				QString str_time = line_data.time.toString("hh:mm:ss");
				query.bindValue(":timepk", str_time);

				int ses_diff = (line_data.time.hour() - time_nine.hour()) * 60 * 60
					+ (line_data.time.minute() - time_nine.minute()) * 60
					+ (line_data.time.second() - time_nine.second());

				query.bindValue(":sestime", ses_diff);
				query.bindValue(":preclose", line_data.pre_close);
				query.bindValue(":open", line_data.open);
				query.bindValue("close", line_data.close);
				query.bindValue("high", line_data.high);
				query.bindValue("low", line_data.low);
				query.bindValue(":volume", line_data.volume);
				query.bindValue(":turnover", line_data.turn_over);

				bool success = query.exec();
				if (!success)
				{
					pk_line_db.rollback();
					QSqlError lastError = query.lastError();
					qDebug() << lastError.driverText() << QStringLiteral("插入失败");
					return;
				}
			}

			begin++;
		}
		pk_line_db.commit();


		QSqlQuery query_date(pk_line_db);
		query_date.prepare("insert into pkdatestock values(?,?)");
		pk_line_db.transaction();

		QMap<QDate, QList<KLineData>> map_kl_date = history_kline.value(str_stockid);
		QList<QDate> list_date = map_kl_date.keys();

		for (int i = 0; i < list_date.count(); i++)
		{
			query_date.bindValue(0, str_stockid);

			QString str_date = list_date.at(i).toString("yyyy-MM-dd");
			query_date.bindValue(1, str_date);

			if (!query_date.exec())
			{
				pk_line_db.rollback();
				QSqlError lastError = query_date.lastError();
				qDebug() << lastError.driverText() << QStringLiteral("插入失败");
				return;
			}
		}
		pk_line_db.commit();
	}
}

void AlgData::ReqPastPKLineTimeAfter(int day, QDateTime date_time)
{
	//{Seqno, Head, QueryType, DataType, BeginDate, BeginTime, EndDate, EndTime, Stock_id, Market_id}
	QJsonObject obj;
	obj.insert("Seqno", 100);
	obj.insert("head", algorithm::SocketPacketHead::HISTORY_PKLINE_TIMEAFTER_RESP);
	int query = 2;
	obj.insert("QueryType", query);
	int data_type = 51;
	obj.insert("DataType", data_type);
	int begin_date = 20150306;
	obj.insert("BeginDate", begin_date);
	int begin_time = 93000000;
	obj.insert("BeginTime", begin_time);
	int end_date = 20150306;
	obj.insert("EndDate", end_date);
	int int_end_time = -1;
	obj.insert("EndTime", int_end_time);
	int stock_id = 600446;
	obj.insert("Stock_id", stock_id);
	QString str_market = "SH";
	obj.insert("Market_id", str_market);

	QJsonDocument json_doc;
	json_doc.setObject(obj);

	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequestToHangQingServer(40, 81, algorithm::SocketPacketHead::HISTORY_PKLINE_TIMEAFTER_RESP, arr_send);
}

void AlgData::slotPastPKLineTimeAfter(int aint, QByteArray arr)
{
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();


}

void AlgData::ReqPastPKLineTimeDot(int day, QDateTime date_time)
{
	//{Seqno, Head, QueryType, DataType, BeginDate, BeginTime, EndDate, EndTime, Stock_id, Market_id}
	QJsonObject obj;
	obj.insert("Seqno", 100);
	obj.insert("head", algorithm::SocketPacketHead::HISTORY_PKLINE_TIMEDOT_RESP);
	int query = 3;
	obj.insert("QueryType", query);
	int data_type = 51;
	obj.insert("DataType", data_type);
	int begin_date = 20150306;
	obj.insert("BeginDate", begin_date);
	int begin_time = 93000000;
	obj.insert("BeginTime", begin_time);
	int end_date = -1;
	obj.insert("EndDate", end_date);
	int int_end_time = -1;
	obj.insert("EndTime", int_end_time);
	int stock_id = 600446;
	obj.insert("Stock_id", stock_id);
	QString str_market = "SH";
	obj.insert("Market_id", str_market);

	QJsonDocument json_doc;
	json_doc.setObject(obj);

	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);
	twd::AsyncPostRequestToHangQingServer(40, 81, algorithm::HISTORY_PKLINE_TIMEDOT_RESP, arr_send);
}

void AlgData::slotpastPKLineTimeDot(int aint, QByteArray arr)
{
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_obj = json_doc.object();

}


//查询服务器时间
void AlgData::ReqServerTime()
{
	QJsonObject obj;
	//obj.insert("time", QString());
	QJsonDocument doc;
	doc.setObject(obj);
	QByteArray arr_send = doc.toJson(QJsonDocument::Compact);

	//twd::AsyncPostRequest(25, 186, algorithm::GETSERVERTIME_RESP, arr_send);//oms未定
}

void AlgData::slotGetServerTime(int aint, QByteArray arr)
{
	if (arr.isEmpty())
		return;

	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_oms = json_doc.object().value("oms").toObject();
	QString str_time = json_oms.value("time").toString();
	QDateTime date_time = QDateTime::fromString(str_time, "yyyy-MM-dd hh:mm:ss");
	if (date_time.isValid())
	{
		server_time = date_time;
	}
}



int AlgData::GetEnumFromTypeName(QString directive)
{
	int i_d = 0;
	if (directive == QStringLiteral("普通买入"))
	{
		i_d = algorithm::Directive::GENERAL_BUY;
	}
	else if (directive == QStringLiteral("普通卖出"))
	{
		i_d = algorithm::Directive::GENERAL_SELL;
	}
	else if (directive == QStringLiteral("担保品买入"))
	{
		i_d = algorithm::Directive::DANBAOPIN_BUY;
	}
	else if (directive == QStringLiteral("担保品卖出"))
	{
		i_d = algorithm::Directive::DANBAOPIN_SELL;
	}
	else if (directive == QStringLiteral("买券还券"))
	{
		i_d = algorithm::Directive::BUYQUAN_HUANQUAN;
	}
	else if (directive == QStringLiteral("融券卖出"))
	{
		i_d = algorithm::Directive::RONG_QUAN_SELL;
	}
	else if (directive == QStringLiteral("卖券还款"))
	{
		i_d = algorithm::Directive::SELLQUAN_HUANKUAN;
	}
	else if (directive == QStringLiteral("融资买入"))
	{
		i_d = algorithm::Directive::RONGZI_BUY;
	}
	else if (directive == QStringLiteral("预约券卖出"))
	{
		i_d = algorithm::Directive::YUYUEQUAN_SELL;
	}
	else if (directive == QStringLiteral("担保品转入"))
	{
		i_d = algorithm::Directive::DANBAO_ZHUANRU;
	}
	else if (directive == QStringLiteral("担保品转出"))
	{
		i_d = algorithm::Directive::DANBAO_ZHUANCHU;
	}
	else if (directive == QStringLiteral("涨停融券卖出"))
	{
		i_d = algorithm::Directive::ZHANGTING_RONGQUAN_SELL;
	}
	return i_d;
}

QString AlgData::GetTyepNameFromEnum(int enum_d)
{
	QString str_directive;
	if (enum_d == algorithm::Directive::GENERAL_BUY)
	{
		str_directive = QStringLiteral("普通买入");
	}
	else if (enum_d == algorithm::Directive::GENERAL_SELL)
	{
		str_directive = QStringLiteral("普通卖出");
	}
	else if (enum_d == algorithm::Directive::DANBAOPIN_BUY)
	{
		str_directive = QStringLiteral("担保品买入");
	}
	else if (enum_d == algorithm::Directive::DANBAOPIN_SELL)
	{
		str_directive = QStringLiteral("担保品卖出");
	}
	else if (enum_d == algorithm::Directive::BUYQUAN_HUANQUAN)
	{
		str_directive = QStringLiteral("买券还券");
	}
	else if (enum_d == algorithm::Directive::RONG_QUAN_SELL)
	{
		str_directive = QStringLiteral("融券卖出");
	}
	else if (enum_d == algorithm::Directive::SELLQUAN_HUANKUAN)
	{
		str_directive = QStringLiteral("卖券还款");
	}
	else if (enum_d == algorithm::Directive::RONGZI_BUY)
	{
		str_directive = QStringLiteral("融资买入");
	}
	else if (enum_d == algorithm::Directive::YUYUEQUAN_SELL)
	{
		str_directive = QStringLiteral("预约券卖出");
	}
	else if (enum_d == algorithm::Directive::DANBAO_ZHUANRU)
	{
		str_directive = QStringLiteral("担保品转入");
	}
	else if (enum_d == algorithm::Directive::DANBAO_ZHUANCHU)
	{
		str_directive = QStringLiteral("担保品转出");
	}
	else if (enum_d == algorithm::Directive::ZHANGTING_RONGQUAN_SELL)
	{
		str_directive = QStringLiteral("涨停融券卖出");
	}
	return str_directive;
}

QString AlgData::GetDirectiveFromEnum(int enum_d)
{
	QString str_directive;
	if (enum_d == algorithm::Directive::GENERAL_BUY)
	{
		str_directive = "0b";
	}
	else if (enum_d == algorithm::Directive::GENERAL_SELL)
	{
		str_directive = "0s";
	}
	else if (enum_d == algorithm::Directive::DANBAOPIN_BUY)
	{
		str_directive = "1a";
	}
	else if (enum_d == algorithm::Directive::DANBAOPIN_SELL)
	{
		str_directive = "1b";
	}
	else if (enum_d == algorithm::Directive::BUYQUAN_HUANQUAN)
	{
		str_directive = "1c";
	}
	else if (enum_d == algorithm::Directive::RONG_QUAN_SELL)//融券卖出和涨停融券卖出下单时都用"1d"
	{
		str_directive = "1d";
	}
	else if (enum_d == algorithm::Directive::SELLQUAN_HUANKUAN)
	{
		str_directive = "1e";
	}
	else if (enum_d == algorithm::Directive::RONGZI_BUY)
	{
		str_directive = "1f";
	}
	else if (enum_d == algorithm::Directive::YUYUEQUAN_SELL)
	{
		str_directive = "1g";
	}
	else if (enum_d == algorithm::Directive::DANBAO_ZHUANRU)
	{
		str_directive = "1h";
	}
	else if (enum_d == algorithm::Directive::DANBAO_ZHUANCHU)
	{
		str_directive = "1i";
	}
	else if (enum_d == algorithm::Directive::ZHANGTING_RONGQUAN_SELL)
	{
		str_directive = "1d";
	}
	return str_directive;
}

int AlgData::GetEnumFromDirective(QString directive)
{
	int i_d = 0;
	if (directive == "0b")
	{
		i_d = algorithm::Directive::GENERAL_BUY;
	}
	else if (directive == "0s")
	{
		i_d = algorithm::Directive::GENERAL_SELL;
	}
	else if (directive == "1a")
	{
		i_d = algorithm::Directive::DANBAOPIN_BUY;
	}
	else if (directive == "1b")
	{
		i_d = algorithm::Directive::DANBAOPIN_SELL;
	}
	else if (directive == "1c")
	{
		i_d = algorithm::Directive::BUYQUAN_HUANQUAN;
	}
	else if (directive == "1d")//融券卖出和涨停融券卖出下单时都用"1d"
	{
		i_d = algorithm::Directive::RONG_QUAN_SELL;
	}
	else if (directive == "1e")
	{
		i_d = algorithm::Directive::SELLQUAN_HUANKUAN;
	}
	else if (directive == "1f")
	{
		i_d = algorithm::Directive::RONGZI_BUY;
	}
	else if (directive == "1g")
	{
		i_d = algorithm::Directive::YUYUEQUAN_SELL;
	}
	else if (directive == "1h")
	{
		i_d = algorithm::Directive::DANBAO_ZHUANRU;
	}
	else if (directive == "1i")
	{
		i_d = algorithm::Directive::DANBAO_ZHUANCHU;
	}

	return i_d;
}

QString AlgData::GetSideFromType(int type)
{
	QString str_side;
	//algorithm::Directive::BUYQUAN_HUANQUAN
	if (type == algorithm::Directive::GENERAL_BUY
		|| type == algorithm::Directive::DANBAOPIN_BUY
		|| type == algorithm::Directive::RONGZI_BUY)
	{
		str_side = QStringLiteral("买");
	}
	//algorithm::Directive::SELLQUAN_HUANKUAN;
	else if (type == algorithm::Directive::GENERAL_SELL
		|| type == algorithm::Directive::DANBAOPIN_SELL
		|| type == algorithm::Directive::RONG_QUAN_SELL
		|| type == algorithm::Directive::YUYUEQUAN_SELL
		|| type == algorithm::Directive::ZHANGTING_RONGQUAN_SELL)
	{
		str_side = QStringLiteral("卖");
	}

	//algorithm::Directive::DANBAO_ZHUANRU
	//algorithm::Directive::DANBAO_ZHUANCHU;

	return str_side;
}

QString AlgData::GetMidType(QString stock_id)
{
	if (stock_id.isEmpty())
	{
		return QString("");
	}

	if (stock_id.at(0) == '6')
	{
		return "001";
	}
	else if (stock_id.at(0) == '9')
	{
		return "00D";
	}
	else if (stock_id.at(0) == '0')
	{
		return "002";
	}
	else if (stock_id.at(0) == '2')
	{
		return "00H";
	}
	else if (stock_id.contains("HK"))
	{
		return "00K";
	}
	return QString("");
}

QString AlgData::GetMarketType(QString stock_id)
{
	if (stock_id.isEmpty())
	{
		return QString("");
	}
	if (stock_id.at(0) == "6" || stock_id.at(0) == "9")
	{
		return "SH";
	}
	else if (stock_id.at(0) == "0" || stock_id.at(0) == "2")
	{
		return "SZ";
	}
	else
	{
		return QString("");
	}
}

QString AlgData::GetPriceFromType(QString stockid, QString price_type)
{
	QString str_price;
	if (price_type == QStringLiteral("当前价"))
	{
		double last_price = map_level2.value(stockid).LastPrice;
		str_price = QString("%1").arg(last_price, 0, 'f', 2);
	}
	else if (price_type == QStringLiteral("中间价"))
	{
		Level2Obj lev_obj = map_level2.value(stockid);
		double bid1 = (lev_obj.BidPrice1 + lev_obj.AskPrice1) / 2;
		str_price = QString("%1").arg(bid1, 0, 'f', 2);
	}
	else if (price_type == "Bid1")
	{
		double bid1 = map_level2.value(stockid).BidPrice1;
		str_price = QString("%1").arg(bid1, 0, 'f', 2);
	}
	else if (price_type == "Bid2")
	{
		double bid2 = map_level2.value(stockid).BidPrice2;
		str_price = QString("%1").arg(bid2, 0, 'f', 2);
	}
	else if (price_type == "Bid3")
	{
		double bid3 = map_level2.value(stockid).BidPrice3;
		str_price = QString("%1").arg(bid3, 0, 'f', 2);
	}
	else if (price_type == "Bid4")
	{
		double bid4 = map_level2.value(stockid).BidPrice4;
		str_price = QString("%1").arg(bid4, 0, 'f', 2);
	}
	else if (price_type == "Bid5")
	{
		double bid5 = map_level2.value(stockid).BidPrice5;
		str_price = QString("%1").arg(bid5, 0, 'f', 2);
	}
	else if (price_type == "Bid6")
	{
		double bid6 = map_level2.value(stockid).BidPrice6;
		str_price = QString("%1").arg(bid6, 0, 'f', 2);
	}
	else if (price_type == "Bid7")
	{
		double bid7 = map_level2.value(stockid).BidPrice7;
		str_price = QString("%1").arg(bid7, 0, 'f', 2);
	}
	else if (price_type == "Bid8")
	{
		double bid8 = map_level2.value(stockid).BidPrice8;
		str_price = QString("%1").arg(bid8, 0, 'f', 2);
	}
	else if (price_type == "Bid9")
	{
		double bid9 = map_level2.value(stockid).BidPrice9;
		str_price = QString("%1").arg(bid9, 0, 'f', 2);
	}
	else if (price_type == "Bid10")
	{
		double bid10 = map_level2.value(stockid).BidPrice10;
		str_price = QString("%1").arg(bid10, 0, 'f', 2);
	}
	else if (price_type == "Ask1")
	{
		double ask1 = map_level2.value(stockid).AskPrice1;
		str_price = QString("%1").arg(ask1, 0, 'f', 2);
	}
	else if (price_type == "Ask2")
	{
		double ask2 = map_level2.value(stockid).AskPrice2;
		str_price = QString("%1").arg(ask2, 0, 'f', 2);
	}
	else if (price_type == "Ask3")
	{
		double ask3 = map_level2.value(stockid).AskPrice3;
		str_price = QString("%1").arg(ask3, 0, 'f', 2);
	}
	else if (price_type == "Ask4")
	{
		double ask4 = map_level2.value(stockid).AskPrice4;
		str_price = QString("%1").arg(ask4, 0, 'f', 2);
	}
	else if (price_type == "Ask5")
	{
		double ask5 = map_level2.value(stockid).AskPrice5;
		str_price = QString("%1").arg(ask5, 0, 'f', 2);
	}
	else if (price_type == "Ask6")
	{
		double ask6 = map_level2.value(stockid).AskPrice6;
		str_price = QString("%1").arg(ask6, 0, 'f', 2);
	}
	else if (price_type == "Ask7")
	{
		double ask7 = map_level2.value(stockid).AskPrice7;
		str_price = QString("%1").arg(ask7, 0, 'f', 2);
	}
	else if (price_type == "Ask8")
	{
		double ask8 = map_level2.value(stockid).AskPrice8;
		str_price = QString("%1").arg(ask8, 0, 'f', 2);
	}
	else if (price_type == "Ask9")
	{
		double ask9 = map_level2.value(stockid).AskPrice9;
		str_price = QString("%1").arg(ask9, 0, 'f', 2);
	}
	else if (price_type == "Ask10")
	{
		double ask10 = map_level2.value(stockid).AskPrice10;
		str_price = QString("%1").arg(ask10, 0, 'f', 2);
	}
	return str_price;
}

QString AlgData::GetClassNameFromStrategy(QString strategy)
{
	QString class_name;
	if (strategy == "VWAP")
	{
		class_name = "AlgoVwap";
	}
	else if (strategy == "TWAP")
	{
		class_name = "AlgoTwap";
	}
	else if (strategy == "POV")
	{
		class_name = "AlgoPov";
	}
	else if (strategy == "Limit Order")
	{
		class_name = "AlgoLimitOrder";
	}
	else if (strategy == "Smart Order")
	{
		class_name = "";
	}
	else if (strategy == "Minimum Market Effect")
	{
		class_name = "AlgoMiniMarketEffect";
	}
	else if (strategy == "Constant")
	{
		class_name = "";
	}
	else if (strategy == "Momentum")
	{
		class_name = "";
	}
	else if (strategy == "Reversion")
	{
		class_name = "";
	}
	else if (strategy == "MOC")
	{
		class_name = "";
	}
	else if (strategy == QStringLiteral("市价单"))
	{
		class_name = "AlgoMarketOrder";
	}
	else if (strategy == "TestTwo")
	{
		class_name = "";
	}
	return class_name;
}






void AlgData::DealSendOrder(StrategyChildInfo child_info)
{
	QString str_clientid;
	QString str_msg;
	QJsonObject json_order;

	json_order.insert("userid", KGetLoginInfo()->userid_);
	json_order.insert("stockid", child_info.stock_id);
	QString str_vol = child_info.send_volume;
	QString str_vol_temp = str_vol.append(".00");
	json_order.insert("volume", str_vol_temp);
	json_order.insert("price", child_info.send_price);
	QString side_temp;
	if (child_info.side == QStringLiteral("卖"))
	{
		side_temp = "2";
	}
	else if (child_info.side == QStringLiteral("买"))
	{
		side_temp = "1";
	}
	json_order.insert("side", side_temp);
	QString order_type = GetDirectiveFromEnum(child_info.order_type_.toInt());
	json_order.insert("ordertype", order_type);
	json_order.insert("stockname", child_info.stock_name);

	QJsonObject json_alg;
	json_alg.insert("parentorderid", child_info.parent_id);

	bool send_order = KSendOrders(json_order, json_alg, str_clientid, str_msg);
	child_info.client_id = str_clientid;

	if (!send_order)
	{
		child_info.msg = QStringLiteral("下单失败");
	}

	parent_from_child.insert(child_info.client_id, child_info.parent_id);
	//显示子单
	QMap<QString, StrategyChildInfo> child_lists = child_from_parentid.value(child_info.parent_id);

	child_lists.insert(child_info.client_id, child_info);
	child_from_parentid.insert(child_info.parent_id, child_lists);

	StrategyChildWidget *child_widget_vol =
		StrategyChildWidget::widgets.value(child_info.parent_id, nullptr);
	if (child_widget_vol != nullptr)
	{
		QList<StrategyChildInfo> list_child;
		list_child.push_back(child_info);
		child_widget_vol->setChildTable(list_child);
	}
}

void AlgData::slotTradeLogOrder(int, QByteArray arr)
{
	qDebug() << "TradeLog" << arr;
	QJsonObject json_oms;
	QJsonObject json_rephead;
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);
	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	json_oms = json_doc.object().value("oms").toObject();
	json_rephead = json_doc.object().value("rephead").toObject();

	if (!json_rephead.contains("parentorderid"))
		return;

	QString parent = json_rephead.value("parentorderid").toString();
	if (parent.isEmpty())
		return;

	QJsonArray json_array = json_oms.value("tradelog").toArray();
	if (json_array.size() != 1)
		return;

	QJsonObject obj_temp = json_array.at(0).toObject();
	QString client_id = obj_temp.value("clientorderid").toString();
	if (client_id.isEmpty())
		return;

	StrategyChildInfo child_info = child_from_parentid.value(parent).value(client_id);
	double exec_price_temp = obj_temp.value("executedprice").toString().toDouble();
	child_info.executed_price = QString("%1").arg(exec_price_temp, 0, 'f', 2);

	QString str_ex_vol = obj_temp.value("executedvolume").toString();
	int exec_vol = int(str_ex_vol.toDouble());
	child_info.executed_volume = QString::number(exec_vol);

	child_info.sysorderid = obj_temp.value("serverorderid").toString();
	child_info.status = obj_temp.value("tradestatus").toString();
	child_info.trading_datetime = obj_temp.value("tradingdatetime").toString();

	/////////////////////////////////////////////////////////////////////////////////ceshi
	/*if (child_info.status.toInt() == algorithm::PART_CANCELED)
	{
	QMap<QString, StrategyChildInfo> map_child = child_from_parentid.value(parent);

	if (map_child.count() > 2 && map_child.first().status.toInt() == algorithm::PART_EXECUTED_WAIT_CANCEL)
	{
	StrategyChildInfo first = map_child.first();
	first.status = QString::number(algorithm::PART_CANCELED);
	map_child.insert(first.client_id, first);
	child_from_parentid.insert(parent, map_child);

	if (map_parent_info.value(parent)->map_undealed_child.contains(first.client_id))
	{
	map_parent_info.value(parent)->map_undealed_child.insert(first.client_id, first);
	}
	//更新子单
	StrategyChildWidget *child_widget_vol =
	StrategyChildWidget::widgets.value(parent, nullptr);
	QList<StrategyChildInfo> list_child;
	list_child.push_back(first);
	if (child_widget_vol != nullptr)
	{
	connect(this, SIGNAL(SignalToChildWidget(QList<StrategyChildInfo>)),
	child_widget_vol, SLOT(setChildTable(QList<StrategyChildInfo>)));
	emit SignalToChildWidget(list_child);
	}
	}
	else
	{
	return;
	}
	}*/
	///////////////////////////////////////////////////////////////////////////////////ceshi

	double dou_pro = child_info.executed_volume.toDouble() / map_parent_info.value(parent)->volume.toDouble();
	child_info.proportion = QString("%1").arg(dou_pro * 100, 0, 'f', 2) + "%";

	//保存子单
	QMap<QString, StrategyChildInfo> map_child = child_from_parentid.value(parent);
	map_child.insert(child_info.client_id, child_info);
	child_from_parentid.insert(parent, map_child);

	//更新子单
	StrategyChildWidget *child_widget_vol =
		StrategyChildWidget::widgets.value(parent, nullptr);
	QList<StrategyChildInfo> list_child;
	list_child.push_back(child_info);
	if (child_widget_vol != nullptr)
	{
		child_widget_vol->setChildTable(list_child);
	}

	if (child_info.status.toInt() == algorithm::OrderStatus::PART_EXECUTED_WAIT_CANCEL
		|| child_info.status.toInt() == algorithm::OrderStatus::PART_CANCELED
		|| child_info.status.toInt() == algorithm::OrderStatus::PART_FILLED
		|| child_info.status.toInt() == algorithm::OrderStatus::FILLED)
	{
		int total_ex_volume = 0;
		double total_turnover = 0;

		QMap<QString, StrategyChildInfo>::const_iterator begin = map_child.begin();
		QMap<QString, StrategyChildInfo>::const_iterator end = map_child.end();
		while (begin != end)
		{
			total_ex_volume += begin.value().executed_volume.toInt();

			total_turnover += begin.value().executed_price.toDouble() * begin.value().executed_volume.toInt();

			begin++;
		}

		StrategyParentInfo *parent_info = map_parent_info.value(parent);
		//母单总成交股数
		parent_info->exec_vol = QString::number(total_ex_volume);

		if (parent_info->exec_vol == parent_info->volume)
		{
			parent_info->status == QStringLiteral("终止");
		}

		//母单进度
		double exe_percent = (total_ex_volume * 100) / parent_info->volume.toDouble();
		if (total_ex_volume != 0)
		{
			if (exe_percent < 0.01)
			{
				exe_percent = 0.01;
			}
		}
		else
		{
			exe_percent = 0.00;
		}
		parent_info->progress = QString::number(exe_percent);

		//成交均价
		double avg_price = 0.00;
		if (total_ex_volume != 0)
		{
			avg_price = total_turnover / total_ex_volume;
		}
		parent_info->exec_avg_price = QString("%1").arg(avg_price, 0, 'f', 2);

		//更新母单		
		if (StrategyWidget::GetInstance() != nullptr)
		{
			StrategyWidget::GetInstance()->updateParentInfo(parent_info);
		}

	}

}

void AlgData::CancelOrder(StrategyChildInfo c_info)
{
	//{*reqhead:{reqno,cellid,oid},*oms:{mid,clientorderid,sysorderid,ooid}}
	QString str_mid = GetMidType(c_info.stock_id);
	if (str_mid.isEmpty())
		return;

	QJsonObject obj_oms;
	obj_oms.insert("mid", str_mid);
	obj_oms.insert("clientorderid", c_info.client_id);
	obj_oms.insert("sysorderid", c_info.sysorderid);
	obj_oms.insert("ooid", KGetLoginInfo()->oid);

	QJsonDocument json_doc;
	json_doc.setObject(obj_oms);
	QByteArray arr_send = json_doc.toJson(QJsonDocument::Compact);

	twd::AsyncPostRequestAlgorithm(30, 26, algorithm::SocketPacketHead::CANCEL_ORDER_RESP, c_info.parent_id, arr_send);
}

void AlgData::slotCancelOrder(int aint, QByteArray arr)
{
	//{*rephead, *oms:{mid,orderid,clientorderid,sysorderid,ooid},
	//&*ogs:[{sysorderid}],&msgcode, &msgno,&msg}}						
	QJsonParseError json_error;
	QJsonDocument json_doc = QJsonDocument::fromJson(arr, &json_error);

	if (json_error.error != QJsonParseError::NoError || !json_doc.isObject())
		return;

	QJsonObject json_req = json_doc.object().value("reqhead").toObject();
	QJsonObject json_oms = json_doc.object().value("oms").toObject();
	QJsonObject json_ogs = json_doc.object().value("ogs").toObject();

	QString str_prent = json_req.value("parentid").toString();
	if (str_prent.isEmpty())
		return;

	QString str_msg_code = json_ogs.value("msgcode").toString();
	QString str_msgno = json_ogs.value("msgno").toString();
	QString str_msg = json_ogs.value("msg").toString();

}

void AlgData::InquireMaketData(QString stockid)
{
	if (stockid.size() == 6)
	{
		twd::TgwSubHangqing(stockid, Stock_Type_Level2, this,
			SLOT(slot_Level2_Recive(int, unsigned long, unsigned long, QByteArray)));

		twd::TgwSubHangqing(stockid, Stock_Type_Transaction, this,
			SLOT(slot_Level2_Recive(int, unsigned long, unsigned long, QByteArray)));
	}
}

void AlgData::InquireHangQing(QString stock_id)
{
	if (stock_id.size() == 6)
	{
		twd::TgwSubHangqing(stock_id, Stock_Type_Level2, this,
			SLOT(slot_Level2_Recive(int, unsigned long, unsigned long, QByteArray)));
	}
}

void AlgData::InquireSellInTime(QString stock_id)
{
	if (stock_id.size() == 6)
	{
		twd::TgwSubHangqing(stock_id, Stock_Type_Transaction, this,
			SLOT(slot_Level2_Recive(int, unsigned long, unsigned long, QByteArray)));
	}
}

void AlgData::slot_Level2_Recive(int t, unsigned long kw,
	unsigned long sec, QByteArray levelobj)
{
	switch (t)
	{
	case Stock_Type_SysData:

		break;
	case Stock_Type_Level1:

		break;
	case Stock_Type_Level2:
		SetHangQing(QString::number(kw), levelobj);
		break;
	case Stock_Type_OrderQueue:

		break;
	case Stock_Type_Transaction:
		SellInTimeReceive(QString::number(kw), sec, levelobj);
		break;
	default:
		break;
	}
}

void AlgData::SellInTimeReceive(QString code, unsigned long sec, QByteArray sellintime)
{
	QJsonDocument json_doc;
	json_doc = QJsonDocument::fromJson(sellintime);

	SellInTime sell_time;
	sell_time = sell_time.SellInTimeByteArraty(code, sec, sellintime);

	if (code.length() != 6)
	{
		int add_length = 6 - code.length();
		for (int i = 0; i < add_length; i++)
		{
			code.prepend("0");
		}
	}
	sell_time.stockID = code;

	qDebug() << "SELLIN"
		<< sell_time.stockID << "+" << sell_time.price << "+" << sell_time.txnTime << "+" << sell_time.volume
		<< "num" << ++i;

	list_sellin_.push_back(sell_time);
	if (list_sellin_.count() == 100)
	{
		SavaSellInTimeToDB(list_sellin_);
		list_sellin_.clear();
	}
}

void AlgData::SavaSellInTimeToDB(QList<SellInTime> list_sell)
{
	QSqlQuery query(sell_intime_db);
	QString str_q = QString("insert into realtimesell(count,stockid,timesell,inttime,price,volume)")
		+ QString("values(:count, :stockid, :timesell,:inttime,:price,:volume)");
	query.prepare(str_q);

	sell_intime_db.transaction();

	QTime time_temp;
	QTime time_nine = QTime::fromString("09:30:00", "hh:mm:ss");
	for (int i = 0; i < list_sell.count(); i++)
	{
		SellInTime sell_time = list_sell.at(i);
		++sell_in_time_count;

		query.bindValue(":count", sell_in_time_count);
		query.bindValue(":stockid", sell_time.stockID);
		query.bindValue(":timesell", sell_time.txnTime);

		time_temp = QTime::fromString(sell_time.txnTime, "hh:mm:ss");
		int ses_diff = (time_temp.hour() - time_nine.hour()) * 60 * 60
			+ (time_temp.minute() - time_nine.minute()) * 60
			+ (time_temp.second() - time_nine.second());
		query.bindValue(":inttime", ses_diff);

		QString str_price = QString("%1").arg(sell_time.price, 0, 'f', 2);
		query.bindValue(":price", str_price);
		query.bindValue(":volume", sell_time.volume);

		bool success = query.exec();
		if (!success)
		{
			sell_intime_db.rollback();
			QSqlError lastError = query.lastError();
			qDebug() << lastError.driverText() << QStringLiteral("插入失败");
			return;
		}
	}
	sell_intime_db.commit();
}

void AlgData::SetHangQing(QString code, QByteArray levelobj)
{
	Level2Obj level;
	level = level.Level2ByteArraty(code, levelobj);

	if (code.length() != 6)
	{
		int add_length = 6 - code.length();
		for (int i = 0; i < add_length; i++)
		{
			code.prepend("0");
		}
	}
	level.StockID = code;

	map_level2.insert(level.StockID, level);

	qDebug() << "hangqing" << level.StockID << "," << "ASK1" << level.AskPrice1 << "BID1" << level.BidPrice1
		<< "ZUI" << level.LastPrice;

}


/**
* @brief 初始化数据库
* @author LuChenQun
* @date 2015/05/21
* @return void
*/
void AlgData::initSqliteDataBase()
{
	const QString dbFilePath = QDir::currentPath() + "/AlgData";
	const QString dbFileName = "AlglogEvent.db";

	// 如果没有数据库文件，那么先创建相应的文件夹路径，否则会创建数据库文件失败
	if (!QFile::exists(dbFilePath + "/" + dbFileName))
	{
		QDir dir;
		if (!dir.exists(dbFilePath))
		{
			dir.mkpath(dbFilePath);
		}
	}

	if (createDB(dbFilePath, dbFileName))
	{
		const QString sqlCreateTable = "CREATE TABLE IF NOT EXISTS log_event \
		(id integer PRIMARY KEY AUTOINCREMENT, event varchar, \
			data1 varchar, data2 varchar, data3 varchar, data4 varchar, data5 varchar, \
			parent_id vachar, parent_volume vachar, execute_price vachar, price_type vachar, execute_type vachar, \
			bid_price1 double, bid_vol1 int, ask_price1 double, ask_vol1 int, \
			last_price double, execute_time varchar)";
		sqlQuery(sqlCreateTable);
	}
}

/**
* @brief 创建数据库文件
* @author LuChenQun
* @date 2015/05/21
* @param[in] dbFilePath 数据库文件路径。注意，数据库路径请以 "/" 分割，如 "D:/Code/QtSqlLiteTest"
* @param[in] dbFileName 数据库文件名。如："myDB.db"
* @return bool true 创建成功 false 创建失败
*/
bool AlgData::createDB(const QString dbFilePath, const QString dbFileName)
{
	m_sqliteDB = QSqlDatabase::addDatabase("QSQLITE");
	m_sqliteDB.setDatabaseName(dbFilePath + "/" + dbFileName);

	bool openRet = m_sqliteDB.open();
	qDebug() << ((openRet) ? (dbFileName + " open sucees") : (dbFileName + " open fail, " + m_sqliteDB.lastError().driverText()));

	return openRet;
}

/**
* @brief 执行sql语句
* @author LuChenQun
* @date 2015/05/21
* @param[in] sql sql语句
* @return bool true 执行成功 false 执行失败
*/
bool AlgData::sqlQuery(const QString sql)
{
	QSqlQuery query;
	bool queryRet = query.exec(sql);

	if (!queryRet){ qDebug() << (sql + " query fail"); }

	return queryRet;
}