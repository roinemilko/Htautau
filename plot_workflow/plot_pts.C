#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <string>
#include <TH1F.h>
#include<TF1.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TLine.h>

void plot_pts(const char* ak4_data = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/Jet_hadhad.root",
    const char* ak8_data = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/fatJet_hadhad.root",
    const char* ak15_data = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/AK15_hadhad.root") {

    gStyle->SetOptStat(0);

    TFile* AK4_f = new TFile(ak4_data, "READ");
    TFile* AK8_f = new TFile(ak8_data, "READ");
    TFile* AK15_f = new TFile(ak15_data, "READ");

    TTree* AK4_tree = (TTree*)AK4_f->Get("Events");
    TTree* AK8_tree = (TTree*)AK8_f->Get("Events");
    TTree* AK15_tree = (TTree*)AK15_f->Get("Events");

    TCanvas* c1 = new TCanvas("c1", "Uncutted jet pT Distributions", 800, 600);

    TH1F* h_ak4 = new TH1F("h_ak4", ";p_{T} [GeV];Fraction of jets", 700, 0, 700);
    TH1F* h_ak8 = new TH1F("h_ak8", ";p_{T} [GeV];Fraction of jets", 700, 0, 700);
    TH1F* h_ak15 = new TH1F("h_ak15", " ;p_{T} [GeV];Fraction of jets", 700, 0, 700);
 
    h_ak4->SetLineColor(kBlue);
    h_ak8->SetLineColor(kRed);
    h_ak15->SetLineColor(kGreen+2);
    h_ak4->SetLineWidth(2);
    h_ak8->SetLineWidth(2);
    h_ak15->SetLineWidth(2);

    AK4_tree->Draw("ak4_pt >> h_ak4", "", "goff");
    AK8_tree->Draw("fj_pt >> h_ak8", "", "goff");
    AK15_tree->Draw("ak15_pt >> h_ak15", "", "goff");

    h_ak4->Scale(1.f/h_ak4->Integral());
    h_ak8->Scale(1.f/h_ak8->Integral());
    h_ak15-> Scale(1.f/h_ak15->Integral());

    h_ak4->Draw("HIST");
    h_ak4->GetYaxis()->SetTitleOffset(1.6);
    h_ak8->Draw("HIST SAME");
    h_ak15->Draw("HIST SAME");



    TString formula = "[0] * exp([1]*x) * (1.0 + TMath::Erf((x - [2])/[3]))";

    TF1* fit_experf_ak4 = new TF1("fit_ak4", formula, 10, 200);
    fit_experf_ak4->SetParameters(0.05, -0.05, 50.0, 20.0); 
    fit_experf_ak4->SetLineColor(kBlack);
    fit_experf_ak4->SetLineWidth(2);
    h_ak4->Fit(fit_experf_ak4, "R"); 

    fit_experf_ak4->Draw("SAME");

    TF1* fit_experf_ak8 = new TF1("fit_ak8", formula, 170, 600);
    fit_experf_ak8->SetParameters(0.02, -0.007, 220.0, 20.00); 
    fit_experf_ak8->SetLineColor(kBlack);
    fit_experf_ak8->SetLineWidth(2);
    h_ak8->Fit(fit_experf_ak8, "R");

    fit_experf_ak8->Draw("SAME");

    TLatex latex;
    latex.SetNDC();

    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.18, 0.85, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.18, 0.80, "Simulation, Work in Progress");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.18, 0.74, "H #rightarrow #tau#tau (125 GeV)");

    TLegend* leg = new TLegend(0.55, 0.5, 0.88, 0.88); 
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);  
    leg->SetTextSize(0.035);   
    leg->SetTextFont(42);
    leg->SetMargin(0.15);
    leg->AddEntry(h_ak4, "Anti-k_{T}, R = 0.4, |#eta| < 2.5" , "l");
    leg->AddEntry(h_ak8, "Anti-k_{T}, R = 0.8, |#eta| < 2.5", "l");
    leg->AddEntry(h_ak15, "Anti-k_{T}, R = 1.5, |#eta| < 2.5", "l");
    leg->AddEntry(fit_experf_ak4, "Exp * erf -fit", "l");
    leg->SetBorderSize(0); 
    leg->Draw();

    
    c1->SaveAs("/eos/user/m/mroine/www/Jet_FatJet_AK15_pt_nocut_nice.png");
}

