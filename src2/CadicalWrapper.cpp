#include "CadicalWrapper.h"

static inline int Cnf_Lit2Var( int Lit )        { return (Lit & 1)? -(Lit >> 1)-1 : (Lit >> 1)+1;  }
static inline int Cnf_Lit2Var2( int Lit )       { return (Lit & 1)? -(Lit >> 1)   : (Lit >> 1);    }


Cnf_Dat_t* myDarToCnf(Abc_Ntk_t* pNtk, char * pFilename, int fFastAlgo, int fChangePol, int fVerbose){
	Aig_Man_t* pMan;
	Cnf_Dat_t* pCnf;
	Abc_Ntk_t* pNtkNew = NULL;
	abctime clk=Abc_Clock();
	assert(Abc_NtkIsStrash(pNtk));


	pMan = ABC_NAMESPACE::Abc_NtkToDar(pNtk,0,0);
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

	if(fVerbose) {
		Abc_Print( 1, "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d.   ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
		Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
	}
	// Cnf_DataWriteIntoFile(pCnf,pFilename,0,NULL,NULL);
	Aig_ManStop(pMan);
	return pCnf;

}

CadicalWrapper::CadicalWrapper(){
    this->solver.set("incremental",1);
    this->numInputs = 0;
}


CadicalWrapper::CadicalWrapper(AigWrapper* aw){
    solver.set("incremental",1);

    this->numInputs = aw->getNumInputs();
    
    // Aig_Man_t* man = aw->getManager();

    Abc_Ntk_t* ntk = aw->getNtk();
    Cnf_Dat_t* pCnf;

    pCnf = myDarToCnf(ntk,NULL,0,0,0);

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
        assert(this->inputToVarMapping[i+1]!=0);
    }
    Cnf_DataFree(pCnf);
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

void CadicalWrapper::assumeSelectors(int hId){
    std::vector<int> selectors = this->HtoSelectorMapping[hId];
    int sz = selectors.size();
    for(int i=0;i<sz-1;i++){
        this->solver.assume(selectors[i]);
    }
    solver.assume(-selectors[sz-1]);
    return;
}

int CadicalWrapper::solve(){
    return this->solver.solve();
}


std::vector<int> CadicalWrapper::getCex(){
    std::vector<int> cex(this->numInputs);
    for(int i=1;i<=this->numInputs;i++){
        int val=this->solver.val(this->inputToVarMapping[i]);
        cex[i-1]=val>0?1:0;
    }

    return cex;

}

void CadicalWrapper::assume(std::vector<int> assumptions){

    for(auto ass:assumptions){
        if(ass>0){
            this->solver.assume(this->inputToVarMapping[ass]);
        }
        else{
            this->solver.assume(-this->inputToVarMapping[-ass]);
        }
    }
    return;
}

bool CadicalWrapper::failed(int lit){

    if(lit>0) return this->solver.failed(this->inputToVarMapping[lit]);
    else return this->solver.failed(-this->inputToVarMapping[-lit]);
}

void CadicalWrapper::assume(int lit){
    if(lit>0) this->solver.assume(this->inputToVarMapping[lit]);
    else this->solver.assume(-this->inputToVarMapping[-lit]);
    
}

int CadicalWrapper::getNewVar(){

    this->numInputs++;
    int newVarId = this->solver.vars()+1;
    this->inputToVarMapping[this->numInputs] = newVarId;
    return this->numInputs;

}


void CadicalWrapper::synthesize(std::set<int> depVal, int hId, int newAuxVar){

    if(this->caseToAuxMap.find(depVal)==this->caseToAuxMap.end()){
    
        int newCaseVar = this->solver.vars()+2;

        std::vector<int> c1;
        for(auto d:depVal){
            if(d>0){
                c1.push_back(-this->inputToVarMapping[d]);
            }
            else{
                c1.push_back(this->inputToVarMapping[-d]);
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
                solver.add(this->inputToVarMapping[d]);
            }
            else{
                solver.add(-this->inputToVarMapping[-d]);
            }
            solver.add(0);
        }

        this->caseToAuxMap[depVal] = newCaseVar;
    }

    int caseVar = this->caseToAuxMap[depVal];
    int hVar = this->inputToVarMapping[hId];
    int zVar = this->HtoZMapping[hId];

    int newZ = this->solver.vars()+3;
    int newS = this->solver.vars()+4;
    int cnfVar = this->inputToVarMapping[newAuxVar];
    // newZ = ite(caseVar, newAux,zVar)
                //adding clause 1
    this->solver.add(-caseVar);
    this->solver.add(newZ);
    this->solver.add(-cnfVar);
    this->solver.add(0);

    //adding clause 2
    this->solver.add(-caseVar);
    this->solver.add(-newZ);
    this->solver.add(cnfVar);
    this->solver.add(0);

    //adding clause 3
    this->solver.add(caseVar);
    this->solver.add(newZ);
    this->solver.add(-zVar);
    this->solver.add(0);

    //adding clause 4
    this->solver.add(caseVar);
    this->solver.add(-newZ);
    this->solver.add(zVar);
    this->solver.add(0);

    //Now we will add h<=> newZ if ~selector
    //adding h or -newZ or newS
    this->solver.add(hVar);
    this->solver.add(-newZ);
    this->solver.add(newS);
    this->solver.add(0);

    //adding -h or newZ or newS
    this->solver.add(-hVar);
    this->solver.add(newZ);
    this->solver.add(newS);
    this->solver.add(0);

    this->HtoZMapping[hId] = newZ;
    this->HtoSelectorMapping[hId].push_back(newS);

    return;
}

void CadicalWrapper::addClause(std::vector<int> clause){
    for(auto lit:clause){
        if(lit>0){
            this->solver.add(this->inputToVarMapping[lit]);
        }
        else{
            this->solver.add(-this->inputToVarMapping[-lit]);
        
        }
    }

    this->solver.add(0);
    return;

}

CadicalWrapper::~CadicalWrapper(){
    
}