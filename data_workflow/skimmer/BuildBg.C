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
    const char* file_pattern = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/bg_data/DYto2Tau/nano_*.root",
    const char* save_directory = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/background/POWHEG",
    const char* identifyer = "_DYto2Tau",
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

    if (include_AK8) {
        df_AK8 = df
            .Filter("nFatJet >= 1", "Require at least one fatJet")
            .Filter("FatJet_eta < AK8_eta_max && FatJet_pt > AK8_min_pt ", "Same cuts as in signal")
            .Define("FatJet_tautau_Score", "FatJet_globalParT3_Xtauhtauh / (FatJet_globalParT3_Xtauhtauh + FatJet_globalParT3_QCD)")
            .Define("BestFatJetIdx", "GetHighestTauScore(FatJet_tautau_Score)")

            // Standard Kinematics
            .Define("fj_pt", "FatJet_pt[BestFatJetIdx]")
            .Define("fj_eta", "FatJet_eta[BestFatJetIdx]")
            .Define("fj_phi", "FatJet_phi[BestFatJetIdx]")
            .Define("fj_mass", "FatJet_mass[BestFatJetIdx]")
            .Define("fj_msoftdrop", "FatJet_msoftdrop[BestFatJetIdx]")
            .Define("fj_rawFactor", "FatJet_rawFactor[BestFatJetIdx]")

            // Extended FatJet Properties
            .Define("fj_area", "FatJet_area[BestFatJetIdx]")
            .Define("fj_chEmEF", "FatJet_chEmEF[BestFatJetIdx]")
            .Define("fj_chHEF", "FatJet_chHEF[BestFatJetIdx]")
            .Define("fj_chMultiplicity", "FatJet_chMultiplicity[BestFatJetIdx]")
            .Define("fj_globalParT3_QCD", "FatJet_globalParT3_QCD[BestFatJetIdx]")
            .Define("fj_globalParT3_TopbWev", "FatJet_globalParT3_TopbWev[BestFatJetIdx]")
            .Define("fj_globalParT3_TopbWmv", "FatJet_globalParT3_TopbWmv[BestFatJetIdx]")
            .Define("fj_globalParT3_TopbWq", "FatJet_globalParT3_TopbWq[BestFatJetIdx]")
            .Define("fj_globalParT3_TopbWqq", "FatJet_globalParT3_TopbWqq[BestFatJetIdx]")
            .Define("fj_globalParT3_TopbWtauhv", "FatJet_globalParT3_TopbWtauhv[BestFatJetIdx]")
            .Define("fj_globalParT3_WvsQCD", "FatJet_globalParT3_WvsQCD[BestFatJetIdx]")
            .Define("fj_globalParT3_XWW3q", "FatJet_globalParT3_XWW3q[BestFatJetIdx]")
            .Define("fj_globalParT3_XWW4q", "FatJet_globalParT3_XWW4q[BestFatJetIdx]")
            .Define("fj_globalParT3_XWWqqev", "FatJet_globalParT3_XWWqqev[BestFatJetIdx]")
            .Define("fj_globalParT3_XWWqqmv", "FatJet_globalParT3_XWWqqmv[BestFatJetIdx]")
            .Define("fj_globalParT3_Xbb", "FatJet_globalParT3_Xbb[BestFatJetIdx]")
            .Define("fj_globalParT3_Xcc", "FatJet_globalParT3_Xcc[BestFatJetIdx]")
            .Define("fj_globalParT3_Xcs", "FatJet_globalParT3_Xcs[BestFatJetIdx]")
            .Define("fj_globalParT3_Xqq", "FatJet_globalParT3_Xqq[BestFatJetIdx]")
            .Define("fj_globalParT3_Xtauhtaue", "FatJet_globalParT3_Xtauhtaue[BestFatJetIdx]")
            .Define("fj_globalParT3_Xtauhtauh", "FatJet_globalParT3_Xtauhtauh[BestFatJetIdx]")
            .Define("fj_globalParT3_Xtauhtaum", "FatJet_globalParT3_Xtauhtaum[BestFatJetIdx]")
            .Define("fj_globalParT3_massCorrGeneric", "FatJet_globalParT3_massCorrGeneric[BestFatJetIdx]")
            .Define("fj_globalParT3_massCorrX2p", "FatJet_globalParT3_massCorrX2p[BestFatJetIdx]")
            .Define("fj_globalParT3_withMassTopvsQCD", "FatJet_globalParT3_withMassTopvsQCD[BestFatJetIdx]")
            .Define("fj_globalParT3_withMassWvsQCD", "FatJet_globalParT3_withMassWvsQCD[BestFatJetIdx]")
            .Define("fj_globalParT3_withMassZvsQCD", "FatJet_globalParT3_withMassZvsQCD[BestFatJetIdx]")
            .Define("fj_hadronFlavour", "FatJet_hadronFlavour[BestFatJetIdx]")
            .Define("fj_hfEmEF", "FatJet_hfEmEF[BestFatJetIdx]")
            .Define("fj_hfHEF", "FatJet_hfHEF[BestFatJetIdx]")
            .Define("fj_lsf3", "FatJet_lsf3[BestFatJetIdx]")
            .Define("fj_muEF", "FatJet_muEF[BestFatJetIdx]")
            .Define("fj_nConstituents", "FatJet_nConstituents[BestFatJetIdx]")
            .Define("fj_neEmEF", "FatJet_neEmEF[BestFatJetIdx]")
            .Define("fj_neHEF", "FatJet_neHEF[BestFatJetIdx]")
            .Define("fj_neMultiplicity", "FatJet_neMultiplicity[BestFatJetIdx]")
            .Define("fj_tau1", "FatJet_tau1[BestFatJetIdx]")
            .Define("fj_tau2", "FatJet_tau2[BestFatJetIdx]")
            .Define("fj_tau3", "FatJet_tau3[BestFatJetIdx]")
            .Define("fj_tau4", "FatJet_tau4[BestFatJetIdx]")

            // Subjets
            .Define("fj_subJetIdx1", "FatJet_subJetIdx1[BestFatJetIdx]")
            .Define("fj_subJetIdx2", "FatJet_subJetIdx2[BestFatJetIdx]")
            .Define("fj_nMatchedSubjets", "(int)(fj_subJetIdx1 >= 0) + (int)(fj_subJetIdx2 >= 0)")

            // Standard Kinematics
            .Define("fj_Subjet_pt", "FillMatchedSubjet(SubJet_pt, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_eta", "FillMatchedSubjet(SubJet_eta, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_phi", "FillMatchedSubjet(SubJet_phi, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_mass", "FillMatchedSubjet(SubJet_mass, fj_subJetIdx1, fj_subJetIdx2)")

            // Corrections & Area
            .Define("fj_Subjet_area", "FillMatchedSubjet(SubJet_area, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_rawFactor", "FillMatchedSubjet(SubJet_rawFactor, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("afj_Subjet_pt_rawFactorCorrected", "FillMatchedSubjet((1 - SubJet_rawFactor) * SubJet_pt, fj_subJetIdx1, fj_subJetIdx2)")

            // Substructure & N-subjettiness
            .Define("fj_Subjet_tau1", "FillMatchedSubjet(SubJet_tau1, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_tau2", "FillMatchedSubjet(SubJet_tau2, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_tau3", "FillMatchedSubjet(SubJet_tau3, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_tau4", "FillMatchedSubjet(SubJet_tau4, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_n2b1", "FillMatchedSubjet(SubJet_n2b1, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_n3b1", "FillMatchedSubjet(SubJet_n3b1, fj_subJetIdx1, fj_subJetIdx2)")

            // B-Tagging & Flavour
            .Define("fj_Subjet_btagDeepFlavB", "FillMatchedSubjet(SubJet_btagDeepFlavB, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_btagUParTAK4B", "FillMatchedSubjet(SubJet_btagUParTAK4B, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_hadronFlavour", "FillMatchedSubjet(SubJet_hadronFlavour, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_nBHadrons", "FillMatchedSubjet(SubJet_nBHadrons, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_nCHadrons", "FillMatchedSubjet(SubJet_nCHadrons, fj_subJetIdx1, fj_subJetIdx2)")

            // UParT Regressions & Resolutions
            .Define("fj_Subjet_UParTAK4RegPtRawCorr", "FillMatchedSubjet(SubJet_UParTAK4RegPtRawCorr, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_UParTAK4RegPtRawCorrNeutrino", "FillMatchedSubjet(SubJet_UParTAK4RegPtRawCorrNeutrino, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_UParTAK4RegPtRawRes", "FillMatchedSubjet(SubJet_UParTAK4RegPtRawRes, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_UParTAK4V1RegPtRawCorr", "FillMatchedSubjet(SubJet_UParTAK4V1RegPtRawCorr, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_UParTAK4V1RegPtRawCorrNeutrino", "FillMatchedSubjet(SubJet_UParTAK4V1RegPtRawCorrNeutrino, fj_subJetIdx1, fj_subJetIdx2)")
            .Define("fj_Subjet_UParTAK4V1RegPtRawRes", "FillMatchedSubjet(SubJet_UParTAK4V1RegPtRawRes, fj_subJetIdx1, fj_subJetIdx2)");
    }

    if (include_AK15) {
        df_AK15 = df
            .Filter("nAK15Puppi >= 1", "Require at least one AK15")
            .Filter("AK15Puppi_pt >= AK15_min_pt && AK15Puppi_eta < AK15_eta_max", "Same cuts as in signal")
            .Define("ak15_tautau_Score", "AK15Puppi_ParTv3_probXtauhtauh / (AK15Puppi_ParTv3_probXtauhtauh + AK15Puppi_ParTv3_probQCD)")
            .Define("BestAK15Idx", "GetHighestTauScore(ak15_tautau_Score)")

            // Standard Kinematics
            .Define("ak15_pt", "AK15Puppi_pt[BestAK15Idx]")
            .Define("ak15_eta", "AK15Puppi_eta[BestAK15Idx]")
            .Define("ak15_phi", "AK15Puppi_phi[BestAK15Idx]")
            .Define("ak15_mass", "AK15Puppi_mass[BestAK15Idx]")
            .Define("ak15_msoftdrop", "AK15Puppi_msoftdrop[BestAK15Idx]")
            .Define("ak15_rawFactor", "AK15Puppi_rawFactor[BestAK15Idx]")
            .Define("ak15_area", "AK15Puppi_area[BestAK15Idx]")

            // Jet ID & Flavour
            .Define("ak15_nBHadrons", "AK15Puppi_nBHadrons[BestAK15Idx]")
            .Define("ak15_nCHadrons", "AK15Puppi_nCHadrons[BestAK15Idx]")

            // Substructure & N-subjettiness
            .Define("ak15_tau1", "AK15Puppi_tau1[BestAK15Idx]")
            .Define("ak15_tau2", "AK15Puppi_tau2[BestAK15Idx]")
            .Define("ak15_tau3", "AK15Puppi_tau3[BestAK15Idx]")

            // ParTv3 Scores & Corrections
            .Define("ak15_ParTv3_massCorrGeneric", "AK15Puppi_ParTv3_massCorrGeneric[BestAK15Idx]")
            .Define("ak15_ParTv3_massCorrResonance", "AK15Puppi_ParTv3_massCorrResonance[BestAK15Idx]")
            .Define("ak15_ParTv3_massCorrX2p", "AK15Puppi_ParTv3_massCorrX2p[BestAK15Idx]")
            .Define("ak15_ParTv3_probQCD", "AK15Puppi_ParTv3_probQCD[BestAK15Idx]")
            .Define("ak15_ParTv3_probTopbWev", "AK15Puppi_ParTv3_probTopbWev[BestAK15Idx]")
            .Define("ak15_ParTv3_probTopbWmv", "AK15Puppi_ParTv3_probTopbWmv[BestAK15Idx]")
            .Define("ak15_ParTv3_probTopbWq", "AK15Puppi_ParTv3_probTopbWq[BestAK15Idx]")
            .Define("ak15_ParTv3_probTopbWqq", "AK15Puppi_ParTv3_probTopbWqq[BestAK15Idx]")
            .Define("ak15_ParTv3_probTopbWtauhv", "AK15Puppi_ParTv3_probTopbWtauhv[BestAK15Idx]")
            .Define("ak15_ParTv3_probXWW3q", "AK15Puppi_ParTv3_probXWW3q[BestAK15Idx]")
            .Define("ak15_ParTv3_probXWW4q", "AK15Puppi_ParTv3_probXWW4q[BestAK15Idx]")
            .Define("ak15_ParTv3_probXWWqqev", "AK15Puppi_ParTv3_probXWWqqev[BestAK15Idx]")
            .Define("ak15_ParTv3_probXWWqqmv", "AK15Puppi_ParTv3_probXWWqqmv[BestAK15Idx]")
            .Define("ak15_ParTv3_probXbb", "AK15Puppi_ParTv3_probXbb[BestAK15Idx]")
            .Define("ak15_ParTv3_probXcc", "AK15Puppi_ParTv3_probXcc[BestAK15Idx]")
            .Define("ak15_ParTv3_probXcs", "AK15Puppi_ParTv3_probXcs[BestAK15Idx]")
            .Define("ak15_ParTv3_probXqq", "AK15Puppi_ParTv3_probXqq[BestAK15Idx]")
            .Define("ak15_ParTv3_probXtauhtaue", "AK15Puppi_ParTv3_probXtauhtaue[BestAK15Idx]")
            .Define("ak15_ParTv3_probXtauhtauh", "AK15Puppi_ParTv3_probXtauhtauh[BestAK15Idx]")
            .Define("ak15_ParTv3_probXtauhtaum", "AK15Puppi_ParTv3_probXtauhtaum[BestAK15Idx]")

            // ParticleNetMD Scores & Corrections
            .Define("ak15_ParticleNetMD_mass", "AK15Puppi_ParticleNetMD_mass[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probQCDb", "AK15Puppi_ParticleNetMD_probQCDb[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probQCDbb", "AK15Puppi_ParticleNetMD_probQCDbb[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probQCDc", "AK15Puppi_ParticleNetMD_probQCDc[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probQCDcc", "AK15Puppi_ParticleNetMD_probQCDcc[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probQCDothers", "AK15Puppi_ParticleNetMD_probQCDothers[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probXbb", "AK15Puppi_ParticleNetMD_probXbb[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probXcc", "AK15Puppi_ParticleNetMD_probXcc[BestAK15Idx]")
            .Define("ak15_ParticleNetMD_probXqq", "AK15Puppi_ParticleNetMD_probXqq[BestAK15Idx]")

            // Subjets
            .Define("ak15_subJetIdx1", "AK15Puppi_subJetIdx1[BestAK15Idx]")
            .Define("ak15_subJetIdx2", "AK15Puppi_subJetIdx2[BestAK15Idx]")
            .Define("ak15_nMatchedSubjets", "(int)(ak15_subJetIdx1 >= 0) + (int)(ak15_subJetIdx2 >= 0)")

            // Kinematics
            .Define("ak15_Subjet_pt", "FillMatchedSubjet(AK15PuppiSubJet_pt, ak15_subJetIdx1, ak15_subJetIdx2)")
            .Define("ak15_Subjet_eta", "FillMatchedSubjet(AK15PuppiSubJet_eta, ak15_subJetIdx1, ak15_subJetIdx2)")
            .Define("ak15_Subjet_phi", "FillMatchedSubjet(AK15PuppiSubJet_phi, ak15_subJetIdx1, ak15_subJetIdx2)")
            .Define("ak15_Subjet_mass", "FillMatchedSubjet(AK15PuppiSubJet_mass, ak15_subJetIdx1, ak15_subJetIdx2)")
            .Define("ak15_Subjet_rawFactor", "FillMatchedSubjet(AK15PuppiSubJet_rawFactor, ak15_subJetIdx1, ak15_subJetIdx2)")
            .Define("ak15_Subjet_pt_rawFactorCorrected", "FillMatchedSubjet((1 - AK15PuppiSubJet_rawFactor) * AK15PuppiSubJet_pt, ak15_subJetIdx1, ak15_subJetIdx2)")

            // Area
            .Define("ak15_Subjet_area", "FillMatchedSubjet(AK15PuppiSubJet_area, ak15_subJetIdx1, ak15_subJetIdx2)")

            // Flavour
            .Define("ak15_Subjet_nBHadrons", "FillMatchedSubjet(AK15PuppiSubJet_nBHadrons, ak15_subJetIdx1, ak15_subJetIdx2)")
            .Define("ak15_Subjet_nCHadrons", "FillMatchedSubjet(AK15PuppiSubJet_nCHadrons, ak15_subJetIdx1, ak15_subJetIdx2)");
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