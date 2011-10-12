
/**
 * @file
 * CustomizedProcess class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include "threading/CustomizedProcess.h"

using namespace std;



CustomizedProcess::CustomizedProcess( QObject *parent )
   : inherited( parent )
{
   connect( this, SIGNAL( error( QProcess::ProcessError ) ),        this, SLOT( handleError( QProcess::ProcessError ) ),        Qt::DirectConnection );
   connect( this, SIGNAL( finished( int, QProcess::ExitStatus ) ),  this, SLOT( handleFinished( int, QProcess::ExitStatus ) ),  Qt::DirectConnection );
   connect( this, SIGNAL( readyReadStandardOutput() ),              this, SLOT( handleReadyReadStandardOutput() ),              Qt::DirectConnection );
   connect( this, SIGNAL( readyReadStandardError() ),               this, SLOT( handleReadyReadStandardError() ),               Qt::DirectConnection );
   connect( this, SIGNAL( started() ),                              this, SLOT( handleStarted() ),                              Qt::DirectConnection );
   connect( this, SIGNAL( stateChanged( QProcess::ProcessState ) ), this, SLOT( internalHandleStateChanged( QProcess::ProcessState ) ), Qt::DirectConnection );
}


void CustomizedProcess::internalHandleStateChanged( QProcess::ProcessState newState )
{
   // handle user actions first
   handleStateChanged( newState );

   // emit done afterwards
   if( newState == QProcess::NotRunning )
   {
      emit done();
   }
}
