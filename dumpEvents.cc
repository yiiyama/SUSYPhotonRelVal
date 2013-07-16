#include "TString.h"
#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"
#include "TObjArray.h"
#include "TLeaf.h"
#include "TH1F.h"
#include "TBranchBrowsable.h"
#include "TList.h"

#include <iostream>

void
printAllBrowsables(TString const& _name, TLeaf* _leaf, TNonSplitBrowsable* _parent, TTree* _tree, TFile* _output, bool _print)
{
  TList browsables;
  TNonSplitBrowsable::GetBrowsables(browsables, _leaf->GetBranch(), _parent);

  if(browsables.GetEntries() > 0){
    for(int iB = 0; iB < browsables.GetEntries(); ++iB){
      TNonSplitBrowsable* br = (TNonSplitBrowsable*)(browsables.At(iB));
      TString brName(br->GetName());
      if(brName == "first") continue;
      if(_name.Contains("second") && brName == "second") continue;

      printAllBrowsables(_name + "." + brName, _leaf, br, _tree, _output, _print);
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

    _tree->Draw(branchName, "", "goff");

    TH1F* histo = (TH1F*)gDirectory->Get("htemp");
    branchName.ReplaceAll(".", "__");

    if(_print) std::cout << branchName << std::endl;

    histo->Write(branchName);
    delete histo;
  }
}

void
dumpEvents(TString const& _tag, TString const& _file, bool _print = false)
{
  TString outputName("histo_" + _tag + ".root");
  TFile output(outputName, "recreate");

  gSystem->Load("libSusyEvent.so");

  TFile* input = TFile::Open(_file);
  TTree* tree = 0;

  input->GetObject("susyTree", tree);

  TObjArray* leaves = tree->GetListOfLeaves();

  for(int iL = 0; iL < leaves->GetEntries(); ++iL){
    TLeaf* leaf = (TLeaf*)(leaves->At(iL));

    printAllBrowsables(leaf->GetBranch()->GetName(), leaf, 0, tree, &output, _print);
  }

  delete input;

  output.Write();
  output.Close();
}
