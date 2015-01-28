#include "TString.h"
#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"
#include "TChain.h"
#include "TObjArray.h"
#include "TLeaf.h"
#include "TH1F.h"
#include "TBranchBrowsable.h"
#include "TList.h"
#include "TSystem.h"

#include <iostream>

void
printAllBrowsables(TString const& _name, TLeaf* _leaf, TNonSplitBrowsable* _parent, TTree* _tree, TFile* _output, TFile* _binningRef, bool _print)
{
  TList browsables;
  TNonSplitBrowsable::GetBrowsables(browsables, _leaf->GetBranch(), _parent);

  if(browsables.GetEntries() > 0){
    for(int iB = 0; iB < browsables.GetEntries(); ++iB){
      TNonSplitBrowsable* br = (TNonSplitBrowsable*)(browsables.At(iB));
      TString brName(br->GetName());
      if(brName == "first") continue;
      if(_name.Contains("second") && brName == "second") continue;

      printAllBrowsables(_name + "." + brName, _leaf, br, _tree, _output, _binningRef, _print);
    }
  }
  else{
    if(_parent == 0){
      TString typeName(_leaf->GetTypeName());
      if(!typeName.Contains("_t")) return;

      TString leafName(_leaf->GetName());
      if(typeName == "Int_t" && leafName[leafName.Length() - 1] == '_') return;
    }

    _output->cd();

    TString branchName(_name);
    if(branchName.Contains("[")) branchName.Remove(branchName.Index("["), branchName.Length());

    if(_leaf->GetBranch()->GetListOfLeaves()->GetEntries() != 1){
      branchName += ".";
      branchName += _leaf->GetName();
    }

    TString plotName(branchName);
    plotName.ReplaceAll(".", "__");

    TString binning("");
    if(_binningRef){
      TH1F* hRef = (TH1F*)_binningRef->Get(plotName);
      if(hRef) binning = TString::Format("(%d, %f, %f)", hRef->GetNbinsX(), hRef->GetXaxis()->GetXmin(), hRef->GetXaxis()->GetXmax());
    }

    _tree->Draw(branchName + ">>" + plotName + binning, "", "goff");

    if(_print) std::cout << branchName << std::endl;
  }
}

void
dumpEvents(TString const& _tag, TString const& _fileList, TString const& _skipBranches = "", TString const& _binningRefName = "", bool _print = false)
{
  gSystem->Load("libSusyEvent.so");

  TString outputName("histo_" + _tag + ".root");
  TFile output(outputName, "recreate");

  TFile* binningRef(0);
  if(_binningRefName.Length() != 0)
    binningRef = TFile::Open(_binningRefName);

  TChain tree("susyTree");

  TObjArray* names(_fileList.Tokenize(" "));
  for(int iN(0); iN != names->GetEntries(); ++iN)
    tree.Add(names->At(iN)->GetName());
  delete names;

  TObjArray* skipBranchList(_skipBranches.Tokenize(","));
  for(int iS(0); iS != skipBranchList->GetEntries(); ++iS)
    tree.SetBranchStatus(skipBranchList->At(iS)->GetName(), 0);
  delete skipBranchList;
  tree.LoadTree(0);

  TObjArray* leaves(tree.GetListOfLeaves());

  if(!leaves){
    std::cerr << "No leaves in input " << _fileList << std::endl;
    return;
  }

  for(int iL(0); iL < leaves->GetEntries(); ++iL){
    TLeaf* leaf((TLeaf*)(leaves->At(iL)));
    if(!leaf->GetBranch() || leaf->GetBranch()->TestBit(kDoNotProcess)) continue;

    printAllBrowsables(leaf->GetBranch()->GetName(), leaf, 0, &tree, &output, binningRef, _print);

    // need to reset the list of trees in case of tchain
    leaves = tree.GetListOfLeaves();
  }

  output.cd();
  output.Write();
  output.Close();
}
