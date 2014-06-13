
// @(#)root/tmva $Id: TMVAClassification_A.C,v 1.1 2013/03/08 10:50:47 slaunwhj Exp $
/**********************************************************************************
 * Project   : TMVA - a Root-integrated toolkit for multivariate data analysis    *
 * Package   : TMVA                                                               *
 * Root Macro: TMVAClassification                                                 *
 *                                                                                *
 * This macro provides examples for the training and testing of the               *
 * TMVA classifiers.                                                              *
 *                                                                                *
 * As input data is used a toy-MC sample consisting of four Gaussian-distributed  *
 * and linearly correlated input variables.                                       *
 *                                                                                *
 * The methods to be used can be switched on and off by means of booleans, or     *
 * via the prompt command, for example:                                           *
 *                                                                                *
 *    root -l TMVAClassification.C\(\"Fisher,Likelihood\"\)                       *
 *                                                                                *
 * (note that the backslashes are mandatory)                                      *
 * If no method given, a default set is used.                                     *
 *                                                                                *
 * The output file "TMVA.root" can be analysed with the use of dedicated          *
 * macros (simply say: root -l <macro.C>), which can be conveniently              *
 * invoked through a GUI that will appear at the end of the run of this macro.    *
 **********************************************************************************/
 
 
// modified 2/22/12 (Geoffrey Smith) to split training into 4 ANNs. Output weights are found in weights1, weights2, etc.
// These weights are then passed to a secondary ANN training phase (TMVAClassification_B).



#include <cstdlib>
#include <iostream> 
#include <map>
#include <string>

#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TPluginManager.h"

//#include "TMVAGui.C"

#if not defined(__CINT__) || defined(__MAKECINT__)
// needs to be included when makecint runs (ACLIC)
#include "TMVA/Factory.h"
#include "TMVA/Tools.h"
#include "TMVA/Config.h"

#endif

// read input data file with ascii format (otherwise ROOT) ?
Bool_t ReadDataFromAsciiIFormat = kFALSE;
   
//gSystem->Load("${CMSSW_RELEASE_BASE}/external/${SCRAM_ARCH}/lib/libTMVA.so");
   



