#include "dlvhex2/AnnotatedGroundProgram.h"

DLVHEX_NAMESPACE_BEGIN

AnnotatedGroundProgram::AnnotatedGroundProgram(RegistryPtr reg, const OrdinaryASPProgram& groundProgram) :
	reg(reg), groundProgram(groundProgram){

}

void AnnotatedGroundProgram::createEAMasks(){
/*
	eaMasks.resize(factory.innerEatoms.size());
	int eaIndex = 0;
	BOOST_FOREACH (ID eatom, factory.innerEatoms){
		// create an EAMask for each inner external atom
		ExternalAtomMask& eaMask = eaMasks[eaIndex];
		eaMask.setEAtom(reg->eatoms.getByID(eatom), groundIDB);
		eaMask.updateMask();

		// map external auxiliaries back to their external atoms
		bm::bvector<>::enumerator en = eaMask.mask()->getStorage().first();
		bm::bvector<>::enumerator en_end = eaMask.mask()->getStorage().end();
		while (en < en_end){
			if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
				auxToEA[*en].push_back(factory.innerEatoms[eaIndex]);
			}
			en++;
		}
		eaIndex++;
	}
*/
}

DLVHEX_NAMESPACE_END

