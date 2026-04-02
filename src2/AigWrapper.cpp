#include "AigWrapper.h"
#include <fstream>
#include <sstream>
#include <iostream>

// std::map<int, std::pair<Abc_Ntk_t*, Abc_Ntk_t*>> varToBasisMap;


/** Function
 * Composes input variable in initiAig with @param one, returns resulting Aig_Obj
 * @param pMan      [in out]    Aig Manager
 * @param initAig   [in]        Specifies the head of function tree
 * @param varId     [in]        (>=1) Specifies the variable to be substituted
 * @param one       [in]       set to 1 if varId is to be substituted by 1
 */
Aig_Obj_t* Aig_SubstituteConst(Aig_Man_t* pMan, Aig_Obj_t* initAig, int varId, int one) {
	Aig_Obj_t* const1 = Aig_ManConst1(pMan);
	Aig_Obj_t* constf = (one? const1: Aig_Not(const1));
	Aig_Obj_t* currFI = Aig_ObjIsCo(Aig_Regular(initAig))? initAig->pFanin0: initAig;
	Aig_Obj_t* afterCompose = Aig_Compose(pMan, currFI, constf, varId-1);
	assert(!Aig_ObjIsCo(Aig_Regular(afterCompose)));
	return afterCompose;
}

/** Function
 * Compresses Aig using the compressAig() routine
 * Deletes SAig and returns a compressed version
 * @param SAig      [in]        Aig to be compressed
 */
Aig_Man_t* compressAig(Aig_Man_t* SAig) {
	// OUT("Cleaning up...");
	int removed = Aig_ManCleanup(SAig);
	// cout << "Removed " << removed <<" nodes" << endl;

	Aig_Man_t* temp = SAig;
	// Dar_ManCompress2( Aig_Man_t * pAig, int fBalance,
	//                  int fUpdateLevel, int fFanout,
	//                  int fPower, int fVerbose )
	// OUT("Running Dar_ManCompress2...");
	
	// cout<<"HERE\n";
	SAig =  Dar_ManCompress2(SAig, 1, 1, 26, 1, 0);
	// OUT("Stopping Old Aig Manager...");
	Aig_ManStop( temp );
	return SAig;
}


Aig_Man_t* remapInputs(Aig_Man_t* p, std::vector<int> remapIds){
	
	Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;

	pNew = Aig_ManStart(Aig_ManObjNumMax(p));

	Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
	// cout<<"REMAP INPUTS CREATING PIs\n";
	// cout<<Aig_ManCiNum(p)<<endl;

	Aig_ManForEachCi(p,pObj,i){
		Aig_ObjCreateCi(pNew);
	}

	Aig_ManForEachCi(p, pObj, i){
		// cout<<i+1<<" "<<remapIds[i]<<endl;	
		Aig_ManCi(p, i)->pData = Aig_ManCi(pNew, remapIds[i]-1 );
	}

	Aig_ManForEachNode(p, pObj, i){
		pObj->pData = Aig_And(pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj));
	}

	Aig_ManForEachCo(p, pObj, i){
		Aig_ObjCreateCo(pNew,Aig_ObjChild0Copy(pObj));
	}


	return pNew;
}

