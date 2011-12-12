/* Implementation of Example Context for MCS-IE Plugin
 *
 * Copyright (C) 2010  Peter Schüller
 * 
 * This file is part of dlvhex-mcs-equilibrium-plugin.
 *
 * dlvhex-mcs-equilibrium-plugin is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex-mcs-equilibrium-plugin is distributed in the hope that it will
 *  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex-dlplugin; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file   Context3.cpp
 * @author Peter Schueller
 * 
 * @brief  Demo Context which may replace the kb3.dlv DLV context.
 */

#include "ContextInterfaceAtom.h"
#include "ContextInterfacePlugin.h"

DLVHEX_MCSEQUILIBRIUM_PLUGIN(MedExamplePluginContext3,0,1,0)

namespace
{

DLVHEX_MCSEQUILIBRIUM_CONTEXT(Context3,"ontology_context3_acc")

std::set<std::set<std::string> >
Context3::acc(
		const std::string& param,
		const std::set<std::string>& input)
{
  std::set<std::set<std::string> > ret;
	// accept all input
  std::set<std::string> s(input.begin(),input.end());
  if( input.count("pneumonia") == 1 && input.count("marker") == 1 )
	{
		// additionally accept atyppneumonia
    s.insert("atyppneumonia");
  }
	ret.insert(s);
  return ret;
}

void MedExamplePluginContext3::registerAtoms()
{
	registerAtom<Context3>();
}

} // anonymous namespace
