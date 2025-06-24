#include "MqttClient.h"
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QByteArray>


MqttClient::MqttClient(QObject* parent)
    : QObject(parent)
      , m_client(nullptr)
      , m_port(1883)
      , m_connectionCheckEnabled(true)
      , m_autoSendMessageEnabled(true) {
    initializeClient();

    // 初始化连接检查定时器
    m_connectionCheckTimer = new QTimer(this);
    m_connectionCheckTimer->setInterval(60000); // 默认60秒
    connect(m_connectionCheckTimer, &QTimer::timeout, this, &MqttClient::checkConnection);

    // 发送消息定时器
    m_sendMessageTimer = new QTimer(this);
    m_sendMessageTimer->setInterval(10000); // 默认10秒
    connect(m_sendMessageTimer, &QTimer::timeout, this, &MqttClient::autoSendMessage);
}

MqttClient::~MqttClient() {
    stopMqttClient();
}

void MqttClient::initializeClient() {
    if (m_client) {
        m_client->deleteLater();
    }

    m_client = new QMqttClient(this);
    setupSignalSlots();
}

void MqttClient::setupSignalSlots() {
    // 连接状态变化信号
    connect(m_client, &QMqttClient::stateChanged, this, &MqttClient::onStateChanged);
    connect(m_client, &QMqttClient::errorChanged, this, &MqttClient::onErrorChanged);
    connect(m_client, &QMqttClient::connected, this, &MqttClient::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &MqttClient::onDisconnected);

    // 消息接收信号
    connect(m_client, &QMqttClient::messageReceived, this, &MqttClient::onMessageReceived);
    connect(m_client, &QMqttClient::pingResponseReceived, this, &MqttClient::onPingResponseReceived);
}

void MqttClient::setConnectionParameters(const QString& host, quint16 port, const QString& clientId) {
    m_hostname = host;
    m_port = port;
    m_clientId = clientId.isEmpty() ? QString("QtMqttClient_%1").arg(QDateTime::currentMSecsSinceEpoch()) : clientId;

    m_client->setHostname(m_hostname);
    m_client->setPort(m_port);
    m_client->setClientId(m_clientId);

    emit mqttLogMessage(QString("设置连接参数: Host=%1, Port=%2, ClientId=%3")
                        .arg(m_hostname).arg(m_port).arg(m_clientId).toUtf8());
}

void MqttClient::setCredentials(const QString& username, const QString& password) {
    m_username = username;
    m_password = password;

    m_client->setUsername(m_username);
    m_client->setPassword(m_password);

    emit mqttLogMessage(QString("设置认证信息: Username=%1").arg(m_username).toUtf8());
}

void MqttClient::connectToHost() {
    if (m_hostname.isEmpty()) {
        m_lastError = "主机地址未设置";
        emit mqttLogMessage("主机地址未设置");
        return;
    }

    if (m_client->state() == QMqttClient::Connected) {
        emit mqttLogMessage("MQTT客户端已经连接");
        return;
    }

    emit mqttLogMessage(QString("正在连接到MQTT服务器: %1:%2").arg(m_hostname).arg(m_port).toUtf8());
    m_client->connectToHost();
}

void MqttClient::disconnectFromHost() {
    if (m_client->state() != QMqttClient::Disconnected) {
        emit mqttLogMessage("正在断开MQTT连接...");
        m_client->disconnectFromHost();
    }
}

void MqttClient::stopMqttClient() {
    // 停止连接检查定时器
    if (m_connectionCheckTimer) {
        m_connectionCheckTimer->stop();
    }

    // 停止发送消息定时器
    if (m_sendMessageTimer) {
        m_sendMessageTimer->stop();
    }

    // 断开MQTT连接
    disconnectFromHost();

    emit mqttLogMessage("MQTT客户端已停止");
}

