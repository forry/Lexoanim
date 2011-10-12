
/**
 * @file
 * ExtenalApplicationWorker class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef EXTERNAL_APPLICATION_WORKER_H
#define EXTERNAL_APPLICATION_WORKER_H

#include <QProcess>
#include <QStringList>
#include <sstream>
#include "threading/WorkerThread.h"


/**
 * The class is specialized WorkerThread that allows to execute external applications
 * and to perform user-defined actions when the external application finishes its execution.
 * This is useful, for instance, to update GUI, or show the user the results
 * of the computation, etc.
 *
 * It is the programmer responsibility to delete the object unless deleteWhenDone
 * constructor parameter is set to true or launch() function is used.
 * In that case, the object is automatically deleted after done() method is executed.
 */
class ExternalApplicationWorker : public WorkerThread {
   typedef WorkerThread inherited;
public:

   ExternalApplicationWorker( const QString& program, const QStringList& arguments,
                              const QString& workingDirectory, bool redirectErrToOut,
                              bool deleteWhenDone );
   ExternalApplicationWorker( QObject* parent, const QString& program, const QStringList& arguments,
                              const QString& workingDirectory, bool redirectErrToOut,
                              bool deleteWhenDone );

protected:

   QString _program;
   QStringList _arguments;
   QString _workingDirectory;
   bool _redirectErrToOut;

   std::stringstream _outBuf;
   std::stringstream _errBuf;
   QProcess::ExitStatus _exitStatus;
   int _exitCode;

public:

   virtual void run();
   virtual void done();

   virtual void handleError( QProcess &p, QProcess::ProcessError error );
   virtual void handleFinished( QProcess &p, int exitCode, QProcess::ExitStatus exitStatus );
   virtual void handleReadyReadStandardOutput( QProcess &p );
   virtual void handleReadyReadStandardError( QProcess &p );
   virtual void handleStarted( QProcess &p );
   virtual void handleStateChanged( QProcess &p, QProcess::ProcessState newState );

   static void processStream( std::ostream &os, std::stringstream& remainder, QByteArray a, bool flush = false );

};


#endif /* EXTERNAL_APPLICATION_WORKER_H */
