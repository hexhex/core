/*
 * Global.cpp
 *
 *  Created on: 02.04.2010
 *      Author: max
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

//#define DEBUG

#include "Global.h"

namespace dlvhex {
  namespace mcsdiagexpl {

	Global* Global::g = NULL;

	Global*
	Global::getInstance() {
		if (g == NULL) {
		  g = new Global();
		  //g->init();
		}
		return g;
	}

	void
	Global::init(){
		explanation = mindiag = minexpl = noprintopeq = diagnose = kr2010rewriting = rewritingEnabled = false;
	}

	void
	Global::setDiag(){
		diagnose = true;
	}

	void
	Global::setminDiag(){
		mindiag = true;
	}

	void
	Global::setExp(){
		explanation = true;
	}

	void
	Global::setminExp(){
		minexpl = true;
	}

	void
	Global::setnoprintopeq() {
		noprintopeq = true;
	}

	void
	Global::setKR2010rewriting() {
		kr2010rewriting = true;
	}

	void
	Global::setRewritingEnabled(bool value) {
		rewritingEnabled = value;
	}

	bool
	Global::isnoprintopeq() {
		return noprintopeq;
	}

	bool
	Global::isDiag() {
		return diagnose;
	}

	bool
	Global::isminDiag() {
		return mindiag;
	}

	bool
	Global::isExp() {
		return explanation;
	}

	bool
	Global::isminExp() {
		return minexpl;
	}

	bool
	Global::isSet() {
		return explanation || diagnose || minexpl || mindiag;
	}

	bool
	Global::isKR2010rewriting() {
		return kr2010rewriting;
	}

	bool
	Global::isRewritingEnabled() {
		return rewritingEnabled;
	}

  }
}


