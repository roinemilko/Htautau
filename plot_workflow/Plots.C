#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <THStack.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TString.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <iostream>
#include <vector>
#include <TLatex.h>
#include <TStyle.h>

TString CutMissingValues(const TString& expr, float km = -998.f) {

    if (expr.BeginsWith("abs(")) {
        TString inner = expr;
        inner.ReplaceAll("abs(", "");
        inner.ReplaceAll(")", "");
        return Form("%s > %f", inner.Data(), km);
    }
    if (expr.Contains("/")) {
        TObjArray* parts = expr.Tokenize("/");
        TString num = ((TObjString*)parts->At(0))->GetString();
        TString den = ((TObjString*)parts->At(1))->GetString();
        return Form("(%s > %f) && (%s > %f)", num.Data(), km, den.Data(), km);
    }
    return Form("%s > %f", expr.Data(), km);
}

void Plots(bool AK4 = true, bool AK8 = true, bool AK15 = true,  bool Tau = true,
        const char* bg_path = "",
        const char* bg_channel = "",
        const char* params = "X_mass/X_pt, X_eta",
        const char* save_path = "/eos/user/m/mroine/www/",
        bool normalize = true,
        bool require_hadhad = true,
        const char* jet_path = "jets") {


    const int kNBins = 400;
    TString paramStr(params);
    TObjArray* paramArray = paramStr.Tokenize(",");
    int nParams = paramArray->GetEntries();

    TString hadhad_str = require_hadhad ? "_hadhad" : "";

    if (nParams == 0) {
        std::cerr << "Nothing to plot" << std::endl;
        return;
    }

    TFile *AK4_f = nullptr, *AK8_f = nullptr, *AK15_f = nullptr, *Tau_f = nullptr;
    TTree *AK4_tree = nullptr, *AK8_tree = nullptr, *AK15_tree = nullptr, *Tau_tree = nullptr;

    TFile *AK4_bg_f = nullptr, *AK8_bg_f = nullptr, *AK15_bg_f = nullptr, *Tau_bg_f = nullptr;
    TTree *AK4_bg_tree = nullptr, *AK8_bg_tree = nullptr, *AK15_bg_tree = nullptr, *Tau_bg_tree = nullptr;

    bool do_bg = (TString(bg_path) != "");

    if (AK4) {
        TString file_path = TString(jet_path) + "/Jet" + hadhad_str + ".root"; 
        AK4_f = new TFile(file_path, "READ");
        if (AK4_f) {AK4_tree = (TTree*)AK4_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file Jet.root" << std::endl;
        }

        if (do_bg) {
            TString bg_file_path = TString(bg_path) + "/JetBG_" + TString(bg_channel) + ".root"; 
            AK4_bg_f = new TFile(bg_file_path, "READ");
            if (AK4_bg_f && !AK4_bg_f->IsZombie()) AK4_bg_tree = (TTree*)AK4_bg_f->Get("Events");
        }        
    }

    if (AK8) {
        TString file_path = TString(jet_path) + "/fatJet" + hadhad_str + ".root"; 
        AK8_f = new TFile(file_path, "READ");
        if (AK8_f) {AK8_tree = (TTree*)AK8_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file fatJet.root" << std::endl;
        }

        if (do_bg) {
            TString bg_file_path = TString(bg_path) + "/fatJetBG_" + TString(bg_channel) + ".root"; 
            AK8_bg_f = new TFile(bg_file_path, "READ");
            if (AK8_bg_f && !AK8_bg_f->IsZombie()) AK8_bg_tree = (TTree*)AK8_bg_f->Get("Events");
        }  
    }

    if (AK15) {
        TString file_path = TString(jet_path) + "/AK15" + hadhad_str + ".root"; 
        AK15_f = new TFile(file_path, "READ");
        if (AK15_f) {AK15_tree = (TTree*)AK15_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file AK15.root" << std::endl;
        }

        if (do_bg) {
            TString bg_file_path = TString(bg_path) + "/AK15BG_" + TString(bg_channel) + ".root"; 
            AK15_bg_f = new TFile(bg_file_path, "READ");
            if (AK15_bg_f && !AK4_bg_f->IsZombie()) AK15_bg_tree = (TTree*)AK15_bg_f->Get("Events");
        }  
    }

    if (Tau) {
        TString file_path = TString(jet_path) + "/Tau" + hadhad_str + ".root"; 
        Tau_f = new TFile(file_path, "READ");
        if (Tau_f) {Tau_tree = (TTree*)Tau_f->Get("Events");}
        else {
            std::cerr << "Couldn't open file AK15.root" << std::endl;
        }

        if (do_bg) {
            TString bg_file_path = TString(bg_path) + "/TauBG_" + TString(bg_channel) + ".root"; 
            Tau_bg_f = new TFile(bg_file_path, "READ");
            if (Tau_bg_f && !Tau_bg_f->IsZombie()) Tau_bg_tree = (TTree*)Tau_bg_f->Get("Events");
        }  
    }

    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    // https://root.cern.ch/doc/master/classTCanvas.html
    TCanvas* c1 = new TCanvas("c1", "Jet plots", 400 * nParams, 450);
    c1->Divide(nParams, 1); 

    TLegend* leg = new TLegend(0.55, 0.82, 0.98, 0.96);
    leg->SetTextSize(0.03);
    leg->SetNColumns(2); 
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->SetMargin(0.20);

    for (int i = 0; i < nParams; ++i) {
        c1->cd(i + 1); 
        gPad->SetTopMargin(0.20);

        TString baseExpr = ((TObjString*)paramArray->At(i))->GetString();
        baseExpr.ReplaceAll(" ", ""); 

        TString hist_title = baseExpr;
        hist_title.ReplaceAll("X_", "");
        hist_title.ReplaceAll("_", " ");
        hist_title.ReplaceAll("pt", "p_{T}");
        hist_title.ReplaceAll("eta", "#eta");
        hist_title.ReplaceAll("phi", "#phi");
        hist_title.ReplaceAll("minus", " - ");
        hist_title.ReplaceAll("over", " / ");



        if (baseExpr.Contains("pt") || baseExpr.Contains("mass") || baseExpr.Contains("sumEt")) {
            if (!baseExpr.Contains("over") && !baseExpr.Contains("/")) {
                hist_title += " [GeV]";
            }
        }
        
        // https://root.cern.ch/doc/master/classTHStack.html
        THStack* hs = new THStack(Form("hs_%d", i), "");


        if (AK4) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "ak4"); 
            
            if (AK4_bg_tree) {
                TString hNameBg = Form("h4_bg_%d", i);
                if (AK4_bg_tree->Draw(expr + ">>" + hNameBg + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hNameBg);
                    if (h) {
                        h->SetLineColor(kBlue + 5);
                        h->SetLineStyle(2); // Dashed for Background
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h);
                        if (i == 0) leg->AddEntry(h, "Anti k_{T}, R = 0.4 (Background)", "l");
                    }
                }
            }
            if (AK4_tree) {
                TString hName = Form("h4_%d", i);
                if (AK4_tree->Draw(expr + ">>" + hName + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hName);
                    if (h) {
                        h->SetLineColor(kBlue);
                        h->SetLineStyle(1);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h);
                        if (i == 0) leg->AddEntry(h, "Anti k_{T}, R = 0.4, p_{T} > 30, |#eta| < 2.5", "l");
                    }
                }
            }
        }

        if (AK8) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "fj"); 
            
            if (AK8_bg_tree) {
                TString hNameBg = Form("h8_bg_%d", i);
                if (AK8_bg_tree->Draw(expr + ">>" + hNameBg + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hNameBg);
                    if (h) {
                        h->SetLineColor(kRed + 5);
                        h->SetLineStyle(2);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h);
                        if (i == 0) leg->AddEntry(h, "Anti k_{T}, R = 0.8 (Background)", "l");
                    }
                }
            }
            if (AK8_tree) {
                TString hName = Form("h8_%d", i);
                if (AK8_tree->Draw(expr + ">>" + hName + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hName);
                    if (h) {
                        h->SetLineColor(kRed);
                        h->SetLineStyle(1);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h);
                        if (i == 0) leg->AddEntry(h, "Anti k_{T}, R = 0.8, p_{T} > 200, |#eta| < 2.5", "l");
                    }
                }
            }
        }

        if (AK15) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "ak15"); 
            
            if (AK15_bg_tree) {
                TString hNameBg = Form("h15_bg_%d", i);
                if (AK15_bg_tree->Draw(expr + ">>" + hNameBg + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hNameBg);
                    if (h) {
                        h->SetLineColor(kGreen + 7);
                        h->SetLineStyle(2);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h);
                        if (i == 0) leg->AddEntry(h, "Anti k_{T}, R = 1.5 (Background)", "l");
                    }
                }
            }
            if (AK15_tree) {
                TString hName = Form("h15_%d", i);
                if (AK15_tree->Draw(expr + ">>" + hName + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hName);
                    if (h) {
                        h->SetLineColor(kGreen + 2);
                        h->SetLineStyle(1);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h); 
                        if (i == 0) leg->AddEntry(h, "Anti k_{T}, R = 1.5, p_{T} > 150, |#eta| < 2.5", "l");
                    }
                }
            }
        }

        if (Tau) {
            TString expr = baseExpr;
            expr.ReplaceAll("X", "tau"); 
            
            if (Tau_bg_tree) {
                TString hNameBg = Form("htau_bg_%d", i);
                if (Tau_bg_tree->Draw(expr + ">>" + hNameBg + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hNameBg);
                    if (h) {
                        h->SetLineColor(kBlack + 5);
                        h->SetLineStyle(2);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h); 
                        if (i == 0) leg->AddEntry(h, "Slimmed tau (Background)", "l");
                    }
                }
            }
            if (Tau_tree) {
                TString hName = Form("htau_%d", i);
                if (Tau_tree->Draw(expr + ">>" + hName + Form("(%d)", kNBins), CutMissingValues(expr)) != -1) {
                    TH1F* h = (TH1F*)gDirectory->Get(hName);
                    if (h) {
                        h->SetLineColor(kBlack);
                        h->SetLineStyle(1);
                        h->SetLineWidth(2);
                        if (normalize) h->Scale(1./h->Integral());
                        hs->Add(h); 
                        if (i == 0) leg->AddEntry(h, "Slimmed tau after basic selection, |#eta| < 2.5", "l");
                    }
                }
            }
        }
    


        gPad->SetLeftMargin(0.15);
        hs->Draw("NOSTACK HIST");
        gPad->Update();
        hs->GetXaxis()->SetTitle(hist_title); 

        if (normalize) {
            hs->GetYaxis()->SetTitle("A.u.");
        } else {
            hs->GetYaxis()->SetTitle("Number of jets");
        }
        hs->GetYaxis()->SetTitleOffset(2.5);
        
        double dynamic_xmax = 0.0;
        double dynamic_xmin = 0.0;
        TIter next(hs->GetHists());
        TH1F *hist;
        bool have_range = false;
        
        while ((hist = (TH1F*)next())) {
            const double total_integral = hist->Integral();
            if (total_integral <= 0) continue;
            double running_sum = 0;
            double hist_xmin = 0;
            double hist_xmax = 0;
            bool got_min = false;
            bool got_max = false;
            for (int b = 1; b <= hist->GetNbinsX(); ++b) {
                running_sum += hist->GetBinContent(b);
                const double frac = running_sum / total_integral;
                // lower edge at 5% cumulative
                if (!got_min && frac >= 0.05) {
                    hist_xmin = hist->GetBinLowEdge(b);
                    got_min = true;
                }
                // upper edge at 95% cumulative
                if (!got_max && frac >= 0.95) {
                    hist_xmax = hist->GetBinLowEdge(b) + hist->GetBinWidth(b);
                    got_max = true;
                    break;
                }
            }
            if (got_min && got_max) {
                if (!have_range) {
                    dynamic_xmin = hist_xmin;
                    dynamic_xmax = hist_xmax;
                    have_range = true;
                } else {
                    if (hist_xmin < dynamic_xmin) dynamic_xmin = hist_xmin;
                    if (hist_xmax > dynamic_xmax) dynamic_xmax = hist_xmax;
                }
            }
        }
        if (have_range && dynamic_xmax > dynamic_xmin) {
            const double pad = 0.05 * (dynamic_xmax - dynamic_xmin);
            hs->GetXaxis()->SetRangeUser(dynamic_xmin - pad, dynamic_xmax + pad);
        } else {
            hs->GetXaxis()->SetRangeUser(hs->GetXaxis()->GetXmin(), hs->GetXaxis()->GetXmax());
        }
    }

    c1->cd(0); 

    leg->Draw();

    TLatex latex;
    latex.SetNDC();

    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.05, 0.93, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.05, 0.88, "Simulation, Work in Progress");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.05, 0.83, "H #rightarrow #tau#tau (125 GeV)");

    c1->SaveAs(save_path);
}