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
#include <iomanip>

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

double
kolmogorovPValue(TH1F* _plot0, TH1F* _plot1)
{
  if(_plot0->GetNbinsX() != _plot1->GetNbinsX()) return false;
  if(_plot0->GetXaxis()->GetXmin() != _plot1->GetXaxis()->GetXmin()) return false;
  if(_plot0->GetXaxis()->GetXmax() != _plot1->GetXaxis()->GetXmax()) return false;

  return _plot0->KolmogorovTest(_plot1, "UO");
}

void
draw(TString const& _tag0, TString const& _tag1, TH1F* _plot0, TH1F* _plot1, TLegend& _legend, bool _doNorm)
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

    if(_plot0){
      if(_doNorm) _plot1->Scale(_plot0->GetEntries() / _plot1->GetEntries());
      _plot1->Draw("same");
    }
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
writeHeader(std::ofstream& _html, TString const& _tag0, TString const& _tag1, bool _doKtest, bool _differentBName = false)
{
  _html << "<html><head>" << std::endl;
  _html << "<title>RA3 Ntuples release comparison " << _tag0 << " " << _tag1 << "</title>" << std::endl;
  _html << "<style>" << std::endl;
  _html << "table, th, td" << std::endl;
  _html << "{" << std::endl;
  _html << " border:1px solid black;" << std::endl;
  _html << " border-collapse:collapse;" << std::endl;
  _html << "}" << std::endl;
  _html << "</style>" << std::endl;
  _html << "</head>" << std::endl;
  _html << "<body>" << std::endl;
  _html << "<p>release0 = <a href='histo_" << _tag0 << ".root'>" << _tag0 << "</a></p>" << std::endl;
  _html << "<p>release1 = <a href='histo_" << _tag1 << ".root'>" << _tag1 << "</a></p>" << std::endl;
  _html << "<table>" << std::endl;
  if(_differentBName)
    _html << "<colgroup><col style='width:500px;' /><col style='width:500px;' /></colgroup>" << std::endl;
  else
    _html << "<colgroup><col style='width:500px;' /></colgroup>" << std::endl;
  _html << "<tr>";
  if(_differentBName)
    _html << "<th>Branch0</th><th>Branch1</th>";
  else
    _html << "<th>Branch</th>";
  _html << "<th>Status</th>" << (_doKtest ? "<th>P-value</th>" : "") << "<th>Entries0</th><th>Mean0</th><th>RMS0</th><th>Entries1</th><th>Mean1</th><th>RMS1</th>" << "</tr>" << std::endl;
}

void
writeLine(std::ofstream& _html, TH1F* _plot0, TH1F* _plot1, double _ktestCut, TString const& _bName, TString const& _bName1 = "")
{
  _html << "<tr><td>" << _bName << "</td>";
  if(_bName1 != "") _html << "<td>" << _bName1 << "</td>";

  TString imgName(_bName);
  imgName.ReplaceAll(".", "__");
  if(_bName1 != "") (imgName += "_" + _bName1).ReplaceAll(".", "__");

  if(!_plot1){
    _html << "<td style='color:blue;text-align:center;'><a href='img/" << imgName << ".png'>Only release0</a></td>";
    if(_ktestCut > 0.) _html << "<td>0</td>";
    _html << "<td>" << _plot0->GetEntries() << "</td><td>" << _plot0->GetMean() << "</td><td>" << _plot0->GetRMS() << "</td>";
    _html << "<td>-</td><td>-</td><td>-</td>";
  }
  else if(!_plot0){
    _html << "<td style='color:blue;text-align:center;'><a href='img/" << imgName << ".png'>Only release1</a></td>";
    if(_ktestCut > 0.) _html << "<td>0</td>";
    _html << "<td>-</td><td>-</td><td>-</td><td>0</td>";
    _html << "<td>" << _plot1->GetEntries() << "</td><td>" << _plot1->GetMean() << "</td><td>" << _plot1->GetRMS() << "</td>";
  }
  else{
    if(_ktestCut > 0.){
      double pvalue(kolmogorovPValue(_plot0, _plot1));
      if(pvalue >= _ktestCut)
        _html << "<td style='text-align:center;'><a href='img/" << imgName << ".png'>OK</a></td>";
      else
        _html << "<td style='color:red;text-align:center;'><a href='img/" << imgName << ".png'>Disagree</a></td>";

      _html << "<td>" << std::scientific << pvalue << "</td>";
      _html << "<td>" << _plot0->GetEntries() << "</td><td>" << _plot0->GetMean() << "</td><td>" << _plot0->GetRMS() << "</td>";
      _html << "<td>" << _plot1->GetEntries() << "</td><td>" << _plot1->GetMean() << "</td><td>" << _plot1->GetRMS() << "</td>";
    }
    else{
      if(plotsAreIdentical(_plot0, _plot1))
        _html << "<td style='text-align:center;'><a href='img/" << imgName << ".png'>OK</a></td>";
      else
        _html << "<td style='color:red;text-align:center;'><a href='img/" << imgName << ".png'>Disagree</a></td>";

      _html << "<td>" << _plot0->GetEntries() << "</td><td>" << _plot0->GetMean() << "</td><td>" << _plot0->GetRMS() << "</td>";
      _html << "<td>" << _plot1->GetEntries() << "</td><td>" << _plot1->GetMean() << "</td><td>" << _plot1->GetRMS() << "</td>";
    }
  }
  _html << "</tr>" << std::endl;
}

