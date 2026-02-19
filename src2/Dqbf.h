#ifndef DQBF_H
#define DQBF_H

#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "KissatWrapper.h"

class Dqbf
{
private:
    /* data */
    std::set<int> universals;
    std::set<int> existentials;
    std::set<int> depVars;
    std::map<int, std::set<int>> dependencies;
    int numInputs;
    int numClauses;
    std::vector<std::set<int>> clauses;

public:
    //constructor
    Dqbf(std::set<int> universals, std::set<int> existentials, std::set<int> depVars, int numInputs, int numClauses, std::vector<std::set<int>> clauses);


    //field access
    std::set<int> GetUniversals();
    std::set<int> GetExistentials();
    std::set<int> GetDepVars();
    std::set<int> GetDependencySet(int id);
    int GetNumInputs();
    int GetNumClauses();
    std::vector<std::set<int>> GetClauses();

    Dqbf* SubstituteConst(int var, bool setTrue, bool inPlace);
    KissatWrapper* getLocalFormula(int var);


};

#endif // DQBF_H
