
/**
 * @file
 * MainThreadRoutine class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef MAIN_THREAD_ROUTINE_H
#define MAIN_THREAD_ROUTINE_H

#include <QEvent>


/**
 * The class for multithreaded environment that allows threads to schedule
 * a task for the execution in main thread.
 */
class MainThreadRoutine : public QEvent
{
public:
   MainThreadRoutine();
   virtual void post();
protected:
   virtual void exec() = 0;
};


#endif /* MAIN_THREAD_ROUTINE_H */
