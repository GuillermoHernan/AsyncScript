/*
 * TinyJS
 * 
 * Miscellaneous functions implementation file.
 */

#include "ascript_pch.hpp"
#include "utils.h"
#include "OS_support.h"
#include "jsLexer.h"

#include <string>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <stdio.h>
//#include <cmath>
#include <math.h>
#include <sys/stat.h>

using namespace std;

// ----------------------------------------------------------------------------------- Utils

bool isWhitespace(char ch)
{
    return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

bool isNumeric(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

bool isNumber(const string &str)
{
    for (size_t i = 0; i < str.size(); i++)
        if (!isNumeric(str[i])) return false;
    return true;
}

bool isHexadecimal(char ch)
{
    return ((ch >= '0') && (ch <= '9')) ||
            ((ch >= 'a') && (ch <= 'f')) ||
            ((ch >= 'A') && (ch <= 'F'));
}

bool isOctal(char ch)
{
    return (ch >= '0') && (ch <= '7');
}

bool isOctal(const std::string& str)
{
    for (size_t i = 0; i < str.size(); ++i)
        if (!isOctal(str[i]))
            return false;
    
    return true;
}

bool isAlpha(char ch)
{
    return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ch == '_';
}

bool isIDString(const char *s)
{
    if (!isAlpha(*s))
        return false;
    while (*s)
    {
        if (!(isAlpha(*s) || isNumeric(*s)))
            return false;
        s++;
    }
    return true;
}

void replace(string &str, char textFrom, const char *textTo)
{
    int sLen = strlen(textTo);
    size_t p = str.find(textFrom);
    while (p != string::npos)
    {
        str = str.substr(0, p) + textTo + str.substr(p + 1);
        p = str.find(textFrom, p + sLen);
    }
}

/**
 * Checks if 'str' starts with the given prefix.
 * @param str
 * @param prefix
 * @return 
 */
bool startsWith (const std::string& str, const std::string& prefix)
{
    if (prefix.length() > str.length())
        return false;
    
    int i = int(prefix.length())-1;
    
    for (; i>=0 && str[i] == prefix[i]; --i);
    
    return i < 0;    
}

/**
 * Splits a string in several parts, at the occurrences of the separator string
 * @param inputStr
 * @param separator
 * @return 
 */
StringVector split (const std::string& str, const std::string& separator)
{
    StringVector result;

    size_t begin = 0;
    size_t end = str.find(separator);
    while (end != string::npos)
    {
        result.push_back(str.substr(begin, end - begin));
        begin = end + separator.length();
        end = str.find(separator, begin);
    }

    result.push_back(str.substr(begin));

    return result;
}

/**
 * Joins a vector of strings into a single string separated by the separator
 * string.
 * @param strings
 * @param separator
 * @return 
 */
std::string join (const StringVector& strings, const std::string& separator)
{
    ostringstream output;
    const size_t n = strings.size();
    for (size_t i = 0; i < n; i++)
    {
        if (i > 0) 
            output << separator;
        output << strings[i];
    }

    return output.str();
}



int copyWhile(char* dest, const char* src, bool (*conditionFN)(char), int maxLen)
{
    int i = 0;
    for (i = 0; src[i] && i < maxLen && conditionFN(src[i]); ++i)
        dest[i] = src[i];

    dest[i] = 0;

    return i;
}

const char* skipWhitespace(const char* input)
{
    while (isWhitespace(*input))
        ++input;

    return input;
}

const char* skipNumeric(const char* input)
{
    while (isNumeric(*input))
        ++input;

    return input;
}

const char* skipHexadecimal(const char* input)
{
    while (isHexadecimal(*input))
        ++input;

    return input;
}



/// convert the given string into a quoted string suitable for javascript

std::string escapeString(const std::string &str, bool quote)
{
    std::string result;
    
    result.reserve((str.size() * 11) / 10);
    
    for (size_t i = 0; i < str.size(); i++)
    {
        char        szTemp[16];
        const char  c = str[i];

        switch (c)
        {
        case '\\': result += "\\\\";    break;
        case '\n': result += "\\n";     break;
        case '\r': result += "\\r";     break;
        case '\t': result += "\\t";     break;
        case '\a': result += "\\a";     break;
        case '\"': result += "\\\"";    break;
        default:
            if (c >0 && (c < 32 || c == 127))
            {
                sprintf_s(szTemp, "\\x%02X", (int)c);
                result += szTemp;
            }
            else
                result += c;
            break;            
        }//switch
    }
    
    if (quote)
        return "\"" + result + "\"";
    else
        return result;
}

/** Is the string alphanumeric */
bool isAlphaNum(const std::string &str)
{
    if (str.size() == 0) return true;
    if (!isAlpha(str[0])) return false;
    for (size_t i = 0; i < str.size(); i++)
        if (!(isAlpha(str[i]) || isNumeric(str[i])))
            return false;
    return true;
}

/**
 * Transforms a double into a string.
 * @param x
 * @return 
 */
std::string double_to_string(double x)
{
    if (isnan(x))
        return "[NaN]";
    else
    {
        char szTemp[128];
        sprintf_s (szTemp, "%lg", x);
        return szTemp;
    }
}


/**
 * Gets a 'Not a Number' value.
 * @return 
 */
double getNaN()
{
    return nan("");
}

/**
 * Reads a text file an returns its contents as a string
 * @param szPath
 * @return 
 */
std::string readTextFile (const std::string& szPath)
{
    struct stat results;
    if (stat(szPath.c_str(), &results) != 0)
        return "";

    int size = results.st_size;
    FILE *file = fopen(szPath.c_str(), "rb");

    if (!file)
        return "";

    char *buffer = new char[size + 1];
    long actualRead = fread(buffer, 1, size, file);
    memset (buffer + actualRead, 0, size - actualRead);
    fclose(file);
    
    string result(buffer, buffer + actualRead);

    delete[] buffer;
    return result;
    
}

/**
 * Writes a text file
 * @param szPath
 * @param szContent
 * @return true if successful
 */
bool writeTextFile (const std::string& szPath, const std::string& szContent)
{
    string parent = parentPath (szPath);
    
    if (!createDirIfNotExist (parent))
        return false;
    
    FILE* file = fopen (szPath.c_str(), "w");
    if (file == NULL)
        return false;
    
    const size_t result = fwrite (szContent.c_str(), 1, szContent.size(), file);    
    fclose (file);
    
    return result == szContent.size();
}


/**
 * Creates a directory if it does not exist
 * @param szPath
 * @return true if created successfully or it already existed. False if it has not
 * been able to create it (for example, because it exists and is not a directory)
 */
bool createDirIfNotExist (const std::string& szPath)
{
    if (szPath.empty())
        return true;
    
    struct stat st;
    
    if (stat(szPath.c_str(), &st) == 0)
        return S_ISDIR(st.st_mode);
    else
    {
        createDirIfNotExist( parentPath(szPath) );
        return mkdir (szPath.c_str(), S_IRWXU | S_IRWXG) == 0;
    }
}

#ifdef _WIN32
    const char* DIR_SEPARATORS = "\\/";
#else
    const char* DIR_SEPARATORS = "/";
#endif

/**
 * Gets the directory of a file. If the path is already a directory, it returns
 * the input path.
 * @param szPath
 * @return 
 */
std::string dirFromPath (const std::string& szPath)
{
    const size_t  len = szPath.size();
    
    if (len == 0 || szPath.find_last_of(DIR_SEPARATORS) == len-1)
        return szPath;
    else
        return parentPath(szPath);
}


/**
 * Gets the parent path (parent directory) of a given path.
 * @param szPath
 * @return 
 */
std::string parentPath (const std::string& szPath)
{
    size_t index = szPath.find_last_of (DIR_SEPARATORS);
    
    if (index == szPath.size()-1 && szPath.size() > 0)
        index = szPath.find_last_of (DIR_SEPARATORS, index-1);
    
    if (index != string::npos)
        return szPath.substr(0, index+1);
    else
        return "";
}

/**
 * Removes extension from a file path
 * @param szPath
 * @return 
 */
std::string removeExt (const std::string& szPath)
{
    const size_t index = szPath.rfind ('.');
    
    if (index != string::npos)
        return szPath.substr(0, index);
    else
        return szPath;
}

/**
 * Returns the filename + extension part of a path.
 * @param szPath
 * @return 
 */
std::string fileFromPath (const std::string& szPath)
{
    const size_t index = szPath.find_last_of (DIR_SEPARATORS);
    
    if (index != string::npos)
        return szPath.substr(index+1);
    else
        return szPath;
}

/**
 * Transforms the path into a normalized form, in order to avoid two equivalent
 * paths having different representations.
 * 
 * @param path
 * @return 
 */
std::string normalizePath (const std::string& path)
{
    string temp = path;
    
#ifdef _WIN32
    replace(temp, "\\", "/");
#endif
    
    StringVector components = split(temp, "/");
    StringVector filteredComp;
    bool first = true;
    
    for (const string& comp : components)
    {
        if (first || (comp != "" && comp == "."))
        {
            if (comp == ".." && !filteredComp.empty() && filteredComp.back() != "..")
                filteredComp.pop_back();
            else
                filteredComp.push_back(comp);
        }
        
        first = false;
    }
    
    return join(filteredComp, "/");
}

/**
 * Joins two paths
 * 
 * @param base
 * @param relative
 * @return 
 */
std::string joinPaths (const std::string& base, const std::string& relative)
{
    if (base.size() > 0 && *base.rbegin() != '/')
        return base + "/" + relative;
    else
        return base + relative;
}

/**
 * Checks if a path is relative
 * @param path
 * @return 
 */
bool isPathRelative (const std::string& path)
{
#ifdef _WIN32
    if (path.size() >= 3 && path[1] == ':')
        return isPathRelative(path.substr(2));
    else if (path.empty())
        return true;
    else
        return path[0] != '/' && path[0] != '\\';
#else
    if (path.empty())
        return true;
    else 
        return path[0] != '/';
#endif
}

/**
 * Gets the current working directory of the process
 * @return 
 */
std::string getCurrentDirectory()
{
    char * dir = getcwd(NULL, 0);
    string result = dir;
    
    free (dir);
    return result;
}



/**
 * Indents a text in two space increments.
 * @param indent
 * @return 
 */
std::string indentText(int indent)
{
    std::string result;
    
    result.reserve(indent * 2);
    
    for (int i=0; i < indent; ++i)
        result += "  ";
    
    return result;
}
