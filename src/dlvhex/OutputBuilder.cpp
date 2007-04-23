/* -*- C++ -*- */

/**
 * @file   OutputBuilder.cpp
 * @author Roman Schindlauer
 * @date   Mon Feb 20 14:33:28 CET 2006
 * 
 * @brief  Builders for solver result.
 * 
 * 
 */

#include "dlvhex/OutputBuilder.h"
#include "dlvhex/globals.h"
#include "dlvhex/PrintVisitor.h"

/*
OutputBuilder::OutputBuilder()
{ }

OutputBuilder::~OutputBuilder()
{ }
*/


void
OutputTextBuilder::buildAnswerSet(const AnswerSet& facts)
{
	if ((facts.hasWeights()) && !Globals::Instance()->getOption("AllModels"))
		stream << "Best model: ";

	RawPrintVisitor rpv(stream);
	facts.accept(rpv);
	stream << std::endl;

	if (facts.hasWeights())
	{
		stream << "Cost ([Weight:Level]): <";

		//
		// Display all weight values up to the highest specified level
		//
		for (unsigned lev = 1; lev <= AnswerSet::getMaxLevel(); ++lev)
		{
			if (lev > 1)
				stream << ",";

			stream << "[" << facts.getWeight(lev) << ":" << lev << "]";
		}

		stream << ">" << std::endl;
	}

	//
	// empty line
	//
	if (!Globals::Instance()->getOption("Silent"))
		stream << std::endl;
}


void
OutputXMLBuilder::buildPre()
{
	stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

	stream << "<RuleML xmlns=\"http://www.ruleml.org/0.9/xsd\"\n"
		   << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
		   << "xsi:schemaLocation=\"http://www.ruleml.org/0.9/xsd http://www.ruleml.org/0.9/xsd/datalog.xsd\">\n";

	stream << "<Assert mapClosure=\"universal\">\n";

	stream << "<Or>" << std::endl;
}


void
OutputXMLBuilder::buildPost()
{
	stream << "</Or>" << std::endl;
	stream << "</Assert>" << std::endl;
	stream << "</RuleML>" << std::endl;
}


void
OutputXMLBuilder::buildAnswerSet(const AnswerSet& facts)
{
	stream << "<And>" << std::endl;

	for (AnswerSet::const_iterator f = facts.begin();
		 f != facts.end();
		 ++f)
	{
		if ((*f).isStronglyNegated())
			stream << "<Neg>";

		stream << "<Atom>";

		stream << "<Rel>";
		stream << "<![CDATA[" << (*f).getArgument(0) << "]]>";
		stream << "</Rel>";

		for (unsigned i = 1; i <= (*f).getArity(); i++)
		{
			stream << "<Ind>";
			stream << "<![CDATA[" << (*f).getArgument(i) << "]]>";
			stream << "</Ind>";
		}

		stream << "</Atom>";

		if ((*f).isStronglyNegated())
			stream << "</Neg>";

		stream << std::endl;
	}

	stream << "</And>" << std::endl;
}

/* vim: set noet sw=4 ts=4 tw=80: */
