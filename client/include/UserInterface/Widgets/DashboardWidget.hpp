#ifndef MUGEN_DASHBOARDWIDGET_HPP
#define MUGEN_DASHBOARDWIDGET_HPP

#include <global.hpp>

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>

class DashboardWidget : public QWidget
{
    Q_OBJECT

    QLabel*       statLiveTengu  = nullptr;
    QLabel*       statLiveDemon  = nullptr;
    QLabel*       statDead       = nullptr;
    QLabel*       statTotal      = nullptr;

    QTableWidget* recentCreds    = nullptr;
    QTableWidget* recentDownloads= nullptr;

    QFrame* makeCard( QLabel** label, const QString& title, const QColor& accent );

public:
    explicit DashboardWidget( QWidget* parent = nullptr );
    void Refresh();
};

#endif
