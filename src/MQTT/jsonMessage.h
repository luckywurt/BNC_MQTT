#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QList>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QObject>
#include <QMutex>
#include <bncsettings.h>

// 站点状态枚举
enum class StationStatus
{
    Connected, // 正常状态
    Timeout, // 超时状态
    Disconnect, // 断开状态
    Error // 异常状态
};

// 单个站点信息结构体
struct StationInfo {
    QString       id;               // 站点ID
    StationStatus status;           // 站点状态
    QDateTime     lastDateTime;     // 最后正常数据接收时间
    double        lastLatencyMs;    // 最新延迟（毫秒）
    int           lastThroughput;   // 最新吞吐量（字节）
    QString       msg;              // 站点错误信息描述

    StationInfo()
        : status(StationStatus::Disconnect)
        , lastLatencyMs(0)
        , lastThroughput(0) {}

    explicit StationInfo(const QString& stationId)
        : id(stationId)
        , status(StationStatus::Disconnect)
        , lastDateTime(QDateTime::currentDateTime())
        , lastLatencyMs(0)
        , lastThroughput(0) {}
};

// 存储多个站点信息的类
class JsonMessage : public QObject
{
    Q_OBJECT

public:
    explicit JsonMessage(QObject* parent = nullptr);
    ~JsonMessage();

    //// 设置相关
    //----------------------------------------------------------
    void setNodeName(const QString& name); // 设置节点名称
    void addStation(const QString& id); //添加单个站点
    void updateStationList();//更新站点列表

    void initStationList(const QStringList& ids); //手动初始化站点列表
    void updateStation(const QString& id, StationStatus status, const QVariantList& data); //手动更新某个站点信息

    //// 获取信息
    /////----------------------------------------------------------
    QString getNodeName(); // 外部获取节点名称，字符串
    QString getJsonMessage(bool compact = false); // 外部获取JSON消息，字符串
    QJsonObject getJsonObject(); // 获取JSON对象

    //// 清除数据
    /////----------------------------------------------------------
    void clear(); //清空所有站点
    void removeStation(const QString& id); //移除一个站点

public slots:
    void slotUpdateLatency(const QByteArray& staID, double latency);// 更新站点延迟
    void slotUpdateThroughput(const QByteArray& staID, int bytes);// 更新站点吞吐量
    void slotOnStaTimeout(const QByteArray& staID);// 数据读取超时
    void slotOnStaDisconnected(const QByteArray& staID);// 网络连接断开
    void slotOnStaError(const QByteArray& staID, const QString& reason);// 发生异常错误

private:
    // 更新站点数量，增减站点数时触发
    void updateStationCount();
    // 更新站点状态计数即：正常站点数，超时站点数等，站点数据改变时触发
    void updateStatusCount();
    // 增加指定状态的计数（优化性能用）
    void incrementStatusCount(StationStatus status);
    // 减少指定状态的计数（优化性能用）
    void decrementStatusCount(StationStatus status);
    // 按需更新时间戳
    void updateTimeStampIfNeeded();
    // 状态枚举转字符串
    QString statusToString(StationStatus status) const;

private:
    QMutex _mutex;  // 保护站点数据线程安全
    QString _nodeName; // 节点名称
    QDateTime _timeMessage; // 消息时间戳
    int _totalStationCount; // 站点数量
    int _connectedStationCount; // 正常站点数
    int _timeoutStationCount; // 超时站点数
    int _disconnectStationCount; // 断开站点数
    int _errorStationCount; // 异常站点数

    bool _timeStampDirty; // 时间戳脏位标记

    QMap<QString, StationInfo> _stations; // 所有站点信息（使用QMap便于查找）
};

#endif // JSONMESSAGE_H