void Aig_ComposeVec_rec( Aig_Man_t * p, Aig_Obj_t * pObj, std::vector<Aig_Obj_t *>& pFuncVec,
	std::vector<Aig_Obj_t* >& iVarObjVec ) {
	assert( !Aig_IsComplement(pObj) );
	if ( Aig_ObjIsMarkA(pObj) )
		return;
	if ( Aig_ObjIsConst1(pObj) || Aig_ObjIsCi(pObj) ) {
		pObj->pData = pObj;
		int i = 0;
		for (auto iVarObj: iVarObjVec) {
			if(pObj == iVarObj) {
				pObj->pData = pFuncVec[i];
				//cout << " Assigned " << pObj->Id  << ( Aig_ObjIsConst1(Aig_Regular(pFuncVec[i]))? 1 : 0) << endl;
			}
			i++;
		}
		return;
	}
	Aig_ComposeVec_rec( p, Aig_ObjFanin0(pObj), pFuncVec, iVarObjVec );
	Aig_ComposeVec_rec( p, Aig_ObjFanin1(pObj), pFuncVec, iVarObjVec );
	pObj->pData = Aig_And( p, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
	assert( !Aig_ObjIsMarkA(pObj) ); // loop detection
	Aig_ObjSetMarkA( pObj );
}



Aig_Obj_t * Aig_ComposeVec( Aig_Man_t * p, Aig_Obj_t * pRoot, std::vector<Aig_Obj_t *>& pFuncVec,
	std::vector<int>& iVarVec ) {
	// quit if the PI variable is not defined
	for(auto iVar: iVarVec) {
		if (iVar >= Aig_ManCiNum(p)) {
			printf( "Aig_Compose(): The PI variable %d is not defined.\n", iVar );
			return NULL;
		}
	}
	// recursively perform composition
	std::vector<Aig_Obj_t *> iVarObjVec(iVarVec.size());
	int i = 0;
	for (auto iVar: iVarVec) {
		iVarObjVec[i++] = Aig_ManCi(p, iVar);
	}
	Aig_ComposeVec_rec( p, Aig_Regular(pRoot), pFuncVec, iVarObjVec );
	// clear the markings
	Aig_ConeUnmark_rec( Aig_Regular(pRoot) );
	return Aig_NotCond( (Aig_Obj_t *)Aig_Regular(pRoot)->pData, Aig_IsComplement(pRoot) );
}



Aig_Obj_t* Aig_SubstituteVec(Aig_Man_t* pMan, Aig_Obj_t* initAig, std::vector<int> varIdVec,
	std::vector<Aig_Obj_t*>& funcVec) {
	Aig_Obj_t* currFI = Aig_ObjIsCo(Aig_Regular(initAig))? initAig->pFanin0: initAig;
	for (int i = 0; i < funcVec.size(); ++i) {
		funcVec[i] = Aig_ObjIsCo(Aig_Regular(funcVec[i]))? funcVec[i]->pFanin0: funcVec[i];
	}
	for (int i = 0; i < varIdVec.size(); ++i) {
		varIdVec[i]--;
	}
	Aig_Obj_t* afterCompose = Aig_ComposeVec(pMan, currFI, funcVec, varIdVec);

	assert(!Aig_ObjIsCo(Aig_Regular(afterCompose)));
	return afterCompose;
}




AigWrapper::~AigWrapper(){
    Aig_ManStop(this->manager);
}

AigWrapper::AigWrapper(Dqbf* dqbf){
    // this->numInputs = origDqbf->GetNumInputs();
    this->manager = Aig_ManStart(0);
    
    int numInputs = dqbf->GetNumInputs();

    this->addInputs(numInputs);

    Aig_Obj_t* outNode = Aig_ManConst1(this->manager);


    for(auto clause: dqbf->GetClauses()){
        Aig_Obj_t* clauseNode = Aig_ManConst0(this->manager);
        // globalLogger.log(LogLevel::ERROR,fmt::format("Clause: {}",fmt::join(clause," ")));
        for(auto lit:clause){
            if(lit>0){
                clauseNode = Aig_Or(this->manager, clauseNode, Aig_ManCi(this->manager, lit-1));
            }
            else{
                clauseNode = Aig_Or(this->manager, clauseNode, Aig_Not(Aig_ManCi(this->manager, -lit-1)));
            }
            // this->ShowAig();
        }
        outNode = Aig_And(this->manager, outNode, clauseNode);
        // this->ShowAig();
    }
    Aig_ObjCreateCo(this->manager, outNode);
}

AigWrapper::AigWrapper(KissatWrapper* kw){
    this->manager = Aig_ManStart(0);

    int numInputs = kw->getNumVars();

    this->addInputs(numInputs);

    Aig_Obj_t* outNode = Aig_ManConst1(this->manager);

    for(auto clause: kw->getClauses()){
        Aig_Obj_t* clauseNode = Aig_ManConst0(this->manager);
        for(auto lit:clause){
            if(lit>0){
                clauseNode = Aig_Or(this->manager, clauseNode, Aig_ManCi(this->manager, lit-1));
            }
            else{
                clauseNode = Aig_Or(this->manager, clauseNode, Aig_Not(Aig_ManCi(this->manager, -lit-1)));
            }
            // this->ShowAig();
        }
        outNode = Aig_And(this->manager, outNode, clauseNode);
        // this->ShowAig();
    }
    Aig_ObjCreateCo(this->manager, outNode);
    // this->ShowAig();

    std::vector<int> evars = kw->getExistentialVarsToEliminate();
    std::vector<int> uvars = kw->getUniversalVarsToEliminate();
    std::vector<int> dvars = kw->getDepVarsToEliminate();

    // if(!evars.empty()){
    //     kw->callManthan();

    //     std::string manthanDir = "/home/coolboy19/Desktop/BooleanSynthesis/dependencies/manthan";
    //     std::string baseName = "temp_var_" + std::to_string(kw->getOutputVar());
    //     std::string verilogFile = manthanDir + "/" + baseName + "_skolem.v";
        
    //     Abc_Ntk_t* defNtk = Io_ReadVerilog(const_cast<char*>(verilogFile.c_str()), 0);
    //     if (defNtk) {
    //         Abc_Ntk_t* logicNtk = Abc_NtkToLogic(defNtk);
    //         Abc_Ntk_t* strashNtk = Abc_NtkStrash(logicNtk, 0, 1, 0);
            
    //         Abc_NtkDelete(defNtk);
    //         Abc_NtkDelete(logicNtk);
    //         defNtk = strashNtk;
    //     } else {
    //         std::cerr << "Error: Failed to read Verilog file from Manthan: " << verilogFile << std::endl;
    //         exit(1);
    //     }

    //     Aig_Man_t* eDefMan = ABC_NAMESPACE::Abc_NtkToDar(defNtk, 0, 0);
    //     Abc_NtkDelete(defNtk);

    //     int numOrigInputs = Aig_ManCiNum(this->manager);
    //     while(Aig_ManCiNum(eDefMan)<numOrigInputs){
    //         Aig_ObjCreateCi(eDefMan);
    //     }

    //     std::vector<int> ordering;
    //     for(auto e:kw->getInputOrdering()){
    //         ordering.push_back(e);
    //     }

    //     for(auto e:kw->getOutputOrdering()){
    //         ordering.push_back(e);
    //     }

    //     for(auto e:kw->getEliminatedVars()){
    //         ordering.push_back(e);
    //     }

    //     Aig_Man_t* newEDefMan=remapInputs(eDefMan, ordering);
    //     Aig_ManStop(eDefMan);
    //     eDefMan = newEDefMan;
    //     assert(Aig_ManCoNum(eDefMan)==kw->getOutputOrdering().size());
    //     assert(Aig_ManCiNum(eDefMan) == Aig_ManCiNum(this->manager));

    //     Abc_Ntk_t* specNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
    //     defNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(eDefMan);

    //     Abc_NtkAppend(specNtk, defNtk, 1);

    //     Aig_Man_t* specMan = ABC_NAMESPACE::Abc_NtkToDar(specNtk, 0, 0);

    //     std::set<int> outputOrdering = kw->getOutputOrdering();



    //     std::vector<int> varIds;

    //     for(auto e:outputOrdering){
    //         varIds.push_back(e);
    //     }

    //     std::vector<Aig_Obj_t*> funcIds;

    //     for(int i=1;i<Aig_ManCoNum(specMan);i++){
    //         funcIds.push_back(Aig_ManCo(specMan,i));
    //     }

    //     Aig_Obj_t* newDriver = Aig_SubstituteVec(specMan,Aig_ManCo(specMan,0),varIds,funcIds);

    //     Aig_ObjCreateCo(specMan, newDriver);

    //     for(int i=0;i<Aig_ManCoNum(specMan)-1;i++){
    //         Aig_ObjDisconnect(specMan, Aig_ManCo(specMan, i));
    //         Aig_ObjConnect(specMan, Aig_ManCo(specMan, i), Aig_ManConst0(specMan), NULL);
    //         // Aig_ManCoCleanup(specMan);
    //     }

    //     Aig_ManCoCleanup(specMan);
    //     Aig_ManCleanup(specMan);
    //     if(Aig_ManCoNum(specMan) == 0){
    //         Aig_ObjCreateCo(specMan, Aig_ManConst0(specMan));
    //     }

    //     this->manager = specMan;
    // }


    Abc_Ntk_t* ntk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
    Aig_ManStop(this->manager);


    globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk size: {}\n", Abc_NtkNodeNum(ntk)));
    
    for(int i=0;i<evars.size();i++){
        Abc_Ntk_t* newNtk = Abc_NtkMiterQuantify(ntk, evars[i]-1, 1);
        if(newNtk==NULL) exit(2);
        Abc_NtkDelete(ntk);
        ntk = newNtk;
    }
    
    globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk(-e) size: {}\n", Abc_NtkNodeNum(ntk)));


    for(int i=0;i<dvars.size();i++){
        Abc_Ntk_t* newNtk = Abc_NtkMiterQuantify(ntk, dvars[i]-1, 1);
        if(newNtk==NULL) exit(2);
        Abc_NtkDelete(ntk);
        ntk = newNtk;
    }

    globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk(-d) size: {}\n", Abc_NtkNodeNum(ntk)));

    for(int i=0;i<uvars.size();i++){
        Abc_Ntk_t* newNtk = Abc_NtkMiterQuantify(ntk, uvars[i]-1, 0);
        if(newNtk==NULL) exit(2);
        Abc_NtkDelete(ntk);
        ntk = newNtk;
    }

    globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk(-a) size: {}\n", Abc_NtkNodeNum(ntk)));

    this->manager = ABC_NAMESPACE::Abc_NtkToDar(ntk, 0, 0);
    Abc_NtkDelete(ntk);
    // this->ShowAig();


}

AigWrapper::AigWrapper(const AigWrapper& other){
    this->manager = Aig_ManDupOrdered(other.manager);
}

AigWrapper::AigWrapper(AigWrapper* other){
    this->manager = Aig_ManDupOrdered(other->manager);
}


void AigWrapper::ShowAig(){
    Aig_ManShow(this->manager,0,NULL);
    int x;
    std::cin>>x;
    return;
}


Aig_Man_t* AigWrapper::getManager(){
    return this->manager;
}

void AigWrapper::addInputs(int numInputs){
    for(int i=0;i<numInputs;i++){
        Aig_ObjCreateCi(this->manager);
    }
}

void AigWrapper::negateOutput(){
    Aig_ManCo(this->manager, 0)->pFanin0 = Aig_Not(Aig_ManCo(this->manager, 0)->pFanin0);
}

int AigWrapper::getNumInputs(){
    return Aig_ManCiNum(this->manager);
}

void AigWrapper::compress(){
    this->manager = compressAig(this->manager);
}

Abc_Ntk_t* AigWrapper::getNtk(){
    return ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
}


int AigWrapper::DumpVerilogWithFrame(std::string fileName) {
    Abc_Frame_t * pAbc = Abc_FrameGetGlobalFrame();
    
    Abc_Ntk_t* pNtkLogic = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
    if (pNtkLogic == NULL) return 0;

    // Add this missing naming step
    if (pNtkLogic->pName == NULL) {
        pNtkLogic->pName = Extra_UtilStrsav("aig_wrapper_netlist");
    }
    // Ensure consistent PI/PO naming (e.g., n1, n2) like other dump functions
    Abc_NtkShortNames(pNtkLogic);

    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkLogic);

    std::string cmd = "write_verilog " + fileName;
    Cmd_CommandExecute(pAbc, cmd.c_str());

    return 1;
}

