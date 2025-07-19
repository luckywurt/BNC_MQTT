#include "jsonMessage.h"
#include <QDebug>
#include <QMutexLocker>
#include <QUrl>

// 构造与析构
// ----------------------------------------------------------------------------
JsonMessage::JsonMessage(QObject* parent)
    : QObject(parent)
      , _nodeName("DefaultNode")
      , _timeMessage(QDateTime::currentDateTime())
      , _totalStationCount(0)
      , _connectedStationCount(0)
      , _timeoutStationCount(0)
      , _disconnectStationCount(0)
      , _errorStationCount(0)
      , _timeStampDirty(false) {
}

JsonMessage::~JsonMessage() {
}

//// 设置相关
// ----------------------------------------------------------------------------

// 设置节点名称
void JsonMessage::setNodeName(const QString& name) {
    QMutexLocker locker(&_mutex);
    _nodeName = name;
    _timeStampDirty = true; // 标记时间戳需要更新
}

// 添加单个站点（默认断开）
void JsonMessage::addStation(const QString& id) {
    QMutexLocker locker(&_mutex);
    if (!_stations.contains(id)) {
        StationInfo station(id);
        // station.status 默认为 Disconnect
        station.lastDateTime = QDateTime::currentDateTime();
        _stations.insert(id, station);

        // 性能优化：更新计数，无需遍历
        _totalStationCount++;
        _disconnectStationCount++;
        _timeStampDirty = true;
    }
}

// 手动初始化站点列表，全部置为 Disconnect
void JsonMessage::initStationList(const QStringList& ids) {
    QMutexLocker locker(&_mutex);
    _stations.clear();
    _totalStationCount = _connectedStationCount = _timeoutStationCount =
        _disconnectStationCount = _errorStationCount = 0;

    QDateTime currentTime = QDateTime::currentDateTime();
    for (const QString& id : ids) {
        StationInfo station(id);
        station.status = StationStatus::Disconnect;
        station.lastDateTime = currentTime;
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++;
    }

    _timeStampDirty = true;
}

// 从 bncSettings("mountPoints") 更新站点列表，全部标记为 Disconnect
void JsonMessage::updateStationList() {
    QMutexLocker locker(&_mutex);
    _stations.clear();
    _totalStationCount = _connectedStationCount = _timeoutStationCount =
        _disconnectStationCount = _errorStationCount = 0;

    bncSettings settings;
    QStringList mountList = settings.value("mountPoints").toStringList();
    for (const QString& line : mountList) {
        const QStringList parts = line.split(' ');
        if (parts.size() <= 1) continue;
        QUrl url(parts.at(0));
        QString id = url.path().mid(1);

        StationInfo station(id);
        station.status = StationStatus::Disconnect;
        station.lastDateTime = QDateTime::currentDateTime();
        _stations.insert(id, station);

        _totalStationCount++;
        _disconnectStationCount++;
    }

    _timeStampDirty = true;
}

//// 更新数据
/////----------------------------------------------------------

// 手动更新某个站点信息
void JsonMessage::updateStation(const QString& id, StationStatus status, const QVariantList& data) {
    QMutexLocker locker(&_mutex);
    if (!_stations.contains(id)) {
        qWarning() << "Station" << id << "not found, adding automatically";
        // 注意：这里需要解锁后再调用 addStation，或者直接在这里添加站点
        StationInfo station(id);
        station.lastDateTime = QDateTime::currentDateTime();
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++;
    }

    StationInfo& station = _stations[id];
    StationStatus oldStatus = station.status;

    // 状态改变时更新计数
    if (oldStatus != status) {
        decrementStatusCount(oldStatus);
        incrementStatusCount(status);
        station.status = status;
        _timeStampDirty = true;
    }

    station.lastDateTime = QDateTime::currentDateTime();

    // 解析数据内容
    if (status == StationStatus::Connected && data.size() >= 2) {
        station.lastLatencyMs = data[0].toDouble();
        station.lastThroughput = data[1].toInt();
        station.msg.clear();
    }
    else if ((status == StationStatus::Timeout || status == StationStatus::Error) && data.size() >= 1) {
        station.msg = data[0].toString();
    }
    else {
        station.msg.clear();
    }
}

//// 槽函数：实时更新
// ----------------------------------------------------------------------------

