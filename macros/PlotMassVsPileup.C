#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TProfile.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TString.h"
#include "TStyle.h"
#include <iostream>
#include <TGraph.h>

struct JetMassBranches {
    const char* label;
    const char* mass;
    const char* msoftdrop;
};

void StyleProfile(TProfile* p, int color, int marker) {
    p->SetLineColor(color);
    p->SetMarkerColor(color);
    p->SetMarkerStyle(marker);
    p->SetMarkerSize(0.9);
    p->SetLineWidth(2);
}

TProfile* MakeMassVsPileupProfile(
    TTree* t,
    const char* name,
    const char* massBranch,
    const char* pileupBranch,
    const char* title,
    int nBins,
    double xMin,
    double xMax
) {
    TProfile* p = new TProfile(name, title, nBins, xMin, xMax, "s");
    t->Draw(Form("%s:%s >> %s", massBranch, pileupBranch, name), "", "goff prof");
    return p;
}

void DrawMassPanel(
    TProfile* pMass,
    TProfile* pMsd,
    const char* jetLabel,
    double yMin = 0.0,
    double yMax = 250.0
) {
    pMass->SetMinimum(yMin);
    pMass->SetMaximum(yMax);
    pMass->Draw("E1");
    pMsd->Draw("E1 SAME");

    TLegend leg(0.58, 0.70, 0.88, 0.88);
    leg.SetBorderSize(0);
    leg.SetFillStyle(0);
    leg.AddEntry(pMass, "mass", "lp");
    leg.AddEntry(pMsd, "msoftdrop", "lp");
    leg.Draw();

    TLatex lat;
    lat.SetNDC();
    lat.SetTextFont(42);
    lat.SetTextSize(0.045);
    lat.DrawLatex(0.18, 0.82, jetLabel);
}

void DrawCanvasLegend(TCanvas& c) {
    c.cd(0);

    TLegend* leg = new TLegend(0.40, 0.92, 0.60, 0.99);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetNColumns(2);

    TGraph* gMass = new TGraph();
    gMass->SetLineColor(kBlue + 1);
    gMass->SetMarkerColor(kBlue + 1);
    gMass->SetMarkerStyle(20);
    gMass->SetLineWidth(2);

    TGraph* gMsd = new TGraph();
    gMsd->SetLineColor(kRed + 1);
    gMsd->SetMarkerColor(kRed + 1);
    gMsd->SetMarkerStyle(21);
    gMsd->SetLineWidth(2);

    leg->AddEntry(gMass, "mass", "lp");
    leg->AddEntry(gMsd, "msoftdrop", "lp");
    leg->Draw();
}

void PlotMassVsPileup(
    const char* ak8_file  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/fatJet_hadhad.root",
    const char* ak15_file = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/AK15_hadhad.root",
    const char* output_dir = "/eos/user/m/mroine/www/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8"
) {
    gStyle->SetOptStat(0);

    const int nPileupBins = 40;
    const double pileupMin = 10.0;
    const double pileupMax = 70.0;

    JetMassBranches jets[2] = {
        {"AK8",  "fj_mass",  "fj_msoftdrop"},
        {"AK15", "ak15_mass", "ak15_msoftdrop"}
    };

    const char* files[2] = {ak8_file, ak15_file};

    TCanvas c_ntrue("c_mass_vs_ntrue", "Mass vs Pileup_nTrueInt", 1600, 700);
    c_ntrue.Divide(2, 1);
    c_ntrue.SetGrid();

    for (int i = 0; i < 2; ++i) {
        TFile* f = new TFile(files[i], "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "Failed to open " << files[i] << std::endl;
            return;
        }

        TTree* t = (TTree*)f->Get("Events");
        if (!t) {
            std::cerr << "Events tree not found in " << files[i] << std::endl;
            return;
        }

        TString suffix = Form("_%d", i);

        TProfile* pMass = MakeMassVsPileupProfile(
            t,
            Form("p_mass_ntrue%s", suffix.Data()),
            jets[i].mass,
            "Pileup_nTrueInt",
            Form(";%s; mass [GeV]", "Pileup nTrueInt"),
            nPileupBins, pileupMin, pileupMax
        );

        std::cout << jets[i].label << ": plotted " << pMass->GetEntries() << " events" << std::endl;

        TProfile* pMsd = MakeMassVsPileupProfile(
            t,
            Form("p_msd_ntrue%s", suffix.Data()),
            jets[i].msoftdrop,
            "Pileup_nTrueInt",
            Form(";%s; msoftdrop [GeV]", "Pileup nTrueInt"),
            nPileupBins, pileupMin, pileupMax
        );

        std::cout << jets[i].label << ": plotted " << pMsd->GetEntries() << " events" << std::endl;

        StyleProfile(pMass, kBlue + 1, 20);
        StyleProfile(pMsd, kRed + 1, 21);

        c_ntrue.cd(i + 1);
        DrawMassPanel(pMass, pMsd, jets[i].label);

    }

    DrawCanvasLegend(c_ntrue);
    c_ntrue.SaveAs(TString(output_dir) + "/AK8_AK15_mass_vs_Pileup_nTrueInt.png");

    TCanvas c_npvs("c_mass_vs_npvs", "Mass vs PV_npvsGood", 1600, 700);
    c_npvs.Divide(2, 1);
    c_npvs.SetGrid();

    for (int i = 0; i < 2; ++i) {
        TFile* f = TFile::Open(files[i], "READ");
        TTree* t = (TTree*)f->Get("Events");

        TString suffix = Form("_npvs_%d", i);

        TProfile* pMass = MakeMassVsPileupProfile(
            t,
            Form("p_mass_npvs%s", suffix.Data()),
            jets[i].mass,
            "PV_npvsGood",
            Form(";%s;mass [GeV]", "PV nPVsGood"),
            nPileupBins, pileupMin, pileupMax
        );

        std::cout << jets[i].label << ": plotted " << pMass->GetEntries() << " events" << std::endl;

        TProfile* pMsd = MakeMassVsPileupProfile(
            t,
            Form("p_msd_npvs%s", suffix.Data()),
            jets[i].msoftdrop,
            "PV_npvsGood",
            Form(";%s; msoftdrop [GeV]", "PV nPVsGood"),
            nPileupBins, pileupMin, pileupMax
        );

        std::cout << jets[i].label << ": plotted " << pMsd->GetEntries() << " events" << std::endl;

        StyleProfile(pMass, kBlue + 1, 20);
        StyleProfile(pMsd, kRed + 1, 21);

        c_npvs.cd(i + 1);
        DrawMassPanel(pMass, pMsd, jets[i].label);

    }

    DrawCanvasLegend(c_npvs);
    c_npvs.SaveAs(TString(output_dir) + "/AK8_AK15_mass_vs_PV_npvsGood.png");

}
