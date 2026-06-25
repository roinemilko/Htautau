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
#include <TLatex.h>
#include <TStyle.h>


void Plots(bool AK4 = true, bool AK8 = true, bool AK15 = true, 
        const char* params = "X_mass/X_pt, X_eta",
        const char* save_path = "/eos/user/m/mroine/www/",
        bool normalize = true,
        bool require_hadhad = true) {

    TString paramStr(params);
    TObjArray* paramArray = paramStr.Tokenize(",");
    int nParams = paramArray->GetEntries();

    TString hadhad_str = require_hadhad ? "_hadhad" : "";

    if (nParams == 0) {
        std::cerr << "Nothing to plot" << std::endl;
        return;
    }

    TFile *AK4_f = nullptr, *AK8_f = nullptr, *AK15_f = nullptr;
    TTree *AK4_tree = nullptr, *AK8_tree = nullptr, *AK15_tree = nullptr;

    if (AK4) {
        AK4_f = new TFile("jets/Jet" + hadhad_str + ".root", "READ");
        if (AK4_f) {AK4_tree = (TTree*)AK4_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file Jet.root" << std::endl;
        }
    }

    if (AK8) {
        AK8_f = new TFile("jets/fatJet" + hadhad_str + ".root", "READ");
        if (AK8_f) {AK8_tree = (TTree*)AK8_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file fatJet.root" << std::endl;
        }
    }

    if (AK15) {
        AK15_f = new TFile("jets/AK15" + hadhad_str + ".root", "READ");
        if (AK15_f) {AK15_tree = (TTree*)AK15_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file AK15.root" << std::endl;
        }
    }
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    // https://root.cern.ch/doc/master/classTCanvas.html
    TCanvas* c1 = new TCanvas("c1", "Jet plots", 400 * nParams, 450);
    c1->Divide(nParams, 1); 

    TLegend* leg = new TLegend(0.50, 0.82, 0.98, 0.98); 
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.035);
    leg->SetTextFont(42);
    leg->SetMargin(0.25); 

    for (int i = 0; i < nParams; ++i) {
        c1->cd(i + 1); 
        gPad->SetTopMargin(0.20);

        TString baseExpr = ((TObjString*)paramArray->At(i))->GetString();
        baseExpr.ReplaceAll(" ", ""); 

        TString hist_title = baseExpr;
        hist_title.ReplaceAll("X_", "");
        hist_title.ReplaceAll("_", " ");
        hist_title.ReplaceAll("pt", "p_{T}");
        hist_title.ReplaceAll("eta", "#eta");
        hist_title.ReplaceAll("phi", "#phi");
        hist_title.ReplaceAll("minus", " - ");
        hist_title.ReplaceAll("over", " / ");



        if (baseExpr.Contains("pt") || baseExpr.Contains("mass") || baseExpr.Contains("sumEt")) {
            if (!baseExpr.Contains("over") && !baseExpr.Contains("/")) {
                hist_title += " [GeV]";
            }
        }
        
        // https://root.cern.ch/doc/master/classTHStack.html
        THStack* hs = new THStack(Form("hs_%d", i), "");


        if (AK4 && AK4_tree) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "ak4"); 
            
            // Unique name for every hist
            TString hName = Form("h4_%d", i);
            if (AK4_tree->Draw(expr + ">>" + hName, "") == -1) {
                std::cerr << "Parameter " << expr << " doesn't exist!"  << std::endl;
                return;
            };
            TH1F* h = (TH1F*)gDirectory->Get(hName);

            if (h) {
                h->SetLineColor(kBlue);
                h->SetLineWidth(2);
                if (normalize) {h->Scale(1./h->Integral());}
                hs->Add(h);
                if (i == 0) {leg->AddEntry(h, "Anti k_{T}, R = 0.4, p_{T} > 30 GeC, |#eta| < 2.5", "l");}
            }
        }


        if (AK8 && AK8_tree) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "fj"); 
            
            TString hName = Form("h8_%d", i);

            if (AK8_tree->Draw(expr + ">>" + hName, "") == -1) {
                std::cerr << "Parameter " << expr << " doesn't exist!"  << std::endl;
                return;
            };
            
            TH1F* h = (TH1F*)gDirectory->Get(hName);
            if (h) {
                h->SetLineColor(kRed);
                h->SetLineWidth(2);
                if (normalize) {h->Scale(1./h->Integral());}
                hs->Add(h);
                if (i == 0) {leg->AddEntry(h, "Anti k_{T}, R = 0.8, p_{T} > 200 GeV, |#eta| < 2.5", "l");}
            }
        }

        if (AK15 && AK15_tree) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "ak15"); 
            
            TString hName = Form("h15_%d", i);
            if (AK15_tree->Draw(expr + ">>" + hName, "") == -1) {
                std::cerr << "Parameter " << expr << " doesn't exist!"  << std::endl;
                return;
            };
            
            TH1F* h = (TH1F*)gDirectory->Get(hName);
            if (h) {
                h->SetLineColor(kGreen + 2);
                h->SetLineWidth(2);
                if (normalize) {h->Scale(1./h->Integral());}
                hs->Add(h); 
                if (i == 0) {leg->AddEntry(h, "Anti k_{T}, R = 1.5, p_{T} > 150 GeV, |#eta| < 2.5", "l");}
            }
        }


        gPad->SetLeftMargin(0.15);
        hs->Draw("NOSTACK HIST");
        gPad->Update();
        hs->GetXaxis()->SetTitle(hist_title); 

        if (normalize) {
            hs->GetYaxis()->SetTitle("Fraction of jets");
        } else {
            hs->GetYaxis()->SetTitle("# of jets");
        }
        hs->GetYaxis()->SetTitleOffset(2.5);
        hs->GetXaxis()->SetRangeUser(0, 1000);
    }

    c1->cd(0); 

    leg->Draw();

    TLatex latex;
    latex.SetNDC();

    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.07, 0.93, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.07, 0.88, "Simulation, Work in Progress");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.07, 0.83, "H #rightarrow #tau#tau (125 GeV)");

    c1->SaveAs(save_path);
}