// 更新站点延迟，标记为 Connected
void JsonMessage::slotUpdateLatency(const QByteArray& staID, double latency) {
    QMutexLocker locker(&_mutex);
    QString id = QString::fromUtf8(staID);

    // 确保站点存在
    if (!_stations.contains(id)) {
        StationInfo station(id);
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++; // 新站点默认为断开状态
    }

    StationInfo& station = _stations[id];
    StationStatus oldStatus = station.status;

    // 更新状态为连接
    if (oldStatus != StationStatus::Connected) {
        decrementStatusCount(oldStatus);
        incrementStatusCount(StationStatus::Connected);
        station.status = StationStatus::Connected;
        _timeStampDirty = true;
    }

    station.lastLatencyMs = latency;
    station.lastDateTime = QDateTime::currentDateTime();
    // throughput 保留上次值；msg 清空
    station.msg.clear();
}

// 更新站点吞吐量，标记为 Connected
void JsonMessage::slotUpdateThroughput(const QByteArray& staID, int bytes) {
    QMutexLocker locker(&_mutex);
    QString id = QString::fromUtf8(staID);

    // 确保站点存在
    if (!_stations.contains(id)) {
        StationInfo station(id);
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++; // 新站点默认为断开状态
    }

    StationInfo& station = _stations[id];
    StationStatus oldStatus = station.status;

    // 更新状态为连接
    if (oldStatus != StationStatus::Connected) {
        decrementStatusCount(oldStatus);
        incrementStatusCount(StationStatus::Connected);
        station.status = StationStatus::Connected;
        _timeStampDirty = true;
    }

    station.lastThroughput = bytes;
    station.lastDateTime = QDateTime::currentDateTime();
    station.msg.clear();
}

// 数据读取超时，标记为 Timeout，msg 存最后时间
void JsonMessage::slotOnStaTimeout(const QByteArray& staID) {
    QMutexLocker locker(&_mutex);
    QString id = QString::fromUtf8(staID);

    // 确保站点存在
    if (!_stations.contains(id)) {
        StationInfo station(id);
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++; // 新站点默认为断开状态
    }

    StationInfo& station = _stations[id];
    StationStatus oldStatus = station.status;

    // 更新状态为超时
    if (oldStatus != StationStatus::Timeout) {
        decrementStatusCount(oldStatus);
        incrementStatusCount(StationStatus::Timeout);
        station.status = StationStatus::Timeout;
        _timeStampDirty = true;
    }

    // msg 用于传递超时描述
    station.msg = station.lastDateTime.toString("yyyy-MM-dd hh:mm:ss");
}

// 网络连接断开，标记为 Disconnect
void JsonMessage::slotOnStaDisconnected(const QByteArray& staID) {
    QMutexLocker locker(&_mutex);
    QString id = QString::fromUtf8(staID);

    // 确保站点存在
    if (!_stations.contains(id)) {
        StationInfo station(id);
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++; // 新站点默认为断开状态
        return; // 新站点已经是断开状态，无需再次设置
    }

    StationInfo& station = _stations[id];
    StationStatus oldStatus = station.status;

    // 更新状态为断开
    if (oldStatus != StationStatus::Disconnect) {
        decrementStatusCount(oldStatus);
        incrementStatusCount(StationStatus::Disconnect);
        station.status = StationStatus::Disconnect;
        _timeStampDirty = true;
    }

    station.msg.clear();
}

// 发生异常，标记为 Error，msg 存错误原因
void JsonMessage::slotOnStaError(const QByteArray& staID, const QString& reason) {
    QMutexLocker locker(&_mutex);
    QString id = QString::fromUtf8(staID);

    // 确保站点存在
    if (!_stations.contains(id)) {
        StationInfo station(id);
        _stations.insert(id, station);
        _totalStationCount++;
        _disconnectStationCount++; // 新站点默认为断开状态
    }

    StationInfo& station = _stations[id];
    StationStatus oldStatus = station.status;

    // 更新状态为错误
    if (oldStatus != StationStatus::Error) {
        decrementStatusCount(oldStatus);
        incrementStatusCount(StationStatus::Error);
        station.status = StationStatus::Error;
        _timeStampDirty = true;
    }

    station.msg = reason;
}

//// 获取信息
// ----------------------------------------------------------------------------

