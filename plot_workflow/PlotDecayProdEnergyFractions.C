#include "TCanvas.h"
#include "TFile.h"
#include "TH1F.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TTree.h"
#include "TLatex.h"
#include "TProfile.h"

void PlotDecayProdEnergyFractions(
    const char* file_in = "/eos/user/m/mroine/NanoTuples/Htautau/workflow/jets/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/RawEventInfo.root",
    const char* save_path = "/eos/user/m/mroine/www/GluGluH-HTo2Tau_Par-M-125_TuneCP5_13p6TeV_powheg-pythia8/EnergyFractionsOfDecayProds.png",
    const bool normalize = true
) {
    gStyle->SetOptStat(0);

    TFile f(file_in);
    TTree* t = f.Get<TTree>("Events");

    const int n_buckets = 100;
    const double genH_min = 0., genH_max = 1000.;

    // The distributions 
    TH1F h_nu( "h_nu",  ";p_{T}^{prod}/p_{T}^{#tau};A.u.", 50, 0, 1);
    TH1F h_pi( "h_pi",  "", n_buckets, 0, 1);
    TH1F h_gam("h_gam", "", n_buckets, 0, 1);
    TH1F h_mu( "h_mu",  "", n_buckets, 0, 1);
    TH1F h_e(  "h_e",   "", n_buckets, 0, 1);
    TH1F h_nue("h_nue", "", n_buckets, 0, 1);
    TH1F h_num("h_num", "", n_buckets, 0, 1);

    // Average as fn of Hpt
    TProfile p_nu( "p_nu",  ";genH_{T} [GeV];#LT p_{T}^{prod}/p_{T}^{#tau} #GT", n_buckets, genH_min, genH_max);
    TProfile p_pi( "p_pi",  "", n_buckets, genH_min, genH_max);
    TProfile p_gam("p_gam", "", n_buckets, genH_min, genH_max);
    TProfile p_mu( "p_mu",  "", n_buckets, genH_min, genH_max);
    TProfile p_e(  "p_e",   "", n_buckets, genH_min, genH_max);
    TProfile p_nue("p_nue", "", n_buckets, genH_min, genH_max);
    TProfile p_num("p_num", "", n_buckets, genH_min, genH_max);

    h_nu .SetLineColor(kBlack);
    h_pi .SetLineColor(kBlue);
    h_gam.SetLineColor(kGreen + 2);
    h_mu .SetLineColor(kRed);
    h_e  .SetLineColor(kMagenta);
    h_nue.SetLineColor(kOrange + 1);
    h_num.SetLineColor(kCyan + 1);

    p_nu .SetLineColor(kBlack);
    p_pi .SetLineColor(kBlue);
    p_gam.SetLineColor(kGreen + 2);
    p_mu .SetLineColor(kRed);
    p_e  .SetLineColor(kMagenta);
    p_nue.SetLineColor(kOrange + 1);
    p_num.SetLineColor(kCyan + 1);

    for (auto* p : {&p_nu, &p_pi, &p_gam, &p_mu, &p_e, &p_nue, &p_num}) {
        p->SetMarkerStyle(20);
        p->SetMarkerSize(0.5);
        p->SetLineWidth(2);
    }

    t->Draw("DecayProds_ptfrac >> h_nu",  "DecayProds_absid==16",  "goff");
    t->Draw("DecayProds_ptfrac >> h_pi",  "DecayProds_absid==211", "goff");
    t->Draw("DecayProds_ptfrac >> h_gam", "DecayProds_absid==22",  "goff");
    t->Draw("DecayProds_ptfrac >> h_mu",  "DecayProds_absid==13",  "goff");
    t->Draw("DecayProds_ptfrac >> h_e",   "DecayProds_absid==11",  "goff");
    t->Draw("DecayProds_ptfrac >> h_nue", "DecayProds_absid==12",  "goff");
    t->Draw("DecayProds_ptfrac >> h_num", "DecayProds_absid==14",  "goff");

    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_nu",  "DecayProds_absid==16",  "prof goff");
    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_pi",  "DecayProds_absid==211", "prof goff");
    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_gam", "DecayProds_absid==22",  "prof goff");
    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_mu",  "DecayProds_absid==13",  "prof goff");
    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_e",   "DecayProds_absid==11",  "prof goff");
    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_nue", "DecayProds_absid==12",  "prof goff");
    t->Draw("DecayProds_ptfrac:genH_pt_raw >> p_num", "DecayProds_absid==14",  "prof goff");

    if (normalize) {
        h_nu .Scale(1.0 / h_nu .Integral());
        h_pi .Scale(1.0 / h_pi .Integral());
        h_gam.Scale(1.0 / h_gam.Integral());
        h_mu .Scale(1.0 / h_mu .Integral());
        h_e  .Scale(1.0 / h_e  .Integral());
        h_nue.Scale(1.0 / h_nue.Integral());
        h_num.Scale(1.0 / h_num.Integral());
    }

    TCanvas c("c", "", 1800, 700);

    c.Divide(2, 1);

    c.cd(1);
    gPad->SetTopMargin(0.25);
    h_nu.Draw("hist");
    h_pi.Draw("hist same");
    h_gam.Draw("hist same");
    h_mu.Draw("hist same");
    h_e.Draw("hist same");
    h_nue.Draw("hist same");
    h_num.Draw("hist same");

    h_nu.GetYaxis()->SetRangeUser(0,0.1);


    c.cd(2);
    gPad->SetTopMargin(0.25);
    p_nu.GetYaxis()->SetRangeUser(0., 0.5);
    p_nu.Draw("E1");
    p_pi .Draw("E1 SAME");
    p_gam.Draw("E1 SAME");
    p_mu .Draw("E1 SAME");
    p_e  .Draw("E1 SAME");
    p_nue.Draw("E1 SAME");
    p_num.Draw("E1 SAME");



    c.cd(0);
    TLatex latex; 
    latex.SetTextFont(62);
    latex.SetTextSize(0.05);
    latex.DrawLatex(0.07, 0.91, "CMS");
    latex.SetTextFont(42);
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.07, 0.86, "Simulation, Work in Progress");
    latex.SetTextFont(42);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.07, 0.8, "H #rightarrow #tau#tau (125 GeV)");

    TLegend leg(0.65, 0.8, 0.95, 0.98);
    leg.SetBorderSize(0);
    leg.SetNColumns(4);
    leg.SetFillStyle(0);
    leg.AddEntry(&h_nu,  "#nu_{#tau}", "l");
    leg.AddEntry(&h_pi,  "#pi^{#pm}",  "l");
    leg.AddEntry(&h_gam, "#gamma",     "l");
    leg.AddEntry(&h_mu,  "#mu",        "l");
    leg.AddEntry(&h_e,   "e",          "l");
    leg.AddEntry(&h_nue, "#nu_{e}",  "l");
    leg.AddEntry(&h_num, "#nu_{#mu}", "l");
    leg.Draw();
    c.SaveAs(save_path);
}