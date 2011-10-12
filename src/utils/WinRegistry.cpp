/****************************************************************************\
 *
 * Windows registry routines.
 *
 * Author: Martin "martyn" Havlicek
 * Minor contributions: PCJohn (Jan Peciva)
 *
 * License: public domain
 *
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain.
 * You may use, modify or distribute it freely.
 *
 * This source code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY.  ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED.  This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you find the source code useful, authors will kindly welcome
 * if you give them credit and keep their names with their source code,
 * but you are not forced to do so.
 *
\****************************************************************************/

/**
 * @file
 * Windows registry routines implementation.
 *
 * @author Martin "martyn" Havl��ek
 * @author PCJohn (Jan Peciva) - removeKey() implementation, RegCloseKey() fixes,
 *                               removeValue(), getError()
 */


#if defined(__WIN32__) || defined(_WIN32)
# include <assert.h>
# include <shlwapi.h>
# include <errno.h>
# pragma comment(lib, "shlwapi.lib")
#endif
#include "WinRegistry.h"

static int error = 0;



#if defined(__WIN32__) || defined(_WIN32)

static void setRegDataW(HKEY root, LPCWSTR subkey, const LPCWSTR name, DWORD dataType, const BYTE* data, DWORD dataSize)
{

   HKEY key;
   DWORD tmp;
   LONG e;

   // create or open key
   e = RegCreateKeyExW(root, subkey, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE,
                       NULL, &key, &tmp);

   // handle errors when opening the key
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
      return; // return because we can not open the key
   }

   // set value using utf16 encoding
   e = RegSetValueExW(key, name, 0, dataType,
                      data, dataSize);

   // handle errors of setting the value
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   // close key
   e = RegCloseKey(key);
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

}


static void setRegDataA(HKEY root, LPCSTR subkey, const LPCSTR name, DWORD dataType, const BYTE* data, DWORD dataSize)
{

   HKEY key;
   DWORD tmp;
   LONG e;

   // create or open key
   e = RegCreateKeyExA(root, subkey, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE,
                       NULL, &key, &tmp);

   // handle errors of opening the key
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
      return; // return because we can not open the key
   }

   // set value using ANSI encoding
   e = RegSetValueExA(key, name, 0, dataType,
                      data, dataSize);

   // handle errors of setting the value
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   // close key
   e = RegCloseKey(key);
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

}

#endif


/**
 * Sets DWORD key in the registry.
 *
 * This is non-unicode version of the function.
 *
 * @param root   Registry tree root (for example HKEY_CURRENT_USER or
 *               HKEY_CLASSES_ROOT).
 * @param subkey Path to the desired value.
 * @param name   Name of the value.
 * @param value  New value to be set.
 */
void WinRegistry::setDword(HKEY root, const char *subkey, const char *name,
                           const DWORD value)
{
#if defined(__WIN32__) || defined(_WIN32)

   setRegDataA(root, subkey, name, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));

#endif
}


/**
 * Sets DWORD key in the registry.
 *
 * This is unicode version of the function.
 *
 * @param root   Registry tree root (for example HKEY_CURRENT_USER or
 *               HKEY_CLASSES_ROOT).
 * @param subkey Path to the desired value.
 * @param name   Name of the value.
 * @param value  New value to be set.
 */
void WinRegistry::setDword(HKEY root, const std::wstring& subkey, const std::wstring& name,
                           const DWORD value)
{
#if defined(__WIN32__) || defined(_WIN32)

   setRegDataW(root, subkey.c_str(), name.c_str(), REG_DWORD, (const BYTE*)&value, sizeof(DWORD));

#endif
}


/**
 * Reads REG_DWORD key from the registry.
 * This is non-unicode version of the function.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired value.
 * @param name         Name of the value.
 * @param defaultValue Default value returned if can not read the value from registry.
 *
 * @return Key value, on non-Windows systems returns @a defaultValue always.
 */
DWORD WinRegistry::getDword(HKEY root, const char *subkey, const char *name,
                            const DWORD defaultValue)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   DWORD size;
   DWORD result;
   LONG e;

   // open key
   e = RegOpenKeyEx(root, subkey, 0, KEY_ALL_ACCESS, &key);
   if (e != ERROR_SUCCESS) {
      if (e != ENOENT && error == 0) // do not record ENOENT error
         error = e;
      return defaultValue; // return because we can not open the key
   }

   // query value
   size = sizeof(result);
   LONG r = RegQueryValueEx(key, name, 0, NULL, (LPBYTE)&result, &size);

   // close key
   e = RegCloseKey(key);

   // RegQueryValueEx: value not found
   if (r == ENOENT)
      return defaultValue;

   // RegQueryValueEx: other error
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
      return defaultValue;
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return result;

