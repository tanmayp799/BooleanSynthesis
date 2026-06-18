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

    Aig_ManStop(p);
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

/** Function
 * Composes input variable in initiAig with @param func, returns resulting Aig_Obj
 * @param pMan      [in out]    Aig Manager
 * @param initAig   [in]        Specifies the head of function tree
 * @param varId     [in]        (>=1) Specifies the variable to be substituted
 * @param func      [in]        Specifies the function that supplants the input
 */
Aig_Obj_t* Aig_Substitute(Aig_Man_t* pMan, Aig_Obj_t* initAig, int varId, Aig_Obj_t* func) {
	Aig_Obj_t* currFI = Aig_ObjIsCo(Aig_Regular(initAig))? initAig->pFanin0: initAig;
	func = Aig_ObjIsCo(Aig_Regular(func))? func->pFanin0: func;
	Aig_Obj_t* afterCompose = Aig_Compose(pMan, currFI, func, varId-1);
	assert(!Aig_ObjIsCo(Aig_Regular(afterCompose)));
	return afterCompose;
}









/*
Quantification using PHI(X,Y,X',Y')
*/
void quantify2(Aig_Man_t* pMan, std::vector<int>& varsToElim){
    assert(Aig_ManCiNum(pMan) == 2* numOrigInputs);


    std::vector<int> varsToSub;
    std::vector<Aig_Obj_t*> funcIds;

    for(auto e:varsToElim){
        varsToSub.push_back(e+1);
        varsToSub.push_back(numOrigInputs+e+1);
        
        funcIds.push_back(Aig_ManConst1(pMan));
        funcIds.push_back(Aig_ManConst1(pMan));
    }



    Aig_Obj_t* newDriver = Aig_SubstituteVec(pMan, Aig_ManCo(pMan, 0), varsToSub, funcIds);
    Aig_ObjCreateCo(pMan, newDriver);

    int numOuts=Aig_ManCoNum(pMan);
    for(int j=0;j<numOuts-1;j++){
        Aig_ObjDisconnect(pMan, Aig_ManCo(pMan, j));
        Aig_ObjConnect(pMan, Aig_ManCo(pMan, j), Aig_ManConst0(pMan), NULL);
    }

    Aig_ManCoCleanup(pMan);
    Aig_ManCleanup(pMan);

    if(Aig_ManCoNum(pMan) == 0){
        Aig_ObjCreateCo(pMan, Aig_ManConst0(pMan));
    }
    return;


}



/** Function
 * Compresses Aig by converting it to an Ntk and performing a bunch of steps on it.
 * Deletes SAig and returns a compressed version
 * @param SAig      [in]        Aig to be compressed
 * @param times     [in]        Number of compression cycles to be run
 */
Aig_Man_t* compressAigByNtkMultiple(Aig_Man_t* SAig, int times) {
	Aig_Man_t* temp;
	std::string command;
    Abc_Frame_t * pAbc = Abc_FrameGetGlobalFrame();

	// OUT("Cleaning up...");
	int removed = Aig_ManCleanup(SAig);
	std::cout << "Removed " << removed <<" nodes" << std::endl;

	// SAig =  Dar_ManCompress2(temp = SAig, 1, 1, 26, 1, 0);
	// Aig_ManStop(temp);

	Abc_Ntk_t * SNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(SAig);
	Abc_FrameSetCurrentNetwork(pAbc, SNtk);

	// TODO: FIX
	// assert(options.evalAigAtNode);
	command = "rewrite -lz; refactor -l;";

	// cout << "balancing..." << endl;
	if (Cmd_CommandExecute(pAbc, "balance;")) {
		std::cout << "Cannot preprocess SNtk" << std::endl;
		return NULL;
	}

	for (int i = 0; i < times; ++i)	{
		std::cout << "cycle " << i << ": " << command;
		// TIME_MEASURE_START

		
		if (Cmd_CommandExecute(pAbc, (char*)command.c_str())) {
			// cout << "Cannot preprocess SNtk, took " << TIME_MEASURE_ELAPSED << endl;
			return NULL;
		}
		// cout << "took " << TIME_MEASURE_ELAPSED << endl;
	}

	// cout << "balancing..." << endl;
	if (Cmd_CommandExecute(pAbc, "balance;")) {
		std::cout << "Cannot preprocess SNtk" << std::endl;
		return NULL;
	}

	SNtk = Abc_FrameReadNtk(pAbc);
	temp = ABC_NAMESPACE::Abc_NtkToDar(SNtk, 0, 0);
	Aig_ManStop(SAig);
	return temp;
}



AigWrapper::~AigWrapper(){
    Aig_ManStop(this->manager);
}

