#include "CadicalWrapper.h"

static inline int Cnf_Lit2Var( int Lit )        { return (Lit & 1)? -(Lit >> 1)-1 : (Lit >> 1)+1;  }
static inline int Cnf_Lit2Var2( int Lit )       { return (Lit & 1)? -(Lit >> 1)   : (Lit >> 1);    }


Cnf_Dat_t* myDarToCnf(Abc_Ntk_t* pNtk, char * pFilename, int fFastAlgo, int fChangePol, int fVerbose){
	Aig_Man_t* pMan;
	Cnf_Dat_t* pCnf;
	Abc_Ntk_t* pNtkNew = NULL;
	abctime clk=Abc_Clock();
	assert(Abc_NtkIsStrash(pNtk));


	pMan = Abc_NtkToDar(pNtk,0,0);
	if(pMan==NULL){
		return NULL;
	}
	if(!Aig_ManCheck(pMan)){
		std::cerr<<"myDarToCnf: Aig_ManCheck failed"<<std::endl;
		Aig_ManStop(pMan);
		return NULL;
	}

	if(fVerbose)
	Aig_ManPrintStats(pMan);

	if(fFastAlgo){
		pCnf = Cnf_DeriveFast(pMan,0);
	}
	else{
		pCnf = Cnf_Derive(pMan,0);
	}

	if(fChangePol){
		Cnf_DataTranformPolarity(pCnf,0);
	}

	Abc_Print( 1, "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d.   ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
	// Cnf_DataWriteIntoFile(pCnf,pFilename,0,NULL,NULL);
	Aig_ManStop(pMan);
	return pCnf;

}



CadicalWrapper::CadicalWrapper(AigWrapper* aw){
    solver.set("incremental",1);

    this->numInputs = aw->getNumInputs();
    
    // Aig_Man_t* man = aw->getManager();

    Abc_Ntk_t* ntk = aw->getNtk();
    Cnf_Dat_t* pCnf;

    pCnf = myDarToCnf(ntk,NULL,0,0,1);

    int * pLit, * pStop;
    // printf("Num Vars: %d, Num Clauses: %d\n",pCnf->nVars, pCnf->nClauses);
    for(int i=0;i<pCnf->nClauses;i++){
        for(pLit=pCnf->pClauses[i],pStop=pCnf->pClauses[i+1];pLit<pStop;pLit++){
            // printf("%d ", Cnf_Lit2Var2(*pLit));
            this->solver.add(Cnf_Lit2Var2(*pLit));
        }
        // printf("0\n");
        this->solver.add(0);
    }

    for(int i=0;i<this->numInputs;i++){
        this->inputToVarMapping[i+1] = pCnf->pVarNums[Abc_ObjId(Abc_NtkCi(ntk,i))];
    }
}

void CadicalWrapper::setDefaultValue(int hVar, bool defaultVal){

    int zVarId = this->solver.vars()+1;
    int sVarId = this->solver.vars()+2;

    if(defaultVal){
        this->solver.add(zVarId);
    }
    else{
        this->solver.add(-zVarId);
    }
    this->solver.add(0);

    solver.add(-hVar);
    solver.add(zVarId);
    solver.add(sVarId);
    solver.add(0);
    
    //h or -z or s
    solver.add(hVar);
    solver.add(-zVarId);
    solver.add(sVarId);
    solver.add(0);
    
    this->HtoZMapping[hVar] = zVarId;
    this->HtoSelectorMapping[hVar].push_back(sVarId);

}