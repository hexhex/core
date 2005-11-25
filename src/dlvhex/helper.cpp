/* -*- C++ -*- */

/**
 * @file   helper.cpp
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 *  
 * @brief  Definition of helper functions.
 *  
 *  
 */ 


#include "dlvhex/helper.h"


std::vector<std::string>
helper::stringExplode(const std::string& inString, const std::string& separator)
{
    std::vector<std::string> returnVector;
    
    std::string::size_type start = 0;
    std::string::size_type end = 0;

    while ((end = inString.find(separator, start)) != std::string::npos)
    {
        returnVector.push_back(inString.substr(start, end - start));
        start = end + separator.size();
    }

    returnVector.push_back(inString.substr(start));

    return returnVector;
}


void
helper::escapeQuotes(std::string& str)
{
    std::string single_quote = "\"";
    std::string escape_quote = "\\\"";
    std::string::size_type i = 0;
    
    while (std::string::npos != (i = str.find(single_quote, i)))
    {
        str.replace(i, 1, escape_quote);
        i += 2;
    }
}

