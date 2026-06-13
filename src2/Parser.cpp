#include "Parser.h"
#include <string>
#include <cstdlib>
#include <algorithm>

Parser::Parser(int argc, char** argv){
    this->argc = argc;
    this->argv = argv;
}

Dqbf* Parser::ParseDqbf(){
    if(this->argc < 2){
        std::cerr << "Error: Input file not provided." << std::endl;
        exit(1);
    }

    std::ifstream infile(this->argv[1]);
    if(!infile.is_open()){
        std::cerr << "Error: Unable to open file " << this->argv[1] << std::endl;
        exit(1);
    }

    std::set<int> universals;
    std::set<int> existentials;
    std::set<int> depVars;
    std::map<int, std::set<int>> dependencies;
    std::vector<std::set<int>> clauses;
    int numInputs = 0;
    // int maxVar = 0;

    std::vector<std::set<int>> existentialBlocks;
    std::vector<std::pair<std::set<int>, char>> blocks;
    std::string token;
    while(infile >> token){
        if(token == "c"){
            std::string dummy;
            std::getline(infile, dummy);
        }
        else if(token == "p"){
            std::string dummy;
            infile >> dummy >> numInputs;
            int numClauses;
            infile >> numClauses;
        }
        else if(token == "a"){
            int var;
            std::set<int> tmp;
            while(infile >> var && var != 0){
                universals.insert(var);
                tmp.insert(var);
                // if(var > maxVar) maxVar = var;
            }
            blocks.push_back({tmp,'a'});
        }
        else if(token == "e"){
            int var;
            std::set<int> tmp;
            while(infile >> var && var != 0){
                tmp.insert(var);
                // depVars.insert(var);
                dependencies[var] = std::set<int>(universals);
                // if(var > maxVar) maxVar = var;
            }
            // existentialBlocks.push_back(tmp);
            blocks.push_back({tmp,'e'});
        }
        else if(token == "d"){
            int var;
            infile >> var;
            // existentials.insert(var);
            depVars.insert(var);
            // if(var > maxVar) maxVar = var;
            
            int dep;
            while(infile >> dep && dep != 0){
                dependencies[var].insert(dep);
            }
            blocks.push_back({{var}, 'd'});
        }
        else{
            std::set<int> clause;
            int lit = std::stoi(token);
            if(lit != 0){
                clause.insert(lit);
                // if(abs(lit) > maxVar) maxVar = abs(lit);
                int l;
                while(infile >> l && l != 0){
                    clause.insert(l);
                    // if(abs(l) > maxVar) maxVar = abs(l);
                }
            }
            clauses.push_back(clause);
        }
    }

    // std::vector<int> varsToMove;
    // for(int e : existentials) {
    //     if(dependencies[e].size() < universals.size()) {
    //         varsToMove.push_back(e);
    //     }
    // }
    // for(int e : varsToMove) {
    //     existentials.erase(e);
    //     depVars.insert(e);
    // }
    int numBlocks = blocks.size();

    if(blocks.back().second=='e'){
        existentials = blocks.back().first;
        blocks.pop_back();
    }
    for(auto block:blocks){
        if(block.second=='e'){
            for(auto var:block.first){
                depVars.insert(var);
            }
        }
    }

    if(!existentialBlocks.empty()) {
        existentials = existentialBlocks.back();
        existentialBlocks.pop_back();
    }
    for(auto block:existentialBlocks){
        for(auto v:block){
            depVars.insert(v);
        }
    }



    globalLogger.log(LogLevel::ERROR, fmt::format("Existentials: {}", fmt::join(existentials, " ")));
    globalLogger.log(LogLevel::ERROR, fmt::format("DepVars: {}", fmt::join(depVars, " ")));

    // if(numInputs < maxVar) numInputs = maxVar;

    // for(int i=1; i<=numInputs; i++){
    //     if(universals.find(i) == universals.end() && existentials.find(i) == existentials.end()){
    //         existentials.insert(i);
    //         depVars.insert(i);
    //     }
    // }

    // globalLogger.log(LogLevel::DEBUG, "Printing Clauses...");
    // globalLogger.log(LogLevel::DEBUG, std::format("Num Inputs: {}", numInputs));
    // globalLogger.log(LogLevel::DEBUG, fmt::format("Universals: {}", fmt::join(universals, " ")));
    // globalLogger.log(LogLevel::DEBUG, fmt::format("Existentials: {}", fmt::join(existentials, " ")));
    // globalLogger.log(LogLevel::DEBUG, fmt::format("Dependency Vars: {}", fmt::join(depVars, " ")));
    // for(auto clause:clauses){
    //     globalLogger.log(LogLevel::DEBUG, fmt::format("{}", fmt::join(clause, " ")));
    // }
    // std::cerr<<depVars.size()<<std::endl;
    // std::cerr<<fmt::format("{}", fmt::join(depVars, " "))<<std::endl;
    // int nn;
    // std::cin>>nn;
    
    return new Dqbf(universals, existentials, depVars, numInputs, clauses.size(), clauses, dependencies);
}