AigWrapper::AigWrapper(std::string verilogFile){
    // 1. Read Verilog into a Netlist
    Abc_Ntk_t* defNtk = Io_ReadVerilog(const_cast<char*>(verilogFile.c_str()), 0);
    if (!defNtk) {
        std::cerr << "Error: Failed to read Verilog file: " << verilogFile << std::endl;
        exit(1);
    }
    
    // 2. Convert to Logic Network -> Strash -> Dar Manager
    Abc_Ntk_t* logicNtk = Abc_NtkToLogic(defNtk);
    Abc_Ntk_t* strashNtk = Abc_NtkStrash(logicNtk, 0, 1, 0);
    this->manager = ABC_NAMESPACE::Abc_NtkToDar(strashNtk, 0, 0);
    
    Abc_NtkDelete(defNtk);
    Abc_NtkDelete(logicNtk);
    Abc_NtkDelete(strashNtk);
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

// AigWrapper::AigWrapper(KissatWrapper* kw){
//     this->manager = Aig_ManStart(0);

//     int numInputs = kw->getNumVars();

//     this->addInputs(numInputs);

//     Aig_Obj_t* outNode = Aig_ManConst1(this->manager);

//     for(auto clause: kw->getClauses()){
//         Aig_Obj_t* clauseNode = Aig_ManConst0(this->manager);
//         for(auto lit:clause){
//             if(lit>0){
//                 clauseNode = Aig_Or(this->manager, clauseNode, Aig_ManCi(this->manager, lit-1));
//             }
//             else{
//                 clauseNode = Aig_Or(this->manager, clauseNode, Aig_Not(Aig_ManCi(this->manager, -lit-1)));
//             }
//             // this->ShowAig();
//         }
//         outNode = Aig_And(this->manager, outNode, clauseNode);
//         // this->ShowAig();
//     }
//     Aig_ObjCreateCo(this->manager, outNode);
//     // this->ShowAig();

//     std::vector<int> evars = kw->getExistentialVarsToEliminate();
//     std::vector<int> uvars = kw->getUniversalVarsToEliminate();
//     std::vector<int> dvars = kw->getDepVarsToEliminate();

//     // if(!evars.empty()){
//     //     kw->callManthan();

//     //     std::string manthanDir = "/home/coolboy19/Desktop/BooleanSynthesis/dependencies/manthan";
//     //     std::string baseName = "temp_var_" + std::to_string(kw->getOutputVar());
//     //     std::string verilogFile = manthanDir + "/" + baseName + "_skolem.v";
        
//     //     Abc_Ntk_t* defNtk = Io_ReadVerilog(const_cast<char*>(verilogFile.c_str()), 0);
//     //     if (defNtk) {
//     //         Abc_Ntk_t* logicNtk = Abc_NtkToLogic(defNtk);
//     //         Abc_Ntk_t* strashNtk = Abc_NtkStrash(logicNtk, 0, 1, 0);
            
//     //         Abc_NtkDelete(defNtk);
//     //         Abc_NtkDelete(logicNtk);
//     //         defNtk = strashNtk;
//     //     } else {
//     //         std::cerr << "Error: Failed to read Verilog file from Manthan: " << verilogFile << std::endl;
//     //         exit(1);
//     //     }

//     //     Aig_Man_t* eDefMan = ABC_NAMESPACE::Abc_NtkToDar(defNtk, 0, 0);
//     //     Abc_NtkDelete(defNtk);

//     //     int numOrigInputs = Aig_ManCiNum(this->manager);
//     //     while(Aig_ManCiNum(eDefMan)<numOrigInputs){
//     //         Aig_ObjCreateCi(eDefMan);
//     //     }

//     //     std::vector<int> ordering;
//     //     for(auto e:kw->getInputOrdering()){
//     //         ordering.push_back(e);
//     //     }

//     //     for(auto e:kw->getOutputOrdering()){
//     //         ordering.push_back(e);
//     //     }

//     //     for(auto e:kw->getEliminatedVars()){
//     //         ordering.push_back(e);
//     //     }

//     //     Aig_Man_t* newEDefMan=remapInputs(eDefMan, ordering);
//     //     Aig_ManStop(eDefMan);
//     //     eDefMan = newEDefMan;
//     //     assert(Aig_ManCoNum(eDefMan)==kw->getOutputOrdering().size());
//     //     assert(Aig_ManCiNum(eDefMan) == Aig_ManCiNum(this->manager));

//     //     Abc_Ntk_t* specNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
//     //     defNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(eDefMan);

//     //     Abc_NtkAppend(specNtk, defNtk, 1);

//     //     Aig_Man_t* specMan = ABC_NAMESPACE::Abc_NtkToDar(specNtk, 0, 0);

//     //     std::set<int> outputOrdering = kw->getOutputOrdering();



//     //     std::vector<int> varIds;

//     //     for(auto e:outputOrdering){
//     //         varIds.push_back(e);
//     //     }

//     //     std::vector<Aig_Obj_t*> funcIds;

//     //     for(int i=1;i<Aig_ManCoNum(specMan);i++){
//     //         funcIds.push_back(Aig_ManCo(specMan,i));
//     //     }

//     //     Aig_Obj_t* newDriver = Aig_SubstituteVec(specMan,Aig_ManCo(specMan,0),varIds,funcIds);

//     //     Aig_ObjCreateCo(specMan, newDriver);

//     //     for(int i=0;i<Aig_ManCoNum(specMan)-1;i++){
//     //         Aig_ObjDisconnect(specMan, Aig_ManCo(specMan, i));
//     //         Aig_ObjConnect(specMan, Aig_ManCo(specMan, i), Aig_ManConst0(specMan), NULL);
//     //         // Aig_ManCoCleanup(specMan);
//     //     }

//     //     Aig_ManCoCleanup(specMan);
//     //     Aig_ManCleanup(specMan);
//     //     if(Aig_ManCoNum(specMan) == 0){
//     //         Aig_ObjCreateCo(specMan, Aig_ManConst0(specMan));
//     //     }

//     //     this->manager = specMan;
//     // }


//     Abc_Ntk_t* ntk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
//     Aig_ManStop(this->manager);


//     globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk size: {}\n", Abc_NtkNodeNum(ntk)));
    
//     for(int i=0;i<evars.size();i++){
//         Abc_Ntk_t* newNtk = Abc_NtkMiterQuantify(ntk, evars[i]-1, 1);
//         if(newNtk==NULL) exit(1);
//         Abc_NtkDelete(ntk);
//         ntk = newNtk;
//     }
    
//     globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk(-e) size: {}\n", Abc_NtkNodeNum(ntk)));


//     for(int i=0;i<dvars.size();i++){
//         Abc_Ntk_t* newNtk = Abc_NtkMiterQuantify(ntk, dvars[i]-1, 1);
//         if(newNtk==NULL) exit(1);
//         Abc_NtkDelete(ntk);
//         ntk = newNtk;
//     }

//     globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk(-d) size: {}\n", Abc_NtkNodeNum(ntk)));

//     for(int i=0;i<uvars.size();i++){
//         Abc_Ntk_t* newNtk = Abc_NtkMiterQuantify(ntk, uvars[i]-1, 0);
//         if(newNtk==NULL) exit(1);
//         Abc_NtkDelete(ntk);
//         ntk = newNtk;
//     }

//     globalLogger.log(LogLevel::DEBUG, fmt::format("Tanmay ntk(-a) size: {}\n", Abc_NtkNodeNum(ntk)));

//     this->manager = ABC_NAMESPACE::Abc_NtkToDar(ntk, 0, 0);
//     Abc_NtkDelete(ntk);
//     // this->ShowAig();


// }

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
    this->manager = compressAigByNtkMultiple(this->manager, 1);
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
    
    if (Aig_ManCoNum(this->manager) == 0)
    {
        Aig_ObjCreateCo(this->manager, Aig_ManConst0(this->manager));
    }
    

    Abc_NtkDelete(baseNtk);
    Abc_NtkDelete(srcNtk);
    // Aig_ManCoCleanup(this->manager);
    return;




}



