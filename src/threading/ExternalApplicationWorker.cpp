
/**
 * @file
 * ExternalApplicationWorker class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include "threading/ExternalApplicationWorker.h"
#include "threading/CustomizedProcess.h"
#include "utils/Log.h"

using namespace std;



class MyProcess : public CustomizedProcess {
   typedef CustomizedProcess inherited;
public:

   MyProcess( ExternalApplicationWorker *worker ) : _worker( worker ) {}

   ExternalApplicationWorker *_worker;

protected:

   virtual void handleError( QProcess::ProcessError error )  { _worker->handleError( *this, error ); }
   virtual void handleFinished( int exitCode, QProcess::ExitStatus exitStatus )  { _worker->handleFinished( *this, exitCode, exitStatus ); }
   virtual void handleReadyReadStandardOutput()  { _worker->handleReadyReadStandardOutput( *this ); }
   virtual void handleReadyReadStandardError()  { _worker->handleReadyReadStandardError( *this ); }
   virtual void handleStarted()  { _worker->handleStarted( *this ); }
   virtual void handleStateChanged( QProcess::ProcessState newState )  { _worker->handleStateChanged( *this, newState ); }

};



ExternalApplicationWorker::ExternalApplicationWorker( const QString& program,
                                                      const QStringList& arguments,
                                                      const QString& workingDirectory,
                                                      bool redirectErrToOut,
                                                      bool deleteWhenDone )
   : WorkerThread( deleteWhenDone ),
     _program( program ),
     _arguments( arguments ),
     _workingDirectory( workingDirectory ),
     _redirectErrToOut( redirectErrToOut ),
     _exitStatus( QProcess::CrashExit ),
     _exitCode( 0 )
{
}


ExternalApplicationWorker::ExternalApplicationWorker( QObject* parent,
                                                      const QString& program,
                                                      const QStringList& arguments,
                                                      const QString& workingDirectory,
                                                      bool redirectErrToOut,
                                                      bool deleteWhenDone )
   : WorkerThread( parent, deleteWhenDone ),
     _program( program ),
     _arguments( arguments ),
     _workingDirectory( workingDirectory ),
     _redirectErrToOut( redirectErrToOut ),
     _exitStatus( QProcess::CrashExit ),
     _exitCode( 0 )
{
}


void ExternalApplicationWorker::run()
{
   Log::info() << "ExternalApplicationWorker: thread started." << endl;

   // create process
   MyProcess p( this );
   connect( &p, SIGNAL( done() ), this, SLOT( quit() ), Qt::QueuedConnection );
   if( _redirectErrToOut )
      p.setProcessChannelMode( QProcess::MergedChannels );
   p.setWorkingDirectory( _workingDirectory );

   // start process
   Log::info() << "ExternalApplicationWorker: starting \"" << _program << "\" process." << endl;
   p.start( _program, _arguments, QIODevice::ReadOnly );

   // enter thread main loop
   exec();

   Log::info() << "ExternalApplicationWorker: stopping thread." << endl;
}


void ExternalApplicationWorker::done()
{
}


void ExternalApplicationWorker::processStream( ostream &os, stringstream& remainder, QByteArray a, bool flush )
{
   int i = a.lastIndexOf( '\n' );
   if( i == -1 )
      remainder << a.data();
   else
   {
      os << remainder.str() << a.left( i ).data() << endl;
      remainder.str( "" );
      remainder << a.mid( i+1 ).data();
   }

   if( flush && !remainder.str().empty() )
   {
      os << remainder.str() << endl;
      remainder.str( "" );
   }
}


void ExternalApplicationWorker::handleReadyReadStandardOutput( QProcess &p )
{
   processStream( Log::info(), _outBuf, p.readAllStandardOutput() );
}


void ExternalApplicationWorker::handleReadyReadStandardError( QProcess &p )
{
   processStream( Log::warn(), _errBuf, p.readAllStandardError() );
}


void ExternalApplicationWorker::handleStarted( QProcess &p )
{
}


void ExternalApplicationWorker::handleFinished( QProcess &p, int exitCode, QProcess::ExitStatus exitStatus )
{
   processStream( Log::warn(), _errBuf, p.readAllStandardError(), true );
   processStream( Log::info(), _outBuf, p.readAllStandardOutput(), true );

   _exitStatus = exitStatus;
   _exitCode = exitCode;

   Log::info() << "ExternalApplicationWorker: The application terminated with exit status " << ( _exitStatus == QProcess::NormalExit ? "Normal" : "Crash" )
               << " and exit code " << _exitCode << "." << endl;
}


void ExternalApplicationWorker::handleError( QProcess &p, QProcess::ProcessError error )
{
   QString msg;
   switch( error ) {
      case QProcess::FailedToStart: msg = "Application failed to start."; break;
      case QProcess::Crashed:       msg = "Application crashed."; break;
      default:                      msg = "Unknown error."; break;
   }
   Log::warn() << "ExternalApplicationWorker error: " << msg << endl;
}


void ExternalApplicationWorker::handleStateChanged( QProcess &p, QProcess::ProcessState newState )
{
   if( newState == QProcess::NotRunning )
   {
      Log::info() << "ExternalApplicationWorker: external application terminated." << endl;
   }
}
