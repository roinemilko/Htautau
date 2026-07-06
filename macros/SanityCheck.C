#include "TLegend.h"
#include "TStyle.h"
#include "TAxis.h"
#include <TString.h>
#include <TH1F.h>
#include "TCanvas.h"
#include <TFile.h>
#include <TTree.h>
#include <THStack.h>
#include <iostream>

void SanityCheck(const char* file_hadhad = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/RawEventInfo_hadhad.root",
                 const char* file_nohadhad = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/RawEventInfo.root") {

    TFile* f_hadhad = new TFile(file_hadhad, "READ");
    TTree* t_hadhad = (TTree*)f_hadhad->Get("Events");

    TFile* f_nohadhad = new TFile(file_nohadhad, "READ");
    TTree* t_nohadhad = (TTree*)f_nohadhad->Get("Events");

    // Widened canvas for side-by-side plots
    TCanvas* c1 = new TCanvas("c1", "Sanity Check Plot", 1200, 600);
    c1->Divide(2,1);

    TLegend* leg = new TLegend(0.65, 0.6, 0.88, 0.88);
    leg->SetBorderSize(0); 

    auto DrawPlot = [&](int padNum, bool hadhad) {
        c1->cd(padNum);
        
        // Add titles to the top of the pads to tell them apart
        TString padTitle = "";
        THStack* hs = new THStack(Form("full_plot_%d", padNum), padTitle);

        int colorIndex = 1;

        for (float pt = 300; pt < 400; pt = pt + 5.0f) {
            TString hname = Form("hist_%d_%d", (int)pt, padNum);

            TTree* t;
            if (hadhad) {t = t_hadhad;} else {t = t_nohadhad;}
            t->Draw(Form("dR_fJ_nolim[0] >> %s(100, 0.0, 1.5)", hname.Data()), Form("genH_pt_raw >= %g && genH_pt_raw <= %g", pt, pt + 100.0f), "goff");

            TH1F* h = (TH1F*)gDirectory->Get(hname);
            if (h) {
                h->SetLineColor(colorIndex);
                h->SetLineWidth(2);
                if (h->Integral() > 0) {
                    h->Scale(1.0f / h->Integral());
                }
                h->SetFillColorAlpha(colorIndex, 0.3);
                hs->Add(h);

                if (padNum == 1) {
                    TString legLabel = Form("%d #leq p_{T} < %d GeV", (int)pt, (int)(pt + 100.0f));
                    leg->AddEntry(h, legLabel, "f");
                }

                colorIndex++;
                if (colorIndex == 5) colorIndex++;
            }
        } 


        hs->Draw("nostack HIST");
        hs->GetXaxis()->SetTitle("min(max(#Delta R(jet, tau1), #Delta R(jet, tau2))");
        
        // Set specific Y-axis label based on the dataset
        if (hadhad) {
            hs->GetYaxis()->SetTitle("FatJets (require hadhad)");
        } else {
            hs->GetYaxis()->SetTitle("FatJets");
        }
        hs->GetYaxis()->SetTitleOffset(1.6);
        if (padNum == 1) {
            leg->Draw();
        }

    };
    
    DrawPlot(1, true);
    DrawPlot(2, false);
    c1->SaveAs("/eos/user/m/mroine/www/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/FatJet_dR_SanityCheck_300to400.png");

}