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
    g->SetMarkerSize(0.7);

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
    const char* save_path = "/eos/user/m/mroine/www/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8",
    const char* jet_path  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8"
) {
    gStyle->SetOptStat(0);

    const int nBins = 100;
    const float vMin = 0.f;
    const float vMax = 800.f;

    TString rawHad = jet_path + TString("/RawEventInfo_hadhad.root");
    TString rawInc = jet_path + TString("/RawEventInfo.root");

    TString gAK4_path_hadhad = TString(jet_path) + "/Jet_hadhad.root";
    TString gAK4_path = TString(jet_path) + "/Jet.root";
    TGraphAsymmErrors* gAK4 = MakeJetEffDiff(
        "ak4",
        rawHad, rawInc,
        gAK4_path_hadhad, gAK4_path,
        "genH_pt_raw", "genH_pt",
        nBins, vMin, vMax, kBlue, 20
    );

    TString gAK8_path_hadhad = TString(jet_path) + "/fatJet_hadhad.root";
    TString gAK8_path = TString(jet_path) + "/fatJet.root";
    TGraphAsymmErrors* gAK8 = MakeJetEffDiff(
        "ak8",
        rawHad, rawInc,
        gAK8_path_hadhad, gAK8_path,
        "genH_pt_raw", "genH_pt",
        nBins, vMin, vMax, kRed, 20
    );

    TString gAK15_path_hadhad = TString(jet_path) + "/AK15_hadhad.root";
    TString gAK15_path = TString(jet_path) + "/AK15.root";
    TGraphAsymmErrors* gAK15 = MakeJetEffDiff(
        "ak15",
        rawHad, rawInc,
        gAK15_path_hadhad, gAK15_path,
        "genH_pt_raw", "genH_pt",
        nBins, vMin, vMax, kGreen + 2, 20
    );


    const double s = 0.77;

    TEfficiency* eHadAK4  = BuildHadEff("ak4_had",  rawHad, gAK4_path_hadhad,  "genH_pt_raw", "genH_pt", nBins, vMin, vMax);
    TEfficiency* eHadAK8  = BuildHadEff("ak8_had",  rawHad, gAK8_path_hadhad,  "genH_pt_raw", "genH_pt", nBins, vMin, vMax);
    TEfficiency* eHadAK15 = BuildHadEff("ak15_had", rawHad, gAK15_path_hadhad, "genH_pt_raw", "genH_pt", nBins, vMin, vMax);

    TGraph* pAK8  = PredictEffDiffFromHad(eHadAK8,  s, "pred_ak8",  (2 * 125) / 0.8, vMax, 0.8);
    TGraph* pAK15 = PredictEffDiffFromHad(eHadAK15, s, "pred_ak15", (2 * 125) / 1.5, vMax, 1.5);

    pAK8 ->SetLineColor(kRed);     pAK8 ->SetLineStyle(2); pAK8 ->SetLineWidth(2);
    pAK15->SetLineColor(kGreen+2); pAK15->SetLineStyle(2); pAK15->SetLineWidth(2);



    TCanvas c("c_diff_eff", "", 900, 700);
    gPad->SetTopMargin(0.12);

    gAK4->SetTitle(";genH_pt [GeV];#Delta Matching Efficiency");
    gAK4->Draw("AP");
    gAK4->GetYaxis()->SetRangeUser(-0.3, 0.3);
    gAK4->GetXaxis()->SetRangeUser(0, 800);

    gAK8->Draw("P SAME");
    gAK15->Draw("P SAME");

    pAK8 ->Draw("L SAME");
    pAK15->Draw("L SAME");

    TLine zero(0, 0, 800, 0);
    zero.SetLineStyle(2);
    zero.Draw("same");

    TLegend leg(0.55, 0.60, 0.83, 0.83);
    leg.SetBorderSize(0);
    leg.SetFillStyle(0);
    leg.AddEntry(gAK4,  "Anti k_{T}, R = 0.4, p_{T} > 30 GeV, |#eta| < 2.5", "lp");
    leg.AddEntry(gAK8,  "Anti k_{T}, R = 0.8, p_{T} > 200 GeV, |#eta| < 2.5", "lp");
    leg.AddEntry(pAK8,  "prediction", "l");
    leg.AddEntry(gAK15, "Anti k_{T}, R = 1.5, p_{T} > 150 GeV, |#eta| < 2.5", "lp");
    leg.AddEntry(pAK15, "prediction", "l");
    leg.Draw();

    TLatex latex;
    latex.SetNDC();
    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.13, 0.81, "CMS");
    latex.SetTextFont(52);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.13, 0.76, "Simulation, Work in Progress");
    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.13, 0.7, "H #rightarrow #tau#tau (125 GeV)");

    c.SaveAs(TString(save_path) + "/MatchingEffDiff_inclusive_minus_hadhad_allJets.png");
}   