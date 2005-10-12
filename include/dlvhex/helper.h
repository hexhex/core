/** @file helper.h
 * Declaration of helper functions
 *  
 * @date 2005.07.09
 * @author Roman Schindlauer
 */

#ifndef _HELPER_H
#define _HELPER_H

#include <string>
#include <vector>

namespace helper
{
    std::vector<std::string> stringExplode(const std::string &inString, const std::string &separator);
    
    void escapeQuotes(std::string &str);
}

#endif // _HELPER_H