void finalSub(AigWrapper* finalFormula, std::vector<AigWrapper*>& finalSkolems, std::set<int>& depVars){
    Abc_Ntk_t* finalNtk = finalFormula->getNtk();
    for(auto sk:finalSkolems){
        // printf("hi\n");
        Abc_Ntk_t* skNtk = sk->getNtk();
        // sk->ShowAig();
        Abc_NtkAppend(finalNtk, skNtk, 1);
        // Abc_NtkDelete(skNtk);
    }

    Aig_Man_t* finalMan = ABC_NAMESPACE::Abc_NtkToDar(finalNtk, 0, 0);
    Abc_NtkDelete(finalNtk);

    finalFormula->SetManager(finalMan);
    printf("After appending final skolems\n");
    // finalFormula->ShowAig();

    std::vector<int> varIds(depVars.begin(), depVars.end());
    std::vector<Aig_Obj_t*> funcIds;
    int numOuts=Aig_ManCoNum(finalMan);
    for(int i=1;i<numOuts;i++){
        funcIds.push_back(Aig_ManCo(finalMan,i));
    }

    Aig_Obj_t* newDriver = Aig_SubstituteVec(finalMan,Aig_ManCo(finalMan,0),varIds,funcIds);
    Aig_ObjCreateCo(finalMan, newDriver);
    numOuts=Aig_ManCoNum(finalMan);
    for(int i=0;i<numOuts-1;i++){
        Aig_ObjDisconnect(finalMan, Aig_ManCo(finalMan, i));
        Aig_ObjConnect(finalMan, Aig_ManCo(finalMan, i), Aig_ManConst0(finalMan), NULL);
        // Aig_ManCoCleanup(specMan);
    }

    Aig_ManCoCleanup(finalMan);
    Aig_ManCleanup(finalMan);
    if(Aig_ManCoNum(finalMan) == 0){
        Aig_ObjCreateCo(finalMan, Aig_ManConst0(finalMan));
    }
    finalMan=compressAig(finalMan);
    finalFormula->SetManager(finalMan);
    return;

}

void AigWrapper::substituteSkolem(AigWrapper* skolemAig, int target_d, std::string flag){
    std::string orderingFile = "./testFolder/order_target_d"+std::to_string(target_d)+ flag+".txt";

    std::vector<int> varsToEliminate;
    std::ifstream orderIn(orderingFile);
    if (orderIn.is_open()) {
        std::string line;
        while (std::getline(orderIn, line)) {
            if (line.length() > 2 && line.substr(0, 2) == "pi") {
                try {
                    varsToEliminate.push_back(std::stoi(line.substr(2))+1);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Could not parse integer from line: " << line << std::endl;
                }
            }
        }
        orderIn.close();
    } else {
        std::cerr << "Error: Unable to open ordering file: " << orderingFile << std::endl;
    }

    // skolemAig->ShowAig();

    std::set<int> varsToElim_set(varsToEliminate.begin(), varsToEliminate.end());

    int numInputs= skolemAig->getNumInputs();
    std::vector<int> remapIds;
    for(int i=1;i<=numInputs;i++){
        if(varsToElim_set.find(i)==varsToElim_set.end()){
            remapIds.push_back(i);
        }
    }
    for(auto e:varsToEliminate){
        remapIds.push_back(e);
    }
    globalLogger.log(LogLevel::INFO, fmt::format("Remapping Skolem AIG with ordering: {}", fmt::join(remapIds, " ")));
    Aig_Man_t* skolemMan = remapInputs(skolemAig->getManager(), remapIds);

    while(Aig_ManCoNum(skolemMan)<varsToEliminate.size()){
        Aig_ObjCreateCo(skolemMan, Aig_ManConst0(skolemMan));
        // Aig_ObjCreateCo(skolemMan, falseDriver);
    }

    skolemAig->SetManager(skolemMan);
    // printf("Remapped skolemAIG\n");
    // skolemAig->ShowAig();

    Abc_Ntk_t* specNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
    Abc_Ntk_t* skolemNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(skolemMan);

    Abc_NtkAppend(specNtk, skolemNtk, 1);

    Aig_Man_t* specMan = ABC_NAMESPACE::Abc_NtkToDar(specNtk, 0, 0);

    Abc_NtkDelete(specNtk);
    Abc_NtkDelete(skolemNtk);
    std::vector<Aig_Obj_t*> skolemNodeVec;

    int numAigOuts = Aig_ManCoNum(specMan);
    for(int i=1;i<numAigOuts;i++){
        skolemNodeVec.push_back(Aig_ManCo(specMan,i));
    }

    this->manager=specMan;
    // Aig_ManCleanMarkA(specMan);
    // Aig_ManCleanData(specMan);
    // printf("Before skolem function substitution\n");
    // this->ShowAig();
    Aig_Obj_t* newDriver=  Aig_SubstituteVec(specMan, Aig_ManCo(specMan, 0), varsToEliminate, skolemNodeVec);
    Aig_ObjCreateCo(specMan, newDriver);

    numAigOuts = Aig_ManCoNum(specMan);
    for(int i=0;i<numAigOuts-1;i++){
        Aig_ObjDisconnect(specMan, Aig_ManCo(specMan, i));
        Aig_ObjConnect(specMan, Aig_ManCo(specMan, i), Aig_ManConst0(specMan), NULL);
        // Aig_ManCoCleanup(specMan);
    }

    Aig_ManCoCleanup(specMan);
    Aig_ManCleanup(specMan);
    if(Aig_ManCoNum(specMan) == 0){
        Aig_ObjCreateCo(specMan, Aig_ManConst0(specMan));
    }

    this->manager = Aig_ManDupOrdered(specMan);
    Aig_ManStop(specMan);

    // printf("Printing localSpec after substitution\n");
    // this->ShowAig();

    




}





void getMonoAig(Aig_Man_t* pMan){
    assert(Aig_ManCiNum(pMan) == 2*numOrigInputs);

    std::vector<int> negVarsToSub;
    
    for(int i=0;i<numOrigInputs;i++){
        negVarsToSub.push_back(numOrigInputs+i+1);
    }
    // globalLogger.log(LogLevel::ERROR, fmt::format("varstosub: {}", fmt::join(negVarsToSubstitute, " ")));
    std::vector<Aig_Obj_t*> negFuncIds;
    for(int i=0;i<numOrigInputs;i++){
        // std::cout<<i+1<<std::endl;
        negFuncIds.push_back(Aig_Not(Aig_ManCi(pMan,i)));
    }
    Aig_Obj_t* newDriver2 = Aig_SubstituteVec(pMan, Aig_ManCo(pMan, 0), negVarsToSub, negFuncIds);
    Aig_ObjCreateCo(pMan, newDriver2);


    int numOuts2 = Aig_ManCoNum(pMan);
    for(int j=0; j<numOuts2-1; j++){
        Aig_ObjDisconnect(pMan, Aig_ManCo(pMan, j));
        Aig_ObjConnect(pMan, Aig_ManCo(pMan, j), Aig_ManConst0(pMan), NULL);
    }

    Aig_ManCoCleanup(pMan);
    Aig_ManCleanup(pMan);
    if(Aig_ManCoNum(pMan) == 0){
        Aig_ObjCreateCo(pMan, Aig_ManConst0(pMan));
    }


    // pMan = compressAig(pMan);

    return;

}