#else

   return defaultValue;

#endif
}


/**
 * Reads REG_DWORD key from the registry.
 * This is unicode version of the function.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired value.
 * @param name         Name of the value.
 * @param defaultValue Default value returned if can not read the value from registry.
 *
 * @return Key value, on non-Windows systems returns @a defaultValue always.
 */
DWORD WinRegistry::getDword(HKEY root, const std::wstring& subkey, const std::wstring& name,
                            const DWORD defaultValue)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   DWORD size;
   DWORD result;
   LONG e;

   // open key
   e = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_ALL_ACCESS, &key);
   if (e != ERROR_SUCCESS) {
      if (e != ENOENT && error == 0) // do not record ENOENT error
         error = e;
      return defaultValue; // return because we can not open the key
   }

   // query value
   size = sizeof(result);
   LONG r = RegQueryValueExW(key, name.c_str(), 0, NULL, (LPBYTE)&result, &size);

   // close key
   e = RegCloseKey(key);

   // RegQueryValueEx: value not found
   if (r == ENOENT)
      return defaultValue;

   // RegQueryValueEx: other error
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
      return defaultValue;
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return result;

#else

   return defaultValue;

#endif
}


/**
 *  Stores the string in the registry.
 *
 *  This is non-unicode version of setString function.
 *  For more details, see std::wstring version of this function.
 */
void WinRegistry::setString(HKEY root, const char *subkey, const char *name,
                            const char *value)
{
#if defined(__WIN32__) || defined(_WIN32)

   // set value using ANSI encoding (including terminating \0)
   setRegDataA(root, subkey, name, REG_SZ, (const BYTE*)value, strlen(value)+1);

#endif
}


/**
 * Stores unicode string to the registry.
 * The function stores the string as REG_SZ type.
 *
 * std::wstring and Windows stores unicode strings as utf16,
 * so that is the way used to pass unicode strings through Win API.
 *
 * Performance considerations: Non-unicode Win API internally converts
 * all the strings to unicode, as all registry data are stored as utf16.
 * So, to avoid conversions, it should be the best option to have all the
 * application strings already at unicode.
 *
 * @param root   Registry tree root (for example HKEY_CURRENT_USER or
 *               HKEY_CLASSES_ROOT).
 * @param subkey Path to the desired value.
 * @param name   Name of the value.
 * @param value  New value to be set.
 */
void WinRegistry::setString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& value)
{
#if defined(__WIN32__) || defined(_WIN32)

   // set value using utf16 encoding (including terminating \0)
   setRegDataW(root, subkey.c_str(), name.c_str(), REG_SZ, (const BYTE*)value.c_str(), (value.length()+1)*2);

#endif
}


/**
 * Stores unicode string to the registry.
 * The function works the same way as its std::string version,
 * except that it takes QString parameters. QString handles strings internally as utf16.
 * For more details, see std::wstring version of the function.
 *
 * @param root   Registry tree root (for example HKEY_CURRENT_USER or
 *               HKEY_CLASSES_ROOT).
 * @param subkey Path to the desired value.
 * @param name   Name of the value.
 * @param value  New value to be set.
 */
void WinRegistry::setString(HKEY root, const QString& subkey, const QString& name, const QString& value)
{
#if defined(__WIN32__) || defined(_WIN32)

   // set value using utf16 encoding (including terminating \0)
   setRegDataW(root, (LPCWSTR)subkey.utf16(), (LPCWSTR)name.utf16(), REG_SZ,
               (const BYTE*)value.constData(), (value.length()+1)*2);

#endif
}


/**
 *  This is non-unicode version of getString function.
 */
std::string WinRegistry::getString(HKEY root, const char *subkey, const char *name, const char *defaultValue)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   DWORD size;
   char *buf;
   std::string result;
   LONG e;

   // open key
   e = RegOpenKeyEx(root, subkey, 0, KEY_READ, &key);
   if (e != ERROR_SUCCESS) {
      if (e != ENOENT && error == 0) // do not record ENOENT error
         error = e;
      return std::string(defaultValue); // return because we can not open the key
   }

   // determine buffer size
   LONG r = RegQueryValueEx(key, name, 0, NULL,
                            NULL, &size);

   // get value
   if (r == ERROR_SUCCESS) {
      buf = new char[size];
      r = RegQueryValueEx(key, name, 0, NULL,
                          (LPBYTE)buf, &size);
      result = buf;
      delete[] buf;
   }

   // close key
   e = RegCloseKey(key);


   // RegQueryValueEx: value not found
   if (r == ENOENT)
      return std::string(defaultValue);

   // RegQueryValueEx: other error
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
      return std::string(defaultValue);
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return result;

