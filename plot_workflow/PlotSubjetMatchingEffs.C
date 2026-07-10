    #include <iostream>
    #include "TCanvas.h"
    #include "TLegend.h"
    #include "TStyle.h"
    #include "TAxis.h"
    #include "TString.h"
    #include "TLatex.h"
    #include "eff_helpers.h"
    #include "TLine.h"
    #include "TFile.h"
    #include "TTree.h"
    #include "TH1F.h"
    #include "TEfficiency.h"
    #include "TGraphAsymmErrors.h"


    void PlotSubjetMatchingEffs(
        const char* save_path = "/eos/user/m/mroine/www/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8",
        const char* fRaw  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/RawEventInfo.root",
        const char* fAK8  = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/fatJet.root",
        const char* fAK15 = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/VBFHHto2B2Tau_Par-CV-1-C2V-0-C3-1_TuneCP5_13p6TeV_madgraph-pythia8/AK15.root",
        const char* subjetCutAK8  = "fj_nMatchedSubjets >= 1",
        const char* subjetCutAK15 = "ak15_nMatchedSubjets >= 1"
    ) {
        gStyle->SetOptStat(0);

        const int   nBins = 100;
        const float vMin  = 0.f;
        const float vMax  = 800.f;
        const double yMax = 1.05;

        TCanvas* c1 = new TCanvas("c_subjet_eff", "", 1600, 850);

        TPad* pad1_top = new TPad("pad1_top", "", 0.0, 0.28, 0.5, 1.0);
        TPad* pad1_bot = new TPad("pad1_bot", "", 0.0, 0.0,  0.5, 0.28);
        pad1_top->SetBottomMargin(0.02); 
        pad1_top->SetLeftMargin(0.12);
        pad1_bot->SetTopMargin(0.04);
        pad1_bot->SetBottomMargin(0.35); 
        pad1_bot->SetLeftMargin(0.12);
        pad1_top->Draw(); 
        pad1_bot->Draw();

        TPad* pad2_top = new TPad("pad2_top", "", 0.5, 0.28, 1.0, 1.0);
        TPad* pad2_bot = new TPad("pad2_bot", "", 0.5, 0.0,  1.0, 0.28);
        pad2_top->SetBottomMargin(0.02);
        pad2_top->SetLeftMargin(0.12);
        pad2_bot->SetTopMargin(0.04);
        pad2_bot->SetBottomMargin(0.35);
        pad2_bot->SetLeftMargin(0.12);
        pad2_top->Draw();
        pad2_bot->Draw();      
        
        
        auto drawSubjetEff = [&](TPad* pTop, TPad* pBot, int padNum,
                                const char* denFile,
                                const char* denVar,
                                const char* denCut,
                                const char* numFileAK8,
                                const char* numFileAK15,
                                const char* numVar,
                                const char* numCutAK8,
                                const char* numCutAK15,
                                const char* padTitle)
        {
            c1->cd(padNum);
            gPad->SetTopMargin(0.20);
            gPad->SetLeftMargin(0.20);

            TString prefix = Form("pad%d", padNum);

            TH1F* h_bg = new TH1F(prefix + "_bg", Form(";genH_pT [GeV];%s", padTitle), nBins, vMin, vMax);
            ProjectFromTree(fRaw, h_bg, "genH_pt_raw", "");

            TH1F* h_den       = new TH1F(prefix + "_den",       "", nBins, vMin, vMax);
            TH1F* h_num_ak8   = new TH1F(prefix + "_num_ak8",   "", nBins, vMin, vMax);
            TH1F* h_num_ak15  = new TH1F(prefix + "_num_ak15",  "", nBins, vMin, vMax);
            TH1F* h_den_ak8   = new TH1F(prefix + "_den_ak8",   "", nBins, vMin, vMax);
            TH1F* h_den_ak15  = new TH1F(prefix + "_den_ak15",  "", nBins, vMin, vMax);

            if (padNum == 1) {
                ProjectFromTree(denFile, h_den, denVar, denCut);
                ProjectFromTree(fAK8,  h_num_ak8,  numVar, numCutAK8);
                ProjectFromTree(fAK15, h_num_ak15, numVar, numCutAK15);
            } else {
                ProjectFromTree(fAK8,  h_den_ak8,  denVar, denCut);
                ProjectFromTree(fAK15, h_den_ak15, denVar, denCut);
                ProjectFromTree(fAK8,  h_num_ak8,  numVar, numCutAK8);
                ProjectFromTree(fAK15, h_num_ak15, numVar, numCutAK15);
            }

            pTop->cd();
            
            TH1* frame_top = pTop->DrawFrame(vMin, 0.0, vMax, yMax, Form("; ;%s", padTitle));
            frame_top->GetXaxis()->SetLabelSize(0); 
            frame_top->GetXaxis()->SetTitleSize(0);
            frame_top->GetYaxis()->SetTitleSize(0.05);
            frame_top->GetYaxis()->SetLabelSize(0.04);

            TEfficiency* effAK8  = (padNum == 1)
                ? new TEfficiency(*h_num_ak8,  *h_den)
                : new TEfficiency(*h_num_ak8,  *h_den_ak8);

            TEfficiency* effAK15 = (padNum == 1)
                ? new TEfficiency(*h_num_ak15, *h_den)
                : new TEfficiency(*h_num_ak15, *h_den_ak15);

            effAK8->SetMarkerStyle(20);
            effAK8->SetMarkerSize(0.7);
            effAK8->SetMarkerColor(kRed);
            effAK8->SetLineColor(kRed);

            effAK15->SetMarkerStyle(20);
            effAK15->SetMarkerSize(0.7);
            effAK15->SetMarkerColor(kGreen + 2);
            effAK15->SetLineColor(kGreen + 2);

            effAK8->Draw("P SAME");
            gPad->Update();
            if (auto* g = effAK8->GetPaintedGraph()) {
                g->GetYaxis()->SetRangeUser(0.0, yMax);
                g->GetXaxis()->SetRangeUser(vMin, vMax);
            }
            effAK15->Draw("P SAME");

            TLegend* leg = new TLegend(0.55, 0.72, 0.88, 0.88);
            leg->SetBorderSize(0);
            leg->SetFillStyle(0);
            leg->SetTextSize(0.035);
            leg->AddEntry(effAK8,  "Anti k_{T}, R = 0.8, p_{T} > 200 GeV", "lp");
            leg->AddEntry(effAK15, "Anti k_{T}, R = 1.5, p_{T} > 150 GeV", "lp");
            leg->Draw();

            pBot->cd();
            
            TH1* frame_bot = pBot->DrawFrame(vMin, 0.0, vMax, h_bg->GetMaximum() * 1.15, ";genH_pT} [GeV];Events");
            
            frame_bot->GetXaxis()->SetTitleSize(0.12);
            frame_bot->GetXaxis()->SetLabelSize(0.10);
            frame_bot->GetXaxis()->SetTitleOffset(1.1);
            frame_bot->GetYaxis()->SetTitleSize(0.10);
            frame_bot->GetYaxis()->SetLabelSize(0.08);
            frame_bot->GetYaxis()->SetTitleOffset(0.5);
            frame_bot->GetYaxis()->SetNdivisions(505);

            h_bg->SetFillColorAlpha(kGray+1, 0.5);
            h_bg->SetLineColor(kBlack);
            h_bg->Draw("HIST SAME");

            std::cout << padTitle << "\n";
            std::cout << "  AK8  integral num/den = "
                    << h_num_ak8->Integral() << " / "
                    << (padNum == 1 ? h_den->Integral() : h_den_ak8->Integral()) << "\n";
            std::cout << "  AK15 integral num/den = "
                    << h_num_ak15->Integral() << " / "
                    << (padNum == 1 ? h_den->Integral() : h_den_ak15->Integral()) << "\n";
        };



        // subjet matched / all events
        drawSubjetEff(
            pad1_top, pad1_bot,
            1,
            fRaw, "genH_pt_raw", "",
            fAK8, fAK15,
            "genH_pt",
            subjetCutAK8, subjetCutAK15,
            "Subjet matching efficiency (loose) / all events"
        );

        // subjet matched / jet matched
        drawSubjetEff(
            pad2_top, pad2_bot,
            2,
            fAK8, "genH_pt", "",
            fAK8, fAK15,
            "genH_pt",
            subjetCutAK8, subjetCutAK15,
            "Subjet matching efficiency (loose) / matched jet"
        );

        pad1_top->cd();
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
        latex.DrawLatex(0.13, 0.70, "H #rightarrow #tau#tau (125 GeV)");
    
        c1->SaveAs(save_path);
        std::cout << "Saved: " << save_path <<std::endl;
    }