AigWrapper::AigWrapper(){
    this->manager=nullptr;
}

void AigWrapper::generateDef(int outputVar, int hVar){
    MEASURE_TIME(fmt::format("generateDef id: {}", outputVar),outputVar, LogLevel::ERROR);
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


    if(Aig_ObjFanin0(Aig_ManCo(AMan,0)) ==  Aig_ManConst0(AMan) 
            && Aig_ObjFaninC0(Aig_ManCo(AMan,0))){
                // printf("A_i is const 0 for id: %d\n",id);
                global_metrics.is_trivial_a[dep_to_id[outputVar]] = true;
            }
            else{
                global_metrics.is_trivial_a[dep_to_id[outputVar]] = false;
            }


            std::filesystem::path dump_path(global_metrics.benchmark_name);
            std::string pure_name = dump_path.stem().string();
            std::string a_path = "./experiment/basis_a/"+pure_name+".aig";
            std::string b_path = "./experiment/basis_b/"+pure_name+".aig";

            // Io_WriteAiger()




    // Aig_ManShow(AMan,0,NULL);
    // int xx;
    // std::cin>>xx;

    Abc_Ntk_t* ANtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(AMan);

    globalLogger.log(LogLevel::INFO, fmt::format("Generated A_i for id: {}", outputVar));


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

        if(Aig_ObjFanin0(Aig_ManCo(BMan,0)) ==  Aig_ManConst1(BMan) 
            && Aig_ObjFaninC0(Aig_ManCo(AMan,0))){
                // printf("A_i is const 0 for id: %d\n",id);
                global_metrics.is_trivial_a[dep_to_id[outputVar]] = true;
            }
            else{
                global_metrics.is_trivial_a[dep_to_id[outputVar]] = false;
            }

    // Aig_ManShow(BMan,0,NULL);
    // std::cin>>xx;

    Abc_Ntk_t* BNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(BMan);
    globalLogger.log(LogLevel::INFO, fmt::format("Generated B_i for id: {}", outputVar));


    varToBasisMap[outputVar] = std::make_pair(ANtk, BNtk);


    Io_WriteAiger(ANtk, (char*)a_path.c_str(),0,1,0);
    Io_WriteAiger(BNtk, (char*)b_path.c_str(),0,1,0);

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

    Abc_NtkDelete(ANtk2);
    Abc_NtkDelete(BNtk2);

    return;


}



// Helper function to build an AND-cube from an array of PI indices
DdNode * BuildVariableCube( DdManager * dd, std::vector<int> &pIndices) {
    DdNode * bCube = Cudd_ReadOne( dd );
    Cudd_Ref( bCube );
    
    for ( int i = 0; i < pIndices.size(); i++ ) {
        DdNode * bVar = Cudd_bddIthVar( dd, pIndices[i] );
        DdNode * bTemp = Cudd_bddAnd( dd, bCube, bVar );
        Cudd_Ref( bTemp );
        Cudd_RecursiveDeref( dd, bCube );
        bCube = bTemp;
    }
    return bCube;
}




#include "aig/aig/aig.h"
#include "bdd/cudd/cudd.h"
#include "misc/st/st.h"

// --------------------------------------------------------------------------
// Recursive DFS to map BDD nodes to Safe Dual-Rail AIG nodes
// --------------------------------------------------------------------------
static Aig_Obj_t * Bdd_To_SafeDualRail_Rec( DdManager * dd, DdNode * bdd, Aig_Man_t * pNew, 
                                            Aig_Obj_t ** pMapPos, Aig_Obj_t ** pMapNeg, st__table * visited ) 
{
    // 1. Base Cases
    if ( bdd == Cudd_ReadOne(dd) ) return Aig_ManConst1(pNew);
    if ( bdd == Cudd_ReadLogicZero(dd) ) return Aig_ManConst0(pNew);

    // 2. Handle CUDD complemented edges
    int isComp = Cudd_IsComplement(bdd);
    DdNode * bddReg = Cudd_Regular(bdd);

    // 3. Memoization Check: Have we visited this node already?
    Aig_Obj_t * pRes;
    if ( st__lookup(visited, (char*)bddReg, (char**)&pRes) ) {
        return isComp ? Aig_Not(pRes) : pRes;
    }

    // 4. Recurse down the High (Then) and Low (Else) branches
    DdNode * childT = Cudd_T(bddReg);
    DdNode * childE = Cudd_E(bddReg);

    Aig_Obj_t * c_high = Bdd_To_SafeDualRail_Rec(dd, childT, pNew, pMapPos, pMapNeg, visited);
    Aig_Obj_t * c_low  = Bdd_To_SafeDualRail_Rec(dd, childE, pNew, pMapPos, pMapNeg, visited);

    // 5. Look up the positive and negative AIG inputs for this BDD variable index
    int varIdx = bddReg->index; 
    Aig_Obj_t * x_pos = pMapPos[varIdx];
    Aig_Obj_t * x_neg = pMapNeg[varIdx];

    // 6. Build the safe MUX: (x_pos AND c_high) OR (x_neg AND c_low)
    Aig_Obj_t * and_high = Aig_And( pNew, x_pos, c_high );
    Aig_Obj_t * and_low  = Aig_And( pNew, x_neg, c_low );
    pRes = Aig_Or( pNew, and_high, and_low );

    // 7. Cache the result for future DAG reconvergence
    st__insert( visited, (char*)bddReg, (char*)pRes );

    // 8. Return applying the complement if necessary
    return isComp ? Aig_Not(pRes) : pRes;
}

