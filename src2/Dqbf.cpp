#include "Dqbf.h"


int Dqbf::GetNumInputs(){
    return this->numInputs;
}

std::set<int> Dqbf::GetUniversals(){
    return this->universals;
}

std::set<int> Dqbf::GetExistentials(){
    return this->existentials;
}

std::set<int> Dqbf::GetDepVars(){
    return this->depVars;
}

std::set<int> Dqbf::GetDependencySet(int id){
    return this->dependencies[id];
}

int Dqbf::GetNumClauses(){
    return this->numClauses;
}

std::vector<std::set<int>> Dqbf::GetClauses(){
    return this->clauses;
}

Dqbf::Dqbf(std::set<int> universals, std::set<int> existentials, std::set<int> depVars, int numInputs, int numClauses, std::vector<std::set<int>> clauses, std::map<int, std::set<int>> dependencies) {
    this->universals = universals;
    this->existentials = existentials;
    this->depVars = depVars;
    this->numInputs = numInputs;
    this->numClauses = numClauses;
    this->clauses = clauses;
    this->dependencies = dependencies;
}


KissatWrapper* Dqbf::getLocalFormula(int var){

    std::set<int> UniversalVarsToEliminate(this->universals.begin(), this->universals.end());
    std::set<int> ExistentialVarsToEliminate(this->existentials.begin(), this->existentials.end());

    ExistentialVarsToEliminate.erase(var);
    std::set<int> depSet = this->GetDependencySet(var);
    for(auto e:depSet){
        UniversalVarsToEliminate.erase(e);
    }

    KissatWrapper* kw = new KissatWrapper();
    kw->setVarsToEliminate(std::vector<int>(ExistentialVarsToEliminate.begin(), ExistentialVarsToEliminate.end()), std::vector<int>(UniversalVarsToEliminate.begin(), UniversalVarsToEliminate.end()));
    kw->setOutputVar(var);
    kw->setDependencySet(std::vector<int>(depSet.begin(), depSet.end()));
    kw->setNumVars(this->numInputs);

    for(auto clause:this->clauses){
        kw->addClause(clause);
    }

    return kw;

}