/**
 * @file
 * Log class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include <cassert>
#include <sstream>
#include <QMainWindow>
#include <QMessageBox>
#include <QPointer>
#include <QTime>
#include <osg/Notify>
#include <osg/Timer>
#if defined(__WIN32__) || defined(_WIN32)
# include <windows.h>
# include <fstream>
#endif
#include "Log.h"
#include "gui/LogWindow.h"

using namespace std;
using namespace osg;

static QPointer< LogWindow > logWindow;
const Log::MsgEnd *Log::endm = NULL;
NotifySeverity logLevel;
bool showInDialog;

// log data may need to be initialized before
// other global variables are initialized ->
// using function static data
struct LogData {
   Timer startTime;
   list< Log::MessageRec > messageList;
   int numMessages;
   OpenThreads::Mutex mutex;
   bool logLevelGivenByEnv;
   bool outputToConsole;
   LogData() : numMessages( 0 ), logLevelGivenByEnv( false ), outputToConsole( false ) {};
};
static LogData *logData;
static inline LogData& getLogData()
{
   static LogData data;
   logData = &data;
   return data;
}


class OsgNotifyHandler : public NotifyHandler
{
public:

   virtual void notify(osg::NotifySeverity severity, const char *message)
   {
      // make sure log data are initialized
      getLogData();

      if( !showInDialog )
      {

         // print to stderr
         if( logData->outputToConsole )
            cerr << message;

         // print to Log window
         Log::msg( QString::fromLocal8Bit( message ), severity );

      }
      else
      {

         // remove terminating \n, if any
         QString s( message );
         if( s.endsWith( '\n' ) )
            s.chop( 1 ); // remove endl

         // QMessageBox
         switch( logLevel ) {

            case INFO:    QMessageBox::information( NULL, "Information", s ); break;
            case NOTICE:  QMessageBox::information( NULL, "Notice", s ); break;
            case WARN:    QMessageBox::warning( NULL, "Warning", s ); break;

            case FATAL:
            default:      QMessageBox::critical( NULL, "Error", s );
         }

      }
   }

};


static void qtMsgHandler( QtMsgType type, const char *msg )
{
   // note: msg parameter includes terminating '\n', more precisely #13#10
   //       (seen on Qt 4.5.2, Win7, MSVC2005)

   // convert to QString and remove terminating \n
   QString s( msg );
   if( s.endsWith( '\n' ) )
      s.chop( 1 ); // remove endl

   // forward message to OSG notify system
   switch( type ) {
   case QtDebugMsg:    OSG_INFO  << s << endl; break;
   case QtWarningMsg:  OSG_WARN  << s << endl; break;
   case QtCriticalMsg: OSG_FATAL << s << endl; break;
   case QtFatalMsg:    OSG_FATAL << s << endl; break;
   default: OSG_FATAL << s << endl; break;
   }
}


LogNotifyRedirectProxy::LogNotifyRedirectProxy()
{
   // make sure log data are initialized
   getLogData();

   // set OSG notify handler
   setNotifyHandler( new OsgNotifyHandler );

   // set Qt message handler
   qInstallMsgHandler( qtMsgHandler );

   // set notify level only if not set by environment variable
   char *s1 = getenv( "OSG_NOTIFY_LEVEL" );
   char *s2 = getenv( "OSGNOTIFYLEVEL" );
   logData->logLevelGivenByEnv = (s1 && strlen( s1 ) > 0) ||
                                 (s2 && strlen( s2 ) > 0);

   // user can disable output to console
   // (OSG_NOFITY_NO_CONSOLE is Lexolights specific environment variable)
   char *s3 = getenv( "OSG_NOTIFY_NO_CONSOLE" );
   bool noConsole = s3 && strlen( s3 ) > 0 && strcmp( s3, "0" ) != 0;

   // output to console is activated by OSG_NOTIFY_LEVEL
   // while it can be disabled by OSG_NOTIFY_LEVEL_NO_CONSOLE
   logData->outputToConsole = logData->logLevelGivenByEnv && !noConsole;

   // allocate console on Windows
   // (this is necessary for GUI applications that does not have console by default)
#if defined(__WIN32__) || defined(_WIN32)
   if( logData->outputToConsole )
   {
      AllocConsole();
      static ofstream consoleCerr( "CONOUT$" );
      cerr.rdbuf( consoleCerr.rdbuf() );
   }
#endif

   // set default log level if not given by environment
   if( !logData->logLevelGivenByEnv )
   {
      setNotifyLevel( INFO );
   }

   // put help message in DEBUG only
   Log::msg( "Log started. To output the messages to console, set OSG_NOTIFY_LEVEL environment variable. "
# if defined(__WIN32__) || defined(_WIN32)
             "(On MSVC, go to project properties (Lexolights properties) -> "
             "Debugging -> Environment and set value to OSG_NOTIFY_LEVEL=INFO, for example)",
# else
             "(On Linux: OSG_NOTIFY_LEVEL=INFO ./lexolights, for example)",
# endif
             ALWAYS );
}


LogNotifyRedirectProxy::LogNotifyRedirectProxy( osg::NotifySeverity severity )
{
   // make sure log data are initialized
   getLogData();

   notify( ALWAYS ).flush();
   setNotifyHandler( new OsgNotifyHandler );
   qInstallMsgHandler( qtMsgHandler );
   setNotifyLevel( severity );
   logData->outputToConsole = false;
}


LogNotifyRedirectProxy::~LogNotifyRedirectProxy()
{
   // flush the stream
   // (this flushes the last message(s) if not terminated with endl)
   notify( ALWAYS ).flush();

   // If log level not given by the environment,
   // set log level to NOTICE for the last moments of the application's life.
   // This avoids excessive info messages printed to the console
   // after the finalization of Log class.
   if( !logData->logLevelGivenByEnv )
   {
      setNotifyLevel( NOTICE );
   }

   // set default handler that outputs messages to console
   setNotifyHandler( new StandardNotifyHandler() );
}


ostream& Log::info()
{
   showInDialog = false;
   return notify( INFO );
}


ostream& Log::notice()
{
   showInDialog = false;
   return notify( NOTICE );
}


ostream& Log::warn()
{
   showInDialog = false;
   return notify( WARN );
}


ostream& Log::fatal()
{
   showInDialog = false;
   return notify( FATAL );
}


ostream& Log::always()
{
   showInDialog = false;
   return notify( ALWAYS );
}


ostream& Log::dlgInfo()
{
   showInDialog = true;
   return notify( INFO );
}


ostream& Log::dlgNotice()
{
   showInDialog = true;
   return notify( NOTICE );
}


ostream& Log::dlgWarn()
{
   showInDialog = true;
   return notify( WARN );
}


ostream& Log::dlgFatal()
{
   showInDialog = true;
   return notify( FATAL );
}


ostream& operator<<( ostream& os, const Log::MsgEnd* )
{
   //assert( &os == &ss && "Bad Log::MsgEnd usage. Please, use it with Log streams only." );

   os << endl;
   return os;
}


ostream& operator<<( ostream& os, const QString& s )
{
   return os << s.toLocal8Bit().data();
}


void Log::msg( const QString &message, osg::NotifySeverity severity )
{
   // make sure log data are initialized
   getLogData();

   msg( message, severity, logData->startTime.time_s() );
}


void Log::msg( const QString &message, osg::NotifySeverity severity, double time )
{
   // make sure log data are initialized
   getLogData();

   // do not log empty strings,
   // (this still allows for empty new lines, as they should be "\n")
   if( message.isEmpty() )
      return;

   // construct message record
   MessageRec rec( severity, time, message );

   {
      // append message record
      OpenThreads::ScopedLock< OpenThreads::Mutex > lock( logData->mutex );
      logData->messageList.push_back( rec );
      logData->numMessages++;

      // send message to logWindow
      if( logWindow && logWindow->isVisible() )
         logWindow->message( rec );
   }
}


bool Log::spawnTimeMsg( const QString &envVar,
                        const QString &message,
                        const QString &failMsg,
                        osg::NotifySeverity severity )
{
   // make sure log data are initialized
   getLogData();

   // get spawn time from environment variable
   // note: there must be, for example, an utility
   //       that sets the variable and starts
   //       the application.
   const char *value = getenv( envVar.toLocal8Bit() );

   if( value && value[0] != '\0' ) {

      // compute the time
      double t1 = atof( value );
      if( t1 ) {

         // convert time to QTime
         // note: we are using modulo of 86400 (seconds per day, e.g. 24*60*60)
         t1 = t1 - ( 86400 * int( t1 / 86400 ) );

         // convert QTime to double
         // note: we have to use QDateTime to convert local time it to UTC
         QTime now = QDateTime::currentDateTime().toUTC().time();
         double t2 = now.hour() * 3600 + now.minute() * 60 +
                     now.second() + now.msec() * 0.001;

         // get time delta
         // and handle possible "day" overflow
         double d = t2 - t1;
         if( d < 0. )  d += 86400.;
         if( d >= 86400. )  d -= 86400.;
#if 0 // debug
         notify( severity ) << QString( "t1: %1,  t2: %2,  d: %3" ).arg( t1, t2, d ) << endl;
#endif

         // print message with time information
         Timer savedTime = logData->startTime;
         logData->startTime.setStartTick();
         logData->startTime.setStartTick( logData->startTime.getStartTick() -
                                          d / logData->startTime.getSecondsPerTick() );
         notify( severity ) << message.arg( d * 1000., 0, 'f', 2 ) << endl;
         logData->startTime = savedTime;
         return true;
      }
   }

   // print failMsg, but only if not empty
   if( !failMsg.isEmpty() )
      notify( severity ) << failMsg << endl;
   return false;
}


void Log::startMsg( const QString &startMsg, osg::NotifySeverity severity )
{
   // make sure log data are initialized
   getLogData();

   // initialize log time
   logData->startTime.setStartTick();

   // print message, but only if not empty
   if( !startMsg.isEmpty() )
      if( startMsg.contains( QChar('%') ) )
         notify( severity ) << startMsg.arg( QTime::currentTime().toString( "h:mm:ss.zzz") ) << endl;
      else
         notify( severity ) << startMsg << endl;
}


const Log::MessageList& Log::lockMessageList()
{
   // make sure log data are initialized
   getLogData();

   logData->mutex.lock();
   return logData->messageList;
}


void Log::unlockMessageList()
{
   logData->mutex.unlock();
}


int Log::getNumMessages()
{
   // make sure log data are initialized
   getLogData();

   return logData->numMessages;
}


bool Log::isLogLevelGivenByEnv()
{
   // make sure log data are initialized
   getLogData();

   return logData->logLevelGivenByEnv;
}


bool Log::isPrintingToConsole()
{
   // make sure log data are initialized
   getLogData();

   return logData->outputToConsole;
}


void Log::showWindow( QMainWindow *parent,
                      QObject *visibilitySignalReceiver,
                      const char *visibilitySignalSlot )
{
   // make sure log data are initialized
   getLogData();

   if( !logWindow ) {

      // create log window
      logWindow = new LogWindow( parent );
      if( parent )
         parent->addDockWidget( Qt::BottomDockWidgetArea,
                                logWindow, Qt::Horizontal );

      // connect visibilityChanged signal
      // note: visibilityChanged signal is available since Qt 4.3
      if( visibilitySignalReceiver && visibilitySignalSlot )
         QObject::connect( logWindow, SIGNAL( visibilityChanged(bool)),
                  visibilitySignalReceiver, visibilitySignalSlot );

      // schedule content update
      logWindow->invalidateMessages();

   } else

      // show the window
      logWindow->setVisible( true );
}


void Log::hideWindow()
{
   // hide log window
   if( logWindow )
      logWindow->hide();
}


bool Log::isVisible()
{
   if( !logWindow )
      return false;
   else
      return logWindow->isVisible();
}


LogWindow* Log::getWindow()
{
   return logWindow;
}
