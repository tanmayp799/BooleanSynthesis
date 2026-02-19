#ifndef CADICAL_WRAPPER_H
#define CADICAL_WRAPPER_H


#include "AigWrapper.h"
#include "cadical.hpp"


class CadicalWrapper {
public:
    CadicalWrapper();
    ~CadicalWrapper();
    CadicalWrapper(AigWrapper* aw);

    void setDefaultValue(int hVar, bool defaultVal);

private:
    CaDiCaL::Solver solver;
    // std::map<int, int> exToHMapping;
    std::map<int, int> inputToVarMapping;
    int numInputs;
    std::map<int, int> HtoZMapping;
    std::map<int, std::vector<int>> HtoSelectorMapping;
    std::map<std::set<int>, int> caseToAuxMap;
};



#endif // "CADICAL_WRAPPER_H"