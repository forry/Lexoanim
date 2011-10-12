/****************************************************************************\
 *
 * Windows registry routines.
 *
 * Authors: Martin "martyn" Havlicek
 *          PCJohn (Jan Peciva)
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
 * Windows registry routines.
 *
 * @author Martin "martyn" Havlicek
 * @author PCJohn (Jan Peciva)
 */


#ifndef WIN_REGISTRY_H
#define WIN_REGISTRY_H

#include <string>
#include <list>
#include <QString>
#if defined(__WIN32__) || defined(_WIN32)
# include <windows.h>
#else
// provide dummy types for HKEY and DWORD:
# define HKEY void*
# define DWORD unsigned long
#endif


/**
 * Windows registry routines.
 *
 * All the functions in this namespace do nothing on other platforms.
 */
namespace WinRegistry
{
    // DWORD
    void setDword(HKEY root, const char *subkey, const char *name, const DWORD value);
    void setDword(HKEY root, const std::wstring& subkey, const std::wstring& name, const DWORD value);
    DWORD getDword(HKEY root, const char *subkey, const char *name, const DWORD defaultValue = 0);
    DWORD getDword(HKEY root, const std::wstring& subkey, const std::wstring& name, const DWORD defaultValue = 0);

    // String
    void setString(HKEY root, const char *subkey, const char *name, const char *value);
    void setString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& value);
    std::string getString(HKEY root, const char *subkey, const char *name, const char *defaultValue = "");
    std::wstring getString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& defaultValue = std::wstring());
    inline void setWString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& value);
    inline std::wstring getWString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& defaultValue = std::wstring());

    // QString
    void setString(HKEY root, const QString& subkey, const QString& name, const QString& value);
    QString getString(HKEY root, const QString& subkey, const QString& name, const QString& defaultValue = "");
    inline void setQString(HKEY root, const QString& subkey, const QString& name, const QString& value);
    inline QString getQString(HKEY root, const QString& subkey, const QString& name, const QString& defaultValue = "");

    // Removing
    bool removeKey(HKEY root, const char *subkey, bool evenIfNotEmpty = true);
    bool removeKey(HKEY root, const QString& subkey, bool evenIfNotEmpty = true);
    bool removeValue(HKEY root, const char *subkey, const char *name);
    bool removeValue(HKEY root, const QString& subkey, const QString& name);

    // Existence of key or value
    bool exists(HKEY root, const char *subkey, const char *valueName = NULL);
    bool exists(HKEY root, const QString& subkey, const QString& valueName = "");

    // Num subelements
    enum NumSubElementsType { KEYS, VALUES, KEYS_AND_VALUES };
    DWORD getNumSubElements(HKEY root, const char *subkey, const NumSubElementsType queryType);

    // SubElements
    std::list<std::wstring> getSubElements(HKEY root, const std::wstring& subkey, const NumSubElementsType queryType);

    // Errors
    int getError();
}


//
// inline functions
//
inline void WinRegistry::setWString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& value)
{
   WinRegistry::setString(root, subkey, name, value);
}
inline std::wstring WinRegistry::getWString(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& defaultValue)
{
   return WinRegistry::getString(root, subkey, name, defaultValue);
}
inline void WinRegistry::setQString(HKEY root, const QString& subkey, const QString& name, const QString& value)
{
   WinRegistry::setString(root, subkey, name, value);
}
inline QString WinRegistry::getQString(HKEY root, const QString& subkey, const QString& name, const QString& defaultValue)
{
   return WinRegistry::getString(root, subkey, name, defaultValue);
}

#endif /* WIN_REGISTRY_H */
