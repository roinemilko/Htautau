#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <THStack.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TString.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <iostream>
#include <vector>

void Plots(bool AK4 = true, bool AK8 = true, bool AK15 = true, 
        const char* params = "X_mass/X_pt, X_eta",
        const char* save_path = "/eos/user/m/mroine/www/") {

    TString paramStr(params);
    TObjArray* paramArray = paramStr.Tokenize(",");
    int nParams = paramArray->GetEntries();

    if (nParams == 0) {
        std::cerr << "Nothing to plot" << std::endl;
        return;
    }

    TFile *AK4_f = nullptr, *AK8_f = nullptr, *AK15_f = nullptr;
    TTree *AK4_tree = nullptr, *AK8_tree = nullptr, *AK15_tree = nullptr;

    if (AK4) {
        AK4_f = new TFile("jets/Jet.root", "READ");
        if (AK4_f) {AK4_tree = (TTree*)AK4_f->Get("AK4_skimmed_tree");}
        else {
            std::cerr << "Couldn't open file Jet.root" << std::endl;
        }
    }

    if (AK8) {
        AK8_f = new TFile("jets/fatJet.root", "READ");
        if (AK8_f) {AK8_tree = (TTree*)AK8_f->Get("AK8_skimmed_tree");}
        else {
            std::cerr << "Couldn't open file fatJet.root" << std::endl;
        }
    }

    if (AK15) {
        AK15_f = new TFile("jets/AK15.root", "READ");
        if (AK15_f) {AK15_tree = (TTree*)AK15_f->Get("AK15_skimmed_tree");}
        else {
            std::cerr << "Couldn't open file AK15.root" << std::endl;
        }
    }


    // https://root.cern.ch/doc/master/classTCanvas.html
    TCanvas* c1 = new TCanvas("c1", "Jet plots", 400 * nParams, 400);
    c1->Divide(nParams, 1); 

    for (int i = 0; i < nParams; ++i) {
        c1->cd(i + 1); 

        TString baseExpr = ((TObjString*)paramArray->At(i))->GetString();
        baseExpr.ReplaceAll(" ", ""); 

        TString hist_title = baseExpr;
        hist_title.ReplaceAll("X", "");
        hist_title.ReplaceAll("_", "");

        // https://root.cern.ch/doc/master/classTHStack.html
        THStack* hs = new THStack(Form("hs_%d", i), hist_title);
        TLegend* leg = new TLegend(0.65, 0.75, 0.9, 0.9);

        if (AK4 && AK4_tree) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "ak4"); 
            
            // Unique name for every hist
            TString hName = Form("h4_%d", i);
            AK4_tree->Draw(expr + ">>" + hName);
            TH1F* h = (TH1F*)gDirectory->Get(hName);

            if (h) {
                h->SetLineColor(kBlue);
                h->SetLineWidth(2);
                hs->Add(h);
                leg->AddEntry(h, "AK4", "l");
            }
        }


        if (AK8 && AK8_tree) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "fj"); 
            
            TString hName = Form("h8_%d", i);
            AK8_tree->Draw(expr + ">>" + hName);
            
            TH1F* h = (TH1F*)gDirectory->Get(hName);
            if (h) {
                h->SetLineColor(kRed);
                h->SetLineWidth(2);
                hs->Add(h);
                leg->AddEntry(h, "AK8", "l");
            }
        }

        if (AK15 && AK15_tree) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "ak15"); 
            
            TString hName = Form("h15_%d", i);
            AK15_tree->Draw(expr + ">>" + hName, "", "goff");
            
            TH1F* h = (TH1F*)gDirectory->Get(hName);
            if (h) {
                h->SetLineColor(kGreen + 2);
                h->SetLineWidth(2);
                hs->Add(h); 
                leg->AddEntry(h, "AK15", "l");
            }
        }

        hs->Draw("NOSTACK HIST");
        hs->GetXaxis()->SetTitle(hist_title); // Label the X-axis
        leg->Draw();
    }

    c1->SaveAs(save_path);
}