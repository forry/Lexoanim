
/**
 * @file
 * SceneInfoDialog class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef SCENE_INFO_DIALOG_H
#define SCENE_INFO_DIALOG_H


#include <QDialog>

namespace Ui {
   class SystemInfoDialog;
}



class SceneInfoDialog : public QDialog
{
   typedef QDialog inherited;

public:

   SceneInfoDialog( QWidget *parent = NULL );
   virtual ~SceneInfoDialog();

   virtual void refreshInfo();

private:
   Ui::SystemInfoDialog *ui;
};


#endif /* SYSTEM_INFO_DIALOG_H */
