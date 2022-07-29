 /* 
 * ConvertSimRoa.cxx
 *
 *
 * Copyright (C) by the COSI team.
 * All rights reserved.
 *
 * By copying, distributing or modifying the Program (or any work
 * based on the Program) you indicate your acceptance of this statement,
 * and all its terms.
 *
 */

// Standard
#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>
using namespace std;

// ROOT
#include <TROOT.h>
#include <TEnv.h>
#include <TSystem.h>
#include <TApplication.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

// MEGAlib
#include "MGlobal.h"
#include "MGeometryRevan.h"
#include "MRERawEvent.h"
#include "MRawEventAnalyzer.h"

#include "MFileReadOuts.h"
#include "MReadOutSequence.h"
#include "MReadOutElementDoubleStrip.h"
#include "MReadOutDataADCValue.h"

#include "MREStripHit.h"
#include "MString.h"
#include "MTime.h"


////////////////////////////////////////////////////////////////////////////////


//! A ConvertSimRoa program based on MEGAlib and ROOT
//!
//! ConvertSimRoa -f COSITestCryostat_HighRes.inc1.id1.sim.gz -g COSITestCryostat.geo.setup -r 3922 HP508381_B71_12112019_1.Spe
//!
//!
class ConvertSimRoa
{
public:
  //! Default constructor
  ConvertSimRoa();
  //! Default destructor
  ~ConvertSimRoa();
  
  //! Parse the command line
  bool ParseCommandLine(int argc, char** argv);
  //! Analyze what eveer needs to be analyzed...
  bool Analyze();
  //! Interrupt the analysis
  void Interrupt() { m_Interrupt = true; }

private:
  //! True, if the analysis needs to be interrupted
  bool m_Interrupt;
  //! The sim file
  vector<MString> m_SimulationFileNames;
  //! The geometry file
  MString m_GeometryFileName;
};


////////////////////////////////////////////////////////////////////////////////


//! Default constructor
ConvertSimRoa::ConvertSimRoa() : m_Interrupt(false)
{
  gStyle->SetPalette(1, 0);
}


////////////////////////////////////////////////////////////////////////////////


//! Default destructor
ConvertSimRoa::~ConvertSimRoa()
{
  // Intentionally left blank
}


////////////////////////////////////////////////////////////////////////////////


