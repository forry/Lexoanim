
/**
 * @file
 * MainThreadRoutine class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include <QObject>
#include <QCoreApplication>
#include "threading/MainThreadRoutine.h"


static int mainThreadRoutineEventId = QEvent::registerEventType();

class MyEventReceiver : public QObject
{
   virtual void customEvent( QEvent *event )
   {
      if( event->type() == mainThreadRoutineEventId )
      {
         // class to get access to protected exec() method
         class MyMainThreadRoutine : public MainThreadRoutine {
         public:
            static inline void callExec( QEvent *event )  { ((MyMainThreadRoutine*)event)->exec(); }
         };

         // call exec() through MyMainThreadRoutine class
         MyMainThreadRoutine::callExec( event );
      }
   }
};

static MyEventReceiver eventReceiver;


MainThreadRoutine::MainThreadRoutine() : QEvent( QEvent::Type( mainThreadRoutineEventId ) )
{
}


void MainThreadRoutine::post()
{
   QCoreApplication::postEvent( &eventReceiver, this );
}
