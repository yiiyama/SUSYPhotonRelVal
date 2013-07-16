#include "TString.h"
#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <fstream>

int
testLibs(TString const& _file)
{
  using namespace std;

  int status = gSystem->Load("libSusyEvent.so");
  if(status == -1){
    cerr << "libSusyEvent.so does not exist" << endl;
    return -1;
  }
  TFile* input = TFile::Open(_file);
  if(!input || input->IsZombie()){
    cerr << "File " << _file << " cannot be opened" << endl;
    delete input;
    return -1;
  }
  TTree* tree = 0;
  input->GetObject("susyTree", tree);
  if(!tree || tree->GetEntries() == 0){
    cerr << _file + " does not contain a valid tree" << endl;
    delete input;
    return -1;
  }

  ifstream entriesFile("entries.txt");
  if(entriesFile.is_open()){
    double entries;
    entriesFile >> entries;
    if(tree->GetEntries() != entries){
      cerr << "Event numbers in the trees don't match" << endl;
      delete input;
      return -1;
    }
  }
  else{
    ofstream output("entries.txt");
    output << tree->GetEntries() << endl;
    output.close();
  }

  delete input;
  return 0;
}
