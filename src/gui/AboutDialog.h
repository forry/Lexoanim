
/**
 * @file
 * AboutDialog class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H


#include <QDialog>

namespace Ui {
   class AboutDialog;
}



class AboutDialog : public QDialog
{
   typedef QDialog inherited;

public:

   AboutDialog( QWidget *parent = NULL );
   virtual ~AboutDialog();

   virtual void refreshInfo();

private:
   Ui::AboutDialog *ui;
};


#endif /* ABOUT_DIALOG_H */