// --------------------------------------------------------------------------
// Top-Level API: Converts the BDD root into a Safe Dual-Rail AIG
// --------------------------------------------------------------------------
Aig_Man_t * Aig_ManCreateSafeDualRail( DdManager * dd, DdNode * bFunc ) 
{
    // Cudd_ReadSize gives the total number of variables currently in the manager
    int nVars = Cudd_ReadSize(dd);

    // Initialize the new AIG manager (estimate 3 nodes per BDD node)
    Aig_Man_t * pNew = Aig_ManStart( Cudd_DagSize(bFunc) * 3 );
    // pNew->pName = Abc_UtilStrsav( "safe_dual_rail_ckt" );

    // Allocate arrays to map CUDD var index -> AIG Primary Inputs
    Aig_Obj_t ** pMapPos = ABC_ALLOC( Aig_Obj_t *, nVars );
    Aig_Obj_t ** pMapNeg = ABC_ALLOC( Aig_Obj_t *, nVars );
    
    // Create the Dual-Rail inputs (interleaved: var0_pos, var0_neg, var1_pos...)
    for ( int i = 0; i < nVars; i++ ) {
        pMapPos[i] = Aig_ObjCreateCi( pNew );
        pMapNeg[i] = Aig_ObjCreateCi( pNew );
    }

    // Initialize ABC's hash table for memoization
    st__table * visited = st__init_table( st__ptrcmp, st__ptrhash );

    // Build the internal logic
    Aig_Obj_t * pRoot = Bdd_To_SafeDualRail_Rec( dd, bFunc, pNew, pMapPos, pMapNeg, visited );

    // Cap it off with a Primary Output
    Aig_ObjCreateCo( pNew, pRoot );

    // Clean up memory
    st__free_table( visited );
    ABC_FREE( pMapPos );
    ABC_FREE( pMapNeg );
    Aig_ManCleanup( pNew );

    return pNew;
}


std::vector<int> Get_Bdd_Order( DdManager * dd ) {
    int nVars = Cudd_ReadSize(dd);
    int level, index;

    std::vector<int> retVal;

    // printf("\nRecommended Skolem Extraction Order (Bottom to Top):\n");
    for ( level = nVars - 1; level >= 0; level-- ) {
        index = Cudd_ReadInvPerm(dd, level);
        // printf("Extracting Index %d first...\n", index);
        // Add index to your extraction queue/list here
        retVal.push_back(index);
    }

    return retVal;
}

// 1. The Recursive Helper: Builds the logic strictly by dependency
Aig_Obj_t* GetSingleSkolem_rec(Aig_Man_t* pNew, Aig_Obj_t* pObj) {
    // ABC safety check: Ensure we are working with raw pointers, not complemented ones
    assert(!Aig_IsComplement(pObj));
    
    // If this node was already mapped (or if it's a pre-mapped CI/Const), return its new counterpart
    if (pObj->pData) {
        return (Aig_Obj_t*)pObj->pData;
    }

    // If we reach here and it's not mapped, it MUST be an internal AND node.
    assert(Aig_ObjIsNode(pObj));

    // Recursively dig down and get the mapped fanins
    Aig_Obj_t* pFan0 = GetSingleSkolem_rec(pNew, Aig_ObjFanin0(pObj));
    Aig_Obj_t* pFan1 = GetSingleSkolem_rec(pNew, Aig_ObjFanin1(pObj));

    // Apply the original complement bits to the newly mapped fanins
    pFan0 = Aig_NotCond(pFan0, Aig_ObjFaninC0(pObj));
    pFan1 = Aig_NotCond(pFan1, Aig_ObjFaninC1(pObj));

    // Create the new AND gate in the single-rail manager
    Aig_Obj_t* pNewNode = Aig_And(pNew, pFan0, pFan1);
    
    // Cache it so we don't process it again
    pObj->pData = pNewNode;
    
    return pNewNode;
}


// 2. The Updated Folding Function
Aig_Man_t* GetSingleSkolemAig(Aig_Man_t* pDual) {
    // Scrub all leftover garbage from Aig_SubstituteVec
    Aig_ManCleanData(pDual); 

    int nTotalVars = Aig_ManCiNum(pDual) / 2;
    Aig_Man_t* pNew = Aig_ManStart(Aig_ManObjNumMax(pDual));
    Aig_Obj_t *pNewPi;
    int i;

    // A. Map Constants
    Aig_ManConst0(pDual)->pData = Aig_ManConst0(pNew);

    // B. Create the single-rail Primary Inputs
    for (i = 0; i < nTotalVars; i++) {
        Aig_ObjCreateCi(pNew);
    }

    // C. Map the dual-rail PIs to the single-rail PIs
    for (i = 0; i < nTotalVars; i++) {
        Aig_Obj_t* pOldPos = Aig_ManCi(pDual, 2 * i);
        Aig_Obj_t* pOldNeg = Aig_ManCi(pDual, 2 * i + 1);
        pNewPi = Aig_ManCi(pNew, i);

        pOldPos->pData = pNewPi;
        pOldNeg->pData = Aig_Not(pNewPi);
    }

    // D. RECURSIVE MAPPING: Start at the PO and pull the logic backward
    Aig_Obj_t* pOut = Aig_ManCo(pDual, 0);
    
    // Get the mapped fanin
    Aig_Obj_t* pNewOutFanin = GetSingleSkolem_rec(pNew, Aig_ObjFanin0(pOut));
    
    // Apply the PO's complement bit
    pNewOutFanin = Aig_NotCond(pNewOutFanin, Aig_ObjFaninC0(pOut));
    
    // E. Connect the final output
    Aig_ObjCreateCo(pNew, pNewOutFanin);

    // Clean up any unreachable nodes
    Aig_ManCleanup(pNew); 

    return pNew;
}