void TMVAClassification_A( TString myMethodList = "",  TString jet_tag_label = "jet_tag_category_label",  TCut numJet_cut = "numJets>=4", TCut numTag_cut = "numTags>=2", bool createMVA = true, TString charge = "OS", double ttbb_fraction = -1 )
{
   // root -l TMVAClassification.C\(\"myMethod1,myMethod2,myMethod3\"\)

   // this loads the library
   TMVA::Tools::Instance();

   //---------------------------------------------------------------
   // default MVA methods to be trained + tested
   std::map<std::string,int> Use;

  
   // ---
   Use["MLP"]             = 1; // this is the recommended ANN
   //Use["MLPBFGS"]         = 1; // recommended ANN with optional training method
   Use["CFMlpANN"]        = 1; 
   Use["TMlpANN"]         = 1; 
   Use["BDTG"]         = 1; 
   Use["BDT"]         = 1; 
   // ---------------------------------------------------------------

   std::cout << std::endl;
   std::cout << "==> Start TMVAClassification" << std::endl;

   if (myMethodList != "") {
      for (std::map<std::string,int>::iterator it = Use.begin(); it != Use.end(); it++) it->second = 0;

      std::vector<TString> mlist = TMVA::gTools().SplitString( myMethodList, ',' );
      for (UInt_t i=0; i<mlist.size(); i++) {
         std::string regMethod(mlist[i]);

         if (Use.find(regMethod) == Use.end()) {
            std::cout << "Method \"" << regMethod << "\" not known in TMVA under this name. Choose among the following:" << std::endl;
            for (std::map<std::string,int>::iterator it = Use.begin(); it != Use.end(); it++) std::cout << it->first << " ";
            std::cout << std::endl;
            return;
         }
         Use[regMethod] = 1;
      }
   }


//// --------inputs--------------
      // load the signal and background event samples from ROOT trees
      TFile *input1(0);
      TFile *input2(0);
      TFile *input3(0);
      TFile *input4(0);

      TString background_name = "ttbar_lf";
      TString fname1 = "inputTrees/ttH125.root" ;
      TString fname2 = "inputTrees/"+background_name+".root" ;
      TString fname3 = "inputTrees/ttbar_cc.root" ;
      TString fname4 = "inputTrees/ttbar_bb.root" ; 
      
      if (!gSystem->AccessPathName( fname1 )) {
        input1 = TFile::Open( fname1 ); // check if file in local directory exists
      }
      if (!gSystem->AccessPathName( fname2 )) {
        input2 = TFile::Open( fname2 ); // check if file in local directory exists
      } 
      if (!gSystem->AccessPathName( fname3 )) {
	 input3 = TFile::Open( fname3 ); // check if file in local directory exists
      } 
      if (!gSystem->AccessPathName( fname4 )) {
	 input4 = TFile::Open( fname4 ); // check if file in local directory exists
      } 

      if ((!input1) || (!input2) || (!input3) || (!input4)) {
         std::cout << "ERROR: could not open data file" << std::endl; 
         exit(1);
      }
      std::cout << "--- TMVAClassification       : Using tth input file: " << input1->GetName() << std::endl;
      std::cout << "--- TMVAClassification       : Using ttbar_lf input file: " << input2->GetName() << std::endl;
      std::cout << "--- TMVAClassification       : Using ttbar_cc input file: " << input3->GetName() << std::endl;
      std::cout << "--- TMVAClassification       : Using ttbar_bb input file: " << input4->GetName() << std::endl;


      TTree *signal     = (TTree*)input1->Get("summaryTree");
      TTree *backgroundA = (TTree*)input2->Get("summaryTree");
      TTree *backgroundB = (TTree*)input3->Get("summaryTree");
      TTree *backgroundC = (TTree*)input4->Get("summaryTree");
 
      // global event weights per tree (see below for setting event-wise weights)
      Double_t signalWeight     = 1.0;
      Double_t backgroundWeightA = 1.0;
      Double_t backgroundWeightB = 1.0;
      Double_t backgroundWeightC = 1.0;


      // cut selection for input events
      TCut trigger = "( (TwoMuon && isDoubleMuTriggerPass) || (TwoEle && isDoubleElectronTriggerPass) || (MuonEle && isMuEGTriggerPass) ) ";
      TCut general = "isCleanEvent && (dR_leplep > 0.2) && (mass_leplep > 12) ";
      TCut oppositeSign = "(oppositeLepCharge == 1)";
      TCut sameSign = "(oppositeLepCharge == 0) && lepTotalPassSSCut && (mindr_lep1_allJet > 0.2) && (mindr_lep2_allJet > 0.2) && (lep1Pt+lep2Pt+MHT > 85)";
      TCut sign = "";
      if (charge == "SS") sign = sameSign;
      else sign = oppositeSign;
      TCut mycuts = trigger && general && sign && numJet_cut && numTag_cut;    
      TCut mycutb = trigger && general && sign && numJet_cut && numTag_cut;
      

      double signal_in = signal->Draw("numJets","","goff");
      double backgroundA_in = backgroundA->Draw("numJets","","goff");
      double backgroundB_in = backgroundB->Draw("numJets","","goff");
      double backgroundC_in = backgroundC->Draw("numJets","","goff");
      double signal_pass = signal->Draw("numJets",mycuts,"goff");
      double backgroundA_pass = backgroundA->Draw("numJets",mycutb,"goff");
      double backgroundB_pass = backgroundB->Draw("numJets",mycutb,"goff");
      double backgroundC_pass = backgroundC->Draw("numJets",mycutb,"goff");
      double background_in = backgroundA_in+backgroundB_in+backgroundC_in;
      double background_pass = backgroundA_pass+backgroundB_pass+backgroundC_pass;
      
      if (ttbb_fraction >= 0) {
        backgroundWeightA = (1 - ttbb_fraction)*background_pass/(backgroundA_pass+backgroundB_pass);
        backgroundWeightB = (1 - ttbb_fraction)*background_pass/(backgroundA_pass+backgroundB_pass);
        backgroundWeightC = ttbb_fraction*background_pass/backgroundC_pass;
      }
      if (ttbb_fraction == -2) {
        backgroundWeightA = 1.0;
        backgroundWeightB = 0.0001;
        backgroundWeightC = 0.0001;
      }
      
      double nTrain_pass = min(signal_pass,background_pass/max(backgroundWeightA,backgroundWeightC))/2;
      int nTrain_signal = floor(nTrain_pass*(signal_in/signal_pass));
      int nTrain_background = floor(max(backgroundWeightA,backgroundWeightC)*nTrain_pass*(background_in/background_pass));

      string NTS;
      string NTB;
      ostringstream convertS;
      convertS << nTrain_signal;
      NTS = convertS.str();
      ostringstream convertB;
      convertB << nTrain_background;
      NTB = convertB.str();
      string numTrainStr = "nTrain_Signal="+NTS+":nTrain_Background="+NTB+":nTest_Signal="+NTS+":nTest_Background="+NTB+":";
////----------------


//   TString jet_tag_label = "6ormorej_3ormoret";

   // Create new root output files.
   TString outfileName1( "TMVA_trained_" + charge + "_" + jet_tag_label + "_aaa.root" );   

   TFile* outputFile1 = TFile::Open( outfileName1, "RECREATE" );


////-----------run the factories--------------------------

// splitting training up into 4 factories
// have to do factories one at a time because of the way the weights directory is specified..


// first factory:

TMVA::Factory *factory1 = new TMVA::Factory( "TMVAClassification", outputFile1, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D" );

(TMVA::gConfig().GetIONames()).fWeightFileDir = "weights/" + jet_tag_label;
//(TMVA::gConfig().GetIONames()).fWeightFileDir = "simpleweights/" + jet_tag_label;

 if (charge == "SS") {

   // 4 variables vs. ttbar
   factory1->AddVariable( "lep2Pt", 'F' );
   factory1->AddVariable( "maxLepAbsEta", 'F' );
   factory1->AddVariable( "maxLepChargedIso", 'F' );
   factory1->AddVariable( "MHT", 'F' );

   // extra variables vs. ttbar
//    factory1->AddVariable( "correctedD0_leplep", 'F' );
//    factory1->AddVariable( "correctedDZ_leplep", 'F' );
//    factory1->AddVariable( "lep1Pt", 'F' );
//    factory1->AddVariable( "lep1Eta", 'F' );
//    factory1->AddVariable( "lep2Eta", 'F' );
//    factory1->AddVariable( "lep1IP", 'F' );
//    factory1->AddVariable( "lep2IP", 'F' );
//    factory1->AddVariable( "lep1IPError", 'F' );
//    factory1->AddVariable( "lep2IPErrpr", 'F' );
//    factory1->AddVariable( "maxAbsEta", 'F' );
//    factory1->AddVariable( "mindr_lep1_allJet", 'F' );
//    factory1->AddVariable( "mindr_lep1_jet", 'F' );
//    factory1->AddVariable( "mindr_lep2_allJet", 'F' );
//    factory1->AddVariable( "mindr_lep2_jet", 'F' );
//    factory1->AddVariable( "numJets_float", 'F' );
//    factory1->AddVariable( "sum_jet_pt", 'F' );
//    factory1->AddVariable( "wLike_dijet_mass", 'F' );

//    // 6 variables vs. ttW
//    factory1->AddVariable( "lep2Pt", 'F' );
//    factory1->AddVariable( "mindr_lep1_allJet", 'F' );
//    factory1->AddVariable( "mindr_lep1_jet", 'F' );
//    factory1->AddVariable( "mindr_lep2_jet", 'F' );
//    factory1->AddVariable( "numJets_float", 'F' );
//    factory1->AddVariable( "sum_jet_pt", 'F' );
//    factory1->AddVariable( "wLike_dijet_mass", 'F' );

// extra variables vs. ttW
//    factory1->AddVariable( "dR_leplep", 'F' );
//    factory1->AddVariable( "lep1Pt", 'F' );
//    factory1->AddVariable( "lep1Eta", 'F' );
//    factory1->AddVariable( "lep2Eta", 'F' );
//    factory1->AddVariable( "leps_minus_jets_charge", 'F' );
//    factory1->AddVariable( "leps_minus_untagged_jets_charge", 'F' );
//    factory1->AddVariable( "maxLepAbsEta", 'F' );
//    factory1->AddVariable( "mass_of_everything", 'F' );
//    factory1->AddVariable( "mindr_lep2_allJet", 'F' );
//    factory1->AddVariable( "first_jet_pt", 'F' );
//    factory1->AddVariable( "sum_jet_charge", 'F' );
//    factory1->AddVariable( "sum_untagged_jet_charge", 'F' );
//    factory1->AddVariable( "topLike_dijet_lep1_mass" );   
//    factory1->AddVariable( "topLike_dijet_lep2_mass" );   
   
 }

 else {

   factory1->AddVariable("higgsLike_dijet_mass", 'F' );
   factory1->AddVariable( "higgsLike_dijet_mass2", 'F' );
   factory1->AddVariable( "numHiggsLike_dijet_15_float", 'F' ); 
   factory1->AddVariable( "sum_pt", 'F' );
   factory1->AddVariable( "min_dr_jets", 'F' );
   factory1->AddVariable( "avg_btag_disc_btags", 'F' );



   // 6 best variables vs. ttbar and ttbb
   //  ge3t
   //factory1->AddVariable( "min_dr_jets", 'F' );
   //factory1->AddVariable( "higgsLike_dijet_mass", 'F' );
   
//    factory1->AddVariable("higgsLike_dijet_mass", 'F' );
//    factory1->AddVariable("dRJJ_ptJJMax", 'F' );
   

//    factory1->AddVariable( "higgsLike_dijet_mass2", 'F' );
//    factory1->AddVariable( "numHiggsLike_dijet_15_float", 'F' );
//    factory1->AddVariable( "sum_pt", 'F' );
//    factory1->AddVariable( "avg_btag_disc_btags", 'F' );

// ge4je2t
//    factory1->AddVariable( "min_dr_jets", 'F' );
//    factory1->AddVariable( "higgsLike_dijet_mass", 'F' );
//    factory1->AddVariable( "higgsLike_dijet_mass2", 'F' );
//    factory1->AddVariable( "numJets_float", 'F' ); 
//     factory1->AddVariable( "sum_pt", 'F' );
//    factory1->AddVariable( "avg_btag_disc_non_btags", 'F' );


////// e3je2t
//CHARLIE 6/24/2013 @17:14
//    factory1->AddVariable( "min_dr_jets", 'F' );
// //    factory1->AddVariable( "higgsLike_dijet_mass", 'F' );
// //    factory1->AddVariable( "higgsLike_dijet_mass2", 'F' );
//    factory1->AddVariable( "sum_pt", 'F' );
//    factory1->AddVariable( "avg_btag_disc_btags", 'F' );
//    factory1->AddVariable( "avg_btag_disc_non_btags", 'F' );

   
//    //ADDING KEVIN's NEW VARS:
   
//   std::vector<TString> varNames;
//   //Dijet variables
//   varNames.push_back("pJJ");
//   varNames.push_back("ptJJ");
//   varNames.push_back("pzJJ");
//   varNames.push_back("mJJ");
//   varNames.push_back("etaJJ");
//   varNames.push_back("dRJJ");
//   varNames.push_back("dPhiJJ");
//   varNames.push_back("dEtaJJ");
//   varNames.push_back("sumPtJJ");
//   varNames.push_back("ptAsymJJ");
//   varNames.push_back("pOther");
//   varNames.push_back("ptOther");
//   varNames.push_back("pzOther");
//   varNames.push_back("mOther");
//   varNames.push_back("etaOther");
//   varNames.push_back("ptSumOther");
//   varNames.push_back("ptLeadOther");
  
//   //Now, create all the branches!
//   for (std::vector<TString>::iterator i = varNames.begin();
//        i != varNames.end(); ++i) {
//     for (std::vector<TString>::iterator j = varNames.begin();
//          j != varNames.end(); ++j) {
      
//       TString &nameA = *i;
//       TString &nameB = *j;
      
//       TString namemax = nameA + "_" + nameB+"Max";
//       TString namemin = nameA + "_" + nameB+"Min";
//       factory1->AddVariable(namemin, 'F' ); 
//       factory1->AddVariable(namemax, 'F' ); 

//     }
//   }



//// e2je2t
//    factory1->AddVariable( "avg_dr_jets", 'F' );
//    factory1->AddVariable( "first_jet_pt", 'F' );
//    factory1->AddVariable( "mindr_lep1_jet", 'F' );
//    factory1->AddVariable( "sum_pt", 'F' );
//    factory1->AddVariable( "avg_btag_disc_btags", 'F' );

///----------------------------
///-----------

//     factory1->AddVariable( "avg_dr_jets", 'F' );
//    factory1->AddVariable( "min_dr_jets", 'F' );
// //    factory1->AddVariable( "min_dr_tagged_jets", 'F' ); 
// //    factory1->AddVariable( "avg_dr_tagged_jets", 'F' ); 

//    factory1->AddVariable( "higgsLike_dijet_mass", 'F' );
//    factory1->AddVariable( "higgsLike_dijet_mass2", 'F' );

// //      factory1->AddVariable( "numHiggsLike_dijet_15_float", 'F' );
// //       factory1->AddVariable( "numJets_float", 'F' ); 


// //    factory1->AddVariable( "Ht", 'F' );
// //    factory1->AddVariable( "first_jet_pt", 'F' );
// //    factory1->AddVariable( "sum_jet_pt", 'F' );
// //    factory1->AddVariable( "mindr_lep1_jet", 'F' );
// //    factory1->AddVariable( "all_sum_pt", 'F' );
//    factory1->AddVariable( "sum_pt", 'F' );


//    factory1->AddVariable( "avg_btag_disc_btags", 'F' );
//     factory1->AddVariable( "avg_btag_disc_non_btags", 'F' );
//    factory1->AddVariable( "lowest_btag", 'F' );
//    factory1->AddVariable( "first_highest_btag", 'F' );
//    factory1->AddVariable( "second_highest_btag", 'F' );


//    factory1->AddVariable( "dijet_mass_m2H", 'F' );
//    factory1->AddVariable( "dijet_mass_first", 'F' );
//    factory1->AddVariable( "dijet_mass_second", 'F' );
//   factory1->AddVariable( "dijet_mass_third", 'F' );
//    factory1->AddVariable( "avg_dijet_mass", 'F' );


//    factory1->AddVariable( "avg_tagged_dijet_mass", 'F' );

//    factory1->AddVariable( "closest_dijet_mass", 'F' );
//        factory1->AddVariable( "m2H_btag", 'F' );
//        factory1->AddVariable( "first_dibjet_mass", 'F' );
//        factory1->AddVariable( "second_dibjet_mass", 'F' );
//        factory1->AddVariable( "third_dibjet_mass", 'F' );
//        factory1->AddVariable( "closest_tagged_dijet_mass", 'F' );
 
 }
 
factory1->AddSignalTree    ( signal,     signalWeight     );
factory1->AddBackgroundTree( backgroundA, backgroundWeightA );
factory1->AddBackgroundTree( backgroundB, backgroundWeightB );
factory1->AddBackgroundTree( backgroundC, backgroundWeightC );
 factory1->SetWeightExpression( "weight*topPtWgt*csvWgtlf*csvWgthf*lepTotalSF*triggerSF" );  // weight*topPtWgt

//factory1->SetWeightExpression( "weight*weight_Xsec*lepTotalSF" );

// tell the factory to use all remaining events in the trees after training for testing:
 factory1->PrepareTrainingAndTestTree( mycuts, mycutb, numTrainStr+"SplitMode=Random:NormMode=NumEvents:!V" ); 
//   factory1->PrepareTrainingAndTestTree( mycuts, mycutb, "nTrain_Signal=0:nTrain_Background=0:nTest_Signal=0:nTest_Background=0:SplitMode=Random:NormMode=NumEvents:!V" ); 

 //------------------

// TMVA ANN: MLP (recommended ANN) -- all ANNs in TMVA are Multilayer Perceptrons

 // CF(Clermont-Ferrand)ANN
 if (createMVA) {
   if (Use["CFMlpANN"])  
     factory1->BookMethod( TMVA::Types::kCFMlpANN, "CFMlpANN", "!H:!V:NCycles=2000:HiddenLayers=N,N-1"  );

   if (Use["BDTG"]) // Gradient Boost
     factory1->BookMethod( TMVA::Types::kBDT, "BDTG","!H:!V:NTrees=400:BoostType=Grad:Shrinkage=0.1:UseBaggedGrad=kFalse:nCuts=20:NNodesMax=5:NegWeightTreatment=IgnoreNegWeights" );
   //      factory->BookMethod( TMVA::Types::kBDT, "BDTG","!H:!V:NTrees=1000:BoostType=Grad:Shrinkage=0.1:UseBaggedGrad:GradBaggingFraction=0.5:nCuts=20:NNodesMax=5" );
   if (Use["BDT"]) //  Boost
      factory1->BookMethod( TMVA::Types::kBDT, "BDT","!H:!V:NTrees=400:nEventsMin=40:MaxDepth=3:BoostType=AdaBoost:AdaBoostBeta=0.5:SeparationType=GiniIndex:nCuts=20:NegWeightTreatment=IgnoreNegWeights" );     
 }
 else factory1->BookMethod( TMVA::Types::kCFMlpANN, "CFMlpANN", "!H:!V:NCycles=1:HiddenLayers=N,N-1" );
 // n_cycles:#nodes:#nodes:...   N+1,N to start
   
 // ---- Now you can tell the factory to train, test, and evaluate the MVAs
 
 // Train MVAs using the set of training events
 factory1->TrainAllMethods();

 if (createMVA) {
   // ---- Evaluate all MVAs using the set of test events
   factory1->TestAllMethods();
   
   // ----- Evaluate and compare performance of all configured MVAs
   factory1->EvaluateAllMethods();
 }
 
 outputFile1->Close();
 std::cout << "  " << std::endl;
 std::cout << "==> Wrote root file: " << outputFile1->GetName() << std::endl;
 std::cout << "  " << std::endl;


// --------------------------------------------------------------

 std::cout << "  " << std::endl;
 std::cout << "==> TMVAClassification is done!" << std::endl;      

 delete factory1;
//   delete factory2;
//   delete factory3;
//   delete factory4;
   
 return;
   
}
