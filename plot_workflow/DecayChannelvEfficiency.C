#include "TCanvas.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TEfficiency.h"
#include "TGraphAsymmErrors.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TLine.h"
#include "TString.h"
#include "TStyle.h"
#include "TROOT.h"
#include "eff_helpers.h"
#include <iostream>
#include "TF1.h"
#include <map>

struct Channel {
    const char* label;
    const char* cut;
    int color;
    int marker;
};

struct Jet {
    const char* label;
    const char* prettyLabel;
    const char* file;
    const char* ptExpr;
    const char* jetVar;
    const double cut;
};

std::map<int, float> fChannel {
    {0, 2.0/3.0},
    {1, 0.5},
    {2, 0.5},
    {3, 0.509}
};

TF1* MakeOffsetOverXFit(const char* name, double offsetInit,
                        double xMin, double xMax, double cInit = 50.0) {
    TF1* f = new TF1(name, "[0] + [1]/x", xMin, xMax);
    f->SetParameter(0, offsetInit);
    f->SetParameter(1, cInit);    
    f->SetLineWidth(3); 
    return f;
}

void SetCMSStyle() {
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gStyle->SetCanvasColor(kWhite);
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadBorderMode(0);
    gStyle->SetPadColor(kWhite);
    
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");
    gStyle->SetLabelSize(0.05, "XYZ");
    gStyle->SetTitleSize(0.06, "XYZ");
    
    gStyle->SetTitleXOffset(1.2);
    gStyle->SetTitleYOffset(1.5);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
    gStyle->SetTickLength(0.03, "XYZ");
    gStyle->SetLineWidth(2);
}

