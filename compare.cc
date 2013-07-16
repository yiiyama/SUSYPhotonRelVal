#include "TString.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TKey.h"
#include "TIterator.h"
#include "TH1F.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TError.h"
#include "TSystem.h"

#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>

bool
plotsAreIdentical(TH1F* _plot0, TH1F* _plot1)
{
  if(_plot0->GetNbinsX() != _plot1->GetNbinsX()) return false;
  if(_plot0->GetXaxis()->GetXmin() != _plot1->GetXaxis()->GetXmin()) return false;
  if(_plot0->GetXaxis()->GetXmax() != _plot1->GetXaxis()->GetXmax()) return false;

  for(int iBin = 1; iBin <= _plot0->GetNbinsX(); ++iBin)
    if(_plot0->GetBinContent(iBin) != _plot1->GetBinContent(iBin)) return false;

  return true;
}

void
draw(TString const& _tag0, TString const& _tag1, TH1F* _plot0, TH1F* _plot1, TLegend& _legend)
{
  _legend.Clear();

  if(_plot0){
    _plot0->SetLineColor(kRed);
    TString label0(_tag0 + " (");
    label0 += _plot0->GetEntries();
    label0 += " entries)";

    _legend.AddEntry(_plot0, label0);

    _plot0->Draw();
  }
  if(_plot1){
    _plot1->SetLineColor(kBlue);
    TString label1(_tag1 + " (");
    label1 += _plot1->GetEntries();
    label1 += " entries)";

    _legend.AddEntry(_plot1, label1);

    if(_plot0) _plot1->Draw("same");
    else _plot1->Draw();
  }

  if(_plot0 && _plot1){
    double minXmax(std::min(_plot0->GetXaxis()->GetXmax(), _plot1->GetXaxis()->GetXmax()));
    double maxXmin(std::max(_plot0->GetXaxis()->GetXmin(), _plot1->GetXaxis()->GetXmin()));
    double maxYmax(std::max(_plot0->GetMaximum(), _plot1->GetMaximum()));
    _plot0->GetXaxis()->SetRangeUser(maxXmin, minXmax);
    _plot0->GetYaxis()->SetRangeUser(0., maxYmax * 1.1);
    _plot1->GetXaxis()->SetRangeUser(maxXmin, minXmax);
    _plot1->GetYaxis()->SetRangeUser(0., maxYmax * 1.1);
  }

  _legend.Draw();
}

void
compare(TString const& _tag0, TString const& _tag1, TString const& _htmlDir)
{
  using namespace std;

  TString prefix("histo_");
  TFile* file0 = TFile::Open(prefix + _tag0 + ".root");
  TFile* file1 = TFile::Open(prefix + _tag1 + ".root");

  if(!file0 || file0->IsZombie() || !file1 || file1->IsZombie()){
    cerr << "Cannot open input histograms" << endl;
    delete file0;
    delete file1;
    return;
  }

  gSystem->mkdir(_htmlDir + "/img");

  TCanvas* c1(new TCanvas("relval", "relval"));
  TLegend legend(0.6, 0.5, 0.9, 0.6);

  ofstream html(_htmlDir + "/relvalNtuples.html");
  html << "<html><head>" << endl;
  html << "<title>RA3 Ntuples release comparison " << _tag0 << " " << _tag1 << "</title>" << endl;
  html << "<style>" << endl;
  html << "table, th, td" << endl;
  html << "{" << endl;
  html << " border:1px solid black;" << endl;
  html << " border-collapse:collapse;" << endl;
  html << "}" << endl;
  html << "</style>" << endl;
  html << "</head>" << endl;
  html << "<body>" << endl;
  html << "<p>release0 = <a href='histo_" << _tag0 << ".root'>" << _tag0 << "</a></p>" << endl;
  html << "<p>release1 = <a href='histo_" << _tag1 << ".root'>" << _tag1 << "</a></p>" << endl;
  html << "<table><tr><th>Branch</th><th>Status</th><th>Mean</th><th>RMS</th></tr>" << endl;

  TKey* key;
  TIterator* itr = file0->GetListOfKeys()->MakeIterator();
  while((key = (TKey*)(itr->Next()))){
    TString keyName(key->GetName());

    html << "<tr><td>" << TString(keyName).ReplaceAll("__", ".") << "</td>";

    TH1F* plot0 = 0;
    TH1F* plot1 = 0;

    file1->GetObject(keyName, plot1);
    if(!plot1)
      html << "<td style='color:blue;text-align:center;'><a href='img/" << keyName << ".png'>Only release0</a></td><td>" << plot1->GetMean() << "</td><td>" << plot1->GetRMS() << "</td>";
    else{
      plot0 = (TH1F*)(key->ReadObj());

      if(plotsAreIdentical(plot0, plot1))
	html << "<td style='text-align:center;'><a href='img/" << keyName << ".png'>OK</a></td><td>" << plot0->GetMean() << "</td><td>" << plot0->GetRMS() << "</td>";
      else
	html << "<td style='color:red;text-align:center;'><a href='img/" << keyName << ".png'>Disagree</a></td><td></td><td></td>";
    }
    html << "</tr>" << endl;

    draw(_tag0, _tag1, plot0, plot1, legend);
    c1->Print(_htmlDir + "/img/" + keyName + ".png");

    delete plot0;
    delete plot1;
  }
  delete itr;

  itr = file1->GetListOfKeys()->MakeIterator();
  while((key = (TKey*)(itr->Next()))){
    TString keyName(key->GetName());

    TH1F* plot = 0;

    file0->GetObject(keyName, plot);
    if(plot){
      delete plot;
      continue;
    }

    file1->GetObject(keyName, plot);

    html << "<tr><td>" << TString(keyName).ReplaceAll("__", ".") << "</td>";
    html << "<td style='color:blue;text-align:center;'><a href='img/" << keyName << ".png'>Only release1</a></td><td>" << plot->GetMean() << "</td><td>" << plot->GetRMS() << "</td></tr>" << endl;

    draw(_tag0, _tag1, 0, plot, legend);
    c1->Print(_htmlDir + "/img/" + keyName + ".png");
  }
  delete itr;

  html << "</table></body></html>" << endl;
  html.close();
}

