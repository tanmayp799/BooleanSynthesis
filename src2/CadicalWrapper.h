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

    void assumeSelectors(int hId);

    int solve();

    std::vector<int> getCex();
    void assume(std::vector<int> assumptions);
    void assume(int lit);
    bool failed(int lit);

    int getNewVar();

    void synthesize(std::set<int> depVal, int hId, int newAuxVar);

    void addClause(std::vector<int> clause);
    void addRawClause(std::vector<int> clause);
    CaDiCaL::Solver GetSolver();

    std::map<int, int> GetInputToVarMapping(){
        return this->inputToVarMapping;
    }


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