void generateTseitinSkolem(Aig_Man_t* ckt, std::vector<int> &ordering, 
    std::vector<std::pair<int, AigWrapper*>> &tseitinSkolems){
        ckt = compressAig(ckt);
        // Aig_ManCo(ckt,0)->pFanin0 = Aig_Not(Aig_ManCo(ckt,0)->pFanin0);

    // Aig_ManShow(ckt,0,NULL);
    int xxx;
    // std::cin>>xxx;
    int sz = ordering.size();
    for(int i=0;i<sz;i++){
        Aig_Man_t* currSkolem  = Aig_ManDupOrdered(ckt);
        int idx = ordering[i];
        globalLogger.log(LogLevel::ERROR, fmt::format("Generating skolem for id: {}", idx));
        std::vector<int> varIds;
        std::vector<Aig_Obj_t*> funcIds;

        varIds.push_back(2*idx+1);
        varIds.push_back(2*idx+2);

        funcIds.push_back(Aig_ManConst1(currSkolem));
        funcIds.push_back(Aig_ManConst0(currSkolem));


        for(int j = i+1; j<sz;j++){
            int idx2 = ordering[j];
            varIds.push_back(2*idx2+1);
            varIds.push_back(2*idx2+2);

            funcIds.push_back(Aig_ManConst1(currSkolem));
            funcIds.push_back(Aig_ManConst1(currSkolem));

        }

        // Aig_ManShow(currSkolem,0,NULL);
        // std::cin>>xxx;


        Aig_Obj_t* newDriver = Aig_SubstituteVec(currSkolem, Aig_ManCo(currSkolem, 0), varIds, funcIds);
        Aig_ObjCreateCo(currSkolem, newDriver);

        int numOuts=Aig_ManCoNum(currSkolem);
        for(int j=0;j<numOuts-1;j++){
            Aig_ObjDisconnect(currSkolem, Aig_ManCo(currSkolem, j));
            Aig_ObjConnect(currSkolem, Aig_ManCo(currSkolem, j), Aig_ManConst0(currSkolem), NULL);
            // Aig_ManCoCleanup(specMan);
        }

        Aig_ManCoCleanup(currSkolem);
        Aig_ManCleanup(currSkolem);
        if(Aig_ManCoNum(currSkolem) == 0){
            Aig_ObjCreateCo(currSkolem, Aig_ManConst0(currSkolem));
        }
        currSkolem = compressAig(currSkolem);
        // Aig_ManShow(currSkolem,0,NULL);
        // std::cin>>xxx;

        Aig_Man_t* singleSkolem = GetSingleSkolemAig(currSkolem);
        Aig_ManStop(currSkolem);


        // currSkolem=compressAig(currSkolem);
        AigWrapper* skolemAig = new AigWrapper();
        skolemAig->SetManager(singleSkolem);
        tseitinSkolems.push_back(std::make_pair(idx, skolemAig));
        skolemAig->compress();
        globalLogger.log(LogLevel::ERROR, fmt::format("Skolem function for var: {}", idx));
        // skolemAig->ShowAig();

    }
    exit(1);
    return;

}



// AigWrapper* AigWrapper::quantify(std::vector<int>& varsToEliminate, int fExist,
//      std::vector<std::pair<int, AigWrapper*>> &tseitinSkolems){
//     MEASURE_TIME("quantify",0, LogLevel::ERROR);
//     MEASURE_TIME("quantify",0,LogLevel::STATS);
//     Abc_Ntk_t * pNtk, * pLogicNtk, * pStrashNtk;
//     Aig_Man_t * pNewAig;
//     DdManager * dd;
//     DdNode * bFunc, * bCube, * bFinalRes;
//     Abc_Obj_t * pPo, * pPi;
//     Vec_Ptr_t * vPiNames;
//     int i;



//     // ------------------------------------------------------------------
//     // PHASE 1: AIG -> BDD Conversion & Sanitization
//     // ------------------------------------------------------------------
    
//     // 1. Wrap the AIG manager in a standard ABC Network
//     pNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase( this->manager );
//     if ( pNtk == NULL ){
//         globalLogger.log(LogLevel::ERROR, "getLocalSpec: Abc_NtkFromAigPhase failed.");
//         exit(1);
//     }

//     Abc_NtkShortNames(pNtk);


//     // 2. Build Global BDDs and capture the CUDD Manager
//     dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk,10000000,1,1,0,1);
//     if(dd==NULL){
//         globalLogger.log(LogLevel::ERROR, "getLocalSpec: Abc_NtkBuildGlobalBdds failed.");
//         exit(1);
//     }

//     // 3. Extract the original logic function (Assuming 1 Primary Output)
//     pPo = Abc_NtkPo( pNtk, 0 );
//     bFunc = (DdNode *)Abc_ObjGlobalBdd( pPo );


    
//     // ------------------------------------------------------------------
//     // PHASE 2: Formal Logic Quantifications
//     // ------------------------------------------------------------------
    
//     // 1. Existential Quantification
//     if ( !varsToEliminate.empty() ) {
//         bCube = BuildVariableCube( dd, varsToEliminate);
//         if(fExist){
//             bFinalRes  = Cudd_bddExistAbstract( dd, bFunc, bCube );
//         }
//         else{
//             bFinalRes  = Cudd_bddUnivAbstract( dd, bFunc, bCube );
//         }
//         Cudd_Ref( bFinalRes );
//         Cudd_RecursiveDeref( dd, bCube );
//     } else {
//         bFinalRes = bFunc; 
//         Cudd_Ref( bFinalRes );
//     }


//     // ------------------------------------------------------------------
//     // PHASE 3: Port Alignment & Network Derivation
//     // ------------------------------------------------------------------

//     // 1. Extract the sanitized PI names (100% segfault safe now)
//     vPiNames = Vec_PtrAlloc( Abc_NtkPiNum(pNtk) );
//     Abc_NtkForEachPi( pNtk, pPi, i ) {
//         Vec_PtrPush( vPiNames, (void *)Abc_ObjName(pPi) );
//     }

//     // 2. Derive the new Logic Network using the API
//     // (This creates a NEW CUDD manager inside pLogicNtk and safely transfers bFinalRes)
//     pLogicNtk = Abc_NtkDeriveFromBdd( dd, bFinalRes, Abc_ObjName(pPo), vPiNames );
//     Vec_PtrFree( vPiNames );



//     // ------------------------------------------------------------------
//     // PHASE 4: Strashing & Memory Hijack
//     // ------------------------------------------------------------------

//     // 1. Convert the Logic Network back to an AIG Network
//     pStrashNtk = Abc_NtkStrash( pLogicNtk, 0, 1, 0 );

//     pNewAig = ABC_NAMESPACE::Abc_NtkToDar( pStrashNtk, 0, 0 );

//     // ------------------------------------------------------------------
//     // PHASE 5: Strict Garbage Collection
//     // ------------------------------------------------------------------

//     // Dereference the final BDD in the OLD manager
//     Cudd_RecursiveDeref( dd, bFinalRes ); 

//     // Free the original network (This safely shuts down the OLD 'dd' manager)
//     Abc_NtkFreeGlobalBdds( pNtk, 1 ); 
//     Abc_NtkDelete( pNtk );