qint32 MqttClient::publishMessage(const QString& topic, const QString& message, quint8 qos, bool retain) {
    if (!isConnected()) {
        m_lastError = "MQTT客户端未连接";
        emit mqttLogMessage("MQTT客户端未连接");
        return -1;
    }

    if (topic.isEmpty()) {
        m_lastError = "发布主题不能为空";
        emit mqttLogMessage("发布主题不能为空");
        return -1;
    }

    qint32 messageId = m_client->publish(QMqttTopicName(topic), message.toUtf8(), qos, retain);

    if (messageId == -1) {
        m_lastError = "消息发布失败";
        emit mqttLogMessage("消息发布失败");
    }
    else {
        emit mqttLogMessage("发布消息成功");
    }

    return messageId;
}

bool MqttClient::subscribeToTopic(const QString& topic, quint8 qos) {
    if (!isConnected()) {
        m_lastError = "MQTT客户端未连接";
        emit mqttLogMessage("MQTT客户端未连接");
        return false;
    }

    if (topic.isEmpty()) {
        m_lastError = "订阅主题不能为空";
        emit mqttLogMessage("订阅主题不能为空");
        return false;
    }

    auto subscription = m_client->subscribe(QMqttTopicFilter(topic), qos);
    if (subscription) {
        m_topic = topic;
        emit mqttLogMessage(QString("订阅主题: %1 (QoS: %2)").arg(topic).arg(qos).toUtf8());
        return true;
    }
    else {
        m_lastError = QString("订阅主题失败: %1").arg(topic);
        emit mqttLogMessage(m_lastError.toUtf8());
        return false;
    }
}

void MqttClient::unsubscribeFromTopic(const QString& topic) {
    if (!isConnected()) {
        m_lastError = "MQTT客户端未连接";
        emit mqttLogMessage("MQTT客户端未连接");
        return;
    }
    m_client->unsubscribe(QMqttTopicFilter(topic));
    emit mqttLogMessage(QString("取消订阅主题: %1").arg(topic).toUtf8());
}

bool MqttClient::isConnected() const {
    return m_client && m_client->state() == QMqttClient::Connected;
}

QMqttClient::ClientState MqttClient::getState() const {
    return m_client ? m_client->state() : QMqttClient::Disconnected;
}

QString MqttClient::getLastError() const {
    return m_lastError;
}

void MqttClient::setConnectionCheckInterval(int intervalMs) {
    if (m_connectionCheckTimer) {
        m_connectionCheckTimer->setInterval(intervalMs);
        emit mqttLogMessage(QString("设置连接检查间隔: %1ms").arg(intervalMs).toUtf8());
    }
}

void MqttClient::setSendMessageInterval(int intervalMs) {
    if (m_sendMessageTimer) {
        m_sendMessageTimer->setInterval(intervalMs);
        emit mqttLogMessage(QString("设置自动发送消息间隔: %1ms").arg(intervalMs).toUtf8());
    }
}

void MqttClient::enableConnectionCheck(bool enable) {
    m_connectionCheckEnabled = enable;

    if (enable && isConnected()) {
        m_connectionCheckTimer->start();
        emit mqttLogMessage("启用连接检查");
    }
    else {
        m_connectionCheckTimer->stop();
        emit mqttLogMessage("禁用连接检查");
    }
}

void MqttClient::enableAutoSendMessage(bool enable) {
    m_autoSendMessageEnabled = enable;

    if (enable && isConnected()) {
        m_sendMessageTimer->start();
        emit mqttLogMessage("启用自动发送消息");
    }
    else {
        m_sendMessageTimer->stop();
        emit mqttLogMessage("禁用自动发送消息");
    }
}

void MqttClient::onStateChanged(QMqttClient::ClientState state) {
    QString stateStr = stateToString(state);
    emit mqttLogMessage(QString("连接状态变化: %1").arg(stateStr).toUtf8());
    emit connectionStateChanged(state);
}

