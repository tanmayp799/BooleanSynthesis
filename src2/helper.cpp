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
    std::set<int> exis = origDqbf->GetExistentials();

    for(auto id:exis){
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

    std::map<int, std::map<std::set<int>,std::pair<int,int>>> ex_caseToAuxMapping; // ex_id -> {set -> {CI_id,CI_id}}
    std::set<std::set<int>> constraints;
    int iter=0;

    bool verbose=true;
    std::cout<<"Verbose? : ";
    // cin>>verbose;
    bool freq=false;

    std::map<int, int> VarToInput_unsatCoreExtractor;


    // exit(1);
    std::vector<int> auxilaries;
    
    std::map<int, int> HtoZMapping;
    std::map<int, std::vector<int>> HtoSelectorMapping;
    
    std::map<int, int> HtoZMapping_unsatCore;
    std::map<int, std::vector<int>> HtoSelectorMapping_unsatCore;

	std::map<int, bool> defaultVal;
    std::set<int> deps = origDqbf->GetDepVars();
    for(auto e: origDqbf->GetExistentials()){
        deps.insert(e);
    }

    CaDiCaL::Solver solver = solverWrapper->GetSolver();
    CaDiCaL::Solver unsatCoreExtractor = unsatCoreWrapper->GetSolver();
    CaDiCaL::Solver constraintSolver = constraintWrapper->GetSolver();




    std::map<int,int> inputToVarMapping = solverWrapper->GetInputToVarMapping();
    std::map<int,int> inputToVarMapping_unsatCore = unsatCoreWrapper->GetInputToVarMapping();
    int numOrigInputs = origDqbf->GetNumInputs();

	for(auto id:deps){
		// if(this->constAssumption.find(id)!=this->constAssumption.end()) continue;
        int z_0 = solver.vars()+1;
        int s0 = solver.vars()+2;
        int h_id = exToHMapping[id];
        int h_var = inputToVarMapping[h_id];
        
        //z0 <=> 1
        int randomNumber = (rand() % 100);

        // if(randomNumber >=0){

        //     solver.add(z0);
        // }
        // else solver.add(-z0);
        // solver.add(0);
        // if(id==14){
		// 	solver.add(-z_0);
		// 	solver.add(0);
		// 	defaultVal[id]=false;
		// }
		// else{
		// 	solver.add(z_0);
		// 	solver.add(0);
		// 	defaultVal[id] = true;

		// }
		solver.add(-z_0);
		solver.add(0);
		defaultVal[id]=false;


        // -h or z or s
        solver.add(-h_var);
        solver.add(z_0);
        solver.add(s0);
        solver.add(0);
        
        //h or -z or s
        solver.add(h_var);
        solver.add(-z_0);
        solver.add(s0);
        solver.add(0);
        
        HtoZMapping[h_id] = z_0;
        HtoSelectorMapping[h_id].push_back(s0);
        
        // int uc_z0 = unsatCoreExtractor.vars()+1;
        // int uc_s0 = unsatCoreExtractor.vars()+2;
        // int uc_h_var = inputToVarMapping_unsatCore[h_id];
        
        // unsatCoreExtractor.add(-uc_z0);
        // unsatCoreExtractor.add(0);
        
        // unsatCoreExtractor.add(-uc_h_var);
        // unsatCoreExtractor.add(uc_z0);
        // unsatCoreExtractor.add(uc_s0);
        // unsatCoreExtractor.add(0);
        
        // unsatCoreExtractor.add(uc_h_var);
        // unsatCoreExtractor.add(-uc_z0);
        // unsatCoreExtractor.add(uc_s0);
        // unsatCoreExtractor.add(0);
        
        // HtoZMapping_unsatCore[h_id] = uc_z0;
        // HtoSelectorMapping_unsatCore[h_id].push_back(uc_s0);
    }

	// set<int> posUnates = this->get_posUnates();
    // for(auto d:posUnates){
	// 	if(this->constAssumption.find(d)!=this->constAssumption.end()) continue;
    //     solver.add(inputToVarMapping[d]);
    //     solver.add(0);

    //     unsatCoreExtractor.add(inputToVarMapping_unsatCore[d]);
    //     unsatCoreExtractor.add(0);
	// 	this->constAssumption[d] = true;
    // }

    // set<int> negUnates = this->get_negUnates();
    // for(auto d:negUnates){
	// 	if(this->constAssumption.find(d)!=this->constAssumption.end()) continue;
    //     solver.add(-inputToVarMapping[d]);
    //     solver.add(0);

    //     unsatCoreExtractor.add(-inputToVarMapping_unsatCore[d]);
    //     unsatCoreExtractor.add(0);
	// 	this->constAssumption[d]=false;
    // }

	solver.write_dimacs("./f1.dimacs");
    unsatCoreExtractor.write_dimacs("./f2.dimacs");
	// exit(1);

	// FILE* solver_dimacs =fopen("./f1.dimacs","r");


    // unsatCoreExtractor.write_dimacs("./before.dimacs");
    std::map<std::set<int>, int> caseToAuxMap;
    std::map<std::set<int>, int> caseToAuxMap_unsatCore;

    std::map<int, std::vector<int>> exToAuxMap;

	
	// int trivialityStatus = is_trivialSolver("./f1.dimacs");
	// printf("Num Clauses: %d \nNum Vars: %d\n",solver.irredundant(),solver.active());
	// if(trivialityStatus==0) {
	// 	if(solver.irredundant()==0){
	// 		cout<<"Num Clause: 0\n";
	// 	}
	// 	if(solver.active()==0){
	// 		cout<<"Num var: 0\n";
	// 	}
	// 	return false;
	// }

	//return a file pointer if the forumula is trivially UNSAT
	// if(trivialityStatus==1){
	// 	cout<<"here"<<endl;
	// 	string filename = "/home/coolboy19/Desktop/BooleanSynthesis/benchmark_tests/solution/skolem_functions/"+this->filename+".txt";
	// 	cout<<filename<<endl;
    //     FILE* asgFile = fopen(filename.c_str(),"w");
		

	// 	int totalOutputs = this->get_all_edvars().size();
	// 	int constOutputs = this->constAssumption.size();

	// 	fprintf(asgFile, "%d %d\n", constOutputs, totalOutputs-constOutputs);
	// 	auto dep_vars = this->get_all_edvars();

	// 	for(auto p:this->constAssumption){
	// 		int currOutVar = p.first;
	// 		bool constVal = p.second;
	// 		dep_vars.erase(currOutVar);

	// 		fprintf(asgFile, "%d %d\n", currOutVar, (int)(constVal));
	// 	}

	// 	map<int, int> cex_aux;

	// 	for(auto e:auxilaries){
	// 		int val = constraintSolver.val(inputToVarMapping[e]);
	// 		cex_aux[e]=val>0?1:0;
	// 	}
	// 	for(auto currOutVar: dep_vars){
	// 		vector<set<int>> positiveCases;
	// 		vector<set<int>> negativeCases;


	// 		auto cases = ex_caseToAuxMapping[currOutVar];
	// 		for(auto p2:cases){
	// 			set<int> currCase = p2.first;
	// 			int aux = p2.second.first;
	// 			if(cex_aux.find(aux)==cex_aux.end()){
	// 				cerr<<"Error in aux map\n";
	// 				exit(1);
	// 			}
	// 			if(cex_aux[aux]==1){
	// 				positiveCases.push_back(currCase);
	// 			}
	// 			else{
	// 				negativeCases.push_back(currCase);
	// 			}
	// 		}

	// 		fprintf(asgFile, "%d %d %d %d\n",currOutVar,
	// 				 positiveCases.size(), negativeCases.size(),
	// 				 (int)(defaultVal[currOutVar]));
			
	// 		for(auto currCase:positiveCases){
	// 			for(auto e:currCase){
	// 				fprintf(asgFile, "%d ", e);
	// 			}
	// 			fprintf(asgFile,"0\n");
	// 		}
	// 		for(auto currCase:negativeCases){
	// 			for(auto e:currCase){
	// 				fprintf(asgFile,"%d ",e);
	// 			}
	// 			fprintf(asgFile,"0\n");
	// 		}
	// 	}
		
	// 	fclose(asgFile);

	// 	filename = "/home/coolboy19/Desktop/BooleanSynthesis/benchmark_tests/solution/reduced_dqbf/"+this->filename+"_reduced.txt";
	// 	FILE* dqbfFile = fopen(filename.c_str(),"w");
	// 	fprintf(dqbfFile,"%d\n",this->clauses.size());
	// 	for(auto clause:this->clauses){
	// 		for(auto lit:clause){
	// 			fprintf(dqbfFile,"%d ",lit);
	// 		}
	// 		fprintf(dqbfFile,"0\n");
	// 	}
	// 	fclose(dqbfFile);
	// 	// exit(0);



	// 	return true;
	// }
    int numAigInputs = origDqbf->GetNumInputs() + origDqbf->GetDepVars().size()+origDqbf->GetExistentials().size();
    int numX = origDqbf->GetUniversals().size();
    int numY = origDqbf->GetDepVars().size()+origDqbf->GetExistentials().size();
    
	while(true){
        iter++;
        //check for sat
        // if(iter>3000) exit(1);
        
        for(auto id:deps){
			// if(this->constAssumption.find(id)!=this->constAssumption.end()) continue;
            int h_id = exToHMapping[id];

            std::vector<int> selectors = HtoSelectorMapping[h_id];
            int sz = selectors.size();
            for(int i=0;i<sz-1;i++){
                // printf("Assuming %d\n", selectors[i]);
                solver.assume(selectors[i]);
            }
            // printf("Assuming %d\n", -selectors[sz-1]);
            solver.assume(-selectors[sz-1]);
        }
        // solver.write_dimacs("./f1_assumed.dimacs");

        int status = solver.solve();
        if(iter%500==0){
            freq=true;
        }

        if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("**************           ITER: {}        ******************", iter));
		// if(verbose && freq){
        //     string dumpName = "./debug_"+to_string(iter)+".dimacs";

        //     solver.write_dimacs(dumpName.c_str());
        // }

        if(status == 0){
            // std::cerr<<"Dunno what happened\n";
            globalLogger.log(LogLevel::ERROR, "Dunno what happened");
			Abc_Stop();
            return 1;
        }

		if(status == CaDiCaL::UNSATISFIABLE){

            // std::cout<<"UNSAT BUT WHY?"<<std::endl;
            globalLogger.log(LogLevel::INFO, "UNSAT FORMULA. Checking for solution...");
			FILE* ansFile=nullptr;


			// std::string dqbf_filename = "/home/coolboy19/Desktop/BooleanSynthesis/benchmark_tests/solution/reduced_dqbf/"+P+"_reduced.txt";
			// FILE* dqbfFile = fopen(dqbf_filename.c_str(),"w");
			// fprintf(dqbfFile,"%d\n",this->clauses.size());
			// for(auto clause:this->clauses){
			// 	for(auto lit:clause){
			// 		fprintf(dqbfFile,"%d ",lit);
			// 	}
			// 	fprintf(dqbfFile,"0\n");
			// }
			// fclose(dqbfFile);



            for(int asgNo=0;asgNo<3;asgNo++){
                int constrStatus = constraintSolver.solve();

                if(constrStatus == CaDiCaL::UNSATISFIABLE){
                    globalLogger.log(LogLevel::INFO, "Constraint Unsatisfiable, no solution exists.");
                    // printf("Couldn't satisfy constraints, total assignments generated: %d\n",asgNo);
					Abc_Stop();
					// if(asgNo>0) exit(0);
                    // if(asgNo==0) return false;
					return 1;

					// return false;

                }

                if(constrStatus == CaDiCaL::SATISFIABLE){
                    globalLogger.log(LogLevel::INFO,fmt::format("Constraint Satisfiable"));
                    return 0;
                    // map<int, int> cex_aux;

                    // for(auto e:auxilaries){
                    //     int val = constraintSolver.val(inputToVarMapping[e]);
                    //     cex_aux[e] = val>0?1:0;
                        
                    // }
                    // string filename = "/home/coolboy19/Desktop/BooleanSynthesis/benchmark_tests/solution/skolem_functions/"+this->filename+".txt";
                    // FILE* asgFile = fopen(filename.c_str(),"w");

					// int totalOutputs = this->get_deps().size();
					// int constOutputs = this->constAssumption.size();

					// fprintf(asgFile, "%d %d\n", constOutputs, totalOutputs-constOutputs);
					// auto dep_vars = this->get_deps();

					// for(auto p:constAssumption){
					// 	int currOutVar = p.first;
					// 	bool constVal = p.second;
					// 	dep_vars.erase(currOutVar);
					// 	fprintf(asgFile, "%d %d\n", currOutVar, (int)(constVal));
					// }

					// for(auto currOutVar:dep_vars){
					// 	vector<set<int>> positiveCases;
					// 	vector<set<int>> negativeCases;

					// 	auto cases = ex_caseToAuxMapping[currOutVar];

					// 	for(auto p2:cases){
					// 		set<int> currCase = p2.first;
					// 		int aux = p2.second.first;
					// 		if(cex_aux.find(aux)==cex_aux.end()){
					// 			cerr<<"Error in aux map\n";
					// 			exit(1);
					// 		}
					// 		if(cex_aux[aux]==1){
					// 			positiveCases.push_back(currCase);
					// 		}
					// 		else{
					// 			negativeCases.push_back(currCase);
					// 		}
					// 	}

					// 	fprintf(asgFile, "%d %d %d %d\n",currOutVar,
					// 			positiveCases.size(), negativeCases.size(),
					// 			(int)(defaultVal[currOutVar]));
						
					// 	for(auto currCase:positiveCases){
					// 		for(auto e:currCase){
					// 			fprintf(asgFile, "%d ", e);
					// 		}
					// 		fprintf(asgFile,"0\n");
					// 	}
					// 	for(auto currCase:negativeCases){
					// 		for(auto e:currCase){
					// 			fprintf(asgFile,"%d ",e);
					// 		}
					// 		fprintf(asgFile,"0\n");
					// 	}
					// }
					// fclose(asgFile);

                    // // for(auto p:ex_caseToAuxMapping){
                    // //     int d_var = p.first;
                    // //     auto cases = p.second;
                    // //     fprintf(asgFile,"D Variable: %d\n",d_var);
    
                    // //     for(auto p2:cases){
                    // //         set<int> currCase = p2.first;
                    // //         int aux = p2.second.first;
                    // //         if(cex_aux.find(aux)==cex_aux.end()){
                    // //             cerr<<"Error in aux map\n";
                    // //             exit(1);
                    // //         }
                    // //         for(auto e:currCase){
                    // //             fprintf(asgFile,"%d ",e);
                    // //         }
                    // //         fprintf(asgFile,"   =>     H_Val: %d\n",cex_aux[aux]);
                    // //     }
                    // //     fprintf(asgFile,"**************************\n");
                    // // }

					// // if(ansFile==nullptr) ansFile=asgFile;


                    // vector<int> newConstraint;
                    // vector<int> curr_auxilaries = exToAuxMap[19];
                    // for(auto e:curr_auxilaries){
                    //     int val = constraintSolver.val(inputToVarMapping[e]);
                    //     if(val>0){
                    //         newConstraint.push_back(-e);
                    //     }
                    //     else{
                    //         newConstraint.push_back(e);
                    //     }
                    // }

                    // for(auto e:newConstraint){
                    //     if(e>0){
                    //         constraintSolver.add(inputToVarMapping[e]);
                    //     }
                    //     else{
                    //         constraintSolver.add(-inputToVarMapping[-e]);
                    //     }
                    // }
                    // constraintSolver.add(0);
					// Abc_Stop();
					// return true;
                }

                
            }

            

			// std::cout<<"HURRAY\n";
			Abc_Stop();
			return 0;
            
        }

		assert(status==10);
        //The formula is sat. Work on the counter example.
        bool changeFlag = false;
        int cex[numAigInputs];
        for(int i=1;i<=numAigInputs;i++){
            int val = solver.val(inputToVarMapping[i]);
            cex[i-1]=val>0? 1:0;
        }


        if(verbose && freq) {
            // std::cout << "CEX : ";
            std::vector<int> tmpCex;
			for (int i = 0; i < numAigInputs; i++)
			{
				// if(i==numX) std::cout<<"| ";
				// if(i==numX + numY) std::cout <<"| ";
				// if(i==numX + numY + numY) std::cout<<"| ";
				// std::cout << cex[i] << " ";
                tmpCex.push_back(cex[i]);
			}
			// std::cout << std::endl;
            
            globalLogger.log(LogLevel::INFO, fmt::format("CEX : {}",fmt::join(tmpCex," ")));

		}

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
                // currConstraint.insert(-inputToVarMapping[aux]);
                univAssumptions.push_back(id);

            }
            else{
                // currConstraint.insert(inputToVarMapping[aux]);
                univAssumptions.push_back(-id);
            }
        }

		// std::cout<<"Universal Assumptions:\n";
        for(auto e:univAssumptions){
            // std::cout<<e<<endl;
            if(e>0){
                unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);

            }
            else{
                // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
            }
            // unsatCoreExtractor.add(0);
        }
        // std::cout<<"Auxiliary Assumptions:\n";
        for(auto e:currAssumptions){
            // std::cout<<e<<endl;
            if(e>0){
                // cout<<inputToVarMapping_unsatCore[e]<<endl;
                unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);

            }
            else{
                // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
            }
            // unsatCoreExtractor.add(0);
        }

		int unsatCoreStatus = unsatCoreExtractor.solve();

        if(unsatCoreStatus == CaDiCaL::SATISFIABLE){
            // std::cerr<<"Theres a problem... unsatCore is sat after adding the assumptions\n";
            globalLogger.log(LogLevel::ERROR,"Theres a problem... unsatCore is sat after adding the assumptions");
            unsatCoreExtractor.write_dimacs("./sat_core.dimacs");
            int cex[numAigInputs];
            for(int i=1;i<=numAigInputs;i++){
                int val = unsatCoreExtractor.val(inputToVarMapping_unsatCore[i]);
                cex[i-1]=val>0? 1:0;
            }


            if(verbose && freq) {std::cout << "CEX : ";

            for (int i = 0; i < numAigInputs; i++)
            {
                std::cout << cex[i] << " ";
            }
            std::cout << std::endl;}

            exit(1);
        }

		std::set<int> unsatCoreLits;
        std::set<int> unsatCoreUnivVars;
        if(unsatCoreStatus == CaDiCaL::UNSATISFIABLE){
            // unsatCoreExtractor.write_dimacs("./debug.dimacs");
            // cout<<"Unsat Core D Variables:\n";
            for(auto e:currAssumptions){
                if(e>0){
                    if(unsatCoreExtractor.failed(inputToVarMapping_unsatCore[e])){
                        // currConstraint.insert(-e);
                        unsatCoreLits.insert(e);
                        // cout<<e<<endl;
                    }
                }
                else{
                    if(unsatCoreExtractor.failed(-inputToVarMapping_unsatCore[-e])){
                        // currConstraint.insert(-e);
                        unsatCoreLits.insert(e);
                        // cout<<e<<endl;
                    }
                }
            }

            // std::cout<<"Unsat Core A Variables:\n";
            // unsatCoreUnivLits
            for(auto e:univAssumptions){
                if(e>0){
                    if(unsatCoreExtractor.failed(inputToVarMapping_unsatCore[e])){
                        // std::cout<<e<<std::endl;
                        unsatCoreUnivVars.insert(e);

                    }
                }
                else{
                    if(unsatCoreExtractor.failed(-inputToVarMapping_unsatCore[-e])){
                        // std::cout<<e<<std::endl;
                        unsatCoreUnivVars.insert(-e);
                    }
                }
            }
        }
        else{
            // std::cerr<<"SOMETHING WENT WRONG, timeout occured for unsatCoreExtractor\n";
            globalLogger.log(LogLevel::ERROR,"SOMETHING WENT WRONG, timeout occured for unsatCoreExtractor\n");
            exit(1);
        }
		bool setMinimized = false;
        std::vector<int> removed_lits;
        while(true){

            setMinimized=false;
            // set<int> tmp;
            for(auto lit: unsatCoreLits){
                // printf("Trying to remove %d\n",lit);
                if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("Trying to remove {}", lit));
                for(auto e:univAssumptions){
                    // cout<<e<<endl;
                    if(e>0){
                        unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);
        
                    }
                    else{
                        // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                        unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
                    }
                    // unsatCoreExtractor.add(0);
                }            
                for(auto e:unsatCoreLits){
                    if(e==lit) continue;
                    if(e>0){
                        // cout<<inputToVarMapping_unsatCore[e]<<endl;
                        unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);
        
                    }
                    else{
                        // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                        unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
                    }
                }
    
                int unsatCoreStatus = unsatCoreExtractor.solve();

                if(unsatCoreStatus == CaDiCaL::SATISFIABLE){
                    // printf("%d in UNSAT Core\n",lit);
                    if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("{} in UNSAT Core", lit));
                    int cex[numOrigInputs];
                    for(int i=1;i<=numOrigInputs;i++){
                        int val = unsatCoreExtractor.val(inputToVarMapping_unsatCore[i]);
                        cex[i-1]=val>0? 1:0;
                    }


                    // if(verbose && freq) {std::cout << "CEX : ";

                    // for (int i = 0; i < numOrigInputs; i++)
                    // {
                    //     std::cout << cex[i] << " ";
                    // }
                    // std::cout << std::endl;}
                    // tmp.insert(lit);
                }
                else if(unsatCoreStatus == CaDiCaL::UNSATISFIABLE){
                    // printf("Removed %d\n",lit);
                    if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("Removed {}", lit));
                    // unsatCoreLits
                    unsatCoreLits.erase(lit);
                    removed_lits.push_back(lit);
                    setMinimized=true;
                    break;
                }
                else{
                    std::cerr<<"Timeout while minimizing UNSAT CORE\n";
                    globalLogger.log(LogLevel::ERROR,"Timeout while minimizing UNSAT CORE");
                    exit(1);
                    break;
                }   
            }
            // unsatCoreLits;
            if(!setMinimized){
                break;
            }
        }

		// std::cout<<"Printing A_i and B_i for removed lits:\n";
        if(verbose && freq) globalLogger.log(LogLevel::INFO, "Printing A_i and B_i for removed lits:");
        for(auto e:removed_lits){
            int id = abs(e);
            
            int a_i = Abc_NtkVerifySimulatePattern(varToBasisMap[id].first, cex)[0];
            int b_i = Abc_NtkVerifySimulatePattern(varToBasisMap[id].second, cex)[0];
            printf("id: %d | a_i: %d | b_i: %d\n", id, a_i, b_i);
            if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("id: {} | a_i: {} | b_i: {}", id, a_i, b_i));
        }

        // std::cout<<"UNSAT CORE D Variables:\n";
        if(verbose && freq) globalLogger.log(LogLevel::INFO, "UNSAT CORE D Variables:");
        for(auto e:unsatCoreLits){
            // std::cout<<e<<std::endl;
            if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("{}", e));
        }
		std::vector<std::vector<int>> implicationClauses;
        for(auto e:unsatCoreLits){
            int id = abs(e);
            int a_i = Abc_NtkVerifySimulatePattern(varToBasisMap[id].first, cex)[0];
            int b_i = Abc_NtkVerifySimulatePattern(varToBasisMap[id].second, cex)[0];;

            // if(verbose) printf("var: %d => a_i: %d | b_i: %d\n",id,a_i,b_i);
            if(!(a_i==0 && b_i==1)){
                // printf("AiBi Check Passed: %d\n",id);
               if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("AiBi Check Passed: {}", id));
                continue;
            }

            // std::cout<<"Dependent Var_ AiBi check failed: "<<id<<std::endl;
            if(verbose && freq) globalLogger.log(LogLevel::INFO, fmt::format("Dependent Var_ AiBi check failed: {}", id));
            
            std::set<int> depVal;
            std::set<int> depSet = origDqbf->GetDependencySet(id);
            // if(id==13) cout<<"DBBBB: size: "<<depSet.size()<<endl;
            for (auto dep : depSet)
            {
                //unsatCore Filtering for if condition
                // if(unsatCoreUnivVars.find(dep) == unsatCoreUnivVars.end()) continue;
                if (cex[dep - 1] == 0)
                {
                    depVal.insert(-dep);
                }
                else
                {
                    depVal.insert(dep);
                }
                // depVal.insert(cex[dep-1]);
            }
            
            std::cout<<"DepVal for d: "<<id<<" => ";
            for(auto u:depVal){
                std::cout<<u<<" ";
            }
            std::cout<<std::endl;
            if(ex_caseToAuxMapping[id].find(depVal)==ex_caseToAuxMapping[id].end()){
                
                changeFlag = true;
                numAigInputs++;
                int newAux = numAigInputs;
                int cnfVar = solver.vars()+1;
                inputToVarMapping[newAux] = cnfVar;
                // fprintf(mapFile,"INPUT %d , var map: %d\n", newAux, cnfVar);
                exToAuxMap[id].push_back(newAux);
                
                auxilaries.push_back(newAux);
                // int unsatCoreCnfVar = unsatCoreExtractor.vars()+1;
                // inputToVarMapping_unsatCore[newAux] = unsatCoreCnfVar;
                // fprintf(mapFile2, "INPUT %d , var map: %d\n", newAux, unsatCoreCnfVar);
                // VarToInput_unsatCoreExtractor[unsatCoreCnfVar] = newAux;

                std::cout<<"Dependent Var new Aux created: "<<id<<std::endl;


                //Check if auxilary for this depVal exists or Not, if it does, use it else create one.


                if(caseToAuxMap.find(depVal)==caseToAuxMap.end()){
                    int newCaseVar = solver.vars()+2;

                    // dep => var === ~dep or var
                    std::vector<int> c1;
                    for(auto d:depVal){
                        if(d>0){
                            c1.push_back(-inputToVarMapping[d]);
                        }
                        else{
                            c1.push_back(inputToVarMapping[-d]);
                        }
                    }
                    c1.push_back(newCaseVar);
                    c1.push_back(0);

                    for(auto e:c1){
                        solver.add(e);
                    }


                    // var => dep ---> ~ var OR dep
                    for(auto d:depVal){
                        solver.add(-newCaseVar);
                        if(d>0){
                            solver.add(inputToVarMapping[d]);
                        }
                        else{
                            solver.add(-inputToVarMapping[-d]);
                        }
                        solver.add(0);
                    }

                    caseToAuxMap[depVal] = newCaseVar;
                }

                int caseVar = caseToAuxMap[depVal];

                int h_id = exToHMapping[id];
                int hVar = inputToVarMapping[h_id];
                int zVar = HtoZMapping[h_id];

                int newZ = solver.vars()+3;
                int newS = solver.vars()+4;

                // newZ = ite(caseVar, newAux,zVar)
                //adding clause 1
                solver.add(-caseVar);
                solver.add(newZ);
                solver.add(-cnfVar);
                solver.add(0);

                //adding clause 2
                solver.add(-caseVar);
                solver.add(-newZ);
                solver.add(cnfVar);
                solver.add(0);

                //adding clause 3
                solver.add(caseVar);
                solver.add(newZ);
                solver.add(-zVar);
                solver.add(0);

                //adding clause 4
                solver.add(caseVar);
                solver.add(-newZ);
                solver.add(zVar);
                solver.add(0);

                //Now we will add h<=> newZ if ~selector
                //adding h or -newZ or newS
                solver.add(hVar);
                solver.add(-newZ);
                solver.add(newS);
                solver.add(0);

                //adding -h or newZ or newS
                solver.add(-hVar);
                solver.add(newZ);
                solver.add(newS);
                solver.add(0);

                HtoZMapping[h_id] = newZ;
                HtoSelectorMapping[h_id].push_back(newS);

                ///////////////////////////////////////////////////////////////////////////////////

                








                //////////////////////////////////////////////////////////////////////////////
                


                
                if(cex[id-1]>0){
                    currConstraint.push_back(-newAux);
                    // currAssumptions.push_back(newAux);
                }
                else{
                    currConstraint.push_back(newAux);
                    // currAssumptions.push_back(-newAux);
                }

                ex_caseToAuxMapping[id][depVal]= {newAux, newAux};
            }
            else{
                changeFlag=true;
                int aux = ex_caseToAuxMapping[id][depVal].first;
                if(cex[id-1]>0){
                    currConstraint.push_back(-aux);
                    // currAssumptions.push_back(aux);

                }
                else{
                    currConstraint.push_back(aux);
                    // currAssumptions.push_back(-aux);
                }
            }

        }
		if(!changeFlag){
            std::cout<<"No change occured...."<<std::endl;
			Abc_Stop();
            return false;
        }
        std::cout<<"adding constraint clause...\n";
        for(auto e:currConstraint){
            
            if(e>0) {
                solver.add(inputToVarMapping[e]);
                std::cout<<e<<" ";
            }
            else {
                solver.add(-inputToVarMapping[-e]);
                std::cout<<e<<" ";
            }
        }
        std::cout<<std::endl;
        solver.add(0);
        std::cout<<"Printing current constraint:\n";
        for(auto e:currConstraint){
            if(e>0) {
                constraintSolver.add(inputToVarMapping[e]);
            }
            else constraintSolver.add(-inputToVarMapping[-e]);
            std::cout<<e<<std::endl;
        }
        constraintSolver.add(0);
        freq=false;

        // for(auto e:currConstraint){
        //     if(e>0) unsatCoreExtractor.add(inputToVarMapping_unsatCore[e]);
        //     else unsatCoreExtractor.add(-inputToVarMapping_unsatCore[-e]);
        // }
        // unsatCoreExtractor.add(0);

    }

    // int numInputs = origDqbf->GetNumInputs() + origDqbf->GetDepVars().size();
    
    // std::set<int> deps = origDqbf->GetDepVars();

    // for(auto id:deps){
    //     int hId = exToHMapping[id];
    //     globalLogger.log(LogLevel::ERROR, fmt::format("id: {} -> hId: {}", id,hId));

    //     solverWrapper->setDefaultValue(hId, 1);
    // }



    // std::map<int, std::map<std::set<int>,std::pair<int,int>>> ex_caseToAuxMapping;
    // int iter = 0;
    // while(true){
    //     iter++;

    //     if(iter%1==0) globalLogger.log(LogLevel::INFO, fmt::format("******************       Iteration: {}       ******************", iter));



    //     for(auto id:deps){
    //         int hId = exToHMapping[id];
    //         solverWrapper->assumeSelectors(hId);
    //     }

    //     int status = solverWrapper->solve();

    //     if(status == CaDiCaL::UNKNOWN){
    //         break;
    //     }

    //     if(status==CaDiCaL::UNSATISFIABLE){
    //         /*
    //         fill code block
    //         */
    //         int constraintStatus = constraintWrapper->solve();
            
    //         if(constraintStatus == CaDiCaL::UNSATISFIABLE){
    //             globalLogger.log(LogLevel::INFO, "Constraint Unsatisfiable, no solution exists.");
    //             return 1;
    //         }
            
    //         if(constraintStatus == CaDiCaL::SATISFIABLE){
    //             // store the skolem functions
    //             globalLogger.log(LogLevel::INFO, "Constraint Satisfiable");
    //             return 0;
    //         }

    //         if(constraintStatus == CaDiCaL::UNKNOWN){
    //             globalLogger.log(LogLevel::ERROR, "Constrainst Solver returned UNKNOWN Status.");
    //             return 1;
    //         }
    //     }

    //     assert(status == CaDiCaL::SATISFIABLE);


        

    //     bool changeFlag = false;
        
    //     std::vector<int> cex = solverWrapper->getCex();


    //     globalLogger.log(LogLevel::DEBUG, fmt::format("Cex: {}", fmt::join(cex," ")));

    //     std::vector<int> currConstraint;

    //     std::vector<int> currAssumptions;

    //     for(auto id:deps){
    //         if(cex[id-1]>0){
    //             currAssumptions.push_back(id);
    //         }
    //         else{
    //             currAssumptions.push_back(-id);
    //         }
    //     }

    //     std::set<int> univs = origDqbf->GetUniversals();
    //     std::vector<int> univAssumptions;
    //     for(auto id:univs){
    //         if(cex[id-1]>0){
    //             univAssumptions.push_back(id);
    //         }
    //         else{
    //             univAssumptions.push_back(-id);
    //         }
    //     }

    //     unsatCoreWrapper->assume(currAssumptions);
    //     unsatCoreWrapper->assume(univAssumptions);


    //     int unsatCoreStatus = unsatCoreWrapper->solve();
    //     if(unsatCoreStatus==CaDiCaL::SATISFIABLE){
    //         /* log error*/
    //         return 1;
    //     }

    //     std::set<int> unsatCoreLits = getUnsatCore(currAssumptions, univAssumptions, unsatCoreWrapper);


    //     globalLogger.log(LogLevel::DEBUG, fmt::format("UnsatCoreLits: {}", fmt::join(unsatCoreLits," ")));


    //     for(auto e:unsatCoreLits){
    //         int id = abs(e);

    //         int aVal =  Abc_NtkVerifySimulatePattern(varToBasisMap[id].first, cex.data())[0];
    //         int bVal =  Abc_NtkVerifySimulatePattern(varToBasisMap[id].second, cex.data())[0];

    //         globalLogger.log(LogLevel::DEBUG, fmt::format("id: {} aVal: {} bVal: {}", id, aVal, bVal));
            

    //         if(!(aVal==0 && bVal==1)){
    //             printf("AiBi Check Failed: %d\n",id);
    //             continue;
    //         }

    //         std::set<int> depVal;
    //         std::set<int> depSet = origDqbf->GetDependencySet(id);
    //         for(auto dep:depSet){
    //             if(cex[dep-1]==0){
    //                 depVal.insert(-dep);
    //             }
    //             else depVal.insert(dep);
    //         }

    //         globalLogger.log(LogLevel::DEBUG, fmt::format("DepVal for id: {} => : {}",id, fmt::join(depVal," ")));



    //         if(ex_caseToAuxMapping[id].find(depVal)==ex_caseToAuxMapping[id].end()){
            
    //             changeFlag=true;

    //             int newAuxVar = solverWrapper->getNewVar();

    //             solverWrapper->synthesize(depVal, exToHMapping[id], newAuxVar);

    //             if(cex[id-1]>0){
    //                 currConstraint.push_back(-newAuxVar);
    //             }
    //             else{
    //                 currConstraint.push_back(newAuxVar);

    //             }

    //             ex_caseToAuxMapping[id][depVal]= {newAuxVar, newAuxVar};
    //         }
    //         else{
    //             changeFlag=true;
    //             int newAuxVar = ex_caseToAuxMapping[id][depVal].first;
                
    //             if(cex[id-1]>0){
    //                 currConstraint.push_back(-newAuxVar);
    //             }
    //             else{
    //                 currConstraint.push_back(newAuxVar);

    //             }
    //         }
    //     }
    //     if(!changeFlag){
    //         globalLogger.log(LogLevel::INFO, "No Change Occured.");
    //         break;
    //     }

    //     globalLogger.log(LogLevel::DEBUG, fmt::format("CurrConstraint: {}", fmt::join(currConstraint," ")));


    //     solverWrapper->addClause(currConstraint);
    //     constraintWrapper->addClause(currConstraint);

    // }
    // // return 1;
    return 0;
}