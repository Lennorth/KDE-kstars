/***************************************************************************
                          timedialog.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : Sun Feb 11 2001
    copyright            : (C) 2001 by Jason Harris
    email                : jharris@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <klocale.h>
#include "timedialog.h"
#include "kstars.h"
#include <kdatepik.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlayout.h>


TimeDialog::TimeDialog( QDateTime now, QWidget* parent, bool isUTCNow )
    : KDialogBase( KDialogBase::Plain, i18n( "set clock to a new time", "Set Time" ), Ok|Cancel, Ok, parent )
{
        ksw = (KStars*) parent;

	UTCNow = isUTCNow;

	QFrame *page = plainPage();

	vlay = new QVBoxLayout( page, 2, 2 );
	hlay = new QHBoxLayout( 2 ); //this layout will be added to the VLayout
	dPicker = new KDatePicker( page );
	dPicker->setDate( now.date() );

	HourBox = new QSpinBox( page, "HourBox" );
  QFont Box_font(  HourBox->font() );
  Box_font.setPointSize( 12 );
  Box_font.setBold( TRUE );
  HourBox->setFont( Box_font );
  HourBox->setWrapping( TRUE );
  HourBox->setMaxValue( 23 );
	HourBox->setButtonSymbols( QSpinBox::PlusMinus );
	HourBox->setValue( now.time().hour() );

  TextLabel1 = new QLabel( page, "TextLabel1" );
  TextLabel1->setText( " :" );
  TextLabel1->setFont( Box_font );

  MinuteBox = new QSpinBox( page, "MinuteBox" );
  QFont MinuteBox_font(  MinuteBox->font() );
  MinuteBox->setFont( Box_font );
  MinuteBox->setWrapping( TRUE );
  MinuteBox->setMaxValue( 59 );
	MinuteBox->setButtonSymbols( QSpinBox::PlusMinus );
	MinuteBox->setValue( now.time().minute() );

  TextLabel1_2 = new QLabel( page, "TextLabel1_2" );
  TextLabel1_2->setFont( Box_font );

  SecondBox = new QSpinBox( page, "SecondBox" );
  SecondBox->setFont( Box_font );
  SecondBox->setMaxValue( 59 );
  SecondBox->setWrapping( TRUE );
	SecondBox->setButtonSymbols( QSpinBox::PlusMinus );
	SecondBox->setValue( now.time().second() );

  NowButton = new QPushButton( page, "NowButton" );
  NowButton->setText( i18n( "Now"  ) );
  NowButton->setFont( Box_font );

	vlay->addWidget( dPicker, 0, 0 );
	vlay->addLayout( hlay, 0 );

	hlay->addWidget( HourBox, 0, 0 );
	hlay->addWidget( TextLabel1, 0, 0 );
	hlay->addWidget( MinuteBox, 0, 0 );
  hlay->addWidget( TextLabel1_2, 0, 0 );
  hlay->addWidget( SecondBox, 0, 0 );
	hlay->addWidget( NowButton );

	vlay->activate();

  QObject::connect( this, SIGNAL( okClicked() ), this, SLOT( accept() ));
  QObject::connect( this, SIGNAL( cancelClicked() ), this, SLOT( reject() ));
  QObject::connect( NowButton, SIGNAL( clicked() ), this, SLOT( setNow() ));
	QObject::connect( HourBox, SIGNAL( valueChanged( int ) ), this, SLOT( HourPrefix( int ) ));
	QObject::connect( MinuteBox, SIGNAL( valueChanged( int ) ), this, SLOT( MinutePrefix( int ) ));
	QObject::connect( SecondBox, SIGNAL( valueChanged( int ) ), this, SLOT( SecondPrefix( int ) ));

}

/*  Main event handler. Reimplemented to handle application
 *  font changes
 */
bool TimeDialog::event( QEvent* ev )
{
    bool ret = QDialog::event( ev );
    if ( ev->type() == QEvent::ApplicationFontChange ) {
      QFont HourBox_font(  HourBox->font() );
      HourBox_font.setFamily( "helvetica" );
      HourBox_font.setPointSize( 12 );
      HourBox_font.setBold( TRUE );
      HourBox->setFont( HourBox_font );
      QFont TextLabel1_font(  TextLabel1->font() );
      TextLabel1_font.setFamily( "helvetica" );
      TextLabel1_font.setPointSize( 12 );
      TextLabel1_font.setBold( TRUE );
      TextLabel1->setFont( TextLabel1_font );
      QFont MinuteBox_font(  MinuteBox->font() );
      MinuteBox_font.setFamily( "helvetica" );
      MinuteBox_font.setPointSize( 12 );
      MinuteBox_font.setBold( TRUE );
      MinuteBox->setFont( MinuteBox_font );
      QFont TextLabel1_2_font(  TextLabel1_2->font() );
      TextLabel1_2_font.setFamily( "helvetica" );
      TextLabel1_2_font.setPointSize( 12 );
      TextLabel1_2_font.setBold( TRUE );
      TextLabel1_2->setFont( TextLabel1_2_font );
      QFont SecondBox_font(  SecondBox->font() );
      SecondBox_font.setFamily( "helvetica" );
      SecondBox_font.setPointSize( 12 );
      SecondBox_font.setBold( TRUE );
      SecondBox->setFont( SecondBox_font );
      QFont NowButton_font(  NowButton->font() );
      NowButton_font.setFamily( "helvetica" );
      NowButton_font.setPointSize( 14 );
      NowButton_font.setBold( TRUE );
      NowButton->setFont( NowButton_font );
    }
    return ret;
}

void TimeDialog::setNow( void )
{
  QTime t;

  if (UTCNow)
  {
    QDateTime dt( ksw->data()->clock()->UTC() );
    dPicker->setDate( dt.date() );
    t = dt.time();
  }
  else
  {
  dPicker->setDate( QDate::currentDate() );
  t = QTime::currentTime();
  }

  HourBox->setValue( t.hour() );
  MinuteBox->setValue( t.minute() );
  SecondBox->setValue( t.second() );
}

void TimeDialog::HourPrefix( int value ) {
	HourBox->setPrefix( "" );
	if ( value < 10 ) HourBox->setPrefix( "0" );
}

void TimeDialog::MinutePrefix( int value ) {
	MinuteBox->setPrefix( "" );
	if ( value < 10 ) MinuteBox->setPrefix( "0" );
}

void TimeDialog::SecondPrefix( int value ) {
	SecondBox->setPrefix( "" );
	if ( value < 10 ) SecondBox->setPrefix( "0" );
}

QTime TimeDialog::selectedTime( void ) {
	QTime t( HourBox->value(), MinuteBox->value(), SecondBox->value() );
	return t;
}

QDate TimeDialog::selectedDate( void ) {
	QDate d( dPicker->getDate() );
	return d;
}

QDateTime TimeDialog::selectedDateTime( void ) {
	QDateTime dt( selectedDate(), selectedTime() );
	return dt;
}

#include "timedialog.moc"
