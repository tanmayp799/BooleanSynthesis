#include "AigWrapper.h"

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


int AigWrapper::getNumInputs(){
    return Aig_ManCiNum(this->manager);
}

Abc_Ntk_t* AigWrapper::getNtk(){
    return ABC_NAMESPACE::Abc_NtkFromAigPhase(this->manager);
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
    
    Aig_Obj_t* outA = Aig_ManCo(defMan,0);
    Aig_Obj_t* outB = Aig_ManCo(defMan,1);

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
