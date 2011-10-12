
/**
 * @file
 * CadworkFirstPersonManipulator class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include "CadworkFirstPersonManipulator.h"

using namespace osg;

static const double homeRotation = 0.7;
static const double homeElevation = 0.61;


/// Constructor.
CadworkFirstPersonManipulator::CadworkFirstPersonManipulator( int flags )
   : inherited( flags )
{
   setWheelMovement( -0.05, RELATIVE_VALUE );
}


/// Constructor.
CadworkFirstPersonManipulator::CadworkFirstPersonManipulator( const CadworkFirstPersonManipulator& com, const osg::CopyOp& copyOp )
   : inherited( com, copyOp )
{
}


void CadworkFirstPersonManipulator::computeHomePosition( const Camera *camera, bool useBoundingBox )
{
   inherited::computeHomePosition( camera, useBoundingBox );

   // update the home position to "Cadwork-style" one
   setHomePosition( _homeCenter + ( Quat( homeRotation, Vec3d( 0., 0., 1. )) * Quat( homeElevation, Vec3d( -1., 0., 0. ) ) * ( _homeEye - _homeCenter ) ),
                    _homeCenter,
                    Quat( homeElevation, Vec3d( -1., 0., 0. ) ) * _homeUp,
                    _autoComputeHomePosition);
}
