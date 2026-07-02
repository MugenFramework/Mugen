#ifndef MUGEN_SESSIONMAP_HPP
#define MUGEN_SESSIONMAP_HPP

#include <global.hpp>

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QPair>
#include <QMouseEvent>
#include <QSvgRenderer>
#include <QToolTip>

class SessionMap : public QWidget
{
    Q_OBJECT

    struct GeoEntry {
        double lat = 0;
        double lon = 0;
        bool   resolved = false;
    };

    struct AgentPin {
        QString             agentId;
        QString             displayName;
        QString             ip;
        QString             health;
        double              lat = 0;
        double              lon = 0;
    };

    QSvgRenderer*        svgRenderer  = nullptr;
    QNetworkAccessManager* nam        = nullptr;
    QMap<QString, GeoEntry> geoCache;
    QList<AgentPin>      pins;

    void geolocate( const QString& agentId, const QString& ip,
                    const QString& displayName, const QString& health );

    QPointF pinToScreen( double lat, double lon ) const;

public:
    explicit SessionMap( QWidget* parent = nullptr );
    void UpdateMap( const std::vector<MugenNamespace::Util::SessionItem>& sessions );

protected:
    void paintEvent( QPaintEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    void resizeEvent( QResizeEvent* event ) override;
};

#endif
