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
    f->SetLineWidth(2);
    return f;
}

void DecayChannelvEfficiency(
    const char* save_path = "/eos/user/m/mroine/www/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8",
    const char* jet_path  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8"
) {
    gStyle->SetOptStat(0);

    const int   nBins = 40;
    const float vMin  = 0.f;
    const float vMax  = 800.f;

    TString rawInc = TString(jet_path) + "/RawEventInfo.root";

    Channel channels[4] = {
        {"had-had", "is_truth_hadhad == 1", kBlack, 20},
        {"e-had",   "is_truth_ehad == 1",   kRed,   21},
        {"#mu-had", "is_truth_muhad == 1",  kBlue,  22},
        {"all channels", "", kGreen + 2, 23}
    };

    Jet jets[3] = {
        {"Jet",  Form("%s/Jet.root",    jet_path), "ak4_pt/genH_pt",  "genH_pt", 30.0},
        {"FatJet",  Form("%s/fatJet.root", jet_path), "fj_pt/genH_pt",   "genH_pt", 200.0},
        {"AK15", Form("%s/AK15.root",   jet_path), "ak15_pt/genH_pt", "genH_pt", 150.0}
    };

    TCanvas c("c_grid", "", 1600, 1000);
    c.Divide(3, 2);


    for (int j = 0; j < 3; ++j) {
        c.cd(j + 1);
        gPad->SetTopMargin(0.14);
        

        TEfficiency* first = nullptr;

        for (int ch = 0; ch < 4; ++ch) {
            TString tag = Form("eff_%d_%d", j, ch);

            TH1F* h_den = new TH1F(tag + "_den", "", nBins, vMin, vMax);
            TH1F* h_num = new TH1F(tag + "_num", "", nBins, vMin, vMax);

            ProjectFromTree(rawInc,       h_den, "genH_pt_raw", channels[ch].cut);
            ProjectFromTree(jets[j].file, h_num, jets[j].jetVar, channels[ch].cut);

            TEfficiency* eff = new TEfficiency(*h_num, *h_den);
            eff->SetMarkerStyle(channels[ch].marker);
            eff->SetMarkerColor(channels[ch].color);
            eff->SetLineColor(channels[ch].color);
            eff->SetMarkerSize(0.7);

            if (!first) {
                eff->SetTitle(";genH_pT [GeV];Matching Efficiency");
                eff->Draw("AP");
                gPad->Update();
                if (auto* g = eff->GetPaintedGraph()) {
                    g->GetYaxis()->SetRangeUser(0.0, 1.05);
                    g->GetXaxis()->SetRangeUser(vMin, vMax);
                }
                first = eff;
            } else {
                eff->Draw("P SAME");
            }
        }


    }

const double fitLo = 200.0;
const double fitHi = 800.0;

for (int j = 0; j < 3; ++j) {
    c.cd(j + 4);
    gPad->SetTopMargin(0.14);

    TProfile* profs[4] = {nullptr, nullptr, nullptr, nullptr};
    double Cfit[3], CfitErr[3], Nent[3], ffit[3];

    for (int ch = 0; ch < 4; ++ch) {
        TString tag = Form("resp_%d_%d", j, ch);
        TH2F* h2 = new TH2F(tag + "_h2", "", nBins, vMin, vMax, 200, 0.0, 2.0);
        TString expr = Form("%s:genH_pt", jets[j].ptExpr);
        ProjectFromTree(jets[j].file, h2, expr.Data(), channels[ch].cut);

        TProfile* p = h2->ProfileX(tag + "_prof");
        p->SetDirectory(0);
        p->SetMarkerStyle(channels[ch].marker);
        p->SetMarkerColor(channels[ch].color);
        p->SetLineColor(channels[ch].color);
        p->SetMarkerSize(0.7);
        p->SetStats(0);
        profs[ch] = p;

        if (ch == 0) {
            p->SetTitle(";genH_pT [GeV];p_{T}^{reco}/p_{T}^{gen}");
            p->GetYaxis()->SetRangeUser(0.0, 1.5);
            p->GetXaxis()->SetRangeUser(vMin, vMax);
            p->Draw("P");
        } else {
            p->Draw("P SAME");
        }
    }
    TLatex lab; lab.SetNDC(); lab.SetTextFont(42); lab.SetTextSize(0.055);
    if (j == 0) {
        lab.DrawLatex(0.16, 0.95, "Jet");
        continue;
    }

    

    // ---- pass 1: free f AND free C on the 3 exclusive channels ----
    for (int ch = 0; ch < 3; ++ch) {
        TF1* f1 = MakeOffsetOverXFit(Form("p1_%d_%d", j, ch), 0.6, vMin, vMax, 60.0);
        profs[ch]->Fit(f1, "RQ0N", "", fitLo, fitHi);   // N = don't draw/store
        Cfit[ch]    = f1->GetParameter(1);
        CfitErr[ch] = f1->GetParError(1);
        Nent[ch]    = profs[ch]->GetEntries();
    }

    // weighted shared C for this jet
    double sw = 0.0, swc = 0.0;
    for (int ch = 0; ch < 3; ++ch) {
        double e = (CfitErr[ch] > 0) ? CfitErr[ch] : 1e9;
        sw  += 1.0 / (e * e);
        swc += Cfit[ch] / (e * e);
    }
    double Cshared = (sw > 0) ? swc / sw : 0.0;

    // ---- pass 2: fix C = Cshared, free f, refit + draw ----
    TF1* fitLines[3] = {nullptr, nullptr, nullptr};
    for (int ch = 0; ch < 3; ++ch) {
        TF1* f2 = MakeOffsetOverXFit(Form("p2_%d_%d", j, ch), 0.6, vMin, vMax, Cshared);
        f2->FixParameter(1, Cshared);
        profs[ch]->Fit(f2, "RQ0", "", fitLo, fitHi);
        f2->SetLineColor(channels[ch].color);
        f2->SetLineStyle(1);
        f2->SetLineWidth(2);
        f2->Draw("same");
        fitLines[ch] = f2;
        ffit[ch] = f2->GetParameter(0);
    }

    double fAllPred = 0.509;

    TF1* fAll = new TF1(Form("fAll_%d", j), "[0] + [1]/x", vMin, vMax);
    fAll->SetParameter(0, fAllPred);
    fAll->FixParameter(1, Cshared);
    fAll->SetLineColor(kMagenta + 1);
    fAll->SetLineStyle(2);
    fAll->SetLineWidth(2);
    fAll->Draw("same");

    std::cout << jets[j].label
              << "  Cshared=" << Cshared
              << "  f_had=" << ffit[0]
              << "  f_e="   << ffit[1]
              << "  f_mu="  << ffit[2]
              << "  |  f_had/f_e="  << ffit[0]/ffit[1]
              << "  f_had/f_mu=" << ffit[0]/ffit[2]
              << "  (pred 1.333)"
              << "  fAll_pred=" << fAllPred
              << std::endl;

    lab.DrawLatex(0.16, 0.95, Form("%s (C = %f)", jets[j].label, Cshared));

    TLegend* legFit = new TLegend(0.42, 0.58, 0.88, 0.88);
    legFit->SetBorderSize(0); legFit->SetFillStyle(0); legFit->SetTextSize(0.028);
    for (int ch = 0; ch < 3; ++ch)
        legFit->AddEntry(fitLines[ch], Form("%s fit: f=%.3f", channels[ch].label, ffit[ch]), "l");
    legFit->AddEntry(fAll, Form("all channels (pred): f=%.3f", fAllPred), "l");
    legFit->Draw();
}

    c.cd(0);
    TLatex latex;
    latex.SetNDC();
    latex.SetTextFont(62);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.05, 0.965, "CMS");
    latex.SetTextFont(52);
    latex.SetTextSize(0.024);
    latex.DrawLatex(0.11, 0.965, "Simulation, Work in Progress");
    latex.DrawLatex(0.05, 0.935, "H #rightarrow #tau#tau (125 GeV)");    

    TLegend* globalLeg = new TLegend(0.68, 0.92, 0.99, 0.99);
    globalLeg->SetBorderSize(0);
    globalLeg->SetFillStyle(0);
    globalLeg->SetTextSize(0.025);
    globalLeg->SetNColumns(4); 

    for (int ch = 0; ch < 4; ++ch) {
        TGraph* m = new TGraph();
        m->SetMarkerStyle(channels[ch].marker);
        m->SetMarkerColor(channels[ch].color);
        m->SetLineColor(channels[ch].color);
        globalLeg->AddEntry(m, channels[ch].label, "lp");
    }
    globalLeg->Draw();

    c.SaveAs(TString(save_path) + "/effAndResponse_byChannel_grid.png");
}