// 获取节点名称
QString JsonMessage::getNodeName() {
    QMutexLocker locker(&_mutex);
    return _nodeName;
}

// 获取 JSON 字符串
QString JsonMessage::getJsonMessage(bool compact) {
    QJsonDocument doc(getJsonObject());
    return compact
               ? QString::fromUtf8(doc.toJson(QJsonDocument::Compact))
               : QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

// 获取 JSON 对象
QJsonObject JsonMessage::getJsonObject() {
    QMutexLocker locker(&_mutex);
    updateTimeStampIfNeeded();

    QJsonObject root;
    root["node"] = _nodeName;
    root["time"] = _timeMessage.toString("yyyy-MM-dd hh:mm:ss");

    // 统计
    QJsonObject state;
    state["total"] = _totalStationCount;
    state["connected"] = _connectedStationCount;
    state["timeout"] = _timeoutStationCount;
    state["disconnect"] = _disconnectStationCount;
    state["error"] = _errorStationCount;
    root["state"] = state;

    // 逐站点构建
    QJsonArray stationArray;
    for (auto it = _stations.constBegin(); it != _stations.constEnd(); ++it) {
        const StationInfo& s = it.value();
        QJsonObject obj;
        obj["ID"] = s.id;
        obj["status"] = statusToString(s.status);

        // 根据状态填充 data
        QJsonArray dataArr;
        switch (s.status) {
        case StationStatus::Connected:
            dataArr.append(QString::number(s.lastLatencyMs, 'f', 2));
            dataArr.append(s.lastThroughput);
            break;
        case StationStatus::Timeout:
        case StationStatus::Error:
            dataArr.append(s.msg);
            break;
        case StationStatus::Disconnect:
            // 留空
            break;
        }
        obj["data"] = dataArr;
        stationArray.append(obj);
    }
    root["station"] = stationArray;
    return root;
}

//// 清除数据
// ----------------------------------------------------------------------------

void JsonMessage::clear() {
    QMutexLocker locker(&_mutex);
    _stations.clear();
    _totalStationCount = _connectedStationCount = _timeoutStationCount =
        _disconnectStationCount = _errorStationCount = 0;
    _timeStampDirty = true;
}

void JsonMessage::removeStation(const QString& id) {
    QMutexLocker locker(&_mutex);
    auto it = _stations.find(id);
    if (it != _stations.end()) {
        decrementStatusCount(it.value().status);
        _totalStationCount--;
        _stations.erase(it);
        _timeStampDirty = true;
    }
}

//// 私有辅助
// ----------------------------------------------------------------------------

void JsonMessage::updateStationCount() {
    _totalStationCount = _stations.size();
}

void JsonMessage::updateStatusCount() {
    _connectedStationCount = _timeoutStationCount = _disconnectStationCount = _errorStationCount = 0;
    for (auto it = _stations.constBegin(); it != _stations.constEnd(); ++it) {
        incrementStatusCount(it.value().status);
    }
}

void JsonMessage::incrementStatusCount(StationStatus status) {
    switch (status) {
    case StationStatus::Connected: _connectedStationCount++;
        break;
    case StationStatus::Timeout: _timeoutStationCount++;
        break;
    case StationStatus::Disconnect: _disconnectStationCount++;
        break;
    case StationStatus::Error: _errorStationCount++;
        break;
    }
}

void JsonMessage::decrementStatusCount(StationStatus status) {
    switch (status) {
    case StationStatus::Connected:
        if (_connectedStationCount > 0) _connectedStationCount--;
        break;
    case StationStatus::Timeout:
        if (_timeoutStationCount > 0) _timeoutStationCount--;
        break;
    case StationStatus::Disconnect:
        if (_disconnectStationCount > 0) _disconnectStationCount--;
        break;
    case StationStatus::Error:
        if (_errorStationCount > 0) _errorStationCount--;
        break;
    }
}

void JsonMessage::updateTimeStampIfNeeded() {
    if (_timeStampDirty) {
        _timeMessage = QDateTime::currentDateTime();
        _timeStampDirty = false;
    }
}

QString JsonMessage::statusToString(StationStatus status) const {
    switch (status) {
    case StationStatus::Connected: return "connected";
    case StationStatus::Timeout: return "timeout";
    case StationStatus::Disconnect: return "disconnect";
    case StationStatus::Error: return "error";
    }
    return "unknown";
}
