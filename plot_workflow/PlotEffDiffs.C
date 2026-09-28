#include "TCanvas.h"
#include "TFile.h"
#include "TH1F.h"
#include "TEfficiency.h"
#include "TGraphAsymmErrors.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TString.h"
#include "TStyle.h"
#include "TROOT.h"
#include "eff_helpers.h"
#include <iostream>
#include <cmath>

TGraphAsymmErrors* EffDifference(
    TEfficiency* effInc,
    TEfficiency* effHad,
    const char* name
) {
    const int n = effInc->GetTotalHistogram()->GetNbinsX();
    auto* g = new TGraphAsymmErrors(n);
    g->SetName(name);

    for (int i = 1; i <= n; ++i) {
        const double x  = effInc->GetTotalHistogram()->GetBinCenter(i);
        const double yI = effInc->GetEfficiency(i);
        const double yH = effHad->GetEfficiency(i);
        const double eI = effInc->GetEfficiencyErrorUp(i);
        const double eH = effHad->GetEfficiencyErrorUp(i);

        g->SetPoint(i - 1, x, yI - yH);
        g->SetPointError(i - 1, 0, 0,
                         std::sqrt(eI * eI + eH * eH),
                         std::sqrt(eI * eI + eH * eH));
    }
    return g;
}

TGraphAsymmErrors* MakeJetEffDiff(
    const char* prefix,
    const char* rawHad,
    const char* rawInc,
    const char* jetHad,
    const char* jetInc,
    const char* rawVar,
    const char* jetVar,
    int nBins,
    float vMin,
    float vMax,
    int color,
    int marker
) {
    TH1F* h_den_had = new TH1F(TString(prefix) + "_den_had", "", nBins, vMin, vMax);
    TH1F* h_den_inc = new TH1F(TString(prefix) + "_den_inc", "", nBins, vMin, vMax);
    TH1F* h_num_had = new TH1F(TString(prefix) + "_num_had", "", nBins, vMin, vMax);
    TH1F* h_num_inc = new TH1F(TString(prefix) + "_num_inc", "", nBins, vMin, vMax);

    ProjectFromTree(rawHad, h_den_had, rawVar, "");
    ProjectFromTree(rawInc, h_den_inc, rawVar, "");
    ProjectFromTree(jetHad, h_num_had, jetVar, "");
    ProjectFromTree(jetInc, h_num_inc, jetVar, "");

    TEfficiency* eff_had = new TEfficiency(*h_num_had, *h_den_had);
    TEfficiency* eff_inc = new TEfficiency(*h_num_inc, *h_den_inc);

    TGraphAsymmErrors* g = EffDifference(eff_inc, eff_had, TString(prefix) + "_diff");
    g->SetMarkerStyle(marker);
    g->SetMarkerColor(color);
    g->SetLineColor(color);
    g->SetMarkerSize(0.8);

    std::cout << prefix << ": plotted difference" << std::endl;
    return g;
}

#include "TGraph.h"

TEfficiency* BuildHadEff(
    const char* prefix,
    const char* rawHad,
    const char* jetHad,
    const char* rawVar,
    const char* jetVar,
    int nBins, float vMin, float vMax
) {
    TH1F* h_den = new TH1F(TString(prefix) + "_den_had", "", nBins, vMin, vMax);
    TH1F* h_num = new TH1F(TString(prefix) + "_num_had", "", nBins, vMin, vMax);

    ProjectFromTree(rawHad, h_den, rawVar, "");
    ProjectFromTree(jetHad, h_num, jetVar, "");

    return new TEfficiency(*h_num, *h_den);
}

TGraph* PredictEffDiffFromHad(
    TEfficiency* eHad,
    double s,
    const char* name,
    double xMin,
    double xMax,
    double jet_radius,
    double xStep = 5.0
) {
    const TH1* hTot = eHad->GetTotalHistogram();

    TGraph* gHad = new TGraph();
    int nPts = 0;
    for (int i = 1; i <= hTot->GetNbinsX(); ++i) {
        if (hTot->GetBinContent(i) < 5) continue;
        gHad->SetPoint(nPts++,
                       hTot->GetBinCenter(i),
                       eHad->GetEfficiency(i));
    }

    auto eval = [&](double x) -> double {
        if (nPts == 0) return 0.0;
        double x0 = gHad->GetX()[0];
        double xN = gHad->GetX()[nPts - 1];
        double yN = gHad->GetY()[nPts - 1];
        if (x <= x0) return 0.0;
        if (x >= xN) return yN;
        return gHad->Eval(x);
    };

    TGraph* gPred = new TGraph();

    int n = 0;
    for (double x = xMin; x <= xMax; x += xStep) {
        double sq_term = (125.0) / (jet_radius * x);
        double env_term = std::sqrt(
            1 - 4 * (sq_term * sq_term)
        );
        gPred->SetPoint(n++, x, 0.46  * env_term * (eval(s * x) - eval(x)));
    }
    gPred->SetName(name);
    return gPred;
}

