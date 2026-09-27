    #include <iostream>
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
    #include "TFile.h"
    #include "TTree.h"
    #include "TH2F.h"
    #include <cstring>
    #include <vector>

    // For skipping legacy AK4 if not exist
    static bool FileUsable(const char* fname) {
        if (!fname || strlen(fname) == 0) return false;
        TFile* f = TFile::Open(fname, "READ");
        bool ok = f && !f->IsZombie();
        if (f) f->Close();
        return ok;
    }

    void PlotEfficiencies(const char* save_path = "/eos/user/m/mroine/www/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8",
        const char* fRaw  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/RawEventInfo.root",
        const char* fAK4  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/Jet.root",
        const char* fAK8  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/fatJet.root",
        const char* fAK15 = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/AK15.root",
        const char* fTau = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/Tau.root",
        const bool hadhad = false
    ) {

        TString hadhad_string = hadhad ? "_hadhad" : "";
        const bool hasAK4 = FileUsable(fAK4);
        if (!hasAK4) {
            std::cout << "AK4 input not available, dropping AK4 series from this plot." << std::endl;
        }

        TCanvas* c1 = new TCanvas("c1", "", 1800, 1200);
        c1->Divide(3, 2);

        auto drawEffPlot = [&](int padNum, const char* rawVar, const char* jetVar, const char* rawCut, const char* jetCut,
                            int nBins, float vMin, float vMax, const char* xAxisTitle, double yMax) {

            c1->cd(padNum);
            gPad->SetTopMargin(0.10);

            // Unique name
            TString hPrefix = Form("pad%d", padNum);

            TH1F* h_den = new TH1F(hPrefix + "_den",  "", nBins, vMin, vMax);
            ProjectFromTree(fRaw, h_den, rawVar, rawCut);

            struct EffSeries { const char* file; const char* tag; int color; };
            std::vector<EffSeries> series;
            if (hasAK4) series.push_back({fAK4, "ak4", kBlue});
            series.push_back({fAK8, "ak8", kRed});
            series.push_back({fAK15, "ak15", kGreen+2});
            series.push_back({fTau, "tau", kBlack});

            TEfficiency* first = nullptr;
            for (const auto& s : series) {
                TH1F* h_num = new TH1F(hPrefix + "_" + s.tag, "", nBins, vMin, vMax);
                ProjectFromTree(s.file, h_num, jetVar, jetCut);

                TEfficiency* eff = new TEfficiency(*h_num, *h_den);
                eff->SetMarkerStyle(20);
                eff->SetMarkerColor(s.color);
                eff->SetMarkerSize(0.7);

                if (!first) {
                    eff->SetTitle(Form(";%s;Matching Efficiency", xAxisTitle));
                    eff->Draw("AP");
                    first = eff;
                    gPad->Update();
                    if (auto* g = eff->GetPaintedGraph()) {
                        g->GetYaxis()->SetRangeUser(0.0, yMax);
                        g->GetXaxis()->SetRangeUser(vMin, vMax);
                    }
                } else {
                    eff->Draw("P SAME");
                }
            }
            c1->cd(0);
        };


        auto drawProfilePlot = [&](int padNum,
                                    const char* yAK4, const char* yAK8, const char* yAK15, const char* yTau,
                                    const char* xVar, const char* cut,
                                    int nBins, float vMin, float vMax,
                                    const char* xAxisTitle, const char* yAxisTitle,
                                    double yMin, double yMax,
                                    bool drawUnityLine = false)
        {
            c1->cd(padNum);
            gPad->SetTopMargin(0.12);

            TString hPrefix = Form("pad%d", padNum);

            auto makeProfile = [&](const char* fname, const char* yVar, const char* tag, int color) -> TProfile* {
                TH2F* h2 = new TH2F(hPrefix + "_h2_" + tag, "", nBins, vMin, vMax, 200, yMin, yMax);

                TString expr = Form("%s:%s", yVar, xVar);
                ProjectFromTree(fname, h2, expr.Data(), cut);

                TProfile* p = h2->ProfileX(hPrefix + "_prof_" + tag);
                p->SetDirectory(0);

                p->SetMarkerStyle(20);
                p->SetMarkerColor(color);
                p->SetMarkerSize(0.7);
                p->SetLineColor(color);
                p->SetStats(0);

                std::cout << "  " << tag << ": " << p->GetEntries() << " entries" << std::endl;
                return p;
            };

            struct ProfSeries { const char* file; const char* yVar; const char* tag; int color; };
            std::vector<ProfSeries> series;
            if (hasAK4) series.push_back({fAK4, yAK4, "ak4", kBlue});
            series.push_back({fAK8, yAK8, "ak8", kRed});
            series.push_back({fAK15, yAK15, "ak15", kGreen + 2});
            series.push_back({fTau, yTau, "tau", kBlack});

            TProfile* first = nullptr;
            for (const auto& s : series) {
                TProfile* p = makeProfile(s.file, s.yVar, s.tag, s.color);
                if (!first) {
                    p->SetTitle(Form(";%s;%s", xAxisTitle, yAxisTitle));
                    p->GetYaxis()->SetRangeUser(yMin, yMax);
                    p->GetXaxis()->SetRangeUser(vMin, vMax);
                    p->Draw("P");
                    first = p;
                } else {
                    p->Draw("P SAME");
                }
            }

            if (drawUnityLine) {
                TLine* unity = new TLine(vMin, 1.0, vMax, 1.0);
                unity->SetLineStyle(2);
                unity->SetLineColor(kBlack);
                unity->Draw("SAME");
            }
        };


        auto drawPileupEffPlot = [&](int padNum) {
            c1->cd(padNum);
            gPad->SetTopMargin(0.12);

            const char* rawCut = "genH_pt_raw > 200";
            const char* jetCut = "genH_pt > 200";
            int nBins = 10;
            float vMin = 10.0;
            float vMax = 70.0;

            struct PileupVar {
                const char* rawVar;
                const char* jetVar;
                const char* label;
                int markerStyle;
            };

            PileupVar pileupVars[] = {
                {"PV_npvsGood",      "PV_npvsGood",      "PV_npvsGood", kFullCircle},
                {"Pileup_nTrueInt",  "Pileup_nTrueInt",  "Pileup_nTrueInt",         kFullSquare},
                {"PV_npvs",          "PV_npvs",          "PV_npvs",        kFullTriangleUp},
            };

            struct JetSample {
                const char* file;
                const char* tag;
                int color;
            };

            std::vector<JetSample> jets;
            if (hasAK4) jets.push_back({fAK4, "ak4", kBlue});
            jets.push_back({fAK8, "ak8", kRed});
            jets.push_back({fAK15, "ak15", kGreen + 2});
            jets.push_back({fTau, "tau", kBlack});

            TEfficiency* first = nullptr;

            for (const auto& jet : jets) {
                for (const auto& pu : pileupVars) {
                    TString hPrefix = Form("pad%d_%s_%s", padNum, jet.tag, pu.rawVar);

                    TH1F* h_den = new TH1F(hPrefix + "_den", "", nBins, vMin, vMax);
                    TH1F* h_num = new TH1F(hPrefix + "_num", "", nBins, vMin, vMax);

                    ProjectFromTree(fRaw, h_den, pu.rawVar, rawCut);
                    ProjectFromTree(jet.file, h_num, pu.jetVar, jetCut);

                    TEfficiency* eff = new TEfficiency(*h_num, *h_den);
                    eff->SetMarkerStyle(pu.markerStyle);
                    eff->SetMarkerColor(jet.color);
                    eff->SetMarkerSize(0.6);
                    eff->SetLineColor(jet.color);

                    if (!first) {
                        eff->SetTitle(";(genH_{T} > 300 GeV);Matching Efficiency");
                        eff->Draw("AP");
                        first = eff;
                        gPad->Update();
                        if (auto* g = eff->GetPaintedGraph()) {
                            g->GetYaxis()->SetRangeUser(0.0, 1.0);
                            g->GetXaxis()->SetRangeUser(vMin, vMax);
                        }
                    } else {
                        eff->Draw("P SAME");
                    }
                }
            }

            auto* legPU = new TLegend(0.55, 0.18, 0.88, 0.34);
            legPU->SetBorderSize(0);
            legPU->SetFillStyle(0);
            legPU->SetTextSize(0.028);

            TGraph* m0 = new TGraph(); m0->SetMarkerStyle(20); m0->SetMarkerColor(kBlack);
            TGraph* m1 = new TGraph(); m1->SetMarkerStyle(21); m1->SetMarkerColor(kBlack);
            TGraph* m2 = new TGraph(); m2->SetMarkerStyle(22); m2->SetMarkerColor(kBlack);

            legPU->AddEntry(m0, "PV_npvsGood", "p");
            legPU->AddEntry(m1, "Pileup_nTrueInt", "p");
            legPU->AddEntry(m2, "PV_npvs", "p");
            legPU->Draw();
        };

        std::cout << "Generating pT plot..." << std::endl;
        drawEffPlot(1, "genH_pt_raw", "genH_pt", "", "", 100, 0.0, 800.0, "genH_pt [GeV]", 1.0);

        std::cout << "Generating pileup plot..." << std::endl;
        drawPileupEffPlot(2);
        std::cout << "Generating asym plot..." << std::endl;
        drawEffPlot(3, "genTau_pt_asym_raw", "genTau_pt_asym", "genH_pt_raw > 300", "genH_pt > 300", 50, 0.0, 1.0, "genTau_pt_asym (genH_pt > 300 GeV)", 1.0);

        std::cout << "Generating pT response plot..." << std::endl;
        drawProfilePlot(4,
            "ak4_pt/genH_pt",
            "fj_pt/genH_pt",
            "ak15_pt/genH_pt",
            "tau_pt/genH_pt",
            "genH_pt", "genH_pt > 0",
            100, 0.0, 800.0,
            "genH_pt [GeV]", "p_{T}^{reco}/p_{T}^{gen}",
            0.5, 1.5,
            false);
        std::cout << "Generating dR plot..." << std::endl;
        drawProfilePlot(5,
            "dR_ak4_H",            
            "dR_fj_H",
            "dR_ak15_H",  
            "dR_tau_H",
            "genH_pt", "",
            100, 0.0, 800.0,
            "genH_pt [GeV]", "#Delta R(jet, H)",
            0.0, 1.0,
            false);


        c1->cd(0);
        TGraph* pAK8_leg  = new TGraph(); pAK8_leg->SetMarkerStyle(20); pAK8_leg->SetMarkerColor(kRed);      pAK8_leg->SetLineColor(kRed);
        TGraph* pAK15_leg = new TGraph(); pAK15_leg->SetMarkerStyle(20); pAK15_leg->SetMarkerColor(kGreen+2); pAK15_leg->SetLineColor(kGreen+2);
        TGraph* pTau_leg = new TGraph(); pTau_leg->SetMarkerStyle(20); pTau_leg->SetMarkerColor(kBlack); pTau_leg->SetLineColor(kBlack);

        TLatex latex;
        latex.SetNDC();
        latex.SetTextFont(62);
        latex.SetTextSize(0.045);
        latex.DrawLatex(0.68, 0.40, "CMS");
        latex.SetTextFont(52);
        latex.SetTextSize(0.030);
        latex.DrawLatex(0.68, 0.35, "Simulation, Work in Progress");
        latex.SetTextFont(42);
        latex.SetTextSize(0.035);
        latex.DrawLatex(0.68, 0.30, "H #rightarrow #tau#tau (125 GeV)");
        TLegend* leg = new TLegend(0.65, 0.05, 0.95, 0.26);
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        leg->SetTextSize(0.022);
        leg->SetEntrySeparation(0.3);
        if (hasAK4) {
            TGraph* pAK4_leg = new TGraph(); pAK4_leg->SetMarkerStyle(20); pAK4_leg->SetMarkerColor(kBlue); pAK4_leg->SetLineColor(kBlue);
            leg->AddEntry(pAK4_leg, "Anti k_{T}, R = 0.4, p_{T} > 30 GeV, |#eta| < 2.5", "lp");
        }
        leg->AddEntry(pAK8_leg,  "Anti k_{T}, R = 0.8, p_{T} > 200 GeV, |#eta| < 2.5", "lp");
        leg->AddEntry(pAK15_leg, "Anti k_{T}, R = 1.5, p_{T} > 150 GeV, |#eta| < 2.5", "lp");
        leg->AddEntry(pTau_leg, "Skimmed taus after basic selection, |#eta| < 2.5", "lp");
        leg->Draw();


        c1->SaveAs(save_path); 
        std::cout << "Done! Saved to " << save_path << std::endl;
    }