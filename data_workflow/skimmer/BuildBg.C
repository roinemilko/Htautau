#include <TChain.h> 
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <string>
#include "BuildData.h"
#include "helpers_v2.h"
#include "ROOT/RDataFrame.hxx"
#include <ROOT/RDFHelpers.hxx>
#include "BuildBg.h"
#include <TSystem.h>

void BuildBg(
    const char* file_pattern = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/bg_data/TTtoLNu2Q/nano_*.root",
    const char* save_directory = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/background/POWHEG",
    const char* identifyer = "_TTtoLNu2Q",
    const bool include_Tau = true,
    const bool include_AK8 = false,
    const bool include_AK15 = false,
    int threads = 6
) {

    if (!include_Tau && !include_AK8 && !include_AK15) {
        std::cerr << "You have to include somehting" <<std::endl;
        return;
    }

    if (threads > 1) {
        ROOT::EnableImplicitMT(threads);
    }

    gSystem->mkdir(save_directory, true);
    ROOT::RDataFrame df("Events", file_pattern);
    ROOT::RDF::RNode df_Tau = df;
    ROOT::RDF::RNode df_AK4 = df;
    ROOT::RDF::RNode df_AK8 = df;
    ROOT::RDF::RNode df_AK15 = df;

    if (include_Tau) {
        df_Tau = df
        .Filter("nTau >= 2", "Require at least two taus")
        .Define("BestTaus", "GetHighestTauScorePair(Tau_rawPNetVSjet, Tau_pt)")
        .Define("BestTau_1", "BestTaus[0]")
        .Define("BestTau_2", "BestTaus[1]")
        .Filter("Tau_eta[BestTau_1] < 2.5 && Tau_eta[BestTau_2] < 2.5", "Same eta cut as in signal")
        
        .Define("tau_pt",          "ROOT::VecOps::RVec<float>{Tau_pt[BestTau_1], Tau_pt[BestTau_2]}")
        .Define("tau_eta",         "ROOT::VecOps::RVec<float>{Tau_eta[BestTau_1], Tau_eta[BestTau_2]}")
        .Define("tau_phi",         "ROOT::VecOps::RVec<float>{Tau_phi[BestTau_1], Tau_phi[BestTau_2]}")
        .Define("tau_mass",        "ROOT::VecOps::RVec<float>{Tau_mass[BestTau_1], Tau_mass[BestTau_2]}")   
        .Define("tau_dxy",         "ROOT::VecOps::RVec<float>{Tau_dxy[BestTau_1], Tau_dxy[BestTau_2]}")
        .Define("tau_dz",          "ROOT::VecOps::RVec<float>{Tau_dz[BestTau_1], Tau_dz[BestTau_2]}")
        .Define("tau_ipLengthSig", "ROOT::VecOps::RVec<float>{Tau_ipLengthSig[BestTau_1], Tau_ipLengthSig[BestTau_2]}")
        .Define("tau_chargedIso",  "ROOT::VecOps::RVec<float>{Tau_chargedIso[BestTau_1], Tau_chargedIso[BestTau_2]}")
        .Define("tau_neutralIso",  "ROOT::VecOps::RVec<float>{Tau_neutralIso[BestTau_1], Tau_neutralIso[BestTau_2]}")
        .Define("tau_rawIso",      "ROOT::VecOps::RVec<float>{Tau_rawIso[BestTau_1], Tau_rawIso[BestTau_2]}")
        .Define("tau_rawIsodR03",  "ROOT::VecOps::RVec<float>{Tau_rawIsodR03[BestTau_1], Tau_rawIsodR03[BestTau_2]}")
        .Define("tau_puCorr",      "ROOT::VecOps::RVec<float>{Tau_puCorr[BestTau_1], Tau_puCorr[BestTau_2]}")
        .Define("tau_charge",      "ROOT::VecOps::RVec<int>{Tau_charge[BestTau_1], Tau_charge[BestTau_2]}")

        .Define("tau_rawDeepTauVSjet", "ROOT::VecOps::RVec<float>{Tau_rawDeepTau2018v2p5VSjet[BestTau_1], Tau_rawDeepTau2018v2p5VSjet[BestTau_2]}")
        .Define("tau_rawDeepTauVSe",   "ROOT::VecOps::RVec<float>{Tau_rawDeepTau2018v2p5VSe[BestTau_1],   Tau_rawDeepTau2018v2p5VSe[BestTau_2]}")
        .Define("tau_rawDeepTauVSmu",  "ROOT::VecOps::RVec<float>{Tau_rawDeepTau2018v2p5VSmu[BestTau_1],  Tau_rawDeepTau2018v2p5VSmu[BestTau_2]}")

        .Define("tau_rawPNetVSjet",  "ROOT::VecOps::RVec<float>{Tau_rawPNetVSjet[BestTau_1], Tau_rawPNetVSjet[BestTau_2]}")
        .Define("tau_rawPNetVSe",    "ROOT::VecOps::RVec<float>{Tau_rawPNetVSe[BestTau_1],   Tau_rawPNetVSe[BestTau_2]}")
        .Define("tau_rawPNetVSmu",   "ROOT::VecOps::RVec<float>{Tau_rawPNetVSmu[BestTau_1],  Tau_rawPNetVSmu[BestTau_2]}")
        .Define("tau_probDM0PNet",   "ROOT::VecOps::RVec<float>{Tau_probDM0PNet[BestTau_1],  Tau_probDM0PNet[BestTau_2]}")
        .Define("tau_probDM1PNet",   "ROOT::VecOps::RVec<float>{Tau_probDM1PNet[BestTau_1],  Tau_probDM1PNet[BestTau_2]}")
        .Define("tau_probDM2PNet",   "ROOT::VecOps::RVec<float>{Tau_probDM2PNet[BestTau_1],  Tau_probDM2PNet[BestTau_2]}")
        .Define("tau_probDM10PNet",  "ROOT::VecOps::RVec<float>{Tau_probDM10PNet[BestTau_1], Tau_probDM10PNet[BestTau_2]}")
        .Define("tau_probDM11PNet",  "ROOT::VecOps::RVec<float>{Tau_probDM11PNet[BestTau_1], Tau_probDM11PNet[BestTau_2]}")
        .Define("tau_ptCorrPNet",    "ROOT::VecOps::RVec<float>{Tau_ptCorrPNet[BestTau_1],   Tau_ptCorrPNet[BestTau_2]}")
        .Define("tau_qConfPNet",     "ROOT::VecOps::RVec<float>{Tau_qConfPNet[BestTau_1],    Tau_qConfPNet[BestTau_2]}")

        .Define("tau_rawUParTVSjet", "ROOT::VecOps::RVec<float>{Tau_rawUParTVSjet[BestTau_1], Tau_rawUParTVSjet[BestTau_2]}")
        .Define("tau_rawUParTVSe",   "ROOT::VecOps::RVec<float>{Tau_rawUParTVSe[BestTau_1],   Tau_rawUParTVSe[BestTau_2]}")
        .Define("tau_rawUParTVSmu",  "ROOT::VecOps::RVec<float>{Tau_rawUParTVSmu[BestTau_1],  Tau_rawUParTVSmu[BestTau_2]}")
        .Define("tau_probDM0UParT",  "ROOT::VecOps::RVec<float>{Tau_probDM0UParT[BestTau_1],  Tau_probDM0UParT[BestTau_2]}")
        .Define("tau_probDM1UParT",  "ROOT::VecOps::RVec<float>{Tau_probDM1UParT[BestTau_1],  Tau_probDM1UParT[BestTau_2]}")
        .Define("tau_probDM2UParT",  "ROOT::VecOps::RVec<float>{Tau_probDM2UParT[BestTau_1],  Tau_probDM2UParT[BestTau_2]}")
        .Define("tau_probDM10UParT", "ROOT::VecOps::RVec<float>{Tau_probDM10UParT[BestTau_1], Tau_probDM10UParT[BestTau_2]}")
        .Define("tau_probDM11UParT", "ROOT::VecOps::RVec<float>{Tau_probDM11UParT[BestTau_1], Tau_probDM11UParT[BestTau_2]}")
        .Define("tau_ptCorrUParT",   "ROOT::VecOps::RVec<float>{Tau_ptCorrUParT[BestTau_1],   Tau_ptCorrUParT[BestTau_2]}")
        .Define("tau_qConfUParT",    "ROOT::VecOps::RVec<float>{Tau_qConfUParT[BestTau_1],    Tau_qConfUParT[BestTau_2]}")

        .Define("tau_idDeepTauVSjet", "ROOT::VecOps::RVec<int>{Tau_idDeepTau2018v2p5VSjet[BestTau_1], Tau_idDeepTau2018v2p5VSjet[BestTau_2]}")
        .Define("tau_idDeepTauVSe",   "ROOT::VecOps::RVec<int>{Tau_idDeepTau2018v2p5VSe[BestTau_1],   Tau_idDeepTau2018v2p5VSe[BestTau_2]}")
        .Define("tau_idDeepTauVSmu",  "ROOT::VecOps::RVec<int>{Tau_idDeepTau2018v2p5VSmu[BestTau_1],  Tau_idDeepTau2018v2p5VSmu[BestTau_2]}")
        .Define("tau_decayModePNet",  "ROOT::VecOps::RVec<int>{Tau_decayModePNet[BestTau_1],  Tau_decayModePNet[BestTau_2]}")
        .Define("tau_decayModeUParT", "ROOT::VecOps::RVec<int>{Tau_decayModeUParT[BestTau_1], Tau_decayModeUParT[BestTau_2]}")

        .Define("tau_genPartFlav", "ROOT::VecOps::RVec<int>{Tau_genPartFlav[BestTau_1], Tau_genPartFlav[BestTau_2]}")
        .Define("tau_decayMode", "ROOT::VecOps::RVec<int>{Tau_decayMode[BestTau_1], Tau_decayMode[BestTau_2]}")
        .Define("tau_genPartIdx",   "ROOT::VecOps::RVec<int>{Tau_genPartIdx[BestTau_1],   Tau_genPartIdx[BestTau_2]}");

    }

    std::vector<ROOT::RDF::RResultHandle> snapshots;
    ROOT::RDF::RSnapshotOptions opts;
    opts.fLazy = true;

    if (include_Tau) {
        std::cout << "Skimming Tau background... " << std::endl;
        std::string out_dir = std::string(save_directory) + "/TauBG" + std::string(identifyer) + ".root";
        snapshots.push_back(df_Tau.Snapshot("Events", out_dir, TauBG_dict, opts));
    }

    if (!snapshots.empty()) {
        ROOT::RDF::RunGraphs(snapshots);
    }
    
}