// int AigWrapper::DumpVerilog(std::string fileName){

//     // 1. Create a generic logic network from the AIG manager
//     Abc_Ntk_t* pNtkLogic = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
//     if (pNtkLogic == NULL) return 0;

//     // --- MOVE NAMING HERE ---
//     // 2. Assign names to the LOGIC network (before it becomes a Netlist)
//     if (pNtkLogic->pName == NULL) {
//         pNtkLogic->pName = Extra_UtilStrsav("aig_wrapper_netlist");
//     }
//     Abc_NtkShortNames(pNtkLogic); 
//     // ------------------------

//     // 3. Convert the (now named) Logic Network into a Netlist
//     Abc_Ntk_t* pNtkNetlist = Abc_NtkToNetlist(pNtkLogic);
//     if (pNtkNetlist == NULL) {
//         Abc_NtkDelete(pNtkLogic);
//         return 0;
//     }

//     // 4. Convert local node functions from SOP to AIG
//     if ( !Abc_NtkIsAigNetlist(pNtkNetlist) && !Abc_NtkIsMappedNetlist(pNtkNetlist) ) {
//         Abc_NtkToAig(pNtkNetlist);
//     }

//     // 5. Write out the Verilog safely
//     Io_WriteVerilog(pNtkNetlist, const_cast<char*>(fileName.c_str()), 0, 0);

