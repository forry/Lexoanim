
/**
 * @file
 * CadworkOrbitManipulator class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef CADWORK_ORBIT_MANIPULATOR_H
#define CADWORK_ORBIT_MANIPULATOR_H

#include <osgGA/OrbitManipulator>



/**
 * CadworkOrbitManipulator is customized osgGA::OrbitManipulator
 * for Cadwork style manipulation.
 */
class CadworkOrbitManipulator : public osgGA::OrbitManipulator
{
   typedef OrbitManipulator inherited;

public:

   CadworkOrbitManipulator( int flags = DEFAULT_SETTINGS | SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT );
   CadworkOrbitManipulator( const CadworkOrbitManipulator& com,
                            const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY );

   META_Object( osgGA, CadworkOrbitManipulator );

   /** Compute the home position.
    *
    *  CadworkManipulator overrides the method to set initial Cadwork-style camera
    *  rotation and elevation. */
   virtual void computeHomePosition(const osg::Camera *camera = NULL, bool useBoundingBox = false);

   /** Make movement step of manipulator.
    *  This method implements movement for right mouse button.
    *
    *  CadworkManipulator overrides the method to perform the same as left mouse button. */
   virtual bool performMovementRightMouseButton( const double eventTimeDelta, const double dx, const double dy );
};



#endif /* CADWORK_ORBIT_MANIPULATOR_H */