void DecayChannelvEfficiency(
    const char* save_path = "/eos/user/m/mroine/NanoTuples/Htautau/plot_workflow/plots/MADGRAPH",
    const char* jet_path  = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/MADGRAPH"
) {
    SetCMSStyle();

    const int   nBins = 40;
    const float vMin  = 0.f;
    const float vMax  = 800.f;

    TString rawInc = TString(jet_path) + "/RawEventInfo.root";

    // Removed the 4th "all channels" entry completely
    Channel channels[3] = {
        {"had-had", "is_truth_hadhad == 1", kBlack, 20},
        {"e-had",   "is_truth_ehad == 1",   kRed,   21},
        {"#mu-had", "is_truth_muhad == 1",  kBlue,  22}
    };

    Jet jets[3] = {
        {"Jet", "AK4",  Form("%s/Jet.root",    jet_path), "ak4_pt/genH_pt",  "genH_pt", 30.0},
        {"FatJet", "AK8", Form("%s/fatJet.root", jet_path), "fj_pt/genH_pt",   "genH_pt", 200.0},
        {"AK15", "AK15", Form("%s/AK15.root",   jet_path), "ak15_pt/genH_pt", "genH_pt", 150.0}
    };

    TCanvas c("c_resp", "", 1200, 650);
    c.Divide(2, 1);

    const double fitLo = 200.0;
    const double fitHi = 800.0;

    for (int j = 1; j < 3; ++j) {
        c.cd(j); 
        gPad->SetTopMargin(0.12); 
        gPad->SetBottomMargin(0.24); 
        gPad->SetLeftMargin(0.18);   
        gPad->SetRightMargin(0.05);

        TProfile* profs[3] = {nullptr, nullptr, nullptr};
        double Cfit[3], CfitErr[3], Nent[3], ffit[3];

        for (int ch = 0; ch < 3; ++ch) {
            TString tag = Form("resp_%d_%d", j, ch);
            TH2F* h2 = new TH2F(tag + "_h2", "", nBins, vMin, vMax, 200, 0.0, 2.0);
            TString expr = Form("%s:genH_pt", jets[j].ptExpr);
            ProjectFromTree(jets[j].file, h2, expr.Data(), channels[ch].cut);

            TProfile* p = h2->ProfileX(tag + "_prof");
            p->SetDirectory(0);
            p->SetMarkerStyle(channels[ch].marker);
            p->SetMarkerColor(channels[ch].color);
            p->SetLineColor(channels[ch].color);
            p->SetLineWidth(2);
            p->SetMarkerSize(1.5); 
            p->SetStats(0);
            profs[ch] = p;

            if (ch == 0) {
                p->SetTitle(";Higgs p_{T} [GeV];p_{T}^{reco}/p_{T}^{gen}");
                p->GetYaxis()->SetRangeUser(0.0, 1.5);
                p->GetXaxis()->SetRangeUser(vMin, vMax);
                p->Draw("P");
            } else {
                p->Draw("P SAME");
            }
        }
        
        for (int ch = 0; ch < 3; ++ch) {
            TF1* f1 = MakeOffsetOverXFit(Form("p1_%d_%d", j, ch), 0.6, vMin, vMax, 60.0);
            profs[ch]->Fit(f1, "RQ0N", "", fitLo, fitHi);   
            Cfit[ch]    = f1->GetParameter(1);
            CfitErr[ch] = f1->GetParError(1);
            Nent[ch]    = profs[ch]->GetEntries();
        }

        double sw = 0.0, swc = 0.0;
        for (int ch = 0; ch < 3; ++ch) {
            double e = (CfitErr[ch] > 0) ? CfitErr[ch] : 1e9;
            sw  += 1.0 / (e * e);
            swc += Cfit[ch] / (e * e);
        }
        double Cshared = (sw > 0) ? swc / sw : 0.0;

        TF1* fitLines[3] = {nullptr, nullptr, nullptr};
        for (int ch = 0; ch < 3; ++ch) {
            TF1* f2 = MakeOffsetOverXFit(Form("p2_%d_%d", j, ch), 0.6, vMin, vMax, Cshared);
            f2->FixParameter(1, Cshared);
            profs[ch]->Fit(f2, "RQ0", "", fitLo, fitHi);
            f2->SetLineColor(channels[ch].color);
            f2->SetLineStyle(1);
            f2->Draw("same");
            fitLines[ch] = f2;
            ffit[ch] = f2->GetParameter(0);
        }

        // Hard-coded absolute positioning to the top right to avoid alignment bugs
        TLatex lab; 
        lab.SetNDC(); 
        lab.SetTextFont(42); 
        lab.SetTextSize(0.040);
        lab.SetTextAlign(11); // Left-aligned
        lab.DrawLatex(0.75, 0.82, jets[j].prettyLabel); 
        lab.DrawLatex(0.75, 0.76, Form("C = %.1f", Cshared));

        TLegend* legFit = new TLegend(0.48, 0.26, 0.92, 0.46); 
        legFit->SetBorderSize(0); 
        legFit->SetFillStyle(0); 
        legFit->SetTextSize(0.04); 
        for (int ch = 0; ch < 3; ++ch)
            legFit->AddEntry(fitLines[ch], Form("%s fit: f=%.3f", channels[ch].label, ffit[ch]), "l");
        legFit->Draw();

        TLatex latex;
        latex.SetNDC();
        latex.SetTextAlign(11); 
        latex.SetTextFont(61);
        latex.SetTextSize(0.06);
        latex.DrawLatex(0.18, 0.90, "CMS"); 
        latex.SetTextFont(52);
        latex.SetTextSize(0.045);
        latex.DrawLatex(0.31, 0.90, "Simulation Private");
        
        latex.SetTextAlign(31); 
        latex.SetTextFont(42);
        latex.SetTextSize(0.05);
        latex.DrawLatex(0.95, 0.90, "13.6 TeV"); 
    }

    c.cd(0);
    TLegend* globalLeg = new TLegend(0.10, 0.01, 0.90, 0.07); 
    globalLeg->SetBorderSize(0);
    globalLeg->SetFillStyle(0);
    globalLeg->SetTextSize(0.04);
    globalLeg->SetNColumns(3); // Adjusted to 3 columns

    for (int ch = 0; ch < 3; ++ch) {
        TGraph* m = new TGraph();
        m->SetMarkerStyle(channels[ch].marker);
        m->SetMarkerColor(channels[ch].color);
        m->SetLineColor(channels[ch].color);
        m->SetLineWidth(2);
        m->SetMarkerSize(1.5);
        globalLeg->AddEntry(m, channels[ch].label, "p");
    }
    globalLeg->Draw();

    c.SaveAs(save_path);
}