//     // 6. Clean up memory
//     Abc_NtkDelete(pNtkNetlist);
//     Abc_NtkDelete(pNtkLogic);

//     return 1;
// }



void AigWrapper::merge(AigWrapper* aw){


    Abc_Ntk_t* baseNtk = this->getNtk();
    Abc_Ntk_t* srcNtk = aw->getNtk();

    Abc_NtkAppend(baseNtk,srcNtk,1);

    this->manager = ABC_NAMESPACE::Abc_NtkToDar(baseNtk,0,0);

    Aig_Obj_t* out1 = Aig_ManCo(this->manager,0)->pFanin0;
    Aig_Obj_t* out2 = Aig_ManCo(this->manager,1)->pFanin0;

    Aig_Obj_t* newOut = Aig_And(this->manager, out1, out2);

    Aig_ObjCreateCo(this->manager, newOut);

    Aig_ObjDisconnect(this->manager, Aig_ManCo(this->manager, 0));
    Aig_ObjConnect(this->manager, Aig_ManCo(this->manager, 0), Aig_ManConst0(this->manager), NULL);
    
    Aig_ObjDisconnect(this->manager, Aig_ManCo(this->manager, 1));
    Aig_ObjConnect(this->manager, Aig_ManCo(this->manager, 1), Aig_ManConst0(this->manager), NULL);

    Aig_ManCoCleanup(this->manager);
    Aig_ManCleanup(this->manager);
    
    // Aig_ManCoCleanup(this->manager);
    return;




}

