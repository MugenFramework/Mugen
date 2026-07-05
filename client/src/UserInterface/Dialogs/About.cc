#include <global.hpp>
#include <UserInterface/Dialogs/About.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

About::About( QDialog* dialog )
{
    AboutDialog = dialog;
    AboutDialog->setObjectName( QString::fromUtf8( "AboutDialog" ) );
    AboutDialog->setWindowTitle( "About Mugen" );
    AboutDialog->setFixedSize( 380, 410 );
    AboutDialog->setStyleSheet(
        "QDialog {"
        "  background-color: #111118;"
        "}"
        "QLabel {"
        "  color: #f0f0ee;"
        "  background: transparent;"
        "}"
        "QPushButton {"
        "  background-color: #1a1a22;"
        "  color: #f0f0ee;"
        "  border: 1px solid #ff6b9d;"
        "  border-radius: 4px;"
        "  padding: 6px 24px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #ff6b9d;"
        "  color: #0a0a0f;"
        "}"
        "QFrame[frameShape='4'] {"  /* HLine */
        "  color: #1e1e28;"
        "  background-color: #1e1e28;"
        "  border: none;"
        "  max-height: 1px;"
        "}"
    );

    auto* mainLayout = new QVBoxLayout( AboutDialog );
    mainLayout->setContentsMargins( 32, 30, 32, 24 );
    mainLayout->setSpacing( 0 );
    mainLayout->setAlignment( Qt::AlignHCenter );

    // --- Logo (white) ---
    auto* logoLabel = new QLabel( AboutDialog );
    logoLabel->setAlignment( Qt::AlignCenter );

    QPixmap logoPixmap( ":/images/MugenLogo" );
    if ( !logoPixmap.isNull() ) {
        QImage img = logoPixmap.toImage().convertToFormat( QImage::Format_ARGB32 );
        for ( int y = 0; y < img.height(); y++ ) {
            for ( int x = 0; x < img.width(); x++ ) {
                QColor c = img.pixelColor( x, y );
                if ( c.alpha() > 0 )
                    img.setPixelColor( x, y, QColor( 255, 255, 255, c.alpha() ) );
            }
        }
        logoLabel->setPixmap(
            QPixmap::fromImage( img ).scaled( 72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation )
        );
    }
    mainLayout->addWidget( logoLabel );
    mainLayout->addSpacing( 16 );

    // --- Title ---
    auto* titleLabel = new QLabel( "Mugen", AboutDialog );
    titleLabel->setAlignment( Qt::AlignCenter );
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize( 20 );
    titleFont.setBold( true );
    titleLabel->setFont( titleFont );
    mainLayout->addWidget( titleLabel );
    mainLayout->addSpacing( 6 );

    // --- Version ---
    auto* versionLabel = new QLabel(
        QString( "v%1" ).arg( QString::fromStdString( MugenNamespace::Version ) ),
        AboutDialog
    );
    versionLabel->setAlignment( Qt::AlignCenter );
    versionLabel->setStyleSheet( "color: #ff6b9d; font-size: 13px; font-weight: 600;" );
    mainLayout->addWidget( versionLabel );
    mainLayout->addSpacing( 5 );

    // --- Code name ---
    auto* codeNameLabel = new QLabel(
        QString::fromStdString( MugenNamespace::CodeName ),
        AboutDialog
    );
    codeNameLabel->setAlignment( Qt::AlignCenter );
    QFont codeFont = codeNameLabel->font();
    codeFont.setItalic( true );
    codeNameLabel->setFont( codeFont );
    codeNameLabel->setStyleSheet( "color: #555555; font-size: 11px;" );
    codeNameLabel->setWordWrap( true );
    mainLayout->addWidget( codeNameLabel );
    mainLayout->addSpacing( 22 );

    // --- Separator ---
    auto* separator = new QFrame( AboutDialog );
    separator->setFrameShape( QFrame::HLine );
    separator->setFrameShadow( QFrame::Plain );
    mainLayout->addWidget( separator );
    mainLayout->addSpacing( 18 );

    // --- Description ---
    auto* descLabel = new QLabel(
        "Open-source post-exploitation C2 framework.<br>"
        "Forked from Havoc by "
        "<a href=\"https://twitter.com/C5pider\" style=\"color:#ff6b9d; text-decoration:none;\">C5pider</a>."
        " &nbsp;GPL-3.0.",
        AboutDialog
    );
    descLabel->setAlignment( Qt::AlignCenter );
    descLabel->setTextFormat( Qt::RichText );
    descLabel->setOpenExternalLinks( true );
    descLabel->setStyleSheet( "color: #888888; font-size: 11px;" );
    descLabel->setWordWrap( true );
    mainLayout->addWidget( descLabel );
    mainLayout->addSpacing( 10 );

    // --- Links ---
    auto* linksLabel = new QLabel(
        "<a href=\"https://github.com/MugenFramework/Mugen\" style=\"color:#ff6b9d; text-decoration:none;\">GitHub</a>"
        "<span style=\"color:#333333;\"> &nbsp;·&nbsp; </span>"
        "<a href=\"https://mugenframework.github.io/\" style=\"color:#ff6b9d; text-decoration:none;\">Documentation</a>",
        AboutDialog
    );
    linksLabel->setAlignment( Qt::AlignCenter );
    linksLabel->setTextFormat( Qt::RichText );
    linksLabel->setOpenExternalLinks( true );
    linksLabel->setStyleSheet( "font-size: 11px;" );
    mainLayout->addWidget( linksLabel );

    mainLayout->addStretch();

    // --- Close button ---
    pushButton = new QPushButton( "Close", AboutDialog );
    pushButton->setFixedWidth( 90 );

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget( pushButton );
    mainLayout->addLayout( btnLayout );

    QObject::connect( pushButton, &QPushButton::clicked, this, &About::onButtonClose );
}

void About::setupUi() {}

void About::onButtonClose()
{
    AboutDialog->close();
}
