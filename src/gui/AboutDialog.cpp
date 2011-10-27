
/**
 * @file
 * AboutDialog class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include "gui/AboutDialog.h"
#include "ui_AboutDialog.h"



AboutDialog::AboutDialog( QWidget *parent )
   : inherited( parent )
{
   ui = new Ui::AboutDialog;
   ui->setupUi( this );
   refreshInfo();
}


AboutDialog::~AboutDialog()
{
   delete ui;
}


void AboutDialog::refreshInfo()
{
   ui->versionLabel->setText( ui->versionLabel->text() + " " +
                              QString( "%1.%2" )
                                 .arg( LEXOLIGHTS_VERSION_MAJOR )
                                 .arg( LEXOLIGHTS_VERSION_MINOR ) );
}
