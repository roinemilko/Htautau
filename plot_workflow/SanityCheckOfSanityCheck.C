#include "TLegend.h"
#include "TStyle.h"
#include "TAxis.h"
#include <TString.h>
#include <TH2D.h>
#include <TH1F.h>   
#include "TCanvas.h"
#include <TFile.h>
#include <TTree.h>
#include <TLine.h>
#include <TLatex.h>
#include <iostream>

std::map<TString, std::array<float, 6>> Ranges {
    {"Jet", {100.0f, 300.0f, 300.0f, 600.0f, 600.0f, 1000.0f}},
    {"fJ", {100.0f, 280.0f, 300.0f, 600.0f, 300.0f, 1000.0f}},
    {"AK15", {150.0f, 170.0f, 300.0f, 600.0f, 600.0f, 1000.0f}},
    {"Tau", {100.0f, 300.0f, 300.0f, 600.0f, 600.0f, 1000.0f}}
};

std::map<TString, TString> JetNamesPretty {
    {"Jet", "Anti k_{T}, R = 0.4, p_{T} > 30 GeV, |#eta| < 2.5"},
    {"fJ", "Anti k_{T}, R = 0.8, p_{T} > 200 GeV, |#eta| < 2.5"},
    {"AK15", "Anti k_{T}, R = 1.5, p_{T} > 150 GeV, |#eta| < 2.5"},
    {"Tau", "Skimmed taus after basic selection, |#eta| < 2.5"}
};

float higgs_mass = 125.0f;