void
writeFooter(std::ofstream& _html)
{
  _html << "</table></body></html>" << std::endl;
}

void
compare(TString const& _tag0, TString const& _tag1, TString const& _htmlDir, double _ktestCut = -1., bool _doNorm = false)
{
  TString prefix("histo_");
  TFile* file0 = TFile::Open(prefix + _tag0 + ".root");
  TFile* file1 = TFile::Open(prefix + _tag1 + ".root");

  if(!file0 || file0->IsZombie() || !file1 || file1->IsZombie()){
    cerr << "Cannot open input histograms" << std::endl;
    delete file0;
    delete file1;
    return;
  }

  gSystem->mkdir(_htmlDir + "/img");

  TCanvas* c1(new TCanvas("relval", "relval"));
  TLegend legend(0.6, 0.5, 0.9, 0.6);

  std::ofstream html(_htmlDir + "/relvalNtuples.html");
  writeHeader(html, _tag0, _tag1, _ktestCut > 0.);

  TKey* key;
  TIterator* itr = file0->GetListOfKeys()->MakeIterator();
  while((key = (TKey*)(itr->Next()))){
    TString keyName(key->GetName());
    TString bName(keyName);
    bName.ReplaceAll("__", ".");

    TH1F* plot0 = 0;
    TH1F* plot1 = 0;

    plot0 = (TH1F*)(key->ReadObj());
    file1->GetObject(keyName, plot1);

    writeLine(html, plot0, plot1, _ktestCut, bName);

    draw(_tag0, _tag1, plot0, plot1, legend, _doNorm);
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

    TString bName(keyName);
    bName.ReplaceAll("__", ".");

    file1->GetObject(keyName, plot);

    writeLine(html, 0, plot, _ktestCut, bName);

    draw(_tag0, _tag1, 0, plot, legend, _doNorm);
    c1->Print(_htmlDir + "/img/" + keyName + ".png");
  }
  delete itr;

  writeFooter(html);
  html.close();
}

void
compare(TString const& _tag0, TString const& _tag1, TString const& _branchListName, TString const& _htmlDir, double _ktestCut = -1., bool _doNorm = false)
{
  ifstream branchList(_branchListName);
  if(!branchList.is_open()){
    cerr << "Cannot open " << _branchListName << std::endl;
    return;
  }

  TString prefix("histo_");
  TFile* file0 = TFile::Open(prefix + _tag0 + ".root");
  TFile* file1 = TFile::Open(prefix + _tag1 + ".root");
  if(!file0 || file0->IsZombie() || !file1 || file1->IsZombie()){
    cerr << "Cannot open input histograms" << std::endl;
    delete file0;
    delete file1;
    return;
  }

  gSystem->mkdir(_htmlDir + "/img");

  TCanvas* c1(new TCanvas("relval", "relval"));
  TLegend legend(0.6, 0.5, 0.9, 0.6);

  std::ofstream html(_htmlDir + "/relvalNtuples.html");
  writeHeader(html, _tag0, _tag1, _ktestCut > 0., true);

  std::string line;
  std::stringstream linebuf; 
  while(true){
    std::getline(branchList, line);
    if(!branchList.good()) break;

    linebuf.clear();
    linebuf.str(line);

    TString plot0Name;
    TString plot1Name;
    linebuf >> plot0Name >> plot1Name;

    TString bName0(plot0Name);
    TString bName1(plot1Name);
    bName0.ReplaceAll("__", ".");
    bName1.ReplaceAll("__", ".");

    TH1F* plot0 = 0;
    TH1F* plot1 = 0;

    file0->GetObject(plot0Name, plot0);
    file1->GetObject(plot1Name, plot1);

    writeLine(html, plot0, plot1, _ktestCut, bName0, bName1);

    draw(_tag0, _tag1, plot0, plot1, legend, _doNorm);
    c1->Print(_htmlDir + "/img/" + plot0Name + "_" + plot1Name + ".png");
  }

  writeFooter(html);
  html.close();
}