#else

   return std::string(defaultValue);

#endif
}


/**
 * Reads string key from the registry. The function is unicode compatible.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired value.
 * @param name         Name of the value.
 * @param defaultValue Default value returned if can not read it from registry.
 *
 * @return Key value, on non-Windows systems returns @a defaultValue always.
 */
std::wstring WinRegistry::getString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& defaultValue)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   DWORD size;
   wchar_t *buf;
   std::wstring result;
   LONG e;

   // open key
   e = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ, &key);
   if (e != ERROR_SUCCESS) {
      if (e != ENOENT && error == 0) // do not record ENOENT error
         error = e;
      return defaultValue; // return because we can not open the key
   }

   // determine buffer size
   LONG r = RegQueryValueExW(key, name.c_str(), 0, NULL,
                             NULL, &size);

   // get value
   if (r == ERROR_SUCCESS) {
      buf = new wchar_t[size];
      r = RegQueryValueExW(key, name.c_str(), 0, NULL,
                           (LPBYTE)buf, &size);
      result.assign(buf, (size-1)/2);
      delete[] buf;
   }

   // close key
   e = RegCloseKey(key);

   // RegQueryValueEx: value not found
   if (r == ENOENT)
      return defaultValue;

   // RegQueryValueEx: other error
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
      return defaultValue;
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return result;

#else

   return defaultValue;

#endif
}


/**
 * Reads string key from the registry. The function is unicode compatible.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired value.
 * @param name         Name of the value.
 * @param defaultValue Default value returned if can not read it from registry.
 *
 * @return Key value, on non-Windows systems returns @a defaultValue always.
 */
QString WinRegistry::getString(HKEY root, const QString& subkey, const QString& name, const QString& defaultValue)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   DWORD size;
   char *buf;
   QString result;
   LONG e;

   // open key
   e = RegOpenKeyExW(root, (LPCWSTR)subkey.utf16(), 0, KEY_READ, &key);
   if (e != ERROR_SUCCESS) {
      if (e != ENOENT && error == 0) // do not record ENOENT error
         error = e;
      return defaultValue; // return because we can not open the key
   }

   // determine buffer size
   const ushort* nameUtf16 = name.utf16();
   LONG r = RegQueryValueExW(key, (LPCWSTR)nameUtf16, 0, NULL,
                             NULL, &size);

   // get value
   if (r == ERROR_SUCCESS) {
      buf = new char[size];
      r = RegQueryValueExW(key, (LPCWSTR)nameUtf16, 0, NULL,
                           (LPBYTE)buf, &size);
      result.setUtf16((ushort*)buf, (size-1)/2);
      delete[] buf;
   }

   // close key
   e = RegCloseKey(key);

   // RegQueryValueEx: value not found
   if (r == ENOENT)
      return defaultValue;

   // RegQueryValueEx: other error
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
      return defaultValue;
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return result;

#else

   return defaultValue;

#endif
}


/**
 *  This is non-unicode version of removeKey function.
 */
bool WinRegistry::removeKey(HKEY root, const char *subkey, bool evenIfNotEmpty)
{
#if defined(__WIN32__) || defined(_WIN32)

   return removeKey(root, QString().fromLocal8Bit(subkey), evenIfNotEmpty);

#else
   return true;
#endif
}


/**
 * Removes the key from the registry.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired value.
 * @param evenIfNotEmpty
 *                     If true, the key is removed including all subkeys.
 *                     If false, the key is removed only in the case it is empty,
 *                     e.g. no subkeys and no values except default one.
 *
 * @return TRUE if the key was removed and is not existing any more.
 *         If the key does not exist, the function does nothing and returns TRUE.
 *         In all other cases the function returns FALSE.
 */