void SanityCheckOfSanityCheck_run(const char* jet_id, const char* file_hadhad = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/RawEventInfo_hadhad.root",
                                    const char* out_file = "") {

    gStyle->SetOptTitle(0);
    TFile* f_hadhad = new TFile(file_hadhad, "READ");
    TTree* t_hadhad = (TTree*)f_hadhad->Get("Events");

    TString plot_param = TString::Format("dR_%s_nolim", jet_id);

    // Scatter plots

    t_hadhad->Draw(Form("%s:dR_tau1_tau2_raw >> h2_scatter_bin1(100, 0.0, 2.5, 100, 0.0, 2.5)", plot_param.Data()), Form("genH_pt_raw >= %f && genH_pt_raw <= %f", Ranges[jet_id][0], Ranges[jet_id][1]), "goff");
    t_hadhad->Draw(Form("%s:dR_tau1_tau2_raw >> h2_scatter_bin2(100, 0.0, 2.5, 100, 0.0, 2.5)", plot_param.Data()), Form("genH_pt_raw >= %f && genH_pt_raw <= %f", Ranges[jet_id][2], Ranges[jet_id][3]), "goff");
    t_hadhad->Draw(Form("%s:dR_tau1_tau2_raw >> h2_scatter_bin3(100, 0.0, 2.5, 100, 0.0, 2.5)", plot_param.Data()), Form("genH_pt_raw >= %f && genH_pt_raw <= %f", Ranges[jet_id][4], Ranges[jet_id][5]), "goff");
    t_hadhad->Draw(Form("%s:dR_tau1_tau2_raw >> h2_scatter_all(100, 0.0, 2.5, 100, 0.0, 2.5)", plot_param.Data()), "", "goff");

    TH2D* h2_Scatter_bin2 = (TH2D*)gDirectory->Get("h2_scatter_bin2");
    if (h2_Scatter_bin2) {
        h2_Scatter_bin2->SetTitle("; dR_tau1_tau2_raw; min(max(#Delta R(jet, tau1), #Delta R(jet, tau2))");
        h2_Scatter_bin2->SetMinimum(10);
    }

    TH2D* h2_Scatter_bin1 = (TH2D*)gDirectory->Get("h2_scatter_bin1");
    if (h2_Scatter_bin1) {
        h2_Scatter_bin1->SetTitle("; dR_tau1_tau2_raw; min(max(#Delta R(jet, tau1), #Delta R(jet, tau2))");
        h2_Scatter_bin1->SetMinimum(0);
    }

    TH2D* h2_Scatter_bin3 = (TH2D*)gDirectory->Get("h2_scatter_bin3");
    if (h2_Scatter_bin3) {
        h2_Scatter_bin3->SetTitle("; dR_tau1_tau2_raw; min(max(#Delta R(jet, tau1), #Delta R(jet, tau2))");
        h2_Scatter_bin3->SetMinimum(0);
    }

    TH2D* h2_Scatter_all = (TH2D*)gDirectory->Get("h2_scatter_all");
    if (h2_Scatter_all) {
        h2_Scatter_all->SetTitle(";dR_tau1_tau2_raw; min(max(#Delta R(jet, tau1), #Delta R(jet, tau2))");
        h2_Scatter_all->SetStats(0);
        h2_Scatter_all->SetMinimum(0);
    }

    h2_Scatter_bin1->SetStats(0);
    h2_Scatter_bin2->SetStats(0);
    h2_Scatter_bin3->SetStats(0);


    // Distributions

    t_hadhad->Draw(Form("%s[0] >> h_dist_bin1(100, 0.0, 2.5)", plot_param.Data()), Form("genH_pt_raw >= %f && genH_pt_raw <= %f", Ranges[jet_id][0], Ranges[jet_id][1]), "goff");
    t_hadhad->Draw(Form("%s[0] >> h_dist_bin2(100, 0.0, 2.5)", plot_param.Data()), Form("genH_pt_raw >= %f && genH_pt_raw <= %f", Ranges[jet_id][2], Ranges[jet_id][3]));
    t_hadhad->Draw(Form("%s[0] >> h_dist_bin3(100, 0.0, 2.5)", plot_param.Data()), Form("genH_pt_raw >= %f && genH_pt_raw <= %f", Ranges[jet_id][4], Ranges[jet_id][5]), "goff");
    t_hadhad->Draw(Form("%s[0] >> h_dist_all(100, 0.0, 2.5)", plot_param.Data()), "", "goff");
    
    TH1F* h_dist_bin1 = (TH1F*)gDirectory->Get("h_dist_bin1");
    TH1F* h_dist_bin2 = (TH1F*)gDirectory->Get("h_dist_bin2");
    TH1F* h_dist_bin3 = (TH1F*)gDirectory->Get("h_dist_bin3");
    TH1F* h_dist_all = (TH1F*)gDirectory->Get("h_dist_all");

    h_dist_bin1->Scale(1.0f /  h_dist_bin1->Integral());
    h_dist_bin2->Scale(1.0f /  h_dist_bin2->Integral());
    h_dist_bin3->Scale(1.0f /  h_dist_bin3->Integral());
    h_dist_all->Scale(1.0f /  h_dist_all->Integral());

    h_dist_bin1->SetTitle(";min(max(#Delta R(jet, tau1), #Delta R(jet, tau2));A.u.");
    h_dist_bin2->SetTitle(";min(max(#Delta R(jet, tau1), #Delta R(jet, tau2));A.u.");
    h_dist_bin3->SetTitle(";min(max(#Delta R(jet, tau1), #Delta R(jet, tau2));A.u.");    

    h_dist_bin1->SetStats(0);
    h_dist_bin2->SetStats(0);
    h_dist_bin3->SetStats(0);

    h_dist_bin1->SetLineWidth(2);
    h_dist_bin2->SetLineWidth(2);
    h_dist_bin3->SetLineWidth(2);

    h_dist_all->SetTitle(";min(max(#Delta R(jet, tau1), #Delta R(jet, tau2));A.u.");
    h_dist_all->SetStats(0);
    h_dist_all->SetLineWidth(2);


    TCanvas c1("c1", "c1", 1500, 1000);
    


    TLine yeqx(0.0, 0.0, 2.5, 2.5);
    TLine yeghalfx(0.0, 0.0, 2.5, 1.25);
    yeqx.SetLineColor(kRed);
    yeghalfx.SetLineColor(kRed + 5);

    TLegend* lines_leg = new TLegend(0.8, 0.1, 0.95, 0.25);
    lines_leg->SetBorderSize(0);
    lines_leg->SetTextSize(0.030);
    lines_leg->SetFillStyle(0);
    lines_leg->SetTextFont(42);
    lines_leg->SetMargin(0.25);  

    lines_leg->AddEntry(&yeqx, "y = x ");
    lines_leg->AddEntry(&yeghalfx, "y = #frac(1)(2) x");

    c1.Divide(3,2);

    c1.cd(1);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);
    gPad->SetTopMargin(0.25);
    h2_Scatter_bin1->Draw("colz");
    gPad->SetLogz(true);

    TLatex pad_latex;
    pad_latex.SetNDC();
    pad_latex.SetTextFont(42);   
    pad_latex.SetTextSize(0.03); 
    pad_latex.SetTextAlign(31);  
    yeqx.Draw("same");
    yeghalfx.Draw("same");


    pad_latex.DrawLatex(0.88, 0.77, Form("%i GeV < p_{T}^{H} < %i GeV", (int)Ranges[jet_id][0], (int)Ranges[jet_id][1]));

    TLatex latex;
    latex.SetNDC();
    
    c1.cd(2);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);
    gPad->SetTopMargin(0.25);
    h2_Scatter_bin2->Draw("colz");
    gPad->SetLogz(true);
    yeqx.Draw("same");
    yeghalfx.Draw("same");
    
    pad_latex.DrawLatex(0.88, 0.77, Form("%i GeV < p_{T}^{H} < %i GeV", (int)Ranges[jet_id][2], (int)Ranges[jet_id][3]));

    c1.cd(3);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);
    gPad->SetTopMargin(0.25);
    h2_Scatter_bin3->Draw("colz");
    gPad->SetLogz(true);
    yeqx.Draw("same");
    yeghalfx.Draw("same");

    pad_latex.DrawLatex(0.88, 0.77, Form("%i GeV < p_{T}^{H} < %i GeV", (int)Ranges[jet_id][4], (int)Ranges[jet_id][5]));

    c1.cd(4);
    h_dist_bin1->Draw("HIST SAME");
    gPad->SetTopMargin(0.15);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);
    pad_latex.DrawLatex(0.88, 0.87, Form("%i GeV < p_{T}^{H} < %i GeV", (int)Ranges[jet_id][0], (int)Ranges[jet_id][1]));

    c1.cd(5);
    h_dist_bin2->Draw("HIST SAME");
    gPad->SetTopMargin(0.15);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);
    pad_latex.DrawLatex(0.88, 0.87, Form("%i GeV < p_{T}^{H} < %i GeV", (int)Ranges[jet_id][2], (int)Ranges[jet_id][3]));

    c1.cd(6);
    h_dist_bin3->Draw("HIST SAME");
    gPad->SetTopMargin(0.15);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);
    pad_latex.DrawLatex(0.88, 0.87, Form("%i GeV < p_{T}^{H} < %i GeV", (int)Ranges[jet_id][4], (int)Ranges[jet_id][5]));


    c1.cd(0);
    latex.SetTextAlign(11); 
    latex.SetTextFont(62);  
    latex.SetTextSize(0.035);

    latex.DrawLatex(0.04, 0.96, "CMS");

    latex.SetTextFont(52);  
    latex.SetTextSize(0.02);
    latex.DrawLatex(0.04, 0.93, "Simulation, Work in Progress"); 


    latex.SetTextFont(42);  
    latex.SetTextSize(0.02);
    latex.DrawLatex(0.04, 0.90, "H #rightarrow #tau#tau (125 GeV)");


    latex.SetTextAlign(31); 
    latex.SetTextSize(0.023);

    latex.DrawLatex(0.96, 0.93, JetNamesPretty[jet_id]);

    TCanvas c2("c2", "All Events", 1000, 500);
    c2.Divide(2, 1);

    c2.cd(1);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.12);    
    gPad->SetTopMargin(0.20);

    if (h2_Scatter_all) {
        h2_Scatter_all->Draw("colz");
        gPad->SetLogz(true);
        yeqx.Draw("same");
        yeghalfx.Draw("same"); 
    }

    

    c2.cd(2);
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.12);
    gPad->SetTopMargin(0.20);
    if (h_dist_all) {
        h_dist_all->Draw("HIST");
        latex.SetTextSize(0.035);
        latex.SetTextAlign(31); 
        latex.SetTextFont(42);
        latex.DrawLatex(0.95, 0.90, JetNamesPretty[jet_id]); 
    }

    c2.cd(0); 

    latex.SetNDC();

    latex.SetTextAlign(11);
    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.075, 0.93, "CMS");

    latex.SetTextFont(52);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.075, 0.88, "Simulation, Work in Progress");

    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.075, 0.83, "H #rightarrow #tau#tau (125 GeV)");

    TString filename_c1 = TString(out_file) + "_" + plot_param + "_All.png";
    TString filename_c2 = TString(out_file) + "_" + plot_param +  + ".png";
    c2.SaveAs(filename_c1);
    c1.SaveAs(filename_c2);
}

void SanityCheckOfSanityCheck(const char* input_file, const char* out_prefix) {
    SanityCheckOfSanityCheck_run("Jet", input_file, out_prefix);
    SanityCheckOfSanityCheck_run("fJ", input_file, out_prefix);
    SanityCheckOfSanityCheck_run("AK15", input_file, out_prefix);
    SanityCheckOfSanityCheck_run("Tau", input_file, out_prefix);
    return;

}