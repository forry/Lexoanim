/**
 * @file
 * PhotorealismData class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef PHOTOREALISM_DATA_H
#define PHOTOREALISM_DATA_H

#include <string>
#include <sstream>


class PhotorealismData
{
public:

   static std::string getToken( std::stringstream &in );

   static bool isKey( const std::string &token );
   static bool isValue( const std::string &token );

   static std::string getValue( const std::string &text, const std::string &valueName );

};


#endif /* PHOTOREALISM_DATA_H */
