
/**
 * @file
 * CustomizedProcess class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef CUSTOMIZED_PROCESS_H
#define CUSTOMIZED_PROCESS_H

#include <QProcess>


/** CustomizedProcess is QProcess derived class providing virtual methods
 *  for all signals that the QProcess can generate. This allows for easy
 *  overriding and implementation of custom actions taken upon the generated signals. */
class CustomizedProcess : public QProcess {
   typedef QProcess inherited;
   Q_OBJECT
public:

   CustomizedProcess( QObject *parent = NULL );

signals:

   /// The signal is generated when the process terminated or failed to start.
   void done();

protected slots:

   virtual void handleError( QProcess::ProcessError error ) {}
   virtual void handleFinished( int exitCode, QProcess::ExitStatus exitStatus ) {}
   virtual void handleReadyReadStandardOutput() {}
   virtual void handleReadyReadStandardError() {}
   virtual void handleStarted() {}
   virtual void handleStateChanged( QProcess::ProcessState newState ) {}

private slots:

   void internalHandleStateChanged( QProcess::ProcessState newState );

};


#endif /* CUSTOMIZED_PROCESS_H */
