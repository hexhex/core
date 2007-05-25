/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* -*- C++ -*- */

/**
 * @file   globals.h
 * @author Roman Schindlauer
 * @date   Sat Nov  5 15:26:18 CET 2005
 * 
 * @brief  Global variable declarations.
 * 
 */


#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <string>
#include <vector>
#include <map>


/**
 * @brief Definition of global variables.
 */
class Globals
{
protected:
	Globals();

public:

	/**
	 * @brief List of possible verbose actions.
	 */
	typedef enum { DUMP_CONVERTED_PROGRAM,
	               DUMP_PARSED_PROGRAM,
	               DUMP_REWRITTEN_PROGRAM,
				   SAFETY_ANALYSIS,
	               DUMP_DEPENDENCY_GRAPH,
	               DUMP_OPTIMIZED_PROGRAM,
	               COMPONENT_EVALUATION,
	               MODEL_GENERATOR,
	               GRAPH_PROCESSOR,
	               PROFILING } verboseAction_t;

	/**
	 * Singleton instance.
	 */
	static Globals*
	Instance();

	/**
	 * Return the value of the specified option identifier.
	 */
	unsigned
	getOption(std::string);

	/**
	 * @brief Check if the specified verbose action can be carried out.
	 *
	 * This function checks if the predefined (see Globals::Globals()) value of
	 * the specified verbose action (see Globals::verboseLevel) is less or
	 * equal than the verbose level given as a parameter.
	 */
	bool
	doVerbose(verboseAction_t);

	/**
	 * Set an option with specified identifier to a value.
	 * 
	 * Not using a reference here, because we use explicit strings in main to
	 * call this method!
	 */
	void 
	setOption(std::string, unsigned);

	/**
	 * @brief Add a predicate to be filtered.
	 */
	void
	addFilter(std::string&);

	/**
	 * @brief Returns list of predicates to be filtered.
	 */
	const std::vector<std::string>&
	getFilters() const;

	/**
	 * Get the stream for verbose output.
	 */
	std::ostream&
	getVerboseStream() const;

	/**
	 * temporary hack
	 */
	std::string maxint;

	/**
	 * Filename of the (first, if more than one were specified) logic program
	 * dlvhex was called with.
	 */
	std::string lpfilename;

private:

	/**
	 * @brief Associates a verbose action with a verbose level.
	 */
	std::map<verboseAction_t, unsigned> verboseLevel;

	/**
	 * Singleton instance.
	 */
	static Globals* _instance;

	/**
	 * @brief Associates option names with values.
	 */
	std::map<std::string, unsigned> optionMap;

	/**
	 * @brief List of filter-predicates.
	 */
	std::vector<std::string> optionFilter;

	/**
	 * Messages returned from external computation sources, which do not necessarily
	 * lead to an abortion of the evaluation (i.e., which can be treated as warnings).
	 */
	//std::vector<std::string> Messages;

};

#define DEBUG_START_TIMER boost::posix_time::ptime boosttimerstart = boost::posix_time::microsec_clock::local_time(); \
	boost::posix_time::ptime boosttimerend; // std::cerr << "timer started..." << std::endl;

#define DEBUG_RESTART_TIMER boosttimerstart = boost::posix_time::microsec_clock::local_time(); // std::cerr << "timer started..." << std::endl;

#define DEBUG_STOP_TIMER(msg) boosttimerend = boost::posix_time::microsec_clock::local_time(); \
		if (Globals::Instance()->doVerbose(Globals::PROFILING)) { \
			boost::posix_time::time_duration diff = boosttimerend - boosttimerstart; \
			Globals::Instance()->getVerboseStream() \
				<< msg  << diff << "s" << std::endl; \
        boosttimerstart = boosttimerend; }

#endif // _GLOBALS_H
