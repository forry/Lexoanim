/**
 * @file
 * LogWindow class header.
 *
 * @author Tomáš "Shamot" Burian
 *         PCJohn (Jan Pečiva)
 */


#ifndef LOG_WINDOW_H
#define LOG_WINDOW_H

#include <QDockWidget>
#include <QMap>
#include <osg/Notify>
#include "utils/Log.h"


/**
* Docking window for various application messages.
*/
class LogWindow : public QDockWidget
{
   Q_OBJECT

public:

   LogWindow( QWidget *parent = 0, Qt::WFlags flags = 0 );
   ~LogWindow();

public slots:

   virtual void setShowLevel( osg::NotifySeverity level );

   virtual void updateMessages();
   virtual void invalidateMessages();
   virtual void appendMessages( Log::MessageList::const_iterator start,
                                Log::MessageList::const_iterator end );

   virtual void message( const Log::MessageRec &msg );
   static QString message2html( const Log::MessageRec &msg );

   bool isScrolledDown() const;
   void makeScrolledDown();

protected:

   virtual void buildGui();
   virtual void showEvent( QShowEvent* event );

   QWidget *mainWidget;

   class QVBoxLayout *mainLayout;
   class QHBoxLayout *controlsLayout;

   class QComboBox *showLevelChooser;
   class QComboBox *chooseSomething;
   class QToolButton *printSomething;
   class QTextEdit *messageWidget;

   class QTimer *timer;
   Log::MessageList pendingMessages;

   QMap< QString, QString > highlightMap;
   osg::NotifySeverity showLevel;

   bool updateMessagesScheduled;

protected slots:

   virtual void printSomethingCB();
   virtual void showLevelChanged( const QString& text );
   virtual void startPendingMessagesTimer();
   virtual void processPendingMessages();

signals:

   void updateMessagesSignal();
   void pendingMessagesSignal();

};


#endif /* LOG_WINDOW_H */
