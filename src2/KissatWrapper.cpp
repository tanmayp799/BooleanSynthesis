#include "KissatWrapper.h"

#include <memory>

KissatWrapper::KissatWrapper() {
    solver = kissat_init();
    numVars = 0;
    outputVar = 0;
}

KissatWrapper::~KissatWrapper() {
    if (solver) {
        kissat_release(solver);
    }
}

void KissatWrapper::addClause(const std::set<int>& clause) {
    for (int lit : clause) {
        kissat_add(solver, lit);
    }
    kissat_add(solver, 0);
}

int KissatWrapper::solve() {
    return kissat_solve(solver);
}

void KissatWrapper::setDependencySet(std::vector<int> dependencySet){
    this->dependencySet = dependencySet;
}

void KissatWrapper::setOutputVar(int outputVar){
    this->outputVar = outputVar;
}

void KissatWrapper::setVarsToEliminate(std::vector<int> ExistentialVarsToEliminate, std::vector<int> UniversalVarsToEliminate){
    this->ExistentialVarsToEliminate = ExistentialVarsToEliminate;
    this->UniversalVarsToEliminate = UniversalVarsToEliminate;
}

void KissatWrapper::setNumVars(int numVars){
    this->numVars = numVars;
}

int KissatWrapper::getOutputVar(){
    return this->outputVar;
}

std::vector<int> KissatWrapper::getExistentialVarsToEliminate(){
    return this->ExistentialVarsToEliminate;
}

std::vector<int> KissatWrapper::getUniversalVarsToEliminate(){
    return this->UniversalVarsToEliminate;
}

std::vector<int> KissatWrapper::getDependencySet(){
    return this->dependencySet;
}

int KissatWrapper::getNumVars(){
    return this->numVars;
}

int KissatWrapper::eliminateExistentialVars(){
    return kissat_eliminate_variables(this->solver, this->ExistentialVarsToEliminate.data(), this->ExistentialVarsToEliminate.size());
}


void KissatWrapper::eliminateUniversalVars(){

    unsigned int result_size;
    std::unique_ptr<int[]> cnf_array(kissat_dump_cnf(solver, &result_size));


    std::set<int> varsToElim(this->UniversalVarsToEliminate.begin(), this->UniversalVarsToEliminate.end());

    // std::vector<std::vector<int>> localSpec;
    std::vector<int> currClause;
    for(int i=0;i<result_size;i++){
        if(cnf_array[i]==0){
            if(!currClause.empty()){
                this->localSpec.push_back(currClause);
                currClause.clear();
            }
        }
        else{
            if(varsToElim.find(abs(cnf_array[i]))!=varsToElim.end()){
                continue;
            }
            currClause.push_back(cnf_array[i]);
        }
    }
    return;
}