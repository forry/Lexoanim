
/**
 * @file
 * WorkerThread class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include "threading/WorkerThread.h"



WorkerThread::WorkerThread( bool deleteWhenDone )
   : QThread( NULL ), _deleteWhenDone( deleteWhenDone )
{
   connect( this, SIGNAL( finished() ), this, SLOT( handleDone() ) );
   connect( this, SIGNAL( terminated() ), this, SLOT( handleDone() ) );
}


WorkerThread::WorkerThread( QObject* parent , bool deleteWhenDone )
   : QThread( parent ), _deleteWhenDone( deleteWhenDone )
{
   connect( this, SIGNAL( finished() ), this, SLOT( handleDone() ) );
   connect( this, SIGNAL( terminated() ), this, SLOT( handleDone() ) );
}


void WorkerThread::handleDone()
{
   done();

   if( _deleteWhenDone )
      delete this;
}
