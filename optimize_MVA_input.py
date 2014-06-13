import sys, os, string, subprocess, getopt, pickle, re, linecache
##############################################################
##############################################################
##############################################################
### This script takes input variables, added in array_of_vars
### and trains an MVA with combinations of two vars at a time
### then pipes the output to a final output csv file that
### allows you to sort each combo based on ROC-Integ and
### separation. This seems to work better than picking the
### top variables based on rank that TMVA outputs.
##############################################################
####### Charlie Mueller (cmuelle2@nd.edu) 6/4/2013 ###########
##############################################################

temp_dir = "output_files1/"
output_csv ="var_list.csv"

array_of_vars = ["avg_btag_disc_btags","ptSumOther_ptAsymJJMax", "ptSumOther_mOtherMax", "ptSumOther_dEtaJJMax", "sum_pt", "ptSumOther_ptOtherMax", "ptSumOther_pOtherMin", "ptSumOther_etaOtherMin", "ptSumOther_dPhiJJMax", "ptSumOther_pzJJMax", "ptSumOther_pzOtherMin", "ptSumOther_sumPtJJMax", "ptSumOther_ptSumOtherMin", "ptSumOther_ptLeadOtherMin", "ptSumOther_sumPtJJMin", "ptSumOther_ptSumOtherMax", "dEtaJJ_dEtaJJMin", "ptSumOther_etaJJMax", "ptSumOther_dEtaJJMin", "ptSumOther_ptJJMax", "higgsLike_dijet_mass2", "ptSumOther_etaJJMin", "ptSumOther_dPhiJJMin", "ptSumOther_dRJJMax", "ptSumOther_ptJJMin", "ptSumOther_mJJMin", "ptSumOther_pJJMax", "ptSumOther_pzJJMin", "ptSumOther_pzOtherMax", "ptSumOther_pJJMin", "ptSumOther_mOtherMin", "ptSumOther_etaOtherMax", "mOther_dEtaJJMax", "ptSumOther_ptOtherMin", "ptSumOther_dRJJMin", "ptLeadOther_sumPtJJMax", "ptLeadOther_ptSumOtherMin", "ptLeadOther_ptLeadOtherMin", "sumPtJJ_ptLeadOtherMax", "ptSumOther_mJJMax", "mOther_dRJJMax", "ptSumOther_pOtherMax", "min_dr_jets", "dRJJ_dRJJMin", "numHiggsLike_dijet_15_float", "ptSumOther_ptLeadOtherMax", "ptSumOther_ptAsymJJMin","higgsLike_dijet_mass","dRJJ_ptJJMax"]
 
n = len(array_of_vars)

cmd1 = "mkdir "+temp_dir
cmd2 = "rm -rf "+temp_dir
subprocess.call(cmd1, shell=True)


output = open(output_csv,'wb')
output.write(" ROC-integ, Separation, Significance, Variables\n")

for i in range(0,n):
    for j in range(i,n):
        if i != j:
            #make sure lines 286 and 287 have factory1->AddVariable("variable_here",'F');
            #if they don't go to the appropriate lines in the TMVAClassifier_A.C and 
            edit1 = "sed -i '286 s/\".*\"/\""+array_of_vars[i]+"\"/' TMVAClassification_A.C"
            edit2 = "sed -i '287 s/\".*\"/\""+array_of_vars[j]+"\"/' TMVAClassification_A.C"
            runit = "root -l -q TMVAClassification_A.C+\'(\"BDTG\",\"ge3jge3t\",\"numJets>=3\",\"numTaggedJets>=3\",true,\"OS\",-2)\' > "+temp_dir+array_of_vars[i]+"++"+array_of_vars[j]+".txt"
	                
            subprocess.call(edit1, shell=True)
            subprocess.call(edit2, shell=True)
            subprocess.call(runit, shell=True)
            
            input = temp_dir+array_of_vars[i]+"++"+array_of_vars[j]+".txt"
	    str1 = linecache.getline(input,236)
            str = str1[82:-1]
            str= str.replace("    | ",",")
            str = str.replace("    ",",")
            str = str+", "+ array_of_vars[i]+", "+array_of_vars[j]
            output.write(str+"\n")
            
subprocess.call(cmd2, shell=True)