void PlotEffDiffs(
    const char* save_path = "/eos/user/m/mroine/NanoTuples/Htautau/plot_workflow/plots/MADGRAPH",
    const char* jet_path  = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/MADGRAPH"
) {
    gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);

    const int nBins = 100;
    const float vMin = 0.f;
    const float vMax = 800.f;

    TString rawHad = jet_path + TString("/RawEventInfo_hadhad.root");
    TString rawInc = jet_path + TString("/RawEventInfo.root");

    TString gAK8_path_hadhad = TString(jet_path) + "/fatJet_hadhad.root";
    TString gAK8_path = TString(jet_path) + "/fatJet.root";
    TGraphAsymmErrors* gAK8 = MakeJetEffDiff(
        "ak8", rawHad, rawInc, gAK8_path_hadhad, gAK8_path, "genH_pt_raw", "genH_pt", nBins, vMin, vMax, kBlack, 20
    );

    TString gAK15_path_hadhad = TString(jet_path) + "/AK15_hadhad.root";
    TString gAK15_path = TString(jet_path) + "/AK15.root";
    TGraphAsymmErrors* gAK15 = MakeJetEffDiff(
        "ak15", rawHad, rawInc, gAK15_path_hadhad, gAK15_path, "genH_pt_raw", "genH_pt", nBins, vMin, vMax, kBlack, 20
    );
  
    const double s = 0.75; 

    TEfficiency* eHadAK8  = BuildHadEff("ak8_had",  rawHad, gAK8_path_hadhad,  "genH_pt_raw", "genH_pt", nBins, vMin, vMax);
    TEfficiency* eHadAK15 = BuildHadEff("ak15_had", rawHad, gAK15_path_hadhad, "genH_pt_raw", "genH_pt", nBins, vMin, vMax);

    TGraph* pAK8  = PredictEffDiffFromHad(eHadAK8,  s, "pred_ak8",  (2 * 125) / 0.8, vMax, 0.8);
    TGraph* pAK15 = PredictEffDiffFromHad(eHadAK15, s, "pred_ak15", (2 * 125) / 1.5, vMax, 1.5);

    pAK8 ->SetLineColor(kBlue);  pAK8 ->SetLineStyle(1); pAK8 ->SetLineWidth(2);
    pAK15->SetLineColor(kBlue); pAK15->SetLineStyle(1); pAK15->SetLineWidth(2);

    // Create a wide canvas to hold two side-by-side pads
    TCanvas c("c_diff_eff", "", 1200, 600);
    c.Divide(2, 1);

    // Helper lambda to draw CMS styling text in each pad
    auto drawCMSLabels = [](const char* jetLabel) {
        TLatex latex;
        latex.SetNDC();
        latex.SetTextFont(62);
        latex.SetTextSize(0.045);
        latex.DrawLatex(0.12, 0.92, "CMS");
        
        latex.SetTextFont(52);
        latex.SetTextSize(0.045);
        latex.DrawLatex(0.22, 0.92, "Simulation Private");
        
        latex.SetTextAlign(31); 
        latex.SetTextFont(42);
        latex.SetTextSize(0.045);
        latex.DrawLatex(0.92, 0.92, "13.6 TeV");
        
        // Draw the Jet label (AK8/AK15) in the top right, inside the plot frame
        latex.SetTextAlign(31);
        latex.DrawLatex(0.88, 0.83, jetLabel);
    };

    // --- Pad 1: AK8 ---
    c.cd(1);
    gPad->SetTopMargin(0.10);
    gPad->SetRightMargin(0.06);
    gPad->SetLeftMargin(0.12);

    gAK8->SetTitle(";genH p_{T} [GeV];#Delta Matching Efficiency");
    gAK8->Draw("APLE"); 
    gAK8->GetYaxis()->SetRangeUser(-0.3, 0.1); // Scaled for delta efficiency range
    gAK8->GetXaxis()->SetRangeUser(0, 800);
    gAK8->GetYaxis()->SetTitleOffset(1.3);

    pAK8->Draw("L SAME");

    TLine zero8(0, 0, 800, 0);
    zero8.SetLineStyle(2);
    zero8.Draw("same");

    drawCMSLabels("AK8");

    TLegend leg8(0.40, 0.15, 0.88, 0.30);
    leg8.SetBorderSize(0);
    leg8.SetFillStyle(0);
    leg8.SetTextSize(0.04);
    leg8.AddEntry(gAK8, "Simulation", "pe");
    leg8.AddEntry(pAK8, "Prediction", "l");
    leg8.Draw();

    // --- Pad 2: AK15 ---
    c.cd(2);
    gPad->SetTopMargin(0.10);
    gPad->SetRightMargin(0.06);
    gPad->SetLeftMargin(0.12);

    gAK15->SetTitle(";genH p_{T} [GeV];#Delta Matching Efficiency");
    gAK15->Draw("APLE");
    gAK15->GetYaxis()->SetRangeUser(-0.3, 0.1);
    gAK15->GetXaxis()->SetRangeUser(0, 800);
    gAK15->GetYaxis()->SetTitleOffset(1.3);

    pAK15->Draw("L SAME");

    TLine zero15(0, 0, 800, 0);
    zero15.SetLineStyle(2);
    zero15.Draw("same");

    drawCMSLabels("AK15");

    TLegend leg15(0.40, 0.15, 0.88, 0.30);
    leg15.SetBorderSize(0);
    leg15.SetFillStyle(0);
    leg15.SetTextSize(0.04);
    leg15.AddEntry(gAK15, "Simulation", "pe");
    leg15.AddEntry(pAK15, "Prediction", "l");
    leg15.Draw();

    c.SaveAs(TString(save_path) + "/MatchingEffDiff_inclusive_minus_hadhad_allJets.png");
}