bool WinRegistry::removeKey(HKEY root, const QString& subkey, bool evenIfNotEmpty)
{
#if defined(__WIN32__) || defined(_WIN32)

   bool reallyDelete = true;

   if (evenIfNotEmpty == false) {

      // does the key have any subkeys or values?
      // If yes, set reallyDelete to true.

      // open key
      HKEY key;
      DWORD numSubKeys, numValues;
      FILETIME ft;
      LONG r = RegOpenKeyExW(root, (LPCWSTR)subkey.utf16(), 0, KEY_ALL_ACCESS, &key);
      if (r != ERROR_SUCCESS) {

         // non-existing key is ok
         if (r == ENOENT)
            return true;

         // otherwise record the error
         if (error == 0)
            error = r;

         return false;  // return because we can not open the key
                        // and we would probably be not able to delete it
      }

      // query key
      int queryErrorCode = RegQueryInfoKeyW(key, NULL, NULL, NULL,
                                            &numSubKeys, NULL, NULL,
                                            &numValues, NULL, NULL, NULL, &ft);

      // close key
      r = RegCloseKey(key);

      // handle errors
      if (queryErrorCode != ERROR_SUCCESS) {
         if (error == 0)
            error = queryErrorCode;
         return false; // can not query number of subkeys?
      }
      if (r != ERROR_SUCCESS) {
         if (error == 0)
            error = r;
      }

      // set reallyDelete
      reallyDelete = numSubKeys==0 && numValues==0;
   }

   // do not delete
   if (!reallyDelete)
      return false;

   // delete key (including subkeys)
   int r = SHDeleteKeyW(root, (LPCWSTR)subkey.utf16());
   if (r == ERROR_SUCCESS || r == ENOENT)
      return true;

   // handle error
   if (error == 0)
      error = r;
   return false;

#else
   return true;
#endif
}


/**
 *  This is non-unicode version of removeValue function.
 */
bool WinRegistry::removeValue(HKEY root, const char *subkey, const char *name)
{
#if defined(__WIN32__) || defined(_WIN32)

   return removeValue(root, QString().fromLocal8Bit(subkey), QString().fromLocal8Bit(name));

#else
   return true;
#endif
}


/**
 * Removes the value from the registry.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired value.
 * @param name         Name of the value to be removed.
 *
 * @return TRUE if the key was removed and is not existing any more.
 *         If the key do not exist, the function does nothing and returns TRUE.
 *         In all other cases the function returns FALSE.
 */
bool WinRegistry::removeValue(HKEY root, const QString& subkey, const QString& name)
{
#if defined(__WIN32__) || defined(_WIN32)

   // open key
   HKEY key;
   int r = RegOpenKeyExW(root, (LPCWSTR)subkey.utf16(), 0, KEY_ALL_ACCESS, &key);
   if (r != ERROR_SUCCESS) {

      // non-existing key is ok
      if (r == ENOENT)
         return true;

      // otherwise record the error
      if (error == 0)
         error = r;

      return false;  // return because we can not open the key
                     // and we would probably be not able to delete it
   }

   // delete value
   int deleteErrorCode = RegDeleteValueW(key, (LPCWSTR)name.utf16());

   // close key
   r = RegCloseKey(key);

   // handle errors
   if (deleteErrorCode != ERROR_SUCCESS && deleteErrorCode != ENOENT) {
      if (error == 0)
         error = deleteErrorCode;
      return false;
   }
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
   }

   // success
   return true;

#else
   return true;
#endif
}


/**
 *  This is non-unicode version of exists function.
 */
bool WinRegistry::exists(HKEY root, const char *subkey, const char *valueName)
{
#if defined(__WIN32__) || defined(_WIN32)

   return exists(root, QString().fromLocal8Bit(subkey), QString().fromLocal8Bit(valueName));

#else

   return false;

#endif
}


/**
 * Returns whether the subelement exists.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired registry key.
 * @param valueName    If the parameter is NULL, subkey existence is determined.
 *                     Otherwise, value existence given by valueName parameter is
 *                     investigated.
 *
 * @return Returns whether the element exists or not.
 *         Return value defaults to FALSE in the case of error.
 */
bool WinRegistry::exists(HKEY root, const QString& subkey, const QString& valueName)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   LONG e,r;
   bool result = true;

   // open key
   e = RegOpenKeyExW(root, (LPCWSTR)subkey.utf16(), 0, KEY_READ, &key);
   if (e != ERROR_SUCCESS) {
      if (e == ENOENT)
         return false;
      if (error == 0)
         error = e;
      return false; // return because we can not open the key
   }

   // querying existence of the value
   if (!valueName.isEmpty()) {
      DWORD tmp;
      r = RegQueryValueExW(key, (LPCWSTR)valueName.utf16(), NULL, &tmp, NULL, NULL);
   }

   // close key
   e = RegCloseKey(key);

   // querying existence of the value
   if (!valueName.isEmpty()) {

      // RegQueryValueEx: return value determine the result
      result = (r == ERROR_SUCCESS);

      // RegQueryValueEx errors
      if (r != ERROR_SUCCESS && r != ENOENT) {
         if (error == 0)
            error = r;
         result = false; // error defaults the result to FALSE
      }
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
      result = false; // error defaults the result to FALSE
   }

   return result;

#else

   return false;

#endif
}


