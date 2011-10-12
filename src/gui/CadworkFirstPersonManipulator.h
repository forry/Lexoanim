
/**
 * @file
 * CadworkFirstPersonManipulator class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef CADWORK_FIRST_PERSON_MANIPULATOR_H
#define CADWORK_FIRST_PERSON_MANIPULATOR_H

#include <osgGA/FirstPersonManipulator>



/**
 * CadworkFirstPersonManipulator is customized osgGA::FirstPersonManipulator
 * for Cadwork style manipulation.
 */
class CadworkFirstPersonManipulator : public osgGA::FirstPersonManipulator
{
   typedef FirstPersonManipulator inherited;

public:

   CadworkFirstPersonManipulator( int flags = DEFAULT_SETTINGS | SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT );
   CadworkFirstPersonManipulator( const CadworkFirstPersonManipulator& fpm,
                                  const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY );

   META_Object( osgGA, CadworkFirstPersonManipulator );

   /** Compute the home position.
    *
    *  CadworkManipulator overrides the method to set initial Cadwork-style camera
    *  rotation and elevation. */
   virtual void computeHomePosition(const osg::Camera *camera = NULL, bool useBoundingBox = false);
};



#endif /* CADWORK_FIRST_PERSON_MANIPULATOR_H */
