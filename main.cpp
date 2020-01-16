#include <QCoreApplication>
#include "cMqttUtils.h"
#include "cParseConfigureFile.h"
#include "cConnectionInfo.h"
#include "cDataUtils.h"
#include "cSerialPortGateway.h"
#include "cSerialWorker.h"
#include "cGatewayUID.h"
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QThread>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Device>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/WiredDevice>

void myMessageHandler(QtMsgType type,const QMessageLogContext &context,const QString &msg)
{
    const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QFile outFile(homeFolder + "/" + "gwapplog.txt");

    if (outFile.exists()) {
        qint64 sizeInByte = outFile.size();
        if (sizeInByte > 10485760) {
            outFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        } else {
            outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        }
    } else {
         outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    }


    QTextStream ts(&outFile);
    QString txt;
    switch (type) {
    case QtInfoMsg:
        txt = QString("Info: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << txt <<"\t"<< QString(msg)<<endl;
        break;
    case QtDebugMsg:
        txt = QString("Debug: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << txt <<"\t"<< QString(msg)<<endl;
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << txt <<"\t"<< QString(msg)<<endl;
    break;
    case QtCriticalMsg:
        txt = QString("Critical: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << txt <<"\t"<< QString(msg)<<endl;
    break;
    case QtFatalMsg:
        txt = QString("Fatal: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << txt <<"\t"<< QString(msg)<<endl;
        abort();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef LOG_TO_FILE
    qInstallMessageHandler(myMessageHandler);
#endif
    cParseConfigureFile m_parseConfigure;
    cConnectionInfo m_connInfo;
    cMqttUtils *m_mqttUtils = nullptr;
    cSerialPortGateway *m_SerialPortGw = nullptr;
    cSerialWorker *m_SerialWorker = nullptr;
    QThread *m_SerialThread = new QThread(&a);
    NetworkManager::Device::List deviceList = NetworkManager::networkInterfaces();
    NetworkManager::WirelessDevice::Ptr wifiDevice;

    qDebug() << "Gateway UID: " << cGatewayUID::getGateWayUID();

//    // Set Default Value
//    m_connInfo.setHostName("103.232.120.72");
//    //m_connInfo.setHostName("tailor.cloudmqtt.com");
//    m_connInfo.setPort(30200);
//    // m_connInfo.setPort(17082);
//    m_connInfo.setClientId("paho2541357560754");
//    m_connInfo.setUserName("device@microbase.tech");
//    m_connInfo.setPassword("123456789");
//    //m_connInfo.setUserName("khlrlaie");
//    //m_connInfo.setPassword("oxSj4ZD4xqEI");
//    m_connInfo.setQoS(2);
//    m_connInfo.setTopicPublicNoResponse("/Oe8lpCDMj6tKh5DrBHvRonshuDQa/GATEWAY/data");
//    m_connInfo.setTopicSubscribeNoResponse("/Oe8lpCDMj6tKh5DrBHvRonshuDQa/GATEWAY/cmd");
//    m_connInfo.setSerialPortName("/dev/ttyACM0");
//    m_connInfo.setSerialPortBaudrate(static_cast<quint32>(115200));
//    m_parseConfigure.setConfigurationData(m_connInfo);
//    //End of Set Default Value

    m_connInfo = m_parseConfigure.getConfigurationData();


    m_SerialWorker = cSerialWorker::instance();
    m_SerialWorker->setPollPeriod(m_connInfo.getPollPeriod());
    m_SerialWorker->moveToThread(m_SerialThread);
    QObject::connect(m_SerialThread, &QThread::started, m_SerialWorker, &cSerialWorker::mainLoop);
    QObject::connect(m_SerialWorker, &cSerialWorker::finished, m_SerialThread, &QThread::quit, Qt::DirectConnection);

    m_SerialPortGw = cSerialPortGateway::instance();
    QObject::connect(m_SerialWorker, &cSerialWorker::sigReadNodeData, m_SerialPortGw, &cSerialPortGateway::on_ReadNodeData);
    QObject::connect(m_SerialWorker, &cSerialWorker::sigForwardCommand, m_SerialPortGw, &cSerialPortGateway::on_ForwardCommandToNode);

    m_mqttUtils = cMqttUtils::instance(&a);


    while (!m_SerialPortGw->initSerialPort(m_connInfo.getSerialPortName(), m_connInfo.getSerialPortBaudrate())) {
       QThread::sleep(4);
    }

    m_SerialPortGw->on_SetDateTime();

    a.connect(m_SerialPortGw, &cSerialPortGateway::sigSendDataToServer, [m_mqttUtils] (QByteArray dataToServer) {
        if (m_mqttUtils->getState() == QMqttClient::Connected) {
            qDebug() << "Data Ready To Server";
            m_mqttUtils->on_PublicDataToServer(dataToServer);
        } else if (m_mqttUtils->getState() == QMqttClient::Connecting){
            qDebug() << "Connecting ...";
            qDebug() << "Data Lost: " << dataToServer;
        } else {
            qDebug() << "Connection Lost, Waiting For Network Connection....";
        }
    });

    a.connect(m_mqttUtils, &cMqttUtils::sigConnectedToServer, [m_SerialThread] () {
        qDebug() << "Connected To Mqtt Server";
        if (!m_SerialThread->isRunning())
            QTimer::singleShot(3000, m_SerialThread, SLOT(start()));


    });

    a.connect(NetworkManager::notifier(), &NetworkManager::Notifier::primaryConnectionChanged, [m_mqttUtils] (QString uni) {
        qDebug() << "primaryConnectionChanged: " << uni;
        qDebug() << "Disconnecting From Server...";
        m_mqttUtils->disconnectToServer();
        if (QString::compare(uni, "/")) {
            m_mqttUtils->connectToServer();
        }
    });

//    QObject::connect(m_mqttUtils, &cMqttUtils::sigSendCommandToNode, m_SerialPortGw, &cSerialPortGateway::on_ForwardCommandToNode);

//    a.connect(m_mqttUtils, &cMqttUtils::sigDisconnectedFromServer, [m_mqttUtils] () {
//        m_mqttUtils->connectToServer();;
//    });

    foreach (NetworkManager::Device::Ptr dev, deviceList)
    {
        qDebug() << "Found Devices: " << dev->type();
        if (dev->state() == NetworkManager::Device::State::Activated) {
            m_mqttUtils->connectToServer();
            break;
        }
    }
    return a.exec();
}