//! Parse the command line
bool ConvertSimRoa::ParseCommandLine(int argc, char** argv)
{
  ostringstream Usage;
  Usage<<endl;
  Usage<<"  Usage: ConvertSimRoa <options>"<<endl;
  Usage<<"    General options:"<<endl;
  Usage<<"         -f:   simulation file name"<<endl;
  Usage<<"         -g:   geometry file name"<<endl;
  Usage<<"         -h:   print this help"<<endl;
  Usage<<endl;

  string Option;

  // Check for help
  for (int i = 1; i < argc; i++) {
    Option = argv[i];
    if (Option == "-h" || Option == "--help" || Option == "?" || Option == "-?") {
      cout<<Usage.str()<<endl;
      return false;
    }
  }

  // Now parse the command line options:
  for (int i = 1; i < argc; i++) {
    Option = argv[i];

    // First check if each option has sufficient arguments:
    // Single argument
    if (Option == "-f") {
      if (!((argc > i+1) && 
            (argv[i+1][0] != '-' || isalpha(argv[i+1][1]) == 0))){
        cout<<"Error: Option "<<argv[i][1]<<" needs a second argument!"<<endl;
        cout<<Usage.str()<<endl;
        return false;
      }
    } 
    // Multiple arguments template
    /*
    else if (Option == "-??") {
      if (!((argc > i+2) && 
            (argv[i+1][0] != '-' || isalpha(argv[i+1][1]) == 0) && 
            (argv[i+2][0] != '-' || isalpha(argv[i+2][1]) == 0))){
        cout<<"Error: Option "<<argv[i][1]<<" needs two arguments!"<<endl;
        cout<<Usage.str()<<endl;
        return false;
      }
    }
    */

    // Then fulfill the options:
    if (Option == "-f") {
      m_SimulationFileNames.push_back(argv[++i]);
      cout<<"Accepting simulation file name: "<<m_SimulationFileNames.back()<<endl;
    } else if (Option == "-g") {
      m_GeometryFileName = argv[++i];
      cout<<"Accepting geometry file name: "<<m_GeometryFileName<<endl;
    } else {
      cout<<"Error: Unknown option \""<<Option<<"\"!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////


//! Do whatever analysis is necessary
bool ConvertSimRoa::Analyze()
{

  MGeometryRevan* Geometry = new MGeometryRevan();
  if (Geometry->ScanSetupFile(m_GeometryFileName, false) == false) {
    mgui<<"Loading of geometry \""<<m_GeometryFileName<<"\" failed!"<<endl;
    mgui<<"Please check the output for geometry errors and correct them!"<<error;
    delete Geometry;
    return false;
  }

  double MaxTime = 10;
  int NStripHits = 0;
  int NEvents = 0;

  vector<MRERawEvent*> m_Events;

  for (unsigned int i = 0; i < m_SimulationFileNames.size(); ++i) {
    MRawEventAnalyzer Analyzer;
    Analyzer.SetGeometry(Geometry);
    if (Analyzer.SetInputModeFile(m_SimulationFileNames[i]) == false) return false;

    MRERawEvent* RE = nullptr;
    while ((RE = Analyzer.GetNextInitialRawEventFromFile()) != 0) {
      if (RE->GetEventTime() > MaxTime) {
        delete RE;
        break;
      }
      for (int r = 0; r < RE->GetNRESEs(); ++r) {
        MREStripHit* SH = dynamic_cast<MREStripHit*>(RE->GetRESEAt(r));
        if (SH != nullptr) {
          // Do whatever we need to do check if the hit was accepted
        }
      }

      m_Events.push_back(RE);
      if (m_Interrupt == true) break;
    }  
  }
  
  
  sort(m_Events.begin(), m_Events.end(), [](const MRERawEvent* a, const MRERawEvent* b) -> bool { return a->GetEventTime() < b->GetEventTime(); });
  
  ofstream out;
  out.open("FPGA_Input.txt");
  out<<"TYPE ROA"<<endl;
  out<<"UF doublesidedstrip adc_timing_hastiming_hastrigger"<<endl;
  
  map<long, int> DetectorIDs;
  
  for (unsigned int i = 0; i < m_Events.size(); ++i) {
    MRERawEvent* RE = m_Events[i]; 
    out<<"SE"<<endl;
    out<<"TI "<<RE->GetEventTime().GetLongIntsString()<<endl;
    for (int i = 0; i < RE->GetNRESEs(); ++i) {
      MREStripHit* SH = dynamic_cast<MREStripHit*>(RE->GetRESEAt(i));
      if (SH != nullptr) {
        if (DetectorIDs.find(SH->GetDetectorID()) == DetectorIDs.end()) {
          DetectorIDs[SH->GetDetectorID()] = DetectorIDs.size()+1;
        }
        
        out<<"UH "<<DetectorIDs[SH->GetDetectorID()]<<" ";
        out<<(SH->IsXStrip() ? "x" : "y")<<" ";
        out<<SH->GetStripID()<<" ";
        double Energy = SH->GetEnergy();
        if (Energy < 1) {
          Energy = -1;
          do {
            Energy = gRandom->Gaus(0, 2);
          } while (Energy < 0.5);
        } 
        out<<(Energy >= 2000 ? 16384 : int(16384 * Energy / 2000))<<" ";
        out<<int(128 * (0.75 + SH->GetDepthPosition()) / 1.5)<<" ";
        out<<(SH->GetNoiseFlags().Contains("NODEPTH") ? "0" : "1")<<" ";
        out<<(SH->GetEnergy() > 12 ? "1" : "0")<<" ";
        out<<endl;
      }
    }
  }
  out.close();
  
  cout<<"Hits:   "<<NStripHits/MaxTime<<endl;  
  cout<<"Events: "<<NEvents/MaxTime<<endl;


  return true;
}


////////////////////////////////////////////////////////////////////////////////


ConvertSimRoa* g_Prg = 0;
int g_NInterruptCatches = 1;


////////////////////////////////////////////////////////////////////////////////


//! Called when an interrupt signal is flagged
//! All catched signals lead to a well defined exit of the program
void CatchSignal(int a)
{
  if (g_Prg != 0 && g_NInterruptCatches-- > 0) {
    cout<<"Catched signal Ctrl-C (ID="<<a<<"):"<<endl;
    g_Prg->Interrupt();
  } else {
    abort();
  }
}


////////////////////////////////////////////////////////////////////////////////


//! Main program
int main(int argc, char** argv)
{
  // Catch a user interupt for graceful shutdown
  signal(SIGINT, CatchSignal);

  // Initialize global MEGALIB variables, especially mgui, etc.
  MGlobal::Initialize("ConvertSimRoa", "a ConvertSimRoa example program");

  TApplication ConvertSimRoaApp("ConvertSimRoaApp", 0, 0);

  g_Prg = new ConvertSimRoa();

  if (g_Prg->ParseCommandLine(argc, argv) == false) {
    cerr<<"Error during parsing of command line!"<<endl;
    return -1;
  } 
  if (g_Prg->Analyze() == false) {
    cerr<<"Error during analysis!"<<endl;
    return -2;
  } 

  ConvertSimRoaApp.Run();

  cout<<"Program exited normally!"<<endl;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
