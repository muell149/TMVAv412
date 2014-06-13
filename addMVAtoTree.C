#include<TH1F.h>
#include<TFile.h>
#include<TTree.h>


vector<double> Add2Tree(TFile* mva_file, TFile* tree_file, char* hist_name){
  char path[256];
  sprintf(path,"Method_BDT/BDTG/%s",hist_name);
  TH1F* h1 = (TH1F*)mva_file->Get(path);
  TTree *t1 = (TTree*)tree_file->Get("summaryTree");


}


void addMVAtoTree(void){
  TFile* MVA_input = new TFile("TMVA_trained_OS_ge3t_aaa.root");
  TFile* Tree_input = new TFile("inputTrees/ttbar_bb.root");
  vector<double> addIt;
  
  addIt = Add2Tree(MVA_input,Tree_input,"MVA_BDTG_S");
  

