#pragma once
#include <QObject>
#include <QMap>

class QNetworkReply;
class QNetworkAccessManager;

//* http请求类，包含post，get
class HttpRequest : public QObject
{
	Q_OBJECT
protected:
	//* 构造函数
	HttpRequest();

public:
	//* 单一实例
	static HttpRequest& instance();

public:
	//* 发起一个http get 请求,url是请求的链接
	void httpGet(const QString& url);

	//* 发起一个http post 请求,url是请求的链接
	//* index: url+data 的md5
	//* 返回值为请求的md5(url+data) 用来表示这一个唯一的请求
	QString httpPost(const QString& url, const QByteArray &data);

Q_SIGNALS:
	//* http get 请求得到回复url是请求的地址，data是回复的数据（如果isEmpty则表示请求出错了）
	void httpGetRspReady(QString url, QByteArray data);

	//* http post 请求得到回复postMD5是post的时候url+数据的md5，data是回复的数据（如果isEmpty则表示请求出错了）
	void httpPostRspReady(QString url, QString postMD5, QByteArray data);

protected:
	void onHttpGetRspProgress(qint64 bytesReceived, qint64 bytesTotal); //* http get 回复进度
	void onHttpGetRspFinished(); //* http get 处理完毕

	void onHttpPostRspProgress(qint64 bytesReceived, qint64 bytesTotal); //* http post 回复进度
	void onHttpPostRspFinished(); //* http post 处理完毕

private:
	void clearRp(QNetworkReply* rp);

private:
	QNetworkAccessManager*    mNetAccessManager;
	QMap<QString, bool>       mProcessingRq;                 //* 当前正在处理的请求url 和是否正在处理
	QMap<QString, QByteArray> mDownloadDataCache;            //* 数据缓存url -> data
	QMap<QString, QString>    mRedirectMap;                  //* 重定向的关系
};