void AigWrapper::generateDef(int outputVar, int hVar){

    Aig_Man_t* const1Man = Aig_ManDupOrdered(this->manager);
    Aig_Man_t* const0Man = Aig_ManDupOrdered(this->manager);
    
    Aig_Obj_t* newOut = Aig_SubstituteConst(const1Man, Aig_ManCo(const1Man,0),outputVar,1);

    Aig_ObjCreateCo(const1Man, newOut);

    Aig_ObjDisconnect(const1Man, Aig_ManCo(const1Man, 0));
    Aig_ObjConnect(const1Man, Aig_ManCo(const1Man, 0), Aig_ManConst0(const1Man), NULL);
    Aig_ManCoCleanup(const1Man);
    Aig_ManCleanup(const1Man);

    if (Aig_ManCoNum(const1Man) == 0)
    {
        Aig_ObjCreateCo(const1Man, Aig_ManConst0(const1Man));
    }

    newOut = Aig_SubstituteConst(const0Man, Aig_ManCo(const0Man,0),outputVar,0);

    Aig_ObjCreateCo(const0Man, newOut);

    Aig_ObjDisconnect(const0Man, Aig_ManCo(const0Man, 0));
    Aig_ObjConnect(const0Man, Aig_ManCo(const0Man, 0), Aig_ManConst0(const0Man), NULL);
    Aig_ManCoCleanup(const0Man);
    Aig_ManCleanup(const0Man);

    if (Aig_ManCoNum(const0Man) == 0)
    {
        Aig_ObjCreateCo(const0Man, Aig_ManConst0(const0Man));
    }

    Abc_Ntk_t *phi_1_Ntk = ABC_NAMESPACE::Abc_NtkFromAigPhase(const1Man);
    Abc_Ntk_t *phi_0_Ntk = ABC_NAMESPACE::Abc_NtkFromAigPhase(const0Man);

    Abc_NtkAppend(phi_0_Ntk, phi_1_Ntk, 1);

    Aig_Man_t *tMan = ABC_NAMESPACE::Abc_NtkToDar(phi_0_Ntk, 0, 0);
    Aig_Obj_t *newNode = Aig_And(tMan, Aig_ManCo(tMan, 1)->pFanin0, Aig_Not(Aig_ManCo(tMan, 0)->pFanin0));

    Aig_ObjCreateCo(tMan, newNode);

    // remove old outputs and cleanup the network
    Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 0));
    Aig_ObjConnect(tMan, Aig_ManCo(tMan, 0), Aig_ManConst0(tMan), NULL);
    Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 1));
    Aig_ObjConnect(tMan, Aig_ManCo(tMan, 1), Aig_ManConst0(tMan), NULL);
    Aig_ManCoCleanup(tMan);
    Aig_ManCleanup(tMan);

    if (Aig_ManCoNum(tMan) == 0)
    {
        Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
    }
    // tMan = Aig(tMan);
    tMan = compressAig(tMan);
    Aig_Man_t* AMan = tMan;

    Abc_Ntk_t* ANtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(AMan);



    tMan = ABC_NAMESPACE::Abc_NtkToDar(phi_0_Ntk, 0, 0);

    // phi1 OR !phi_0
    Aig_Obj_t *newNode1 = Aig_Or(tMan, Aig_ManCo(tMan, 1)->pFanin0, Aig_Not(Aig_ManCo(tMan, 0)->pFanin0));

    // !phi OR phi0
    Aig_Obj_t *newNode2 = Aig_Or(tMan, Aig_ManCo(tMan, 0)->pFanin0, Aig_Not(Aig_ManCo(tMan, 1)->pFanin0));

    // newNode1 AND newNode2
    Aig_Obj_t *finalNewNode = Aig_And(tMan, newNode1, newNode2);

    // Create new Output
    Aig_ObjCreateCo(tMan, finalNewNode);

    // remove old outputs and cleanup the network
    Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 0));
    Aig_ObjConnect(tMan, Aig_ManCo(tMan, 0), Aig_ManConst0(tMan), NULL);
    Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 1));
    Aig_ObjConnect(tMan, Aig_ManCo(tMan, 1), Aig_ManConst0(tMan), NULL);
    Aig_ManCoCleanup(tMan);
    Aig_ManCleanup(tMan);

    if (Aig_ManCoNum(tMan) == 0)
    {
        Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
    }
    
    tMan = compressAig(tMan);

    Aig_Man_t* BMan = tMan;
    Abc_Ntk_t* BNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(BMan);


    varToBasisMap[outputVar] = std::make_pair(ANtk, BNtk);


    Abc_Ntk_t* ANtk2 = ABC_NAMESPACE::Abc_NtkFromAigPhase(AMan);
    Abc_Ntk_t* BNtk2 = ABC_NAMESPACE::Abc_NtkFromAigPhase(BMan);

    Abc_NtkAppend(ANtk2, BNtk2, 1);

    Aig_Man_t* defMan = ABC_NAMESPACE::Abc_NtkToDar(ANtk2, 0, 0);
    
    Aig_Obj_t* outA = Aig_ManCo(defMan,0)->pFanin0;
    Aig_Obj_t* outB = Aig_ManCo(defMan,1)->pFanin0;

    Aig_Obj_t* currH = Aig_ManCi(defMan, hVar-1);

    Aig_Obj_t *HAndB = Aig_And(defMan, currH, outB);
    Aig_Obj_t *defin = Aig_Or(defMan, outA, HAndB);
    Aig_Obj_t *yImpDefin = Aig_Or(defMan, Aig_Not(Aig_ManCi(defMan, outputVar - 1)), defin);
    Aig_Obj_t *definImpY = Aig_Or(defMan, Aig_Not(defin), Aig_ManCi(defMan, outputVar - 1));

    Aig_Obj_t* defOutNode = Aig_And(defMan, yImpDefin, definImpY);
    Aig_ObjCreateCo(defMan, defOutNode);

    int numAigOuts = Aig_ManCoNum(defMan);

    for(int i=0;i<numAigOuts-1;i++){
        Aig_ObjDisconnect(defMan, Aig_ManCo(defMan, i));
        Aig_ObjConnect(defMan, Aig_ManCo(defMan, i), Aig_ManConst0(defMan), NULL);
        // Aig_ManCoCleanup(defMan);
    }

    Aig_ManCoCleanup(defMan);
    Aig_ManCleanup(defMan);

    if(Aig_ManCoNum(defMan) == 0){
        Aig_ObjCreateCo(defMan, Aig_ManConst0(defMan));
    }

    //////////////////////////////////////////////

    this->manager = defMan;

    //////////////////////////////////////////////

    Aig_ManStop(const1Man);
    Aig_ManStop(const0Man);
    // Aig_ManStop(tMan);
    Aig_ManStop(AMan);
    Aig_ManStop(BMan);

    return;


}


