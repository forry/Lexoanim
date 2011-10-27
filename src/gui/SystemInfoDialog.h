
/**
 * @file
 * SystemInfoDialog class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef SYSTEM_INFO_DIALOG_H
#define SYSTEM_INFO_DIALOG_H


#include <QDialog>

namespace Ui {
   class SystemInfoDialog;
}



class SystemInfoDialog : public QDialog
{
   typedef QDialog inherited;

public:

   SystemInfoDialog( QWidget *parent = NULL );
   virtual ~SystemInfoDialog();

   virtual void refreshInfo();

private:
   Ui::SystemInfoDialog *ui;
};


#endif /* SYSTEM_INFO_DIALOG_H */
