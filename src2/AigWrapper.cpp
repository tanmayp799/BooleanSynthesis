#include "AigWrapper.h"


int AigWrapper::getNumInputs(){
    return Aig_ManCiNum(this->manager);
}

Abc_Ntk_t* AigWrapper::getNtk(){
    return Abc_NtkFromAigPhase(this->manager);
}