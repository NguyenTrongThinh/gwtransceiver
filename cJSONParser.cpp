#include "cJSONParser.h"
#include "cGatewayUID.h"
#include <QDebug>

cJSONParser::cJSONParser(QObject *parent) : QObject(parent)
{

}

cJSONParser::~cJSONParser()
{

}

QByteArray cJSONParser::rawCommandFromServer(QByteArray jsonFromServer)
{
    QByteArray command;
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFromServer, &jsonError);
    if( jsonError.error != QJsonParseError::NoError )
         qDebug() << QString("Json error: %1").arg(jsonError.errorString());
    else if( jsonDoc.isNull() )
         qDebug() << "Null JsonDocument";
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
//        quint32 gwuid = static_cast<quint32>(jsonObj.take("gwuid").toInt(0));
//        qDebug() << "Gateway UID From Server: " << gwuid;
//        if (cGatewayUID::getGateWayUID() == gwuid) {
        QJsonArray jsonArray = jsonObj.take("command").toArray();
        while(jsonArray.count() > 0)
        {
            command.append(static_cast<quint8>(jsonArray.takeAt(0).toInt()));
        }
//        }
    }
    return command;
}

QJsonDocument cJSONParser::createJSONToServer(QByteArray data)
{
    //quint32 gwuid = cGatewayUID::getGateWayUID();
    QJsonArray commandArray;
    QJsonObject itemObj;
    for (int i = 0; i < data.count(); i++) {
        commandArray.append(QJsonValue(data.at(i)));
    }
    //itemObj.insert("gwuid", static_cast<int>(gwuid));
    itemObj.insert("nodedata", QJsonValue(commandArray));
    QJsonDocument m_configInfoJson = QJsonDocument(itemObj);
    return m_configInfoJson;
}
