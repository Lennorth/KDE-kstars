/***************************************************************************
                          astrocalc.cpp  -  description
                             -------------------
    begin                : wed dec 19 16:20:11 CET 2002
    copyright            : (C) 2001-2005 by Pablo de Vicente
    email                : p.devicente@wanadoo.es
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "astrocalc.h"

#include <QSplitter>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <KTextEdit>
#include <klocale.h>
#include <ktextedit.h>

#include "dms.h"
#include "modcalcjd.h"
#include "modcalcgeodcoord.h"
#include "modcalcgalcoord.h"
#include "modcalcsidtime.h"
#include "modcalcapcoord.h"
#include "modcalcdaylength.h"
#include "modcalcaltaz.h"
#include "modcalcplanets.h"
#include "modcalceclipticcoords.h"
#include "modcalcangdist.h"
#include "modcalcvizequinox.h"
#include "modcalcvlsr.h"
#include "conjunctions.h"

AstroCalc::AstroCalc( QWidget* parent ) :
        KDialog( parent )
{
    // List of messages. Maybe there is better place for it...
    QString message =
        i18n("<QT>"
             "<H2>KStars Astrocalculator</H2>"
             "<P>"
             "The KStars Astrocalculator contains several <B>modules</b> "
             "which perform a variety of astronomy-related calculations.  "
             "The modules are organized into several categories: "
             "<UL>"
             "<LI><B>Time calculators: </B>"
             "Convert between time systems, and predict the timing of celestial events</LI>"
             "<LI><B>Coordinate converters: </B>"
             "Convert between various coordinate systems</LI>"
             "<LI><B>Solar system: </B>"
             "Predict the position of any planet, from a given location on Earth at a given time</LI>"
             "</UL>"
             "</QT>"
            );
    QString messageTime =
        i18n("<QT>"
             "Section which includes algorithms for computing time ephemeris"
             "<UL><LI>"
             "<B>Julian Day:</B> Julian Day/Calendar conversion"
             "</LI><LI>"
             "<B>Sidereal Time:</B> Sidereal/Universal time conversion"
             "</LI><LI>"
             "<B>Almanac:</B> Rise/Set/Transit timing and position data "
             "for the Sun and Moon"
             "</LI><LI>"
             "<B>Equinoxes & Solstices:</B> Equinoxes, Solstices and duration of the "
             "seasons"
             "</LI></UL>"
             "</QT>");
    QString messageCoord =
        i18n("<QT>"
             "Section with algorithms for the conversion of "
             "different astronomical systems of coordinates"
             "<UL><LI>"
             "<B>Galactic:</B> Galactic/Equatorial coordinates conversion"
             "</LI><LI>"
             "<B>Apparent:</B> Computation of current equatorial coordinates"
             " from a given epoch"
             "</LI><LI>"
             "<B>Ecliptic:</B> Ecliptic/Equatorial coordinates conversion"
             "</LI><LI>"
             "<B>Horizontal:</B> Computation of azimuth and elevation for a "
             "given source, time, and location on the Earth"
             "</LI><LI>"
             "<B>Angular Distance:</B> Computation of angular distance between "
             "two objects whose positions are given in equatorial coordinates"
             "</LI><LI>"
             "<B>Geodetic Coords:</B> Geodetic/XYZ coordinate conversion"
             "</LI><LI>"
             "<B>LSR Velocity:</B> Computation of the heliocentric, geocentric "
             "and topocentric radial velocity of a source from its LSR velocity"
             "</LI></UL>"
             "</QT>");
    QString messageSolar =
        i18n("<QT>"
             "Section with algorithms regarding information "
             "on solar system bodies coordinates and times"
             "<UL><LI>"
             "<B>Planets Coords:</B> Coordinates for the planets, moon and sun "
             "at a given time and from a given position on Earth "
             "</LI></UL>"
             "</QT>");

    split = new QSplitter ( this );
    setMainWidget(split);
    setCaption( i18n("Calculator") );
    setButtons( KDialog::Close );

    // Create navigation panel
    navigationPanel = new QTreeWidget(split);
    navigationPanel->setColumnCount(1);
    navigationPanel->setHeaderLabels( QStringList(i18n("Calculator modules")) );
    navigationPanel->setSortingEnabled( false );
    //FIXME: Would be better to make the navigationPanel fit its contents,
    //but I wasn't able to make it work
    navigationPanel->setMinimumWidth( 200 );

    acStack = new QStackedWidget( split );

    splashScreen = new KTextEdit( message, acStack );
    splashScreen->setReadOnly( true );
    acStack->addWidget( splashScreen );


    // Load icons
    QIcon jdIcon = QIcon ("jd.png");
    QIcon geodIcon = QIcon ("geodetic.png");
    QIcon solarIcon = QIcon ("geodetic.png");
    // QIcon sunsetIcon = QIcon ("sunset.png"); // Its usage is commented out.
    QIcon timeIcon = QIcon ("sunclock.png");
    
    /* Populate the tree widget and widget stack */
    // Time-related entries
    QTreeWidgetItem * timeItem = addTreeTopItem(navigationPanel, i18n("Time Calculators"), messageTime);
    timeItem->setIcon(0,timeIcon);

    QTreeWidgetItem * jdItem = addTreeItem(timeItem, i18n("Julian Day"),
                                           addToStack<modCalcJD>());
    jdItem->setIcon(0,jdIcon);

    addTreeItem(timeItem, i18n("Sidereal Time"),
                addToStack<modCalcSidTime>());
    addTreeItem(timeItem, i18n("Almanac"),
                addToStack<modCalcDayLength>());
    addTreeItem(timeItem, i18n("Equinoxes & Solstices"),
                addToStack<modCalcEquinox>());
    //  dayItem->setIcon(0,sunsetIcon);

    // Coordinate-related entries
    QTreeWidgetItem * coordItem = addTreeTopItem(navigationPanel, i18n("Coordinate Converters"), messageCoord);
    addTreeItem(coordItem, i18n("Equatorial/Galactic"),
                addToStack<modCalcGalCoord>());
    addTreeItem(coordItem, i18n("Apparent Coordinates"),
                addToStack<modCalcApCoord>());
    addTreeItem(coordItem, i18n("Horizontal Coordinates"),
                addToStack<modCalcAltAz>());
    addTreeItem(coordItem, i18n("Ecliptic Coordinates"),
                addToStack<modCalcEclCoords>());
    addTreeItem(coordItem, i18n("Angular Distance"),
                addToStack<modCalcAngDist>());
    addTreeItem(coordItem, i18n("Geodetic Coordinates"),
                addToStack<modCalcGeodCoord>());
    addTreeItem(coordItem, i18n("LSR Velocity"),
                addToStack<modCalcVlsr>());

    // Solar System related entries
    QTreeWidgetItem * solarItem = addTreeTopItem(navigationPanel, i18n("Solar System"), messageSolar);
    solarItem->setIcon(0,solarIcon);
    addTreeItem(solarItem, i18n("Planets Coordinates"),
                addToStack<modCalcPlanets>());
    addTreeItem(solarItem, i18n("Conjunctions"),
                addToStack<ConjunctionsTool>());
    
    acStack->setCurrentWidget( splashScreen );
    connect(navigationPanel, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotItemSelection(QTreeWidgetItem *)));
}

