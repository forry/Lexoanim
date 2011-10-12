/**
 * @file
 * LexolightsDocument class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef LEXOLIGHTS_DOCUMENT_H
#define LEXOLIGHTS_DOCUMENT_H

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <QString>
#include <QThread>

class QFileSystemWatcher;
namespace osg {
   class Node;
}


/**
 * The class representing Lexolights document that holds a model.
 *
 * The class serves for the purpose of creating new empty documents,
 * loading the documents from file or zip file and holds the
 * document and model data.
 */
class LexolightsDocument : public QObject, public osg::Referenced
{
   Q_OBJECT
public:

   // Constructor and destructor
   LexolightsDocument();
   virtual ~LexolightsDocument();

   virtual bool openFile( const QString &fileName );
   virtual bool openFile( const QString &fileName, bool background, bool openInMainWindow = false, bool resetViewSettings = true );
   virtual void close();

   virtual bool openFileAsync( const QString &fileName );
   virtual bool openFileAsync( const QString &fileName, bool openInMainWindow, bool resetViewSettings );
   virtual bool isOpenInProgress();
   virtual bool waitForOpenCompleted();

   inline osg::Node* getOriginalScene();
   inline osg::Node* getPPLScene();

   inline const QString& getFileName() const;

signals:

   void sceneChanged();

protected:

   class OpenOperation : public osg::Referenced {
   public:
      QString fileName;
      QString password;
      virtual bool openModel();
      virtual bool openZip();
      virtual bool run();
      inline osg::Node* getOriginalScene() const;
      inline osg::Node* getPPLScene() const;
      inline QString getUnzipDir() const;
      inline bool getSuccess() const;
   protected:
      QString _unzipDir;
      QString _zipFileName;
      QString _modelFileName;
      bool _success;
      osg::ref_ptr< osg::Node > _originalScene;
      osg::ref_ptr< osg::Node > _pplScene;
   };

   class OpenOpThread : public QThread {
      typedef QThread inherited;
   public:
      OpenOpThread( LexolightsDocument *parent, OpenOperation *openOp );
      virtual void run();
      inline OpenOperation* getOpenOperation() const;
   protected:
      osg::ref_ptr< OpenOperation > _openOp;
      virtual void customEvent( QEvent *event );
   };

   QString _fileName;
   QString _unzipDir;
   bool _asyncSuccess;
   bool _openInMainWindow;
   bool _resetViewSettings;
   QFileSystemWatcher *_watcher;
   OpenOpThread *_openOpThread;

   osg::ref_ptr< osg::Node > _originalScene;
   osg::ref_ptr< osg::Node > _pplScene;

private slots:

   virtual void fileChanged( const QString &path );
   virtual void asyncOpenCompleted();

};


inline osg::Node* LexolightsDocument::getOriginalScene()  { return _originalScene; }
inline osg::Node* LexolightsDocument::getPPLScene()  { return _pplScene; }
inline const QString& LexolightsDocument::getFileName() const  { return _fileName; }
inline QString LexolightsDocument::OpenOperation::getUnzipDir() const  { return _unzipDir; }
inline osg::Node* LexolightsDocument::OpenOperation::getOriginalScene() const  { return _originalScene; }
inline osg::Node* LexolightsDocument::OpenOperation::getPPLScene() const  { return _pplScene; }
inline bool LexolightsDocument::OpenOperation::getSuccess() const  { return _success; }
inline LexolightsDocument::OpenOperation* LexolightsDocument::OpenOpThread::getOpenOperation() const  { return _openOp; }


#endif /* LEXOLIGHTS_DOCUMENT_H */
