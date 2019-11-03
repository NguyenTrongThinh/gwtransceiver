#ifndef CCONFIGUREUTILS_H
#define CCONFIGUREUTILS_H

#include <QObject>
#include <QMetaType>

class cConnectionInfo
{

public:
    cConnectionInfo();
    cConnectionInfo(const cConnectionInfo &other);
    ~cConnectionInfo();

    QString getHostName() const;
    quint16 getPort() const;
    QString getClientId() const;
    QString getUserName() const;
    QString getPassword() const;
    quint8 getQoS() const;
    QString getTopicPublicNoResponse() const;
    QString getTopicSubscribeNoResponse() const;
    QString getSerialPortName() const;
    qint32 getSerialPortBaudrate() const;

    void setHostName(QString hostname);
    void setPort(quint16 port);
    void setClientId(QString clientId);
    void setUserName(QString user);
    void setPassword(QString password);
    void setQoS(quint8 qos);
    void setTopicPublicNoResponse(QString topic);
    void setTopicSubscribeNoResponse(QString topic);
    void setSerialPortName(QString portName);
    void setSerialPortBaudrate(quint32 baudrate);

private:
    QString m_HostName = nullptr;
    quint16 m_Port = 1883;
    QString m_ClientId = nullptr;
    QString m_UserName = nullptr;
    QString m_password = nullptr;
    quint8 m_QoS = 0;
    QString m_TopicPublicNoResponse = nullptr;
    QString m_TopicSubscribeNoResponse = nullptr;
    QString m_SerialPortName = nullptr;
    qint32 m_SerialPortBaud = 0;
};

Q_DECLARE_METATYPE(cConnectionInfo)

#endif // CCONFIGUREUTILS_H
