#ifndef MATCHING_HELPERS_H
#define MATCHING_HELPERS_H

#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TEfficiency.h"
#include "TDirectory.h"


inline void ProjectFromTree(const char* filename, TH1* hist, const char* varExpr, const char* cut = "", const char* treeName = "Events") {
    TDirectory* savedDir = gDirectory; 
    
    TFile* f = TFile::Open(filename, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return;
    }
    
    TTree* t = (TTree*)f->Get(treeName);
    if (!t) {
        std::cerr << "Error: Could not find tree '" << treeName << "' in " << filename << std::endl;
        f->Close();
        return;
    }
    
    hist->SetDirectory(f); 
    t->Project(hist->GetName(), varExpr, cut, "goff");

}
#endif