//     // Delete intermediate networks
//     Abc_NtkDelete( pLogicNtk );
//     Abc_NtkDelete( pStrashNtk ); // Safe because we set pManFunc = NULL

//     AigWrapper* retAig = new AigWrapper(this);
//     retAig->SetManager(pNewAig);

//     retAig->compress();
//     return retAig;

// }


AigWrapper* AigWrapper::getLocalSpec(int target_d, std::vector<int>& existentialVarsToEliminate, std::vector<int>& universalVarsToEliminate){
    MEASURE_TIME("GetLocalSpec",target_d, LogLevel::ERROR);
    MEASURE_TIME("GetLocalSpec",target_d, LogLevel::STATS);
    Abc_Ntk_t * pNtk, * pLogicNtk, * pStrashNtk;
    Aig_Man_t * pNewAig;
    DdManager * dd;
    DdNode * bFunc, * bExistCube, * bExistRes, * bUnivCube, * bFinalRes;
    Abc_Obj_t * pPo, * pPi;
    Vec_Ptr_t * vPiNames;
    int i;

    int xx;
    Aig_Man_t* pMan;

    pMan = Aig_ManDupOrdered(this->manager);


    global_metrics.last_checkpoint = "EXIS_QUANT_"+std::to_string(target_d)+"_START";
    // Use pMan to existentially quantify using quantify2()

    auto exis_quant_start_time = std::chrono::high_resolution_clock::now();

    quantify2(pMan,existentialVarsToEliminate);

    auto exis_quant_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> exis_quant_elapsed = exis_quant_end_time - exis_quant_start_time;

    global_metrics.individual_exis_quant_times[dep_to_id[target_d]] = exis_quant_elapsed.count();

    // auto exis_quant_duration = std::chrono::duration_cast<std::chrono::milliseconds>(exis_quant_time_end - exis_quant_time_start);
    // globalLogger.log(LogLevel::INFO, fmt::format("Completed Exis Quant for id: {} in {} ms", target_d, exis_quant_duration.count()));
    

    global_metrics.last_checkpoint = "EXIS_QUANT_"+std::to_string(target_d)+"_END";

    // Aig_ManShow(pMan,0,NULL);
    // std::cin>>xx;


    getMonoAig(pMan);
    // pMan->pName = NULL;
    
    // Aig_ManShow(pMan,0,NULL);
    // std::cin>>xx;

    // ------------------------------------------------------------------
    // PHASE 1: AIG -> BDD Conversion & Sanitization
    // ------------------------------------------------------------------
    
    // 1. Wrap the AIG manager in a standard ABC Network
    pNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase( pMan );
    if ( pNtk == NULL ){
        globalLogger.log(LogLevel::ERROR, "getLocalSpec: Abc_NtkFromAigPhase failed.");
        exit(1);
    }

    Abc_NtkShortNames(pNtk);


    // 2. Build Global BDDs and capture the CUDD Manager

    auto genbdd_start_time = std::chrono::high_resolution_clock::now();


    dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk,10000000,1,1,0,1);

    auto genbdd_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> genbdd_elapsed = genbdd_end_time - genbdd_start_time;

    global_metrics.individual_bdd_gen_times[dep_to_id[target_d]] = genbdd_elapsed.count();

    if(dd==NULL){
        globalLogger.log(LogLevel::ERROR, "getLocalSpec: Abc_NtkBuildGlobalBdds failed.");


        global_metrics.execution_status="BDD_FAILED_"+std::to_string(target_d);
        exit(1);
    }

    // 3. Extract the original logic function (Assuming 1 Primary Output)
    pPo = Abc_NtkPo( pNtk, 0 );
    bFunc = (DdNode *)Abc_ObjGlobalBdd( pPo );

    globalLogger.log(LogLevel::INFO, fmt::format("Generated BDD for id: {}", target_d));

    // ------------------------------------------------------------------
    // PHASE 2: Formal Logic Quantifications
    // ------------------------------------------------------------------
    
    // // 1. Existential Quantification
    // if ( !existentialVarsToEliminate.empty() ) {
    //     bExistCube = BuildVariableCube( dd, existentialVarsToEliminate);
    //     bExistRes  = Cudd_bddExistAbstract( dd, bFunc, bExistCube );
    //     Cudd_Ref( bExistRes );
    //     Cudd_RecursiveDeref( dd, bExistCube );
    // } else {
    //     bExistRes = bFunc; 
    //     Cudd_Ref( bExistRes );
    // }   

    // globalLogger.log(LogLevel::INFO, fmt::format("Completed Exis Quant for id: {}", target_d));
    global_metrics.last_checkpoint = "UNIV_QUANT_"+std::to_string(target_d)+"_START";
    bExistRes = bFunc;
    Cudd_Ref( bExistRes );

    auto univ_quant_start_time = std::chrono::high_resolution_clock::now();


    // 2. Universal Quantification
    if ( !universalVarsToEliminate.empty() ) {
        bUnivCube = BuildVariableCube( dd, universalVarsToEliminate);
        bFinalRes = Cudd_bddUnivAbstract( dd, bExistRes, bUnivCube );
        Cudd_Ref( bFinalRes );
        Cudd_RecursiveDeref( dd, bUnivCube );
    } else {
        bFinalRes = bExistRes;
        Cudd_Ref( bFinalRes );
    }

    // Dereference intermediate existential result
    Cudd_RecursiveDeref( dd, bExistRes );

    long long bdd_size_metric = Cudd_DagSize(bFinalRes);
    global_metrics.individual_bdd_sizes[dep_to_id[target_d]] = bdd_size_metric;

    


    auto univ_quant_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> univ_quant_elapsed = univ_quant_end_time - univ_quant_start_time;

    global_metrics.individual_univ_quant_times[dep_to_id[target_d]] = univ_quant_elapsed.count();


    globalLogger.log(LogLevel::INFO, fmt::format("Completed Univ Quant for id: {}", target_d));
    global_metrics.last_checkpoint = "UNIV_QUANT_"+std::to_string(target_d)+"_END";

    // ------------------------------------------------------------------
    // PHASE 3: Port Alignment & Network Derivation
    // ------------------------------------------------------------------

    // 1. Extract the sanitized PI names (100% segfault safe now)
    vPiNames = Vec_PtrAlloc( Abc_NtkPiNum(pNtk) );
    Abc_NtkForEachPi( pNtk, pPi, i ) {
        Vec_PtrPush( vPiNames, (void *)Abc_ObjName(pPi) );
    }

    // 2. Derive the new Logic Network using the API
    // (This creates a NEW CUDD manager inside pLogicNtk and safely transfers bFinalRes)
    pLogicNtk = Abc_NtkDeriveFromBdd( dd, bFinalRes, Abc_ObjName(pPo), vPiNames );
    Vec_PtrFree( vPiNames );

    // ------------------------------------------------------------------
    // PHASE 4: Strashing & Memory Hijack
    // ------------------------------------------------------------------

    // 1. Convert the Logic Network back to an AIG Network
    pStrashNtk = Abc_NtkStrash( pLogicNtk, 0, 1, 0 );

    pNewAig = ABC_NAMESPACE::Abc_NtkToDar( pStrashNtk, 0, 0 );


    globalLogger.log(LogLevel::INFO, fmt::format("Generated AIG Local Spec for id: {}", target_d));
    // ------------------------------------------------------------------
    // PHASE 5: Strict Garbage Collection
    // ------------------------------------------------------------------

    // Dereference the final BDD in the OLD manager
    Cudd_RecursiveDeref( dd, bFinalRes ); 

    // Free the original network (This safely shuts down the OLD 'dd' manager)
    Abc_NtkFreeGlobalBdds( pNtk, 1 ); 
    Abc_NtkDelete( pNtk );
    Aig_ManStop(pMan);


    // Delete intermediate networks
    Abc_NtkDelete( pLogicNtk );
    Abc_NtkDelete( pStrashNtk ); // Safe because we set pManFunc = NULL

    AigWrapper* retAig = new AigWrapper(this);
    retAig->SetManager(pNewAig);

    
    retAig->compress();
    long long aig_size_metric = Aig_ManObjNum(pNewAig);
    global_metrics.individual_aig_sizes[dep_to_id[target_d]] = aig_size_metric;
    // retAig->ShowAig();
    return retAig;
    
}