/**
 * Returns number of subelements of the key.
 *
 * @param root         Registry tree root (for example HKEY_CURRENT_USER or
 *                     HKEY_CLASSES_ROOT).
 * @param subkey       Path to the desired registry key.
 * @param queryType    Depending on parameter value, return number of subkeys,
 *                     values, or both.
 *
 * @return Returns number of subelements - keys, values, or both, depending
 *         on queryType parameter. Return value defaults to zero in the case
 *         of error.
 */
DWORD WinRegistry::getNumSubElements(HKEY root, const char *subkey, const NumSubElementsType queryType)
{
#ifdef _WIN32

   HKEY key;
   DWORD numSubKeys, numValues;
   LONG e;

   // open key
   e = RegOpenKeyEx(root, subkey, 0, KEY_READ, &key);
   if (e != ERROR_SUCCESS) {
      if (e != ENOENT && error == 0) // do not record ENOENT error
         error = e;
      return 0; // return because we can not open the key
   }

   // query value
   LONG r = RegQueryInfoKey(key, NULL, NULL, NULL, &numSubKeys, NULL,
                            NULL, &numValues, NULL, NULL, NULL, NULL);

   // close key
   e = RegCloseKey(key);

   // RegQueryInfoKey errors
   if (r != ERROR_SUCCESS) {
      if (error == 0)
         error = r;
      return 0;
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return numSubKeys+numValues;

#else

   return 0;

#endif
}


std::list<std::wstring> WinRegistry::getSubElements(HKEY root, const std::wstring& subkey, const NumSubElementsType queryType)
{
#if defined(__WIN32__) || defined(_WIN32)

   HKEY key;
   LONG e;
   LONG r1 = ERROR_SUCCESS;
   LONG r2 = ERROR_SUCCESS;
   std::list<std::wstring> list;

   // key access rights
   REGSAM accessRights = 0;
   if (queryType == KEYS   || queryType == KEYS_AND_VALUES)  accessRights |= KEY_ENUMERATE_SUB_KEYS;
   if (queryType == VALUES || queryType == KEYS_AND_VALUES)  accessRights |= KEY_QUERY_VALUE;

   // open key
   e = RegOpenKeyExW(root, subkey.c_str(), 0, accessRights, &key);
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
      return list; // return empty list because we can not open the key
   }

   // query names of keys
   if (queryType == KEYS || queryType == KEYS_AND_VALUES)
   {
      int i = 0;
      FILETIME ts;
      const int bufsize = 256;
      wchar_t buf[bufsize]; // max key length is limited to 255 characters
      do {
         DWORD size = bufsize;
         r1 = RegEnumKeyExW(key, i, buf, &size, NULL, NULL, NULL, &ts);
         if (r1 != ERROR_SUCCESS) break;
         i++;
         list.push_back(buf);
      } while (true);
   }

   // query names of values
   if (queryType == VALUES || queryType == KEYS_AND_VALUES)
   {
      int i = 0;
      const int bufsize = 16384;
      wchar_t *buf = new wchar_t[bufsize]; // max value name length is limited to 16383 characters
      do {
         DWORD size = bufsize;
         r2 = RegEnumValueW(key, i, buf, &size, NULL, NULL, NULL, NULL);
         if (r2 != ERROR_SUCCESS) break;
         i++;
         list.push_back(buf);
      } while (true);
      delete[] buf;
   }

   // close key
   e = RegCloseKey(key);

   // RegEnumKeyExW errors
   if (r1 != ERROR_SUCCESS && r1 != ERROR_NO_MORE_ITEMS) {
      if (error == 0)
         error = r1;
      return std::list<std::wstring>();
   }

   // RegEnumValueW errors
   if (r2 != ERROR_SUCCESS && r2 != ERROR_NO_MORE_ITEMS) {
      if (error == 0)
         error = r2;
      return std::list<std::wstring>();
   }

   // RegCloseKey error
   if (e != ERROR_SUCCESS) {
      if (error == 0)
         error = e;
   }

   return list;

#else

   return std::list<std::wstring>();

#endif
}


/**
 * Returns error code of the first error that was generated while using registry.
 * The call of getError clears the current error code, so the following calls of
 * getError returns 0 until the next registry related error is generated.
 *
 * Typical usage of the function is detection of whether ceratin registry
 * operations succeed because Windows Vista protects some areas of registry
 * against regular user writes. The application may be required to run as
 * administrator. To detect missing rights, check against ERROR_ACCESS_DENIED
 * error code.
 *
 * @return Error code.
 */
int WinRegistry::getError()
{
    int r = error;
    error = 0;
    return r;
}