template<typename T>
T* AstroCalc::addToStack()
{
    T* t = new T( acStack );
    acStack->addWidget(t);
    return t;
}

QTreeWidgetItem* AstroCalc::addTreeItem(QTreeWidgetItem* parent, QString title, QWidget* widget)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parent, QStringList(title));
    dispatchTable.insert(title, widget);
    return item;
}

QTreeWidgetItem* AstroCalc::addTreeTopItem(QTreeWidget* parent, QString title, QString html)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parent, QStringList(title));
    htmlTable.insert(title, html);
    return item;
}

AstroCalc::~AstroCalc()
{
}

void AstroCalc::slotItemSelection(QTreeWidgetItem *item)
{
    if ( item == 0)
        return;

    //DEBUG
    kDebug() << "Item clicked: " << item->text(0);

    QString s = item->text(0);
    // Lookup in HTML table
    QMap<QString, QString>::iterator iterHTML = htmlTable.find(s);
    if( iterHTML != htmlTable.end() ) {
        splashScreen->setHtml(*iterHTML);
        acStack->setCurrentWidget(splashScreen);
        return;
    }
    // Lookup in frames table
    QMap<QString, QWidget*>::iterator iter = dispatchTable.find(s);
    if( iter != dispatchTable.end() ) {
        acStack->setCurrentWidget( *iter );
    }
}

QSize AstroCalc::sizeHint() const
{
    return QSize(640,430);
}

#include "astrocalc.moc"
