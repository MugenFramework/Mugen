#ifndef MUGEN_CONNECTOR_HPP
#define MUGEN_CONNECTOR_HPP

#include <global.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QAbstractSocket>

#include <Mugen/Packager.hpp>

namespace MugenNamespace
{
    class Connector : public QTcpSocket
    {
    private:
        QWebSocket*           Socket     = nullptr;
        Util::ConnectionInfo* Teamserver = nullptr;
        MugenSpace::Packager* Packager   = nullptr;

    public:
        QString ErrorString = nullptr;

        Connector( Util::ConnectionInfo* );
        ~Connector() noexcept;

        bool Disconnect();

        void SendLogin();
        void SendPackage( Util::Packager::PPackage package );
    };
}

#endif //MUGEN_CONNECTOR_HPP
