/* -*- C++ -*- */

/**
 * @file   OoutputBuilder.cpp
 * @author Roman Schindlauer
 * @date   Mon Feb 20 14:33:28 CET 2006
 * 
 * @brief  Builders for solver result.
 * 
 * 
 */

#include "dlvhex/OutputBuilder.h"


/*
OutputBuilder::OutputBuilder()
{ }

OutputBuilder::~OutputBuilder()
{ }
*/

std::string
OutputBuilder::getString()
{
    std::string str(stream.str());

    stream.str("");
    stream.clear();

    return str;
}

void
OutputTextBuilder::buildAnswerSet(const AtomSet& facts)
{
    stream << "{";

    for (AtomSet::const_iterator f = facts.begin();
         f != facts.end();
         ++f)
    {
        if (f != facts.begin())
            stream << ", ";

        (*f).print(stream, 0);
    }

    stream << "}" << std::endl;

    //
    // empty line
    //
    stream << std::endl;
}


void
OutputXMLBuilder::buildAnswerSet(const AtomSet& facts)
{
    stream << "<answerset>";

    for (AtomSet::const_iterator f = facts.begin();
         f != facts.end();
         ++f)
    {
        stream << "<fact>";

        stream << "<pred><![CDATA[";

            if ((*f).isStrongNegated())
                stream << "-";

            stream << (*f).getArgument(0);

            if (getArity() > 1)
            {
                stream << "(";
                
                for (unsigned i = 1; i < getArity(); i++)
                {
                    stream << getArgument(i);
                    
                    if (i < getArity() - 1)
                        stream << ",";
                }
                
                stream << ")";
            }

        stream << "]]></fact>";// << std::endl;
    }

    stream << "</answerset>" << std::endl;
}
