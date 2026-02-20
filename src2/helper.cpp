#include "helper.h"


std::map<int, std::pair<Abc_Ntk_t*, Abc_Ntk_t*>> varToBasisMap;


std::vector<KissatWrapper*> generateLocalSpecs(Dqbf* origDqbf){
    std::vector<KissatWrapper*> localSpecs;
    std::set<int> deps = origDqbf->GetDepVars();
    for(auto id:deps){
        globalLogger.log(LogLevel::INFO, fmt::format("Generating localFormula for id: {}",id));
        KissatWrapper* kw = origDqbf->getLocalFormula(id);
        // auto tmp = kw->getExistentialVarsToEliminate();
        // globalLogger.log(LogLevel::DEBUG, fmt::format("tmp: {}",fmt::join(tmp," ")));
        localSpecs.push_back(kw);
    }
    return localSpecs;
}


std::set<int> getUnsatCore(std::vector<int> currAssumptions, std::vector<int> univAssumptions, CadicalWrapper* unsatCoreWrapper){

    std::set<int> unsatCoreLits;
    // std::set<int> unsatCoreUnivVars;

    for(auto e:currAssumptions){
        if(unsatCoreWrapper->failed(e)){
            unsatCoreLits.insert(e);
        }
    }

    // for(auto e:univAssumptions){
    //     if(unsatCoreWrapper->failed(e)){
    //         unsatCoreUnivVars.insert(e);
    //     }
    // }

    bool setMinimized=false;
    std::vector<int> removedLits;
    while(true){
        setMinimized=false;
        for(auto lit:unsatCoreLits){

            

            for(auto e:univAssumptions){
                unsatCoreWrapper->assume(e);
            }

            for(auto e:unsatCoreLits){
                if(e==lit) continue;
                unsatCoreWrapper->assume(e);
            }

            int unsatCoreStatus = unsatCoreWrapper->solve();
            if(unsatCoreStatus==CaDiCaL::SATISFIABLE){
                /* log information*/
            }
            else if(unsatCoreStatus == CaDiCaL::UNSATISFIABLE){
                unsatCoreLits.erase(lit);
                removedLits.push_back(lit);
                setMinimized=true;
                break;
            }
            else{
                /* log error*/
                break;
            }
        }
        if(!setMinimized) break;
    }

    return unsatCoreLits;
}





int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping){

    int numInputs = origDqbf->GetNumInputs() + origDqbf->GetDepVars().size();
    
    std::set<int> deps = origDqbf->GetDepVars();

    for(auto id:deps){
        int hId = exToHMapping[id];

        solverWrapper->setDefaultValue(hId, 1);
    }



    std::map<int, std::map<std::set<int>,std::pair<int,int>>> ex_caseToAuxMapping;

    while(true){

        for(auto id:deps){
            int hId = exToHMapping[id];
            solverWrapper->assumeSelectors(hId);
        }

        int status = solverWrapper->solve();

        if(status == CaDiCaL::UNKNOWN){
            break;
        }

        if(status==CaDiCaL::UNSATISFIABLE){
            /*
            fill code block
            */
            int constraintStatus = constraintWrapper->solve();
            
            if(constraintStatus == CaDiCaL::UNSATISFIABLE){
                globalLogger.log(LogLevel::INFO, "Constraint Unsatisfiable, no solution exists.");
                return 1;
            }
            
            if(constraintStatus == CaDiCaL::SATISFIABLE){
                // store the skolem functions

                return 0;
            }

            if(constraintStatus == CaDiCaL::UNKNOWN){
                globalLogger.log(LogLevel::ERROR, "Constrainst Solver returned UNKNOWN Status.");
                return 1;
            }
        }

        assert(status == CaDiCaL::SATISFIABLE);

        bool changeFlag = false;
        
        std::vector<int> cex = solverWrapper->getCex();

        std::vector<int> currConstraint;

        std::vector<int> currAssumptions;

        for(auto id:deps){
            if(cex[id-1]>0){
                currAssumptions.push_back(id);
            }
            else{
                currAssumptions.push_back(-id);
            }
        }

        std::set<int> univs = origDqbf->GetUniversals();
        std::vector<int> univAssumptions;
        for(auto id:univs){
            if(cex[id-1]>0){
                univAssumptions.push_back(id);
            }
            else{
                univAssumptions.push_back(-id);
            }
        }

        unsatCoreWrapper->assume(currAssumptions);
        unsatCoreWrapper->assume(univAssumptions);


        int unsatCoreStatus = unsatCoreWrapper->solve();
        if(unsatCoreStatus==CaDiCaL::SATISFIABLE){
            /* log error*/
            return 1;
        }

        std::set<int> unsatCoreLits = getUnsatCore(currAssumptions, univAssumptions, unsatCoreWrapper);

        for(auto e:unsatCoreLits){
            int id = abs(e);

            int aVal =  Abc_NtkVerifySimulatePattern(varToBasisMap[id].first, cex.data())[0];
            int bVal =  Abc_NtkVerifySimulatePattern(varToBasisMap[id].second, cex.data())[0];


            if(!(aVal==0 && bVal==1)){
                printf("AiBi Check Passed: %d\n",id);
                continue;
            }

            std::set<int> depVal;
            std::set<int> depSet = origDqbf->GetDependencySet(id);
            for(auto dep:depSet){
                if(cex[dep-1]==0){
                    depVal.insert(-dep);
                }
                else depVal.insert(dep);
            }

            if(ex_caseToAuxMapping[id].find(depVal)==ex_caseToAuxMapping[id].end()){
            
                changeFlag=true;

                int newAuxVar = solverWrapper->getNewVar();

                solverWrapper->synthesize(depVal, exToHMapping[id], newAuxVar);

                if(cex[id-1]>0){
                    currConstraint.push_back(-newAuxVar);
                }
                else{
                    currConstraint.push_back(newAuxVar);

                }

                ex_caseToAuxMapping[id][depVal]= {newAuxVar, newAuxVar};
            }
            else{
                changeFlag=true;
                int newAuxVar = ex_caseToAuxMapping[id][depVal].first;
                
                if(cex[id-1]>0){
                    currConstraint.push_back(-newAuxVar);
                }
                else{
                    currConstraint.push_back(newAuxVar);

                }
            }
        }
        if(!changeFlag){
            globalLogger.log(LogLevel::INFO, "No Change Occured.");
            break;
        }

        solverWrapper->addClause(currConstraint);
        constraintWrapper->addClause(currConstraint);

    }
    return 1;
}