/**
 * @file
 * PhotorealismData class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include "PhotorealismData.h"

using namespace std;


string PhotorealismData::getToken( stringstream &in )
{
   string r;
   in >> r;
   if( r.empty() || r[0] != '"' )
      return r;

   // remove starting '"'
   r.erase( 0, 1 );

   // return if terminated with '"'
   if( !r.empty() && r[ r.length()-1 ] == '"' ) {
      // remove terminating '"'
      r.resize( r.length()-1 );
      return r;
   }

   // parse string "..." with spaces between " and "
   string s;
   do {
      in >> s;
      if( !in || s.empty() )  return r;
      r += s;
   } while( s[ s.length()-1 ] != '"' );

   // remove terminating '"'
   if( !r.empty() && r[ r.length()-1 ] == '"' )
      r.resize( r.length()-1 );

   return r;
}


bool PhotorealismData::isKey( const string &token )
{
   return !isValue( token );
}


bool PhotorealismData::isValue( const string &token )
{
   // return true on empty token
   // perhaps empty value may have a meaning
   if( token.empty() )
      return true;

   char c = token[0];
   return ( c >= '0' && c <= '9' ) || // 0..9 - numbers
            c == '.' || // floating values of .123
            c == '"' || // string values "text"
            c == '\'';  // characters 'x'
}


string PhotorealismData::getValue( const string &text, const string &valueName )
{
   stringstream in( text );

   string token = getToken( in );
   while( !token.empty() )
   {
      if( isKey( token ) && token == valueName )
      {
         string followingToken = getToken( in );
         if( isValue( followingToken ) )
            return followingToken;
         else
            return "";
      }

      token = getToken( in );
   }

   return "";
}
