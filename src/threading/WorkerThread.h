
/**
 * @file
 * WorkerThread class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <QThread>


/**
 * The class derived from QThread that allows for user-defined actions
 * when the thread finished its execution. This is useful, for instance, to update
 * GUI, or show the user the results of the computation, etc.
 *
 * The programmer will usually want to override run() method for the work performed
 * by the thread and done() method for the actions performed by main thread
 * when the work is done.
 *
 * It is the programmer responsibility to delete the object unless deleteWhenDone
 * constructor parameter is set to true. In that case, the object is automatically
 * deleted after done() method is executed.
 */
class WorkerThread : public QThread
{
   Q_OBJECT
public:
   WorkerThread( bool deleteWhenDone );
   WorkerThread( QObject* parent = NULL, bool deleteWhenDone = false );
protected slots:
   virtual void handleDone();
protected:
   virtual void done() {};
   bool _deleteWhenDone;
};


#endif /* WORKER_THREAD_H */
