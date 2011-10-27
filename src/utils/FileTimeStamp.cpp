/**
 * @file
 * FileTimeStamp class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <map>
#include <sstream>
#include "FileTimeStamp.h"
#include "utils/Log.h"

using namespace std;


static map< string, map< string, FileTimeStamp > > fileTimeStamps;



bool FileTimeStamp::modified()
{
   return this->operator!=( FileTimeStamp( _fileName ) );
}


void FileTimeStamp::set()
{
   update();
}


void FileTimeStamp::set( const char *fileName )
{
   _fileName = fileName;
   update();
}


void FileTimeStamp::set( const std::string &fileName )
{
   _fileName = fileName;
   update();
}


void FileTimeStamp::update()
{
   if( _fileName == "" )
   {
#if defined(__WIN32__) || defined(_WIN32)
      _modifyTime.dwLowDateTime = 0;
      _modifyTime.dwHighDateTime = 0;
#else
      _modifyTime.tv_sec = 0;
      _modifyTime.tv_nsec = 0;
#endif
   }
   else
   {
#if defined(__WIN32__) || defined(_WIN32)
      int l1 = _fileName.length()+1;
      int l2 = l1 * 2;
      wchar_t *ws = new wchar_t[ l2 ];
      bool r1 = MultiByteToWideChar( CP_UTF8, 0, _fileName.c_str(), l1, ws, l2 ) != 0;
      HANDLE f;
      if( r1 )
         f = CreateFileW( ws, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, NULL );
      delete[] ws;
      BOOL r2;
      if( r1 && f != INVALID_HANDLE_VALUE )
      {
         r2 = GetFileTime( f, NULL, NULL, &_modifyTime );
         CloseHandle( f );
      }
      if( !r1 || f == INVALID_HANDLE_VALUE || r2 == 0 )
         Log::warn() << "FileTimeStamp failed to get time stamp of the file\n"
                        "   " << _fileName << Log::endm;
#else
      struct stat s;
      if( stat( _fileName.c_str(), &s ) != 0 )
         Log::warn() << "FileTimeStamp failed to get time stamp of the file\n"
                        "   " << _fileName << Log::endm;
      else
         _modifyTime = s.st_mtim; // st_mtim provides nanosecond precision
                                  // and it works at least on Kubuntu 11.04.
                                  // It is used instead of st_mtime
                                  // that has only one second resolution.
#endif
   }
}


FileTimeStamp& FileTimeStamp::getRecord( const char *fileName )
{
   return getRecord( "", fileName );
}


FileTimeStamp& FileTimeStamp::getRecord( const string &fileName )
{
   return getRecord( "", fileName );
}


FileTimeStamp& FileTimeStamp::getRecord( const char *key, const char *fileName )
{
   return fileTimeStamps[key][ fileName ];
}


FileTimeStamp& FileTimeStamp::getRecord( const string &key, const string &fileName )
{
   return fileTimeStamps[key][ fileName ];
}


string FileTimeStamp::getTimeStampAsString() const
{
#if defined(__WIN32__) || defined(_WIN32)
   ULARGE_INTEGER value64;
   value64.LowPart = _modifyTime.dwLowDateTime;
   value64.HighPart = _modifyTime.dwHighDateTime;
   ostringstream ss;
   ss << value64.QuadPart;
   return ss.str();
#else
   // write nanoseconds and left-pad them with zeros
   ostringstream ss1;
   ss1 << _modifyTime.tv_nsec;
   string s = ss1.str();
   for( int i=s.length(); i<9; i++ )
      s.insert( 0, "0" );

   // write seconds + "." + padded nanoseconds
   ostringstream ss2;
   ss2 << _modifyTime.tv_sec << "." << s;

   // return the result
   return ss2.str();
#endif
}


void FileTimeStamp::setTimeStampFromString( const char *s )
{
#if defined(__WIN32__) || defined(_WIN32)
   ULARGE_INTEGER value64;
   istringstream ss( s );
   ss >> value64.QuadPart;
   _modifyTime.dwLowDateTime = value64.LowPart;
   _modifyTime.dwHighDateTime = value64.HighPart;
#else
   const char *sep = strchr( s, '.' );
   if( sep == NULL )
   {
      istringstream ss1( s );
      ss1 >> _modifyTime.tv_sec;
      _modifyTime.tv_nsec = 0;
   }
   else
   {
      istringstream ss1( string( s, sep-s ) );
      ss1 >> _modifyTime.tv_sec;
      istringstream ss2( string( sep+1 ) );
      ss2 >> _modifyTime.tv_nsec;
   }
#endif
}
