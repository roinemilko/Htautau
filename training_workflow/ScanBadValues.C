#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <ROOT/RVec.hxx>
#include <TMath.h>
#include <iostream>
#include <vector>
#include <string>

void ScanBadValues(const char* filename = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/POWHEG/Tau_hadhad.root") {
    
    // Only the float variables used in TMVA
    const char* vars = "tau_pt,tau_eta,tau_phi,tau_mass,tau_dxy,tau_dz,tau_ipLengthSig,"
                       "tau_chargedIso,tau_neutralIso,tau_rawIso,tau_rawIsodR03,tau_puCorr,"
                       "tau_rawDeepTauVSjet,tau_rawDeepTauVSe,tau_rawDeepTauVSmu,"
                       "tau_probDM0PNet,tau_probDM1PNet,tau_probDM2PNet,"
                       "tau_probDM10PNet,tau_probDM11PNet,tau_ptCorrPNet,tau_qConfPNet,"
                       "tau_rawPNetVSe,tau_rawPNetVSjet,tau_rawPNetVSmu,"
                       "tau_probDM0UParT,tau_probDM1UParT,tau_probDM2UParT,"
                       "tau_probDM10UParT,tau_probDM11UParT,tau_ptCorrUParT,tau_qConfUParT,"
                       "tau_rawUParTVSe,tau_rawUParTVSjet,tau_rawUParTVSmu";

    // Parse the comma-separated string
    std::vector<std::string> varList;
    TString tVars(vars);
    TObjArray* tokens = tVars.Tokenize(",");
    for (int i = 0; i < tokens->GetEntries(); ++i) {
        TString v = ((TObjString*)tokens->At(i))->GetString();
        v.ReplaceAll(" ", "");
        varList.push_back(v.Data());
    }
    delete tokens;

    // Open file and setup TTreeReader
    TFile* f = TFile::Open(filename, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    TTreeReader reader("Events", f);

    // Create a TTreeReaderValue for every variable dynamically
    std::vector<TTreeReaderValue<ROOT::VecOps::RVec<float>>*> readers;
    for (const auto& v : varList) {
        readers.push_back(new TTreeReaderValue<ROOT::VecOps::RVec<float>>(reader, v.c_str()));
    }

    std::cout << "Scanning " << reader.GetEntries(true) << " events for NaNs or Infs..." << std::endl;

    Long64_t eventNum = 0;
    int badEvents = 0;
    
    // Loop over all events
    while (reader.Next()) {
        bool eventHasBadValue = false;
        
        for (size_t i = 0; i < varList.size(); ++i) {
            if (!readers[i]->IsValid()) continue;
            
            auto& vec = **(readers[i]);
            
            // Check up to the first 2 taus (index 0 and 1)
            for (size_t j = 0; j < vec.size() && j < 2; ++j) { 
                float val = vec[j];
                
                if (TMath::IsNaN(val) || std::isinf(val)) {
                    std::cout << "Event " << eventNum << " | " 
                              << varList[i] << "[" << j << "] = " << val << std::endl;
                    eventHasBadValue = true;
                }
            }
        }
        
        if (eventHasBadValue) badEvents++;
        eventNum++;
        
        // Stop after finding the first few bad events to keep the terminal clean
        if (badEvents >= 20) {
            std::cout << "\nFound 20 bad events. Stopping early..." << std::endl;
            break;
        }
    }

    std::cout << "Scan complete." << std::endl;

    // Cleanup
    for (auto r : readers) {
        delete r;
    }
    f->Close();
}