void
compare(TString const& _tag0, TString const& _tag1, TString const& _branchListName, TString const& _htmlDir)
{
  using namespace std;

  ifstream branchList(_branchListName);
  if(!branchList.is_open()){
    cerr << "Cannot open " << _branchListName << endl;
    return;
  }

  TString prefix("histo_");
  TFile* file0 = TFile::Open(prefix + _tag0 + ".root");
  TFile* file1 = TFile::Open(prefix + _tag1 + ".root");
  if(!file0 || file0->IsZombie() || !file1 || file1->IsZombie()){
    cerr << "Cannot open input histograms" << endl;
    delete file0;
    delete file1;
    return;
  }

  gSystem->mkdir(_htmlDir + "/img");

  TCanvas* c1(new TCanvas("relval", "relval"));
  TLegend legend(0.6, 0.5, 0.9, 0.6);

  ofstream html(_htmlDir + "/relvalNtuples.html");
  html << "<html><head>" << endl;
  html << "<title>RA3 Ntuples release comparison " << _tag0 << " " << _tag1 << "</title>" << endl;
  html << "<style>" << endl;
  html << "table, th, td" << endl;
  html << "{" << endl;
  html << " border:1px solid black;" << endl;
  html << " border-collapse:collapse;" << endl;
  html << "}" << endl;
  html << "</style>" << endl;
  html << "</head>" << endl;
  html << "<body>" << endl;
  html << "<p>release0 = <a href='histo_" << _tag0 << ".root'>" << _tag0 << "</a></p>" << endl;
  html << "<p>release1 = <a href='histo_" << _tag1 << ".root'>" << _tag1 << "</a></p>" << endl;
  html << "<table><tr><th>Branch0</th><th>Branch1</th><th>Status</th><th>Mean</th><th>RMS</th></tr>" << endl;

  string line;
  stringstream linebuf; 

  while(true){
    getline(branchList, line);
    if(!branchList.good()) break;

    linebuf.clear();
    linebuf.str(line);

    TString plot0Name;
    TString plot1Name;
    linebuf >> plot0Name >> plot1Name;

    html << "<tr><td>" << TString(plot0Name).ReplaceAll("__", ".") << "</td><td>" << TString(plot1Name).ReplaceAll("__", ".") << "</td>";

    TH1F* plot0 = 0;
    TH1F* plot1 = 0;

    file0->GetObject(plot0Name, plot0);
    file1->GetObject(plot1Name, plot1);
    if(!plot0){
      cerr << plot0Name << " is missing from input" << endl;
      break;
    }
    if(!plot1){
      cerr << plot1Name << " is missing from input" << endl;
      break;
    }

    if(plotsAreIdentical(plot0, plot1))
      html << "<td style='text-align:center;'><a href='img/" << plot0Name << "_" << plot1Name << ".png'>OK</a></td><td>" << plot0->GetMean() << "</td><td>" << plot0->GetRMS() << "</td>";
    else
      html << "<td style='color:red;text-align:center;'><a href='img/" << plot0Name << "_" << plot1Name << ".png'>Disagree</a></td><td></td><td></td>";

    html << "</tr>" << endl;

    draw(_tag0, _tag1, plot0, plot1, legend);
    c1->Print(_htmlDir + "/img/" + plot0Name + "_" + plot1Name + ".png");
  }

  html << "</table></body></html>" << endl;
  html.close();
}
