
set year = "2012_53x"
set label = "tmva_v1"

set ttbarPartString = ""
set ttbar_bbPartString = ""
set ttbar_ccPartString = ""
foreach iPart ("part1" "part2" "part3" "part4" "part5" "part6" "part7" "part8" "part9" "part10" "part11" "part12" "part13")
	set ttbarPartString = "${ttbarPartString} ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_ll_${iPart}_${year}_${label}_all.root" 
	set ttbar_bbPartString = "${ttbar_bbPartString} ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_bb_ll_${iPart}_${year}_${label}_all.root" 
	set ttbar_ccPartString = "${ttbar_ccPartString} ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_cc_ll_${iPart}_${year}_${label}_all.root"
end

echo "Adding ttbar_ll"
hadd -v 0 -f inputTrees/ttbar_ll_${year}_${label}_all.root ${ttbarPartString}
echo "Adding ttbar_bb_ll"
hadd -v 0 -f inputTrees/ttbar_bb_ll_${year}_${label}_all.root ${ttbar_bbPartString}
echo "Adding ttbar_cc_ll"
hadd -v 0 -f inputTrees/ttbar_cc_ll_${year}_${label}_all.root ${ttbar_ccPartString}
echo "Copying ttbar jj and lj files"
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_jj_${year}_${label}_all.root inputTrees/ttbar_jj_${year}_${label}_all.root
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_lj_${year}_${label}_all.root inputTrees/ttbar_lj_${year}_${label}_all.root
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_bb_jj_${year}_${label}_all.root inputTrees/ttbar_bb_jj_${year}_${label}_all.root
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_bb_lj_${year}_${label}_all.root inputTrees/ttbar_bb_lj_${year}_${label}_all.root
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_cc_jj_${year}_${label}_all.root inputTrees/ttbar_cc_jj_${year}_${label}_all.root
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbar_cc_lj_${year}_${label}_all.root inputTrees/ttbar_cc_lj_${year}_${label}_all.root

echo "Adding ttbar_lf"
hadd -v 0 -f inputTrees/ttbar_lf.root inputTrees/ttbar_jj_${year}_${label}_all.root inputTrees/ttbar_lj_${year}_${label}_all.root inputTrees/ttbar_ll_${year}_${label}_all.root
echo "Adding ttbar_bb"
hadd -v 0 -f inputTrees/ttbar_bb.root inputTrees/ttbar_bb_jj_${year}_${label}_all.root inputTrees/ttbar_bb_lj_${year}_${label}_all.root inputTrees/ttbar_bb_ll_${year}_${label}_all.root
echo "Adding ttbar_cc"
hadd -v 0 -f inputTrees/ttbar_cc.root inputTrees/ttbar_cc_jj_${year}_${label}_all.root inputTrees/ttbar_cc_lj_${year}_${label}_all.root inputTrees/ttbar_cc_ll_${year}_${label}_all.root
echo "Copying ttH125 and ttbarW"
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttH125_${year}_${label}_all.root inputTrees/ttH125.root
cp ../../DrawPlots/bin/treeFiles/dilSummaryTrees_ttbarW_${year}_${label}_all.root inputTrees/ttbarW.root
echo "Done"


