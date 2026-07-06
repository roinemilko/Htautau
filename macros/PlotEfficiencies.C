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

    void PlotEfficiencies(const char* save_path = "/eos/user/m/mroine/www/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8",
        const char* fRaw  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/RawEventInfo.root",
        const char* fAK4  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/Jet.root",
        const char* fAK8  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/fatJet.root",
        const char* fAK15 = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/AK15.root",
        const char* fTau = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/Tau.root"
    ) {

        TCanvas* c1 = new TCanvas("c1", "", 1800, 1200);
        c1->Divide(3, 2);

        auto drawEffPlot = [&](int padNum, const char* rawVar, const char* jetVar, const char* rawCut, const char* jetCut,
                            int nBins, float vMin, float vMax, const char* xAxisTitle, double yMax) {
            
            c1->cd(padNum);
            gPad->SetTopMargin(0.10);

            // Unique name
            TString hPrefix = Form("pad%d", padNum);


            TH1F* h_den = new TH1F(hPrefix + "_den",  "", nBins, vMin, vMax);
            TH1F* h_num_AK4 = new TH1F(hPrefix + "_ak4",  "", nBins, vMin, vMax);
            TH1F* h_num_AK8 = new TH1F(hPrefix + "_ak8",  "", nBins, vMin, vMax);
            TH1F* h_num_AK15 = new TH1F(hPrefix + "_ak15", "", nBins, vMin, vMax);
            TH1F* h_num_Tau = new TH1F(hPrefix + "_Tau", "", nBins, vMin, vMax);

            ProjectFromTree(fRaw, h_den, rawVar, rawCut);
            ProjectFromTree(fAK4, h_num_AK4, jetVar, jetCut);
            ProjectFromTree(fAK8, h_num_AK8, jetVar, jetCut);
            ProjectFromTree(fAK15, h_num_AK15, jetVar, jetCut);
            ProjectFromTree(fTau, h_num_Tau, jetVar, jetCut);

            TEfficiency* effAK4  = new TEfficiency(*h_num_AK4, *h_den);
            TEfficiency* effAK8  = new TEfficiency(*h_num_AK8, *h_den);
            TEfficiency* effAK15 = new TEfficiency(*h_num_AK15, *h_den);
            TEfficiency* effTau = new TEfficiency(*h_num_Tau, *h_den);

            effAK4->SetTitle(Form(";%s;Matching Efficiency", xAxisTitle));
        
            effAK4->SetMarkerStyle(20);
            effAK8->SetMarkerStyle(20);
            effAK15->SetMarkerStyle(20);
            effTau->SetMarkerStyle(20);

            effAK4->SetMarkerColor(kBlue);
            effAK8->SetMarkerColor(kRed);
            effAK15->SetMarkerColor(kGreen+2);
            effTau->SetMarkerColor(kBlack);

            effAK4->SetMarkerSize(0.7);
            effAK8->SetMarkerSize(0.7);
            effAK15->SetMarkerSize(0.7);
            effTau->SetMarkerSize(0.7);

            effAK4->Draw("AP");
            gPad->Update(); 
            auto graphAK4 = effAK4->GetPaintedGraph();
            if (graphAK4) {
                graphAK4->GetYaxis()->SetRangeUser(0.0, yMax);
                graphAK4->GetXaxis()->SetRangeUser(vMin, vMax);
            }

            effAK8->Draw("P SAME");
            effAK15->Draw("P SAME");
            effTau->Draw("P SAME");
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

            TProfile* pAK4  = makeProfile(fAK4,  yAK4,  "ak4",  kBlue);
            TProfile* pAK8  = makeProfile(fAK8,  yAK8,  "ak8",  kRed);
            TProfile* pAK15 = makeProfile(fAK15, yAK15, "ak15", kGreen + 2);
            TProfile* pTau  = makeProfile(fTau, yTau, "tau", kBlack);

            pAK4->SetTitle(Form(";%s;%s", xAxisTitle, yAxisTitle));
            pAK4->GetYaxis()->SetRangeUser(yMin, yMax);
            pAK4->GetXaxis()->SetRangeUser(vMin, vMax);
            pAK4->Draw("P");
            pAK8->Draw("P SAME");
            pAK15->Draw("P SAME");
            pTau->Draw("P SAME");

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

            JetSample jets[] = {
                {fAK4,  "ak4",  kBlue},
                {fAK8,  "ak8",  kRed},
                {fAK15, "ak15", kGreen + 2},
                {fTau, "tau", kBlack}
            };

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
        TGraph* pAK4_leg  = new TGraph(); pAK4_leg->SetMarkerStyle(20); pAK4_leg->SetMarkerColor(kBlue);     pAK4_leg->SetLineColor(kBlue);
        TGraph* pAK8_leg  = new TGraph(); pAK8_leg->SetMarkerStyle(20); pAK8_leg->SetMarkerColor(kRed);      pAK8_leg->SetLineColor(kRed);
        TGraph* pAK15_leg = new TGraph(); pAK15_leg->SetMarkerStyle(20); pAK15_leg->SetMarkerColor(kGreen+2); pAK15_leg->SetLineColor(kGreen+2);
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
        leg->AddEntry(pAK4_leg,  "Anti k_{T}, R = 0.4, p_{T} > 30 GeV, |#eta| < 2.5", "lp");
        leg->AddEntry(pAK8_leg,  "Anti k_{T}, R = 0.8, p_{T} > 200 GeV, |#eta| < 2.5", "lp");
        leg->AddEntry(pAK15_leg, "Anti k_{T}, R = 1.5, p_{T} > 150 GeV, |#eta| < 2.5", "lp");
        leg->Draw();


        c1->SaveAs(TString(save_path) + "/JetFatJetAK15_eff_vs_genH_pt_PV_npvsGood_genTau_pt_asym.png"); 
        std::cout << "Done! Saved to " << save_path << "/JetFatJetAK15_eff_SideBySide.png" << std::endl;
    }