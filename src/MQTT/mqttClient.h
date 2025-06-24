#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtMqtt/qmqttclient.h>
#include <QtMqtt/qmqtttopicname.h>

/**
 * @brief MQTT客户端封装类
 * 提供MQTT通信的完整功能，具有高度可移植性
 */
class MqttClient : public QObject
{
    Q_OBJECT

public:
    explicit MqttClient(QObject *parent = nullptr);
    ~MqttClient();

    // 连接相关方法
    void setConnectionParameters(const QString &host, quint16 port, const QString &clientId = QString());
    void setCredentials(const QString &username, const QString &password);
    void connectToHost();
    void disconnectFromHost();

    // 消息发布和订阅
    qint32 publishMessage(const QString &topic, const QString &message, quint8 qos = 0, bool retain = false);
    bool subscribeToTopic(const QString &topic, quint8 qos = 0);
    void unsubscribeFromTopic(const QString &topic);

    // 状态查询
    bool isConnected() const;
    QMqttClient::ClientState getState() const;
    QString getLastError() const;

    // 连接检查配置
    void setConnectionCheckInterval(int intervalMs = 60000); // 默认60秒检查一次
    void enableConnectionCheck(bool enable = true);

    // 定时发送消息
    void setSendMessageInterval(int intervalMs = 10000); // 默认10秒发送一次
    void enableAutoSendMessage(bool enable = true);

public slots:
    void stopMqttClient(); // 停止MQTT客户端连接

signals:
    // 连接状态信号
    void connected();
    void disconnected();
    void connectionStateChanged(QMqttClient::ClientState state);
    void connectionError(const QString &error);

    // 消息相关信号
    void autoSendMsg(const QString &topic);
    void messageReceived(const QString &topic, const QString &message);
    void messagePublished(qint32 messageId);
    void subscriptionSucceeded(const QString &topic);
    void subscriptionFailed(const QString &topic);

    // 日志信号
    void mqttLogMessage(QByteArray message, bool showOnScreen = true);

private slots:
    void onStateChanged(QMqttClient::ClientState state);
    void onErrorChanged(QMqttClient::ClientError error);
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void onConnected();
    void onDisconnected();
    void onPingResponseReceived();
    void checkConnection(); // 定时检查连接状态
    void autoSendMessage(); // 定时发送消息

private:
    QMqttClient *m_client;          // Qt MQTT客户端实例
    QString m_hostname;             // 服务器地址
    quint16 m_port;                 // 服务器端口
    QString m_clientId;             // 客户端ID
    QString m_topic;                // 主题
    QString m_username;             // 用户名
    QString m_password;             // 密码
    QString m_lastError;            // 最后的错误信息

    int m_reconnectAttempts = 0;          // 错误重连计数器
    int m_MAX_RECONNECT_ATTEMPTS = 3;     // 最大重连次数

    QTimer *m_connectionCheckTimer; // 连接检查定时器
    bool m_connectionCheckEnabled;  // 是否启用连接检查

    QTimer *m_sendMessageTimer; // 发送消息定时器
    bool m_autoSendMessageEnabled;  // 是否启用发送消息

    void initializeClient();       // 初始化客户端
    void setupSignalSlots();       // 设置信号槽连接
    QString stateToString(QMqttClient::ClientState state) const; // 状态转换为字符串
    QString errorToString(QMqttClient::ClientError error) const; // 错误转换为字符串
};

#endif // MQTTCLIENT_H