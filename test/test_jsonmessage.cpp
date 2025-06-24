//
// Created by 13419 on 25-6-24.
//

#include <QCoreApplication>
#include <QDebug>
#include "MQTT/jsonMessage.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    JsonMessage jsonMsg;
    jsonMsg.setNodeName("TestNode");

    // 初始化多个站点
    jsonMsg.initStationList(QStringList() << "STA001" << "STA002");

    // 模拟更新
    QVariantList data1;
    data1 << 12.3 << 45.6 << "Good";
    jsonMsg.updateStation("STA001", StationStatus::Connected, data1);

    QVariantList data2;
    data2 << false << "Timeout";
    jsonMsg.updateStation("STA002", StationStatus::Timeout, data2);

    // 输出 JSON
    QString json = jsonMsg.getJsonMessage(false);
    qDebug().noquote() << json.toUtf8().constData();


    return 0;
}
