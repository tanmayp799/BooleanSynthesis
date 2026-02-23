#include "KissatWrapper.h"

#include <memory>

KissatWrapper::KissatWrapper() {
    solver = kissat_init();
    // kissat_set_option(solver, "quiet", 1);
    // Level 3 provides the most detailed diagnostic output
    kissat_set_option(solver, "verbose", 3);

    // Additionally, ensure 'quiet' is disabled (set to 0)
    kissat_set_option(solver, "quiet", 0);

    // You can also enable 'report' for periodic status lines
    kissat_set_option(solver, "report", 1);

    kissat_set_option(solver, "eliminaterounds",20);
    kissat_set_option(solver,"eliminatebound", 64);
    kissat_set_option(solver, "eliminateeffort", 400);

    kissat_set_option(solver, "eliminateocclim", 10000);
    kissat_set_option(solver, "eliminateclslim", 3000);

    int er = kissat_get_option(solver, "eliminaterounds");
    int eb = kissat_get_option(solver, "eliminatebound");
    int ee = kissat_get_option(solver, "eliminateeffort");
    int eo = kissat_get_option(solver, "eliminateocclim");
    int ec = kissat_get_option(solver, "eliminateclslim");

    globalLogger.log(LogLevel::DEBUG, fmt::format("er: {} eb: {} ee: {} eo: {} ec: {}", er, eb, ee, eo, ec));


    // kissat_set_option(s, "eliminaterounds", 5);

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



// bool KissatWrapper::existentialsPresent(){


//     for(auto var: this->ExistentialVarsToEliminate){
//         if(this->solver->flags[var-1].active) return true;
//     }

//     return false;

// }

int KissatWrapper::eliminateExistentialVars(){

    int maxTries = 1;
    int attempt = 0;
    // this->solver->bounds.eliminate.additional_clauses = 20;
    do{
        
        std::cerr<<"Attempt number: "<<attempt<<std::endl;
        kissat_eliminate_variables(this->solver, this->ExistentialVarsToEliminate.data(), this->ExistentialVarsToEliminate.size());
        attempt++;

    } while(attempt<maxTries);


    return 0;
    // return kissat_eliminate_variables(this->solver, this->ExistentialVarsToEliminate.data(), this->ExistentialVarsToEliminate.size());


    // globalLogger.log(LogLevel::DEBUG, fmt::format("E-Vars to Eliminate: {}", fmt::join(this->ExistentialVarsToEliminate," ")));

    // unsigned int result_size;
    // std::unique_ptr<int[]> cnf_array(kissat_dump_cnf(solver, &result_size));
    // for(int i=0;i<result_size;i++){
    //     std::cout<<cnf_array[i]<<" ";
    //     if(cnf_array[i]==0) std::cout<<std::endl;
    // }
    // return 0;
}

void KissatWrapper::printClauses(){
    unsigned int result_size;
    std::unique_ptr<int[]> cnf_array(kissat_dump_cnf(solver, &result_size));
    for(int i=0;i<result_size;i++){
        std::cout<<cnf_array[i]<<" ";
        if(cnf_array[i]==0) std::cout<<std::endl;
    }
    // return 0;
}

void KissatWrapper::eliminateUniversalVars(){

    unsigned int result_size;
    std::unique_ptr<int[]> cnf_array(kissat_dump_cnf(solver, &result_size));


    std::set<int> varsToElim(this->UniversalVarsToEliminate.begin(), this->UniversalVarsToEliminate.end());
    std::set<int> exisVarsToElim(this->ExistentialVarsToEliminate.begin(), this->ExistentialVarsToEliminate.end());
    // std::vector<std::vector<int>> localSpec;
    std::set<int> failedEliminations;
    std::vector<int> currClause;
    for(int i=0;i<result_size;i++){
        if(cnf_array[i]==0){
            if(!currClause.empty()){
                this->localSpec.push_back(currClause);
                currClause.clear();
            }
        }
        else{

            if(exisVarsToElim.find(abs(cnf_array[i]))!=exisVarsToElim.end()){
                // globalLogger.log(LogLevel::ERROR, "Failed to eliminate e-var: "+std::to_string(abs(cnf_array[i])));
                failedEliminations.insert(abs(cnf_array[i]));
            }

            if(varsToElim.find(abs(cnf_array[i]))!=varsToElim.end()){
                continue;
            }
            currClause.push_back(cnf_array[i]);
        }
    }

    globalLogger.log(LogLevel::ERROR, fmt::format("Failed to eliminate: {}", fmt::join(failedEliminations," ")) );
    // this->localSpec.clear();
    // this->eliminateExistentialVars();


    globalLogger.log(LogLevel::DEBUG, fmt::format("Printing final local matrix: {}", this->outputVar));

    for(auto clause:localSpec){
        globalLogger.log(LogLevel::DEBUG, fmt::format("{}", fmt::join(clause, " ")));
    }

    return;
}


std::vector<std::vector<int>> KissatWrapper::getLocalSpec(){
    return this->localSpec;
}