#include "KissatWrapper.h"

#include <memory>
#include <fstream>
#include <cstdlib>

KissatWrapper::KissatWrapper() {
    solver = kissat_init();
    // kissat_set_option(solver, "quiet", 1);
    // Level 3 provides the most detailed diagnostic output
    kissat_set_option(solver, "verbose", 3);

    // Additionally, ensure 'quiet' is disabled (set to 0)
    kissat_set_option(solver, "quiet", 0);

    // You can also enable 'report' for periodic status lines
    kissat_set_option(solver, "report", 1);

    kissat_set_option(solver, "eliminaterounds",10);
    kissat_set_option(solver,"eliminatebound", 128);
    kissat_set_option(solver, "eliminateeffort", 1000);

    kissat_set_option(solver, "eliminateocclim", 10000);
    kissat_set_option(solver, "eliminateclslim", 3000);
    kissat_set_option(solver, "mineffort", 100);
    // kissat_set_option(solver, "log", 1);

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

void KissatWrapper::setVarsToEliminate(std::vector<int> ExistentialVarsToEliminate, std::vector<int> UniversalVarsToEliminate, std::vector<int> DepVarsToEliminate){
    this->ExistentialVarsToEliminate = ExistentialVarsToEliminate;
    this->UniversalVarsToEliminate = UniversalVarsToEliminate;
    this->DepVarsToEliminate = DepVarsToEliminate;
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

std::vector<int> KissatWrapper::getDepVarsToEliminate(){
    return this->DepVarsToEliminate;
}



std::vector<int> KissatWrapper::getDependencySet(){
    return this->dependencySet;
}

int KissatWrapper::getNumVars(){
    return this->numVars;
}

int KissatWrapper::callManthan() {
    std::string manthanDir = "/home/coolboy19/Desktop/BooleanSynthesis/dependencies/manthan";
    std::string baseName = "temp_var_" + std::to_string(this->outputVar);
    std::string qdimacsPath = manthanDir + "/" + baseName + ".qdimacs";
    
    std::ofstream out(qdimacsPath);
    if (!out.is_open()) {
        globalLogger.log(LogLevel::ERROR, "Could not open temporary CNF file: " + qdimacsPath);
        return -1;
    }
    
    std::vector<std::set<int>> clauses = this->getClauses();


    // for(auto clause:clauses){
    //     for(auto lit: clause){
    //         out<<lit<<" ";
    //     }
    //     out<<"0\n";
    // }
    // exit(1);


    std::set<int> inputs;
    std::set<int> outputs;
    // out<<"a ";
    for(auto v: this->dependencySet){
        // out<<v<<" ";
        inputs.insert(v);
    }
    
    
    if (!this->UniversalVarsToEliminate.empty()) {
        // out << "a ";
        for (int v : this->UniversalVarsToEliminate) {
            // out << v << " ";
            inputs.insert(v);
            
        }
        // out << "0\n";
    }
    // out<<this->outputVar<<" ";
    inputs.insert(this->outputVar);
    // outputs.insert(this->outputVar);
    
    for(auto v: this->DepVarsToEliminate){
        // out<<v<<" ";
        inputs.insert(v);
        
    }
    // out<<"0\n";
    
    // out << "e " << this->outputVar << " ";
    for (int v : this->ExistentialVarsToEliminate) {
        // if (v != this->outputVar) out << v << " ";
        outputs.insert(v);
    }

    this->inputOrdering = inputs;
    this->outputOrdering = outputs;

    // for (int v : this->DepVarsToEliminate) {
        //     if (v != this->outputVar) out << v << " ";
        // }
        // out << "0\n";
    out << "p cnf " << this->numVars << " " << clauses.size() << "\n";
        
    out<<"a ";
    for(auto v:inputs){
        out<<v<<" ";
    }
    out<<"0\n";

    out<<"e ";
    for(auto v:outputs){
        out<<v<<" ";
    }
    out<<"0\n";

    

    for (const auto& clause : clauses) {
        for (int lit : clause) {
            out << lit << " ";
        }
        out << "0\n";
    }
    out.close();

    std::string cmd = "cd " + manthanDir + " && ./manthan-venv/bin/python manthan.py " + qdimacsPath;
    globalLogger.log(LogLevel::INFO, "Executing Manthan: " + cmd);
    
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        globalLogger.log(LogLevel::ERROR, "Manthan failed with return code " + std::to_string(ret));
    }
    
    return ret;
}


// bool KissatWrapper::existentialsPresent(){


//     for(auto var: this->ExistentialVarsToEliminate){
//         if(this->solver->flags[var-1].active) return true;
//     }

//     return false;

// }

std::set<int> KissatWrapper::getInputOrdering(){
    return this->inputOrdering;
}

std::set<int> KissatWrapper::getOutputOrdering(){
    return this->outputOrdering;
}


int KissatWrapper::eliminateExistentialVars(){

    int maxTries = 1;
    int attempt = 0;
    globalLogger.log(LogLevel::ERROR, fmt::format("Attempting quantifier elimination for: {}", this->outputVar));
    // kissat_set_option(this->solver, "log", 1);
    // this->solver->bounds.eliminate.additional_clauses = 20;


    auto tmpclauses = this->getClauses();
    globalLogger.log(LogLevel::ERROR, fmt::format("Clauses size: {}", tmpclauses.size()));

    std::vector<int> eliminationTargets;
    for(auto e:this->ExistentialVarsToEliminate){
        eliminationTargets.push_back(e);
    }
    for(auto e:this->DepVarsToEliminate){
        eliminationTargets.push_back(e);
    
    }
    do{
        
        std::cerr<<"Attempt number: "<<attempt<<std::endl;
        kissat_eliminate_variables(this->solver, eliminationTargets.data(), eliminationTargets.size());
        attempt++;

    } while(attempt<maxTries);


    std::vector<std::set<int>> clauses = this->getClauses();
    globalLogger.log(LogLevel::ERROR, fmt::format("afterClauses size: {}", clauses.size()));

    std::set<int> failedExistentials;
    std::set<int> failedDeps;
    std::set<int> ExisVarsToElim(this->ExistentialVarsToEliminate.begin(),this->ExistentialVarsToEliminate.end());
    std::set<int> depVarsToElim(this->DepVarsToEliminate.begin(), this->DepVarsToEliminate.end());
    // std::set<int> failedUniversals;
    for(auto c:clauses){
        for(auto lit:c){
            if(ExisVarsToElim.find(abs(lit))!=ExisVarsToElim.end()){
                failedExistentials.insert(abs(lit));
            }
            if(depVarsToElim.find(abs(lit))!=depVarsToElim.end()){
                failedDeps.insert(abs(lit));
            }
        }
    }
    std::set<int> tmp(this->ExistentialVarsToEliminate.begin(), this->ExistentialVarsToEliminate.end());
    for(auto e:this->DepVarsToEliminate){
        tmp.insert(e);
    }

    for(auto e:failedDeps){
        tmp.erase(e);
    }
    for(auto e:failedExistentials){
        tmp.erase(e);
    }
    tmp.erase(this->outputVar);
    
    globalLogger.log(LogLevel::ERROR, fmt::format("Failed to eliminate: {}", fmt::join(failedExistentials," ")));
    globalLogger.log(LogLevel::ERROR, fmt::format("Failed to eliminate: {}", fmt::join(failedDeps," ")));

    this->eliminatedVars = tmp;

    this->ExistentialVarsToEliminate = std::vector<int>(failedExistentials.begin(), failedExistentials.end());
    this->DepVarsToEliminate = std::vector<int>(failedDeps.begin(), failedDeps.end());





    


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


std::set<int> KissatWrapper::getEliminatedVars(){
    return this->eliminatedVars;
}



std::vector<std::set<int>> KissatWrapper::getClauses(){
    unsigned int result_size;
    std::unique_ptr<int[]> cnf_array(kissat_dump_cnf(solver, &result_size));
    std::vector<std::set<int>> retClauses;
    std::set<int> tmp;
    for(int i=0;i<result_size;i++){
        if(cnf_array[i]==0){
            if(!tmp.empty()){
                retClauses.push_back(tmp);
                tmp.clear();
            }
        }
        else{
            tmp.insert(cnf_array[i]);
        }
    }
    return retClauses;
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