void AigWrapper::substituteInputs(std::set<int> inputsToReplace, char* skolemFile, char* orderingFile){

    // 1. Read the Skolem function (Verilog file) into an AIG Manager
    Abc_Ntk_t* defNtk = Io_ReadVerilog(skolemFile, 0);
    if (!defNtk) {
        std::cerr << "Error: Failed to read Verilog file: " << skolemFile << std::endl;
        exit(1);
    }
    
    Abc_Ntk_t* logicNtk = Abc_NtkToLogic(defNtk);
    Abc_Ntk_t* strashNtk = Abc_NtkStrash(logicNtk, 0, 1, 0);
    Aig_Man_t* eDefMan = ABC_NAMESPACE::Abc_NtkToDar(strashNtk, 0, 0);
    
    Abc_NtkDelete(defNtk);
    Abc_NtkDelete(logicNtk);
    Abc_NtkDelete(strashNtk);

    // 2. Read the ordering file into vectors
    std::vector<int> inputMapping;
    std::vector<int> outputMapping;
    std::ifstream f(orderingFile);
    
    if (!f.is_open()) {
        std::cerr << "Error: Failed to open ordering file: " << orderingFile << std::endl;
        exit(1);
    }
    
    std::string line;
    if (std::getline(f, line)) {
        std::stringstream ss(line);
        int num;
        while (ss >> num) {
            inputMapping.push_back(num);
        }
    }
    
    if (std::getline(f, line)) {
        std::stringstream ss(line);
        int num;
        while (ss >> num) {
            outputMapping.push_back(num);
        }
    }
    
    std::vector<int> ordering(inputMapping.begin(), inputMapping.end());
    for (auto n : outputMapping) {
        ordering.push_back(n);
    }


    Aig_Man_t* newEDefMan = remapInputs(eDefMan, ordering);
    Aig_ManStop(eDefMan);
    eDefMan = newEDefMan;

    assert(outputMapping.size() == Aig_ManCoNum(eDefMan));

    Abc_Ntk_t* specNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
    defNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(eDefMan);

    Abc_NtkAppend(specNtk, defNtk, 1);

    Aig_Man_t* specMan = ABC_NAMESPACE::Abc_NtkToDar(specNtk, 0, 0);

    std::vector<int> varIds(outputMapping.begin(), outputMapping.end());
    std::vector<Aig_Obj_t*> funcIds;
    for(int i=1;i<Aig_ManCoNum(specMan);i++){
        funcIds.push_back(Aig_ManCo(specMan,i));
    }

    Aig_Obj_t* newDriver = Aig_SubstituteVec(specMan, Aig_ManCo(specMan, 0), varIds, funcIds);

    Aig_ObjCreateCo(specMan, newDriver);

    for (int i = 0; i < Aig_ManCoNum(specMan) - 1; i++) {
        Aig_ObjDisconnect(specMan, Aig_ManCo(specMan, i));
        Aig_ObjConnect(specMan, Aig_ManCo(specMan, i), Aig_ManConst0(specMan), NULL);
    }

    Aig_ManCoCleanup(specMan);
    Aig_ManCleanup(specMan);
    if (Aig_ManCoNum(specMan) == 0) {
        Aig_ObjCreateCo(specMan, Aig_ManConst0(specMan));
    }

    Aig_ManStop(this->manager);
    this->manager = specMan;

}
