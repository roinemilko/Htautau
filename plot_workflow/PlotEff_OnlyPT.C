#include <iostream>
#include <algorithm>
#include "TCanvas.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TGraphAsymmErrors.h"
#include "TString.h"
#include "TROOT.h"
#include <TLatex.h>
#include "eff_helpers.h"
#include "TProfile.h"
#include "TLine.h"

void PlotEff_OnlyPT(const char* save_path = "/eos/user/m/mroine/NanoTuples/Htautau/plot_workflow/plots/POWHEG",
    const char* fRaw  = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/POWHEG/RawEventInfo_hadhad.root",
    const char* fAK4  = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/POWHEG/Tau_hadhad.root",
    const char* fAK8  = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/POWHEG/fatJet_hadhad.root",
    const char* fAK15 = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/POWHEG/AK15_hadhad.root"
) {

    TH1::AddDirectory(kFALSE); 
    gStyle->SetOptStat(0);

    TEfficiency* pT_effAK8 = nullptr;
    TEfficiency* pT_effAK15 = nullptr;


    TCanvas* c1 = new TCanvas("c1", "", 2000, 800);
    c1->Divide(3, 1);

    auto drawEffPlot = [&](int padNum, const char* rawVar, const char* jetVar, const char* rawCut, const char* jetCut,
                           int nBins, float vMin, float vMax, const char* xAxisTitle, double yMax) {
        
        c1->cd(padNum);
        gPad->SetTopMargin(0.20);
        gPad->SetLeftMargin(0.20);
        gPad->SetRightMargin(0.10);
        gPad->SetBottomMargin(0.15);
        gPad->SetGrid(1, 1);

        TString hPrefix = Form("pad%d", padNum);

        TH1F* h_den = new TH1F(hPrefix + "_den",  "", nBins, vMin, vMax);
        TH1F* h_num_AK4 = new TH1F(hPrefix + "_ak4",  "", nBins, vMin, vMax);
        TH1F* h_num_AK8 = new TH1F(hPrefix + "_ak8",  "", nBins, vMin, vMax);
        TH1F* h_num_AK15 = new TH1F(hPrefix + "_ak15", "", nBins, vMin, vMax);

        ProjectFromTree(fRaw, h_den, rawVar, rawCut);
        ProjectFromTree(fAK4, h_num_AK4, jetVar, jetCut);
        ProjectFromTree(fAK8, h_num_AK8, jetVar, jetCut);
        ProjectFromTree(fAK15, h_num_AK15, jetVar, jetCut);

        TEfficiency* effAK4  = new TEfficiency(*h_num_AK4, *h_den);
        TEfficiency* effAK8  = new TEfficiency(*h_num_AK8, *h_den);
        TEfficiency* effAK15 = new TEfficiency(*h_num_AK15, *h_den);

        if (padNum == 1) {
            pT_effAK8 = effAK8;
            pT_effAK15 = effAK15;
        }

        effAK4->SetTitle(Form(";%s;Matching Efficiency", xAxisTitle));
    
        effAK4->SetMarkerStyle(20);
        effAK8->SetMarkerStyle(21);
        effAK15->SetMarkerStyle(22);

        effAK4->SetMarkerColor(kBlue);
        effAK8->SetMarkerColor(kRed);
        effAK15->SetMarkerColor(kGreen+2);

        effAK4->SetLineColor(kBlue);
        effAK8->SetLineColor(kRed);
        effAK15->SetLineColor(kGreen+2);

        effAK4->SetMarkerSize(0.85);
        effAK8->SetMarkerSize(0.85);
        effAK15->SetMarkerSize(0.85);
        
        effAK4->SetLineWidth(3);
        effAK8->SetLineWidth(3);
        effAK15->SetLineWidth(3);

        effAK4->Draw("APLE");
        gPad->Update(); 
        
        auto graphAK4 = effAK4->GetPaintedGraph();
        if (graphAK4) {
            graphAK4->GetYaxis()->SetRangeUser(0.0, yMax);
            graphAK4->GetXaxis()->SetRangeUser(vMin, vMax);
            graphAK4->GetYaxis()->SetTitleSize(0.05);
            graphAK4->GetXaxis()->SetTitleSize(0.05);
            graphAK4->GetXaxis()->SetLabelSize(0.045);
            graphAK4->GetYaxis()->SetLabelSize(0.045);
        }

        effAK8->Draw("PLE SAME");
        effAK15->Draw("PLE SAME");

        TLatex pad_latex;
        pad_latex.SetNDC();
        pad_latex.SetTextAlign(31); 
        pad_latex.SetTextFont(42);
        pad_latex.SetTextSize(0.045);
        pad_latex.DrawLatex(0.90, 0.81, "13.6 TeV");

        c1->cd(0);
        if (padNum == 1) {
            TLegend* leg = new TLegend(0.30, 0.82, 0.50, 0.97);
            leg->SetBorderSize(0);
            leg->SetFillStyle(0);
            leg->SetTextSize(0.035);
            leg->SetEntrySeparation(0.2);
            leg->AddEntry(effAK4, "AK4", "lpe");
            leg->AddEntry(effAK8, "AK8", "lpe");
            leg->AddEntry(effAK15, "AK15", "lpe");
            leg->Draw();
        }
    };

    std::cout << "pT plot..." << std::endl;
    drawEffPlot(1, "genH_pt_raw", "genH_pt", "", "", 25, 0.0, 1000.0, "Higgs p_{T} [GeV]", 1.0);


    std::cout << "Energy Response..." << std::endl;
    c1->cd(2);
    gPad->SetTopMargin(0.20);
    gPad->SetLeftMargin(0.20);
    gPad->SetRightMargin(0.10);
    gPad->SetBottomMargin(0.15);
    gPad->SetGrid(1, 1);

    TProfile* h_prof_AK4  = new TProfile("prof_ak4",  ";Higgs p_{T} [GeV];p_{T}^{Reco} / p_{T}^{Gen}", 15, 0.0, 1000.0);
    TProfile* h_prof_AK8  = new TProfile("prof_ak8",  ";Higgs p_{T} [GeV];p_{T}^{Reco} / p_{T}^{Gen}", 15, 0.0, 1000.0);
    TProfile* h_prof_AK15 = new TProfile("prof_ak15", ";Higgs p_{T} [GeV];p_{T}^{Reco} / p_{T}^{Gen}", 15, 0.0, 1000.0);

    ProjectFromTree(fAK4,  h_prof_AK4,  "(tau_pt[0] + tau_pt[1]) / genH_pt : genH_pt", "");
    ProjectFromTree(fAK8,  h_prof_AK8,  "fj_pt / genH_pt : genH_pt",                   "");
    ProjectFromTree(fAK15, h_prof_AK15, "ak15_pt / genH_pt : genH_pt",                 "");

    h_prof_AK4->SetLineColor(kBlue);     h_prof_AK4->SetMarkerColor(kBlue);     
    h_prof_AK8->SetLineColor(kRed);      h_prof_AK8->SetMarkerColor(kRed);      
    h_prof_AK15->SetLineColor(kGreen+2); h_prof_AK15->SetMarkerColor(kGreen+2); 

    h_prof_AK4->SetMarkerStyle(20);  h_prof_AK4->SetMarkerSize(0.85);  h_prof_AK4->SetLineWidth(3);
    h_prof_AK8->SetMarkerStyle(21);  h_prof_AK8->SetMarkerSize(0.85);  h_prof_AK8->SetLineWidth(3);
    h_prof_AK15->SetMarkerStyle(22); h_prof_AK15->SetMarkerSize(0.85); h_prof_AK15->SetLineWidth(3);

    h_prof_AK4->SetMinimum(0.5);
    h_prof_AK4->SetMaximum(1.1);

    h_prof_AK4->GetXaxis()->SetNdivisions(505);
    h_prof_AK4->GetYaxis()->SetTitleSize(0.05);
    h_prof_AK4->GetXaxis()->SetTitleSize(0.05);
    h_prof_AK4->GetXaxis()->SetLabelSize(0.045);
    h_prof_AK4->GetYaxis()->SetLabelSize(0.045);
    
    h_prof_AK4->Draw("L");       

    h_prof_AK4->Draw("PE SAME"); 
    h_prof_AK8->Draw("L SAME");  
    h_prof_AK8->Draw("PE SAME"); 
    h_prof_AK15->Draw("L SAME"); 
    h_prof_AK15->Draw("PE SAME");

    TLine* ref_line = new TLine(0.0, 0.55, 1000.0, 0.55);
    ref_line->SetLineWidth(4);
    ref_line->SetLineColor(kGray+2);
    ref_line->Draw("SAME");

    TLatex pad2_latex;
    pad2_latex.SetNDC();
    pad2_latex.SetTextAlign(31); 
    pad2_latex.SetTextFont(42);
    pad2_latex.SetTextSize(0.045);
    pad2_latex.DrawLatex(0.90, 0.81, "13.6 TeV"); 
    gPad->Update();

    std::cout << "Asym plot..." << std::endl;
    drawEffPlot(3, "genTau_pt_asym_raw", "genTau_pt_asym", "genH_pt_raw > 300", "genH_pt > 300", 25, 0.0, 1.0, "genTau_pt_asym (genH_pt > 300 GeV)", 1.0);

    c1->cd(0); 
    TLatex latex;
    latex.SetNDC();

    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.07, 0.93, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.07, 0.88, "Simulation Private");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.07, 0.83, "H #rightarrow #tau#tau (125 GeV)");

    c1->SaveAs(Form("%s.png", save_path)); 
    std::cout << "Done! Main plot saved to " << save_path << std::endl;


    if(pT_effAK8 && pT_effAK15) {
        std::cout << "Efficiency Ratio Plot..." << std::endl;
        TCanvas* c2 = new TCanvas("c2", "Efficiency Ratio", 800, 800);
        c2->cd();
        gPad->SetTopMargin(0.10);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        gPad->SetBottomMargin(0.15);
        gPad->SetGrid(1, 1);

        int nBins = pT_effAK8->GetTotalHistogram()->GetNbinsX();
        TGraphAsymmErrors* gr_ratio = new TGraphAsymmErrors(nBins);
        
        for(int i = 1; i <= nBins; ++i) {
            double e8 = pT_effAK8->GetEfficiency(i);
            double e15 = pT_effAK15->GetEfficiency(i);
            double pt = pT_effAK8->GetTotalHistogram()->GetBinCenter(i);
            double ptErr = pT_effAK8->GetTotalHistogram()->GetBinWidth(i) / 2.0;

            if (e8 > 0) {
                double r = e15 / e8;
                
                double err8 = (pT_effAK8->GetEfficiencyErrorUp(i) + pT_effAK8->GetEfficiencyErrorLow(i)) / 2.0;
                double err15 = (pT_effAK15->GetEfficiencyErrorUp(i) + pT_effAK15->GetEfficiencyErrorLow(i)) / 2.0;
                
                double relErr = sqrt(pow(err8 / e8, 2) + pow(err15 / e15, 2));
                double absErr = r * relErr;
                
                gr_ratio->SetPoint(i-1, pt, r);
                gr_ratio->SetPointError(i-1, ptErr, ptErr, absErr, absErr);
            } else {
                gr_ratio->SetPoint(i-1, pt, 0);
                gr_ratio->SetPointError(i-1, ptErr, ptErr, 0, 0);
            }
        }

        gr_ratio->SetTitle(";Higgs p_{T} [GeV];#varepsilon_{AK15} / #varepsilon_{AK8}");
        gr_ratio->SetMarkerStyle(20);
        gr_ratio->SetMarkerColor(kBlack);
        gr_ratio->SetLineColor(kBlack);
        gr_ratio->SetLineWidth(3);
        gr_ratio->SetMarkerSize(1.2);
        
        gr_ratio->Draw("APLE");
        
        gr_ratio->GetYaxis()->SetRangeUser(0.95, 3.5); 
        gr_ratio->GetXaxis()->SetRangeUser(300, 1000);
        gr_ratio->GetYaxis()->SetTitleSize(0.05);
        gr_ratio->GetYaxis()->SetTitleOffset(1.2);
        gr_ratio->GetXaxis()->SetTitleSize(0.05);
        gr_ratio->GetXaxis()->SetLabelSize(0.045);
        gr_ratio->GetYaxis()->SetLabelSize(0.045);

        TLatex c2_latex;
        c2_latex.SetNDC();
        
        c2_latex.SetTextFont(62);
        c2_latex.SetTextSize(0.05);
        c2_latex.DrawLatex(0.15, 0.92, "CMS");

        c2_latex.SetTextFont(52);
        c2_latex.SetTextSize(0.04);
        c2_latex.DrawLatex(0.26, 0.92, "Simulation Private");

        c2_latex.SetTextFont(42);
        c2_latex.SetTextSize(0.04);
        c2_latex.DrawLatex(0.18, 0.83, "H #rightarrow #tau#tau (125 GeV)");
        
        c2_latex.SetTextAlign(31); 
        c2_latex.DrawLatex(0.95, 0.92, "13.6 TeV");
        
        TString ratio_save_path = Form("%s_ratio.png", save_path);
        c2->SaveAs(ratio_save_path);
        std::cout << "Done! Ratio plot saved to " << ratio_save_path << std::endl;
    }
}