// void AigWrapper::substituteInputs(std::set<int> inputsToReplace, char* skolemFile, char* orderingFile){

//     // 1. Read the Skolem function (Verilog file) into an AIG Manager
//     Abc_Ntk_t* defNtk = Io_ReadVerilog(skolemFile, 0);
//     if (!defNtk) {
//         std::cerr << "Error: Failed to read Verilog file: " << skolemFile << std::endl;
//         exit(1);
//     }
    
//     Abc_Ntk_t* logicNtk = Abc_NtkToLogic(defNtk);
//     Abc_Ntk_t* strashNtk = Abc_NtkStrash(logicNtk, 0, 1, 0);
//     Aig_Man_t* eDefMan = ABC_NAMESPACE::Abc_NtkToDar(strashNtk, 0, 0);
    
//     Abc_NtkDelete(defNtk);
//     Abc_NtkDelete(logicNtk);
//     Abc_NtkDelete(strashNtk);

//     // 2. Read the ordering file into vectors
//     std::vector<int> inputMapping;
//     std::vector<int> outputMapping;
//     std::ifstream f(orderingFile);
    
//     if (!f.is_open()) {
//         std::cerr << "Error: Failed to open ordering file: " << orderingFile << std::endl;
//         exit(1);
//     }
    
//     std::string line;
//     if (std::getline(f, line)) {
//         std::stringstream ss(line);
//         int num;
//         while (ss >> num) {
//             inputMapping.push_back(num);
//         }
//     }
    
//     if (std::getline(f, line)) {
//         std::stringstream ss(line);
//         int num;
//         while (ss >> num) {
//             outputMapping.push_back(num);
//         }
//     }
    
//     std::vector<int> ordering(inputMapping.begin(), inputMapping.end());
//     for (auto n : outputMapping) {
//         ordering.push_back(n);
//     }


//     Aig_Man_t* newEDefMan = remapInputs(eDefMan, ordering);
//     Aig_ManStop(eDefMan);
//     eDefMan = newEDefMan;

//     assert(outputMapping.size() == Aig_ManCoNum(eDefMan));

//     Abc_Ntk_t* specNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
//     defNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(eDefMan);

//     Abc_NtkAppend(specNtk, defNtk, 1);

//     Aig_Man_t* specMan = ABC_NAMESPACE::Abc_NtkToDar(specNtk, 0, 0);

//     std::vector<int> varIds(outputMapping.begin(), outputMapping.end());
//     std::vector<Aig_Obj_t*> funcIds;
//     for(int i=1;i<Aig_ManCoNum(specMan);i++){
//         funcIds.push_back(Aig_ManCo(specMan,i));
//     }

//     Aig_Obj_t* newDriver = Aig_SubstituteVec(specMan, Aig_ManCo(specMan, 0), varIds, funcIds);

//     Aig_ObjCreateCo(specMan, newDriver);

//     for (int i = 0; i < Aig_ManCoNum(specMan) - 1; i++) {
//         Aig_ObjDisconnect(specMan, Aig_ManCo(specMan, i));
//         Aig_ObjConnect(specMan, Aig_ManCo(specMan, i), Aig_ManConst0(specMan), NULL);
//     }

//     Aig_ManCoCleanup(specMan);
//     Aig_ManCleanup(specMan);
//     if (Aig_ManCoNum(specMan) == 0) {
//         Aig_ObjCreateCo(specMan, Aig_ManConst0(specMan));
//     }

//     Aig_ManStop(this->manager);
//     this->manager = specMan;

// }

void AigWrapper::substituteConst(int inputVarId, int constVal) {
    int numOuts = Aig_ManCoNum(this->manager);
    std::vector<Aig_Obj_t*> newOuts;

    for (int i = 0; i < numOuts; ++i) {
        Aig_Obj_t* newOut = Aig_SubstituteConst(this->manager, Aig_ManCo(this->manager, i), inputVarId, constVal);
        newOuts.push_back(newOut);
    }

    for (int i = 0; i < numOuts; ++i) {
        Aig_ObjCreateCo(this->manager, newOuts[i]);
    }

    for (int i = 0; i < numOuts; ++i) {
        Aig_ObjDisconnect(this->manager, Aig_ManCo(this->manager, i));
        Aig_ObjConnect(this->manager, Aig_ManCo(this->manager, i), Aig_ManConst0(this->manager), NULL);
    }

    Aig_ManCoCleanup(this->manager);
    Aig_ManCleanup(this->manager);

    if (Aig_ManCoNum(this->manager) == 0) {
        Aig_ObjCreateCo(this->manager, Aig_ManConst0(this->manager));
    }
}
