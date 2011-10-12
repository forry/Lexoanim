
/**
 * @file
 * View load and save functionality.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef VIEW_LOAD_SAVE_H
#define VIEW_LOAD_SAVE_H

class QString;
namespace osg   { class Camera; }
namespace osgGA { class StandardManipulator; }


int saveIVV( const QString& filename, const osg::Camera& camera,
             const osgGA::StandardManipulator& cameraManipulator );
int loadIVV( QString &filename, osg::Camera &camera,
             osgGA::StandardManipulator& cameraManipulator );

/**
 * These save/load functions are used in Delta3D.
 * IMPORTANT! Delta's x and z axes are inverted
 * so we must save/load -x and -z axis.
 */
int saveIVV( const QString& filename, /*const osg::Matrix& modelView*/ osg::Vec3 eye, osg::Vec3 center, const double &fovy  );

int loadIVV( QString &filename, /*osg::Matrix& modelView*/osg::Vec3 &eye, osg::Vec3 &center, double &fovy );


#endif /* VIEW_LOAD_SAVE_H */
