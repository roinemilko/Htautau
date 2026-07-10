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

void PlotEff_OnlyPT(const char* save_path = "/eos/user/m/mroine/www",
    const char* fRaw  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/RawEventInfo_hadhad.root",
    const char* fAK4  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/Jet_hadhad.root",
    const char* fAK8  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/fatJet_hadhad.root",
    const char* fAK15 = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/AK15_hadhad.root"
) {

    TH1::AddDirectory(kFALSE); 
    gStyle->SetOptStat(0);

    TCanvas* c1 = new TCanvas("c1", "", 1600, 800);
    c1->Divide(3, 1);

    auto drawEffPlot = [&](int padNum, const char* rawVar, const char* jetVar, const char* rawCut, const char* jetCut,
                           int nBins, float vMin, float vMax, const char* xAxisTitle, double yMax) {
        
        c1->cd(padNum);
        gPad->SetTopMargin(0.20);

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

        effAK4->SetTitle(Form(";%s;Matching Efficiency", xAxisTitle));
    
        effAK4->SetMarkerStyle(20);
        effAK8->SetMarkerStyle(20);
        effAK15->SetMarkerStyle(20);

        effAK4->SetMarkerColor(kBlue);
        effAK8->SetMarkerColor(kRed);
        effAK15->SetMarkerColor(kGreen+2);

        effAK4->SetMarkerSize(0.7);
        effAK8->SetMarkerSize(0.7);
        effAK15->SetMarkerSize(0.7);

        effAK4->Draw("AP");
        gPad->Update(); 
        auto graphAK4 = effAK4->GetPaintedGraph();
        if (graphAK4) {
            graphAK4->GetYaxis()->SetRangeUser(0.0, yMax);
            graphAK4->GetXaxis()->SetRangeUser(vMin, vMax);
        }

        effAK8->Draw("P SAME");
        effAK15->Draw("P SAME");

        c1->cd(0);
        if (padNum == 1) {
            TLegend* leg = new TLegend(0.40, 0.82, 0.88, 0.97);
            leg->SetBorderSize(0);
            leg->SetFillStyle(0);
            leg->SetTextSize(0.035);
            leg->SetEntrySeparation(0.2);
            leg->AddEntry(effAK4, "Anti k_{T}, R = 0.4, p_{T} > 30 GeV, |#eta| < 2.5", "lp");
            leg->AddEntry(effAK8, "Anti k_{T}, R = 0.8, p_{T} > 200 GeV, |#eta| < 2.5", "lp");
            leg->AddEntry(effAK15, "Anti k_{T}, R = 1.5, p_{T} > 150 GeV, |#eta| < 2.5", "lp");
            leg->Draw();
        }
    };

    // ---------------------------------------------------------
    // PAD 1: pT Efficiency
    // ---------------------------------------------------------
    std::cout << "Generating Pad 1 (pT plot)..." << std::endl;
    drawEffPlot(1, "genH_pt_raw", "genH_pt", "", "", 50, 0.0, 800.0, "genH_pt [GeV]", 1.0);

std::cout << "Generating Pad 2 (Energy Response Profile)..." << std::endl;
    c1->cd(2);
    gPad->SetTopMargin(0.20);
    
    // 1. Create TProfiles (25 bins from 0 to 1000 GeV to match Pad 1)
    TProfile* h_prof_AK4  = new TProfile("prof_ak4",  ";genH_pt [GeV];p_{T}^{Reco} / p_{T}^{Gen}", 50, 0.0, 800.0);
    TProfile* h_prof_AK8  = new TProfile("prof_ak8",  ";genH_pt [GeV];p_{T}^{Reco} / p_{T}^{Gen}", 50, 0.0, 800.0);
    TProfile* h_prof_AK15 = new TProfile("prof_ak15", ";genH_pt [GeV];p_{T}^{Reco} / p_{T}^{Gen}", 50, 0.0, 800.0);

    // 2. Project using "Y:X" syntax. (No cuts, so we see the whole spectrum!)
    ProjectFromTree(fAK4,  h_prof_AK4,  "(ak4_pt[0] + ak4_pt[1]) / genH_pt : genH_pt", "");
    ProjectFromTree(fAK8,  h_prof_AK8,  "fj_pt / genH_pt : genH_pt",                   "");
    ProjectFromTree(fAK15, h_prof_AK15, "ak15_pt / genH_pt : genH_pt",                 "");

    // 3. Style the profiles (Dots and lines like the reference plot)
    h_prof_AK4->SetLineColor(kBlue);     h_prof_AK4->SetMarkerColor(kBlue);     
    h_prof_AK8->SetLineColor(kRed);      h_prof_AK8->SetMarkerColor(kRed);      
    h_prof_AK15->SetLineColor(kGreen+2); h_prof_AK15->SetMarkerColor(kGreen+2); 

    h_prof_AK4->SetMarkerStyle(20);      h_prof_AK4->SetMarkerSize(0.7);
    h_prof_AK8->SetMarkerStyle(20);      h_prof_AK8->SetMarkerSize(0.7);
    h_prof_AK15->SetMarkerStyle(20);     h_prof_AK15->SetMarkerSize(0.7);

    // 4. Set fixed Y-axis range (0 to 1.2 is standard for response plots)
    h_prof_AK4->SetMinimum(0.0);
    h_prof_AK4->SetMaximum(1.2);

    // 5. Draw with "PE" (Points with Error bars)
    h_prof_AK4->Draw("PE");
    h_prof_AK8->Draw("PE SAME");
    h_prof_AK15->Draw("PE SAME");


    // ---------------------------------------------------------
    // PAD 3: Kinematic Asymmetry
    // ---------------------------------------------------------
    std::cout << "Generating Pad 3 (Asym plot)..." << std::endl;
    drawEffPlot(3, "genTau_pt_asym_raw", "genTau_pt_asym", "genH_pt_raw > 200", "genH_pt > 200", 50, 0.0, 1.0, "genTau_pt_asym (genH_pt > 200 GeV)", 1.0);

    // ---------------------------------------------------------
    // Draw CMS Text on the whole Canvas
    // ---------------------------------------------------------
    c1->cd(0); 
    TLatex latex;
    latex.SetNDC();

    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.03, 0.93, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.03, 0.88, "Simulation, Work in Progress");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.03, 0.83, "H #rightarrow #tau#tau (125 GeV)");

    c1->SaveAs(TString(save_path) + "/JetFatJetAK15_eff_with_Response.png"); 
    std::cout << "Done! Saved to " << save_path << "/JetFatJetAK15_eff_with_Response.png" << std::endl;
}