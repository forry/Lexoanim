/**
 * @file
 * FileTimeStamp class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef FILE_TIME_STAMP_H
#define FILE_TIME_STAMP_H

#include <string>
#if defined(__WIN32__) || defined(_WIN32)
# include <windows.h>
#endif


class FileTimeStamp
{
public:
   inline FileTimeStamp();
   inline FileTimeStamp( const char* fileName );
   inline FileTimeStamp( const std::string &fileName );
   inline FileTimeStamp( const char *ts, const char *fileName );
   inline FileTimeStamp( const std::string &ts, const std::string &fileName );

   inline bool operator==( const FileTimeStamp &ts ) const;
   inline bool operator!=( const FileTimeStamp &ts ) const;

   inline const char* fileName() const;
   inline bool valid() const;

   bool modified();
   void set();
   void set( const char *fileName );
   void set( const std::string &fileName );

   static FileTimeStamp& getRecord( const char *fileName );
   static FileTimeStamp& getRecord( const std::string &fileName );
   static FileTimeStamp& getRecord( const char *key, const char *fileName );
   static FileTimeStamp& getRecord( const std::string &key, const std::string &fileName );

   std::string getTimeStampAsString() const;
   void setTimeStampFromString( const char *s );
   inline void setTimeStampFromString( const std::string &s );

protected:
   std::string _fileName;
#if defined(__WIN32__) || defined(_WIN32)
   FILETIME _modifyTime;
#else
   timespec _modifyTime;
#endif
   void update();
};



inline FileTimeStamp::FileTimeStamp( const char* fileName )  { set( fileName ); }
inline FileTimeStamp::FileTimeStamp( const std::string &fileName )  { set( fileName ); }
inline FileTimeStamp::FileTimeStamp( const char *ts, const char *fileName )  { _fileName = fileName; setTimeStampFromString( ts ); }
inline FileTimeStamp::FileTimeStamp( const std::string &ts, const std::string &fileName )  { _fileName = fileName; setTimeStampFromString( ts ); }
inline const char* FileTimeStamp::fileName() const  { return _fileName.c_str(); }
inline bool FileTimeStamp::valid() const  { return _fileName != ""; }
inline bool FileTimeStamp::operator!=( const FileTimeStamp &ts ) const  { return !operator==( ts ); }
inline void FileTimeStamp::setTimeStampFromString( const std::string &s )  { setTimeStampFromString( s.c_str() ); }
#if defined(__WIN32__) || defined(_WIN32)
inline FileTimeStamp::FileTimeStamp()  { _modifyTime.dwLowDateTime = 0; _modifyTime.dwHighDateTime = 0; }
inline bool FileTimeStamp::operator==( const FileTimeStamp &ts ) const  { return _modifyTime.dwLowDateTime == ts._modifyTime.dwLowDateTime && _modifyTime.dwHighDateTime == ts._modifyTime.dwHighDateTime; }
#else
inline FileTimeStamp::FileTimeStamp()  { _modifyTime.tv_sec = 0; _modifyTime.tv_nsec = 0; }
inline bool FileTimeStamp::operator==( const FileTimeStamp &ts ) const  { return _modifyTime.tv_sec == ts._modifyTime.tv_sec && _modifyTime.tv_nsec == ts._modifyTime.tv_nsec; }
#endif


#endif /* FILE_TIME_STAMP_H */
