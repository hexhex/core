/* Implementation of a Example Context for
 * dlvhex-mcs-equilibrium-plugin
 * Calculate Equilibrium Semantics of Multi Context Systems in dlvhex
 *
 * Copyright (C) 2009,2010  Markus Boegl
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
 * @file   BenchContext7.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 16:20:03 2010
 * 
 * @brief  Self Implemented Example Context
 */
#include "ContextInterfaceAtom.h"
#include "ContextInterfacePlugin.h"

DLVHEX_MCSEQUILIBRIUM_PLUGIN(BenchmarkPlugin7,0,1,0)

namespace
{
  DLVHEX_MCSEQUILIBRIUM_CONTEXT(BenchContext1,"benchcontext7_context_acc1")

  std::set<std::set<std::string> > 
  BenchContext1::acc(const std::string& param, const std::set<std::string>& input) {
    std::set<std::set<std::string> > ret;
    for (std::set<std::string>::iterator it = input.begin(); it != input.end(); it++) {
	std::set<std::string> s;
	if (s.find(*it) == s.end()) {
	  s.insert(*it);
	}
	ret.insert(s);
    }
    return ret;
  }

  DLVHEX_MCSEQUILIBRIUM_CONTEXT(BenchContext2,"benchcontext7_context_acc2")

  std::set<std::set<std::string> > 
  BenchContext2::acc(const std::string& param, const std::set<std::string>& input) {
    std::set<std::set<std::string> > ret;
    for (std::set<std::string>::iterator it = input.begin(); it != input.end(); it++) {
	std::set<std::string> s;
	if (s.find(*it) == s.end()) {
	  s.insert(*it);
	}
	ret.insert(s);
    }
    return ret;
  }

  DLVHEX_MCSEQUILIBRIUM_CONTEXT(BenchContext3,"benchcontext7_context_acc3")

  std::set<std::set<std::string> > 
  BenchContext3::acc(const std::string& param, const std::set<std::string>& input) {
    std::set<std::set<std::string> > ret;
    for (std::set<std::string>::iterator it = input.begin(); it != input.end(); it++) {
	std::set<std::string> s;
	if (s.find(*it) == s.end()) {
	  s.insert(*it);
	}
	ret.insert(s);
    }
    return ret;
  }

  DLVHEX_MCSEQUILIBRIUM_CONTEXT(BenchContext4,"benchcontext7_context_acc4")

  std::set<std::set<std::string> > 
  BenchContext4::acc(const std::string& param, const std::set<std::string>& input) {
    std::set<std::set<std::string> > ret;
    if (input.find("d") == input.end()) {
      std::set<std::string> s;
      for (std::set<std::string>::iterator it = input.begin(); it != input.end(); it++) {
	  if (s.find(*it) == s.end()) {
	    s.insert(*it);
	  }
      }
      ret.insert(s);
    }
    return ret;
  }

  void BenchmarkPlugin7::registerAtoms() {
    registerAtom<BenchContext1>();
    registerAtom<BenchContext2>();
    registerAtom<BenchContext3>();
    registerAtom<BenchContext4>();
  }
}