void MqttClient::onErrorChanged(QMqttClient::ClientError error) {
    if (error != QMqttClient::NoError) {
        m_lastError = errorToString(error);
        emit mqttLogMessage(QString("MQTT错误: %1").arg(m_lastError).toUtf8());

        // 需要重连的错误类型
        static const QList<QMqttClient::ClientError> reconnectErrors = {
            QMqttClient::IdRejected,
            QMqttClient::ServerUnavailable,
            QMqttClient::NotAuthorized,
            QMqttClient::TransportInvalid,
            QMqttClient::UnknownError
        };

        // 如果是需要重连的错误类型
        if (reconnectErrors.contains(error)) {
            // 如果当前状态不是连接中或已连接
            if (m_client->state() != QMqttClient::Connected &&
                m_client->state() != QMqttClient::Connecting) {
                // 检查重连次数限制
                if (m_reconnectAttempts >= m_MAX_RECONNECT_ATTEMPTS) {
                    if (m_reconnectAttempts == m_MAX_RECONNECT_ATTEMPTS) {
                        emit mqttLogMessage("已达到最大重连次数，需手动重连MQTT服务");
                        m_reconnectAttempts++; // 避免重复记录
                    }
                    return;
                }
                m_reconnectAttempts++; // 增加重连计数
                // 延迟10秒后重连
                QTimer::singleShot(10000, this, [this]() {
                    if (m_client->state() == QMqttClient::Connected) {
                        // 连接成功时重置计数器
                        m_reconnectAttempts = 0;
                        return;
                    }
                    // 只在重连尝试时提示,并提示次数
                    emit mqttLogMessage(QString("自动重连MQTT服务，尝试次数: %1").arg(m_reconnectAttempts).toUtf8());
                    this->connectToHost();
                });
            }
        }
    }
}

void MqttClient::onMessageReceived(const QByteArray& message, const QMqttTopicName& topic) {
    QString topicStr = topic.name();
    QString messageStr = QString::fromUtf8(message);

    emit messageReceived(topicStr, messageStr);
}

void MqttClient::onConnected() {
    m_lastError.clear();
    emit mqttLogMessage("MQTT连接成功");
    emit connected();

    // 启用连接检查
    if (m_connectionCheckEnabled) {
        m_connectionCheckTimer->start();
    }

    // 启用自动发送消息
    if (m_autoSendMessageEnabled) {
        m_sendMessageTimer->start();
    }
}

void MqttClient::onDisconnected() {
    emit mqttLogMessage("MQTT连接断开");

    // 停止连接检查
    m_connectionCheckTimer->stop();
    // 停止自动发送消息
    m_sendMessageTimer->stop();
}

void MqttClient::onPingResponseReceived() {
}

void MqttClient::checkConnection() {
    if (isConnected()) {
        // 发送ping检查连接
        if (!m_client->requestPing()) {
            emit mqttLogMessage("Ping Timeout");
        }
    }
    else {
        emit mqttLogMessage("连接检查:未连接到服务器");
        m_connectionCheckTimer->stop();
    }
}

void MqttClient::autoSendMessage() {
    if (!isConnected()) {
        emit mqttLogMessage("自动发送消息失败: MQTT未连接");
    }

    emit autoSendMsg(m_topic);
}


QString MqttClient::stateToString(QMqttClient::ClientState state) const {
    switch (state) {
    case QMqttClient::Disconnected:
        return "已断开";
    case QMqttClient::Connecting:
        return "连接中";
    case QMqttClient::Connected:
        return "已连接";
    default:
        return "未知状态";
    }
}

QString MqttClient::errorToString(QMqttClient::ClientError error) const {
    switch (error) {
    case QMqttClient::NoError:
        return "无错误";
    case QMqttClient::InvalidProtocolVersion:
        return "无效的协议版本";
    case QMqttClient::IdRejected:
        return "客户端ID被拒绝";
    case QMqttClient::ServerUnavailable:
        return "服务器不可用";
    case QMqttClient::BadUsernameOrPassword:
        return "用户名或密码错误";
    case QMqttClient::NotAuthorized:
        return "未授权";
    case QMqttClient::TransportInvalid:
        return "传输无效";
    case QMqttClient::ProtocolViolation:
        return "协议违规";
    case QMqttClient::UnknownError:
        return "未知错误";
    case QMqttClient::Mqtt5SpecificError:
        return "MQTT5特定错误";
    default:
        return "未定义错误";
    }
}
