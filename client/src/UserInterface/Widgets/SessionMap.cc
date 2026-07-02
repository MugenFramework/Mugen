#include <global.hpp>
#include <UserInterface/Widgets/SessionMap.hpp>

#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QToolTip>
#include <QResizeEvent>
#include <cmath>

SessionMap::SessionMap( QWidget* parent )
    : QWidget( parent )
{
    setMouseTracking( true );

    svgRenderer = new QSvgRenderer( this );
    svgRenderer->load( QString( ":/map/world" ) );

    nam = new QNetworkAccessManager( this );
}

QPointF SessionMap::pinToScreen( double lat, double lon ) const
{
    // equirectangular projection matching the SVG viewBox "0 0 1000 500"
    // SVG: x = (lon + 180) / 360 * 1000, y = (90 - lat) / 180 * 500
    // We scale to widget size
    double svgX = ( lon + 180.0 ) / 360.0 * 1000.0;
    double svgY = ( 90.0 - lat )  / 180.0 * 500.0;

    double scaleX = static_cast<double>( width() )  / 1000.0;
    double scaleY = static_cast<double>( height() ) / 500.0;

    return QPointF( svgX * scaleX, svgY * scaleY );
}

void SessionMap::geolocate( const QString& agentId, const QString& ip,
                            const QString& displayName, const QString& health )
{
    if ( geoCache.contains( ip ) && geoCache[ip].resolved )
    {
        AgentPin pin;
        pin.agentId     = agentId;
        pin.displayName = displayName;
        pin.ip          = ip;
        pin.health      = health;
        pin.lat         = geoCache[ip].lat;
        pin.lon         = geoCache[ip].lon;
        pins.append( pin );
        update();
        return;
    }

    auto req = QNetworkRequest( QUrl( "http://ip-api.com/json/" + ip + "?fields=status,lat,lon" ) );
    auto reply = nam->get( req );

    connect( reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        if ( reply->error() != QNetworkReply::NoError )
            return;

        auto doc = QJsonDocument::fromJson( reply->readAll() );
        if ( doc.isNull() || !doc.isObject() )
            return;

        auto obj = doc.object();
        if ( obj.value("status").toString() != "success" )
            return;

        double lat = obj.value("lat").toDouble();
        double lon = obj.value("lon").toDouble();

        geoCache[ip] = GeoEntry{ lat, lon, true };

        AgentPin pin;
        pin.agentId     = agentId;
        pin.displayName = displayName;
        pin.ip          = ip;
        pin.health      = health;
        pin.lat         = lat;
        pin.lon         = lon;
        pins.append( pin );
        update();
    } );
}

void SessionMap::UpdateMap( const std::vector<MugenNamespace::Util::SessionItem>& sessions )
{
    pins.clear();

    for ( const auto& s : sessions )
    {
        auto ip = s.External;
        if ( ip.isEmpty() || ip == "N/A" )
            continue;

        // skip RFC1918 addresses - not geolocatable
        if ( ip.startsWith("10.") || ip.startsWith("192.168.") || ip.startsWith("127.") )
            continue;
        if ( ip.startsWith("172.") ) {
            auto parts = ip.split('.');
            if ( parts.size() >= 2 ) {
                int second = parts[1].toInt();
                if ( second >= 16 && second <= 31 )
                    continue;
            }
        }

        geolocate( s.Name, ip, s.User + "@" + s.Computer, s.Health );
    }

    update();
}

void SessionMap::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.setRenderHint( QPainter::Antialiasing );

    // dark background
    p.fillRect( rect(), QColor( "#0d0d1a" ) );

    // draw world SVG
    if ( svgRenderer->isValid() )
        svgRenderer->render( &p, rect() );

    // draw grid lines (subtle)
    p.setPen( QPen( QColor( 40, 44, 68, 80 ), 0.5 ) );
    for ( int lon = -180; lon <= 180; lon += 30 )
    {
        auto top = pinToScreen( 90,  lon );
        auto bot = pinToScreen( -90, lon );
        p.drawLine( top, bot );
    }
    for ( int lat = -60; lat <= 90; lat += 30 )
    {
        auto left  = pinToScreen( lat, -180 );
        auto right = pinToScreen( lat,  180 );
        p.drawLine( left, right );
    }

    // draw agent pins
    for ( const auto& pin : pins )
    {
        QPointF pt = pinToScreen( pin.lat, pin.lon );
        bool live  = pin.health.toLower() == "alive";

        QColor dotColor  = live ? QColor( "#ff6b9d" ) : QColor( "#555566" );
        QColor glowColor = live ? QColor( 255, 107, 157, 60 ) : QColor( 85, 85, 102, 40 );

        // outer glow
        p.setBrush( glowColor );
        p.setPen( Qt::NoPen );
        p.drawEllipse( pt, 10.0, 10.0 );

        // inner dot
        p.setBrush( dotColor );
        p.setPen( QPen( dotColor.lighter(130), 1 ) );
        p.drawEllipse( pt, 5.0, 5.0 );
    }

    // legend (bottom-left)
    p.setPen( QColor("#9090b0") );
    p.setFont( QFont("Monospace", 9) );
    p.drawText( 12, height() - 24, "● Live" );
    p.setPen( QColor("#555566") );
    p.drawText( 12, height() - 12, "● Dead" );
}

void SessionMap::mouseMoveEvent( QMouseEvent* event )
{
    for ( const auto& pin : pins )
    {
        QPointF pt = pinToScreen( pin.lat, pin.lon );
        if ( QLineF( pt, event->pos() ).length() < 10.0 )
        {
            QString tip = QString("%1\nIP: %2\nLocation: %3, %4")
                .arg( pin.displayName )
                .arg( pin.ip )
                .arg( QString::number( pin.lat, 'f', 2 ) )
                .arg( QString::number( pin.lon, 'f', 2 ) );
            QToolTip::showText( event->globalPos(), tip, this );
            return;
        }
    }
    QToolTip::hideText();
}

void SessionMap::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    update();
}
