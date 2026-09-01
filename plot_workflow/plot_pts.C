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

void plot_pts(const char* ak4_data = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/MADGRAPH/Tau.root",
    const char* ak8_data = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/MADGRAPH/fatJet.root",
    const char* ak15_data = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/MADGRAPH/AK15.root") {

    gStyle->SetOptStat(0);

    TFile* AK4_f = new TFile(ak4_data, "READ");
    TFile* AK8_f = new TFile(ak8_data, "READ");
    TFile* AK15_f = new TFile(ak15_data, "READ");

    TTree* AK4_tree = (TTree*)AK4_f->Get("Events");
    TTree* AK8_tree = (TTree*)AK8_f->Get("Events");
    TTree* AK15_tree = (TTree*)AK15_f->Get("Events");

    TCanvas* c1 = new TCanvas("c1", "Uncutted jet pT Distributions", 800, 600);

    c1->SetTopMargin(0.20);
    c1->SetBottomMargin(0.13); 
    c1->SetLeftMargin(0.13);
    c1->SetRightMargin(0.05);

    TH1F* h_ak4 = new TH1F("h_ak4", ";p_{T} [GeV];A.u.", 50, 0, 700);
    TH1F* h_ak8 = new TH1F("h_ak8", ";p_{T} [GeV];A.u.", 50, 0, 700);
    TH1F* h_ak15 = new TH1F("h_ak15", " ;p_{T} [GeV];A.u.", 50, 0, 700);
 
    h_ak4->SetLineColor(kBlue);
    h_ak8->SetLineColor(kRed);
    h_ak15->SetLineColor(kGreen+2);
    h_ak4->SetLineWidth(4);
    h_ak8->SetLineWidth(4);
    h_ak15->SetLineWidth(4);

    AK4_tree->Draw("tau_pt >> h_ak4", "", "goff");
    AK8_tree->Draw("fj_pt >> h_ak8", "", "goff");
    AK15_tree->Draw("ak15_pt >> h_ak15", "", "goff");

    h_ak4->Scale(1.f/h_ak4->Integral());
    h_ak8->Scale(1.f/h_ak8->Integral());
    h_ak15-> Scale(1.f/h_ak15->Integral());

    h_ak4->Draw("HIST");

    h_ak4->GetYaxis()->SetTitleSize(0.045);
    h_ak4->GetXaxis()->SetTitleSize(0.045);


    h_ak8->Draw("HIST SAME");
    h_ak15->Draw("HIST SAME");


    TLatex latex;
    latex.SetNDC();

    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.15, 0.92, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.14, 0.87, "Simulation Private");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.15, 0.82, "H #rightarrow #tau#tau (125 GeV)");

    latex.SetTextAlign(31);
    latex.SetTextFont(42);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.96, 0.82, "13.6 TeV");

    TLegend* leg = new TLegend(0.55, 0.35, 0.88, 0.73); 
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);  
    leg->SetTextSize(0.035);   
    leg->SetTextFont(42);
    leg->SetMargin(0.15);
    leg->AddEntry(h_ak4, "Anti-k_{T}, R = 0.4, |#eta| < 2.5" , "l");
    leg->AddEntry(h_ak8, "Anti-k_{T}, R = 0.8, |#eta| < 2.5", "l");
    leg->AddEntry(h_ak15, "Anti-k_{T}, R = 1.5, |#eta| < 2.5", "l");
    leg->SetBorderSize(0); 
    leg->Draw();

    
    c1->SaveAs("/eos/user/m/mroine/NanoTuples/Htautau/plot_workflow/plots/MADGRAPH/Jet_FatJet_AK15_pt_nocut_nice.png");
}

