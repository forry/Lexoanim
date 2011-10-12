/**
 * @file
 * Log class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef LOG_H
#define LOG_H

#include <QString>
#include <iostream>
#include <list>
#include <osg/Notify>


/**
 * Log class provides message logging capabilities while
 * displaying the messages in the docking window implemented
 * by LogWindow class.
 *
 * It inspects all the osg::notify messages and
 */
class Log {
public:

   static const class MsgEnd {} *endm;

   static std::ostream& info();
   static std::ostream& notice();
   static std::ostream& warn();
   static std::ostream& fatal();
   static std::ostream& always();

   static std::ostream& dlgInfo();
   static std::ostream& dlgNotice();
   static std::ostream& dlgWarn();
   static std::ostream& dlgFatal();

   static void msg( const QString &message, osg::NotifySeverity severity );
   static void msg( const QString &message, osg::NotifySeverity severity, double time );

   static bool spawnTimeMsg( const QString &envVar,
                             const QString &message,
                             const QString &failMsg = QString(""),
                             osg::NotifySeverity severity = osg::NOTICE );
   static void startMsg( const QString &startMsg =
                         "Application started at %1",
                         osg::NotifySeverity severity = osg::NOTICE );

   struct MessageRec {
      osg::NotifySeverity severity;
      double time;
      QString text;
      MessageRec( osg::NotifySeverity s, double t, const QString& msg) :
            severity( s ), time( t ), text( msg )  {}
   };
   typedef std::list< MessageRec > MessageList;

   static const MessageList& lockMessageList();
   static void unlockMessageList();
   static int getNumMessages();
   static bool isPrintingToConsole();
   static bool isLogLevelGivenByEnv();

   static void showWindow( class QMainWindow *parent = NULL,
                           class QObject *visibilitySignalReceiver = NULL,
                           const char *visibilitySignalSlot = NULL );
   static void hideWindow();
   static bool isVisible();
   static class LogWindow* getWindow();

};


std::ostream& operator<<( std::ostream& , const Log::MsgEnd* );
std::ostream& operator<<( std::ostream& , const QString& );


struct LogNotifyRedirectProxy
{
   LogNotifyRedirectProxy();
   LogNotifyRedirectProxy( osg::NotifySeverity severity );
   ~LogNotifyRedirectProxy();
};
#define LOG_NOTIFY_REDIRECT_PROXY() \
static LogNotifyRedirectProxy logNotifyRedirectProxy
#define LOG_NOTIFY_REDIRECT_PROXY_LEVEL( level ) \
static LogNotifyRedirectProxy logNotifyRedirectProxy( level )


#endif /* LOG_H */
