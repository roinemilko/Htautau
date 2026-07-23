#include <TChain.h> 
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <string>
#include "BuildData.h"
#include "helpers_v2.h"
#include "ROOT/RDataFrame.hxx"
#include <ROOT/RDFHelpers.hxx>

// Safe wrappers for some of the helper functions
inline float dphi(float jetphi, float metphi) {
    return (metphi > kMissing) ? DeltaPhi(jetphi, metphi) : kMissing;
}

inline float safeRatio(float met, float jetpt) {
    return (jetpt > 0.f && met > kMissing) ? met / jetpt : kMissing;
}

inline float dR(float eta1, float phi1, float eta2, float phi2) {
    return (eta1 > kMissing && eta2 > kMissing && phi1 > kMissing && phi2 > kMissing) ? DeltaR(eta1, phi1, eta2, phi2) : kMissing;
}

void BuildData(
    const char* file_pattern = "nano_*.root",
    bool include_AK4 = true, bool include_AK8 = true, bool include_AK15 = true, bool include_RawData = false, bool include_Tau = true,
    bool require_hadhad = false,
    const char* save_directory = "jets/",
    int threads = 1
) {

    if (threads > 1) {
        ROOT::EnableImplicitMT(threads);
    }

    if (!include_AK4 && !include_AK8 && !include_AK15 && !include_RawData && !include_Tau) {
        std::cerr << "You have to include somehting" <<std::endl;
        return;
    }
    
    ROOT::RDataFrame df("Events", file_pattern);


    auto df_truth = df
        .Define("tauFromH", "MakeIsHardProcLastCopyTauFromHiggsMask(GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags)")
        .Define("muMask", "TauIsSemileptonicMuMask_Clean(GenPart_pdgId, GenPart_genPartIdxMother)")
        .Define("eMask", "TauIsSemileptonicEMask_Clean(GenPart_pdgId, GenPart_genPartIdxMother)")
        .Define("hadMask", "TauIsHadronicMask_Clean(GenPart_pdgId, muMask, eMask)")

        .Define("nHiggsTauMu", "Sum(muMask[tauFromH > 0])")
        .Define("nHiggsTauE", "Sum(eMask[tauFromH > 0])")
        .Define("nHiggsTauHad", "Sum(hadMask[tauFromH > 0])")
        .Define("nLastCopyTauFromHiggs", "Sum(tauFromH > 0)")

        .Define("is_truth_ehad", "nHiggsTauE == 1 && nHiggsTauHad == 1")
        .Define("is_truth_muhad", "nHiggsTauMu == 1 && nHiggsTauHad == 1")
        .Define("is_truth_hadhad", "nHiggsTauHad == 2")
        .Filter(require_hadhad ? "is_truth_hadhad == 1" : "true", "Require HadHad");


    ROOT::RDF::RNode df_raw_event_data = df_truth;
    ROOT::RDF::RNode df_Tau = df_truth;
    ROOT::RDF::RNode df_AK4 = df_truth;
    ROOT::RDF::RNode df_AK8 = df_truth;
    ROOT::RDF::RNode df_AK15 = df_truth;


    if (include_RawData) {
        df_raw_event_data = df_truth
        .Define("truthHiggsIdx_raw", "FindTruthHiggs(GenPart_pdgId, GenPart_statusFlags)")
        .Filter("truthHiggsIdx_raw >= 0", "Require Truth Higgs")
        .Define("genH_pt_raw", "GenPart_pt[truthHiggsIdx_raw]")
        .Define("genH_eta_raw", "GenPart_eta[truthHiggsIdx_raw]")
        .Define("genH_phi_raw", "GenPart_phi[truthHiggsIdx_raw]")
        
        .Define("truthTauIdxs", "ROOT::VecOps::Nonzero(tauFromH)")
        .Filter("truthTauIdxs.size() >= 2", "Require 2 taus")
        .Define("TauDecayProducts", "GetDecayProducts(GenPart_pt, GenPart_pdgId, GenPart_status, GenPart_genPartIdxMother, tauFromH)")
        .Define("DecayProds_absid", "TauDecayProducts.product_id")
        .Define("DecayProds_ptfrac", "TauDecayProducts.ptfrac")
        .Define("DecayProds_parentTauIdx", "TauDecayProducts.ParentTauIdx")
        

        .Define("genTau1_pt_raw", "GenPart_pt[truthTauIdxs[0]]")
        .Define("genTau2_pt_raw", "GenPart_pt[truthTauIdxs[1]]")
        .Define("dR_tau1_tau2_raw", 
                "dR(GenPart_eta[truthTauIdxs[0]], GenPart_phi[truthTauIdxs[0]], "
                "   GenPart_eta[truthTauIdxs[1]], GenPart_phi[truthTauIdxs[1]])")
        .Define("genTau_pt_asym_raw", "abs(genTau1_pt_raw - genTau2_pt_raw) / (genTau1_pt_raw + genTau2_pt_raw)")
        .Define("dR_fJ_nolim", "MatchJetSanityCheck(FatJet_eta, FatJet_phi, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags)")
        .Define("dR_Jet_nolim", "MatchJetSanityCheck(Jet_eta, Jet_phi, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags)")
        .Define("dR_AK15_nolim", "MatchJetSanityCheck(AK15Puppi_eta, AK15Puppi_phi, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags)")
        .Define("dR_Tau_nolim", "MatchJetSanityCheck(Tau_eta, Tau_phi, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags)");
    }

    if (include_Tau) {
        df_Tau = df_truth
            .Define("goodTaus", Form("MakeGoodJetMask(Tau_pt, Tau_eta, %f, %f)", 0.f, AK4_eta_max))
            .Define("TauMatch", "MatchTwoJetsToHTauTau(Tau_pt, Tau_eta, Tau_phi, goodTaus, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 0.4f)")
            .Filter("TauMatch[0] >= 0 && TauMatch[1] >= 0", "Valid Tau match")
            .Define("matchedTausIdx", "ROOT::VecOps::RVec<int>{TauMatch[0], TauMatch[1]}")
            .Define("matchedGenTau1Idx", "TauMatch[2]")
            .Define("matchedGenTau2Idx", "TauMatch[3]")
            .Define("matchedHiggsIdx", "TauMatch[4]")
            .Define("target_mass", "GenPart_mass[matchedHiggsIdx]")
            .Define("nMatchedGoodTau", "2")

            .Define("tau_pt",   "ROOT::VecOps::RVec<float>{Tau_pt[TauMatch[0]],   Tau_pt[TauMatch[1]]}")
            .Define("tau_eta",  "ROOT::VecOps::RVec<float>{Tau_eta[TauMatch[0]],  Tau_eta[TauMatch[1]]}")
            .Define("tau_phi",  "ROOT::VecOps::RVec<float>{Tau_phi[TauMatch[0]],  Tau_phi[TauMatch[1]]}")
            .Define("tau_mass", "ROOT::VecOps::RVec<float>{Tau_mass[TauMatch[0]], Tau_mass[TauMatch[1]]}")
            .Define("tau_charge", "ROOT::VecOps::RVec<int>{Tau_charge[TauMatch[0]], Tau_charge[TauMatch[1]]}")

            .Define("tau_dxy", "ROOT::VecOps::RVec<float>{Tau_dxy[TauMatch[0]], Tau_dxy[TauMatch[1]]}")
            .Define("tau_dz",  "ROOT::VecOps::RVec<float>{Tau_dz[TauMatch[0]],  Tau_dz[TauMatch[1]]}")
            .Define("tau_ipLengthSig", "ROOT::VecOps::RVec<float>{Tau_ipLengthSig[TauMatch[0]], Tau_ipLengthSig[TauMatch[1]]}")

            .Define("tau_chargedIso",  "ROOT::VecOps::RVec<float>{Tau_chargedIso[TauMatch[0]],  Tau_chargedIso[TauMatch[1]]}")
            .Define("tau_neutralIso",  "ROOT::VecOps::RVec<float>{Tau_neutralIso[TauMatch[0]],  Tau_neutralIso[TauMatch[1]]}")
            .Define("tau_rawIso",      "ROOT::VecOps::RVec<float>{Tau_rawIso[TauMatch[0]],      Tau_rawIso[TauMatch[1]]}")
            .Define("tau_rawIsodR03",  "ROOT::VecOps::RVec<float>{Tau_rawIsodR03[TauMatch[0]],  Tau_rawIsodR03[TauMatch[1]]}")
            .Define("tau_puCorr",      "ROOT::VecOps::RVec<float>{Tau_puCorr[TauMatch[0]],      Tau_puCorr[TauMatch[1]]}")
            .Define("tau_rawDeepTauVSe",  "ROOT::VecOps::RVec<float>{Tau_rawDeepTau2018v2p5VSe[TauMatch[0]],  Tau_rawDeepTau2018v2p5VSe[TauMatch[1]]}")
            .Define("tau_rawDeepTauVSmu", "ROOT::VecOps::RVec<float>{Tau_rawDeepTau2018v2p5VSmu[TauMatch[0]], Tau_rawDeepTau2018v2p5VSmu[TauMatch[1]]}")

            .Define("tau_decayMode",    "ROOT::VecOps::RVec<int>{Tau_decayMode[TauMatch[0]],    Tau_decayMode[TauMatch[1]]}")
            .Define("tau_genPartFlav",  "ROOT::VecOps::RVec<int>{Tau_genPartFlav[TauMatch[0]],  Tau_genPartFlav[TauMatch[1]]}")
            .Define("tau_genPartIdx",   "ROOT::VecOps::RVec<int>{Tau_genPartIdx[TauMatch[0]],   Tau_genPartIdx[TauMatch[1]]}")

            .Define("tau_idDeepTauVSjet", "ROOT::VecOps::RVec<int>{Tau_idDeepTau2018v2p5VSjet[TauMatch[0]], Tau_idDeepTau2018v2p5VSjet[TauMatch[1]]}")
            .Define("tau_idDeepTauVSe",   "ROOT::VecOps::RVec<int>{Tau_idDeepTau2018v2p5VSe[TauMatch[0]],   Tau_idDeepTau2018v2p5VSe[TauMatch[1]]}")
            .Define("tau_idDeepTauVSmu",  "ROOT::VecOps::RVec<int>{Tau_idDeepTau2018v2p5VSmu[TauMatch[0]],  Tau_idDeepTau2018v2p5VSmu[TauMatch[1]]}")
            .Define("tau_rawDeepTauVSjet","ROOT::VecOps::RVec<float>{Tau_rawDeepTau2018v2p5VSjet[TauMatch[0]], Tau_rawDeepTau2018v2p5VSjet[TauMatch[1]]}")

            .Define("tau_decayModePNet", "ROOT::VecOps::RVec<int>{Tau_decayModePNet[TauMatch[0]], Tau_decayModePNet[TauMatch[1]]}")
            .Define("tau_probDM0PNet",   "ROOT::VecOps::RVec<float>{Tau_probDM0PNet[TauMatch[0]], Tau_probDM0PNet[TauMatch[1]]}")
            .Define("tau_probDM1PNet",   "ROOT::VecOps::RVec<float>{Tau_probDM1PNet[TauMatch[0]], Tau_probDM1PNet[TauMatch[1]]}")
            .Define("tau_probDM2PNet",   "ROOT::VecOps::RVec<float>{Tau_probDM2PNet[TauMatch[0]], Tau_probDM2PNet[TauMatch[1]]}")
            .Define("tau_probDM10PNet",  "ROOT::VecOps::RVec<float>{Tau_probDM10PNet[TauMatch[0]], Tau_probDM10PNet[TauMatch[1]]}")
            .Define("tau_probDM11PNet",  "ROOT::VecOps::RVec<float>{Tau_probDM11PNet[TauMatch[0]], Tau_probDM11PNet[TauMatch[1]]}")
            
            .Define("tau_ptCorrPNet",    "ROOT::VecOps::RVec<float>{Tau_ptCorrPNet[TauMatch[0]], Tau_ptCorrPNet[TauMatch[1]]}")
            .Define("tau_qConfPNet",     "ROOT::VecOps::RVec<float>{Tau_qConfPNet[TauMatch[0]], Tau_qConfPNet[TauMatch[1]]}")
            
            .Define("tau_rawPNetVSe",    "ROOT::VecOps::RVec<float>{Tau_rawPNetVSe[TauMatch[0]], Tau_rawPNetVSe[TauMatch[1]]}")
            .Define("tau_rawPNetVSjet",  "ROOT::VecOps::RVec<float>{Tau_rawPNetVSjet[TauMatch[0]], Tau_rawPNetVSjet[TauMatch[1]]}")
            .Define("tau_rawPNetVSmu",   "ROOT::VecOps::RVec<float>{Tau_rawPNetVSmu[TauMatch[0]], Tau_rawPNetVSmu[TauMatch[1]]}")

            .Define("tau_decayModeUParT", "ROOT::VecOps::RVec<int>{Tau_decayModeUParT[TauMatch[0]], Tau_decayModeUParT[TauMatch[1]]}")
            .Define("tau_probDM0UParT",   "ROOT::VecOps::RVec<float>{Tau_probDM0UParT[TauMatch[0]], Tau_probDM0UParT[TauMatch[1]]}")
            .Define("tau_probDM1UParT",   "ROOT::VecOps::RVec<float>{Tau_probDM1UParT[TauMatch[0]], Tau_probDM1UParT[TauMatch[1]]}")
            .Define("tau_probDM2UParT",   "ROOT::VecOps::RVec<float>{Tau_probDM2UParT[TauMatch[0]], Tau_probDM2UParT[TauMatch[1]]}")
            .Define("tau_probDM10UParT",  "ROOT::VecOps::RVec<float>{Tau_probDM10UParT[TauMatch[0]], Tau_probDM10UParT[TauMatch[1]]}")
            .Define("tau_probDM11UParT",  "ROOT::VecOps::RVec<float>{Tau_probDM11UParT[TauMatch[0]], Tau_probDM11UParT[TauMatch[1]]}")
            
            .Define("tau_ptCorrUParT",    "ROOT::VecOps::RVec<float>{Tau_ptCorrUParT[TauMatch[0]], Tau_ptCorrUParT[TauMatch[1]]}")
            .Define("tau_qConfUParT",     "ROOT::VecOps::RVec<float>{Tau_qConfUParT[TauMatch[0]], Tau_qConfUParT[TauMatch[1]]}")
            
            .Define("tau_rawUParTVSe",    "ROOT::VecOps::RVec<float>{Tau_rawUParTVSe[TauMatch[0]], Tau_rawUParTVSe[TauMatch[1]]}")
            .Define("tau_rawUParTVSjet",  "ROOT::VecOps::RVec<float>{Tau_rawUParTVSjet[TauMatch[0]], Tau_rawUParTVSjet[TauMatch[1]]}")
            .Define("tau_rawUParTVSmu",   "ROOT::VecOps::RVec<float>{Tau_rawUParTVSmu[TauMatch[0]], Tau_rawUParTVSmu[TauMatch[1]]}")

            .Define("genH_pt",  "GenPart_pt[matchedHiggsIdx]")
            .Define("genH_eta", "GenPart_eta[matchedHiggsIdx]")
            .Define("genH_phi", "GenPart_phi[matchedHiggsIdx]")
            .Define("genTau1_pt",  "GenPart_pt[matchedGenTau1Idx]")
            .Define("genTau1_eta", "GenPart_eta[matchedGenTau1Idx]")
            .Define("genTau1_phi", "GenPart_phi[matchedGenTau1Idx]")
            .Define("genTau2_pt",  "GenPart_pt[matchedGenTau2Idx]")
            .Define("genTau2_eta", "GenPart_eta[matchedGenTau2Idx]")
            .Define("genTau2_phi", "GenPart_phi[matchedGenTau2Idx]")
            .Define("genTau_pt_asym", "abs(genTau1_pt - genTau2_pt) / (genTau1_pt + genTau2_pt)")

            .Define("dR_tau_tau1", "dR(tau_eta[0], tau_phi[0], genTau1_eta, genTau1_phi)")
            .Define("dR_tau_tau2", "dR(tau_eta[1], tau_phi[1], genTau2_eta, genTau2_phi)")
            .Define("dR_tau1_tau2", "dR(genTau1_eta, genTau1_phi, genTau2_eta, genTau2_phi)")
            .Define("dR_tau_H", "ROOT::VecOps::RVec<float>{"
                                "dR(tau_eta[0], tau_phi[0], genH_eta, genH_phi),"
                                "dR(tau_eta[1], tau_phi[1], genH_eta, genH_phi)"
                                "}");
    }


    if (include_AK4) {
        df_AK4 = df_truth
            .Define("goodJets", Form("MakeGoodJetMask(Jet_pt, Jet_eta, %f, %f)", AK4_min_pt, AK4_eta_max))
            .Define("AK4Match", "MatchTwoJetsToHTauTau(Jet_pt, Jet_eta, Jet_phi, goodJets, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 0.4f)")
            .Filter("AK4Match[0] >= 0 && AK4Match[1] >= 0", "Valid AK4 match")
            .Define("matchedAK4JetIdx", "ROOT::VecOps::RVec<int>{AK4Match[0], AK4Match[1]}")
            .Define("matchedTau1Idx", "AK4Match[2]")
            .Define("matchedTau2Idx", "AK4Match[3]")
            .Define("matchedHiggsIdx", "AK4Match[4]")
            .Define("target_mass", "GenPart_mass[matchedHiggsIdx]")
            .Define("nTauMatchedGoodAK4Jet", "2")

            .Define("ak4_pt", "ROOT::VecOps::RVec<float>{Jet_pt[AK4Match[0]], Jet_pt[AK4Match[1]]}")
            .Define("ak4_eta", "ROOT::VecOps::RVec<float>{Jet_eta[AK4Match[0]], Jet_eta[AK4Match[1]]}")
            .Define("ak4_phi", "ROOT::VecOps::RVec<float>{Jet_phi[AK4Match[0]], Jet_phi[AK4Match[1]]}")
            .Define("ak4_mass", "ROOT::VecOps::RVec<float>{Jet_mass[AK4Match[0]], Jet_mass[AK4Match[1]]}")
            .Define("ak4_rawFactor", "ROOT::VecOps::RVec<float>{Jet_rawFactor[AK4Match[0]], Jet_rawFactor[AK4Match[1]]}")
            .Define("ak4_mass_rawFactorCorrected", "ak4_mass * (1.f - ak4_rawFactor)")

            .Define("genH_pt", "GenPart_pt[matchedHiggsIdx]")
            .Define("genH_eta", "GenPart_eta[matchedHiggsIdx]")
            .Define("genH_phi", "GenPart_phi[matchedHiggsIdx]")
            .Define("genTau1_pt", "GenPart_pt[matchedTau1Idx]")
            .Define("genTau1_eta", "GenPart_eta[matchedTau1Idx]")
            .Define("genTau1_phi", "GenPart_phi[matchedTau1Idx]")
            .Define("genTau2_pt", "GenPart_pt[matchedTau2Idx]")
            .Define("genTau2_eta", "GenPart_eta[matchedTau2Idx]")
            .Define("genTau2_phi", "GenPart_phi[matchedTau2Idx]")
            .Define("genTau_pt_asym", "abs(genTau1_pt - genTau2_pt) / (genTau1_pt + genTau2_pt)")

            .Define("dR_ak4_tau1", "dR(ak4_eta[0], ak4_phi[0], genTau1_eta, genTau1_phi)")
            .Define("dR_ak4_tau2", "dR(ak4_eta[1], ak4_phi[1], genTau2_eta, genTau2_phi)")
            .Define("dR_tau1_tau2", "dR(genTau1_eta, genTau1_phi, genTau2_eta, genTau2_phi)")

            .Define("dR_ak4_H", "ROOT::VecOps::RVec<float>{"
                                "dR(ak4_eta[0], ak4_phi[0], GenPart_eta[matchedHiggsIdx], GenPart_phi[matchedHiggsIdx]),"
                                "dR(ak4_eta[1], ak4_phi[1], GenPart_eta[matchedHiggsIdx], GenPart_phi[matchedHiggsIdx])"
                                "}")

            .Define("dphi_ak4_pfmet", "ROOT::VecOps::RVec<float>{"
                                    "  dphi(ak4_phi[0], RawPFMET_phi),"
                                    "  dphi(ak4_phi[1], RawPFMET_phi)"
                                    "}")

            .Define("dphi_ak4_puppimet", "ROOT::VecOps::RVec<float>{"
                                        " dphi(ak4_phi[0], PuppiMET_phi),"
                                        " dphi(ak4_phi[1], PuppiMET_phi)"
                                "}")

            .Define("ak4_pt_minus_pfmet_pt", "ROOT::VecOps::RVec<float>{"
                                            " (RawPFMET_pt > kMissing) ? ak4_pt[0] - RawPFMET_pt : kMissing,"
                                            " (RawPFMET_pt > kMissing) ? ak4_pt[1] - RawPFMET_pt : kMissing"
                                            "}")

            .Define("ak4_pt_minus_puppimet_pt", "ROOT::VecOps::RVec<float>{"
                                                "(PuppiMET_pt > kMissing) ? ak4_pt[0] - PuppiMET_pt : kMissing,"
                                                "(PuppiMET_pt > kMissing) ? ak4_pt[1] - PuppiMET_pt : kMissing"
                                                "}")    
                                                
            .Define("pfmet_over_ak4_pt", "ROOT::VecOps::RVec<float>{"
                                        " safeRatio(RawPFMET_pt, ak4_pt[0]),"
                                        " safeRatio(RawPFMET_pt, ak4_pt[1])"
                                        "}")
            .Define("puppimet_over_ak4_pt", "ROOT::VecOps::RVec<float>{"
                                            "safeRatio(PuppiMET_pt, ak4_pt[0]),"
                                            "safeRatio(PuppiMET_pt, ak4_pt[1])"
                                            "}");
    }

    if (include_AK8) {
        df_AK8 = df_truth
            .Define("goodJets", Form("MakeGoodJetMask(FatJet_pt, FatJet_eta, %f, %f)", AK8_min_pt, AK8_eta_max))
            .Define("AK8Match", "MatchOneJetToHTauTau(FatJet_eta, FatJet_phi, goodJets, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 0.8f)")
            .Filter("AK8Match[0] >= 0", "Valid AK8 match")
            .Define("matchedFatJetIdx", "AK8Match[0]")
            .Define("matchedTau1Idx", "AK8Match[1]")
            .Define("matchedTau2Idx", "AK8Match[2]")
            .Define("matchedHiggsIdx", "AK8Match[3]")
            .Define("target_mass", "GenPart_mass[matchedHiggsIdx]")
            .Define("nTauMatchedGoodFatJet", "1")
            
            // Standard Kinematics
            .Define("fj_pt", "FatJet_pt[matchedFatJetIdx]")
            .Define("fj_eta", "FatJet_eta[matchedFatJetIdx]")
            .Define("fj_phi", "FatJet_phi[matchedFatJetIdx]")
            .Define("fj_mass", "FatJet_mass[matchedFatJetIdx]")
            .Define("fj_msoftdrop", "FatJet_msoftdrop[matchedFatJetIdx]")
            .Define("fj_rawFactor", "FatJet_rawFactor[matchedFatJetIdx]")
            .Define("fj_mass_rawFactorCorrected", "fj_mass * (1.f - fj_rawFactor)")

            // Generator Matching
            .Define("genH_pt", "GenPart_pt[matchedHiggsIdx]")
            .Define("genH_eta", "GenPart_eta[matchedHiggsIdx]")
            .Define("genH_phi", "GenPart_phi[matchedHiggsIdx]")
            .Define("genTau1_pt", "GenPart_pt[matchedTau1Idx]")
            .Define("genTau1_eta", "GenPart_eta[matchedTau1Idx]")
            .Define("genTau1_phi", "GenPart_phi[matchedTau1Idx]")
            .Define("genTau2_pt", "GenPart_pt[matchedTau2Idx]")
            .Define("genTau2_eta", "GenPart_eta[matchedTau2Idx]")
            .Define("genTau2_phi", "GenPart_phi[matchedTau2Idx]")
            .Define("genTau_pt_asym", "abs(genTau1_pt - genTau2_pt) / (genTau1_pt + genTau2_pt)")
            .Define("dR_fj_tau1", "dR(fj_eta, fj_phi, genTau1_eta, genTau1_phi)")
            .Define("dR_fj_tau2", "dR(fj_eta, fj_phi, genTau2_eta, genTau2_phi)")
            .Define("dR_fj_H", "dR(fj_eta, fj_phi, genH_eta, genH_phi)")
            .Define("dR_tau1_tau2", "dR(genTau1_eta, genTau1_phi, genTau2_eta, genTau2_phi)")


            // MET variables
            .Define("dphi_fj_pfmet", "dphi(fj_phi, RawPFMET_phi)")
            .Define("dphi_fj_puppimet", "dphi(fj_phi,  PuppiMET_phi)")
            .Define("fj_pt_minus_pfmet_pt", "(fj_pt > kMissing && RawPFMET_pt > kMissing) ? fj_pt - RawPFMET_pt : kMissing")
            .Define("fj_pt_minus_puppimet_pt", "(fj_pt > kMissing && PuppiMET_pt > kMissing) ? fj_pt - PuppiMET_pt : kMissing")
            .Define("pfmet_over_fj_pt", "safeRatio(RawPFMET_pt, fj_pt)")
            .Define("puppimet_over_fj_pt", "safeRatio(PuppiMET_pt, fj_pt)")
        
            // Extended FatJet Properties
            .Define("fj_area", "FatJet_area[matchedFatJetIdx]")
            .Define("fj_chEmEF", "FatJet_chEmEF[matchedFatJetIdx]")
            .Define("fj_chHEF", "FatJet_chHEF[matchedFatJetIdx]")
            .Define("fj_chMultiplicity", "FatJet_chMultiplicity[matchedFatJetIdx]")
            .Define("fj_globalParT3_QCD", "FatJet_globalParT3_QCD[matchedFatJetIdx]")
            .Define("fj_globalParT3_TopbWev", "FatJet_globalParT3_TopbWev[matchedFatJetIdx]")
            .Define("fj_globalParT3_TopbWmv", "FatJet_globalParT3_TopbWmv[matchedFatJetIdx]")
            .Define("fj_globalParT3_TopbWq", "FatJet_globalParT3_TopbWq[matchedFatJetIdx]")
            .Define("fj_globalParT3_TopbWqq", "FatJet_globalParT3_TopbWqq[matchedFatJetIdx]")
            .Define("fj_globalParT3_TopbWtauhv", "FatJet_globalParT3_TopbWtauhv[matchedFatJetIdx]")
            .Define("fj_globalParT3_WvsQCD", "FatJet_globalParT3_WvsQCD[matchedFatJetIdx]")
            .Define("fj_globalParT3_XWW3q", "FatJet_globalParT3_XWW3q[matchedFatJetIdx]")
            .Define("fj_globalParT3_XWW4q", "FatJet_globalParT3_XWW4q[matchedFatJetIdx]")
            .Define("fj_globalParT3_XWWqqev", "FatJet_globalParT3_XWWqqev[matchedFatJetIdx]")
            .Define("fj_globalParT3_XWWqqmv", "FatJet_globalParT3_XWWqqmv[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xbb", "FatJet_globalParT3_Xbb[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xcc", "FatJet_globalParT3_Xcc[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xcs", "FatJet_globalParT3_Xcs[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xqq", "FatJet_globalParT3_Xqq[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xtauhtaue", "FatJet_globalParT3_Xtauhtaue[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xtauhtauh", "FatJet_globalParT3_Xtauhtauh[matchedFatJetIdx]")
            .Define("fj_globalParT3_Xtauhtaum", "FatJet_globalParT3_Xtauhtaum[matchedFatJetIdx]")
            .Define("fj_globalParT3_massCorrGeneric", "FatJet_globalParT3_massCorrGeneric[matchedFatJetIdx]")
            .Define("fj_globalParT3_massCorrX2p", "FatJet_globalParT3_massCorrX2p[matchedFatJetIdx]")
            .Define("fj_globalParT3_withMassTopvsQCD", "FatJet_globalParT3_withMassTopvsQCD[matchedFatJetIdx]")
            .Define("fj_globalParT3_withMassWvsQCD", "FatJet_globalParT3_withMassWvsQCD[matchedFatJetIdx]")
            .Define("fj_globalParT3_withMassZvsQCD", "FatJet_globalParT3_withMassZvsQCD[matchedFatJetIdx]")
            .Define("fj_hadronFlavour", "FatJet_hadronFlavour[matchedFatJetIdx]")
            .Define("fj_hfEmEF", "FatJet_hfEmEF[matchedFatJetIdx]")
            .Define("fj_hfHEF", "FatJet_hfHEF[matchedFatJetIdx]")
            .Define("fj_lsf3", "FatJet_lsf3[matchedFatJetIdx]")
            .Define("fj_muEF", "FatJet_muEF[matchedFatJetIdx]")
            .Define("fj_nConstituents", "FatJet_nConstituents[matchedFatJetIdx]")
            .Define("fj_neEmEF", "FatJet_neEmEF[matchedFatJetIdx]")
            .Define("fj_neHEF", "FatJet_neHEF[matchedFatJetIdx]")
            .Define("fj_neMultiplicity", "FatJet_neMultiplicity[matchedFatJetIdx]")
            .Define("fj_tau1", "FatJet_tau1[matchedFatJetIdx]")
            .Define("fj_tau2", "FatJet_tau2[matchedFatJetIdx]")
            .Define("fj_tau3", "FatJet_tau3[matchedFatJetIdx]")
            .Define("fj_tau4", "FatJet_tau4[matchedFatJetIdx]")


            // Subjets
            .Define("fj_nSubjetsPerEventTotal", "nSubJet")
            .Define("fj_goodSubjets", "MakeGoodSubjetMask(SubJet_eta, SubJet_phi, fj_eta, fj_phi, 0.8f, 0.f)")
            .Define("fj_nSubjets", "ROOT::VecOps::Sum(fj_goodSubjets)")
            .Define("fj_matchedSubjets", "MatchTwoJetsToHTauTau(SubJet_pt, SubJet_eta, SubJet_phi, fj_goodSubjets, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 0.4f)")

            .Define("fj_matchedSubjet1_idx", "fj_matchedSubjets[0]")
            .Define("fj_matchedSubjet2_idx", "fj_matchedSubjets[1]")
            .Define("fj_matchedSubjet1_tauIdx", "fj_matchedSubjets[2]")
            .Define("fj_matchedSubjet2_tauIdx", "fj_matchedSubjets[3]")
            .Define("fj_nMatchedSubjets", "(int)(fj_matchedSubjet1_idx >= 0) + (int)(fj_matchedSubjet2_idx >= 0)")
            

            // Standard Kinematics
            .Define("fj_Subjet_pt", "FillMatchedSubjet(SubJet_pt, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_eta", "FillMatchedSubjet(SubJet_eta, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_phi", "FillMatchedSubjet(SubJet_phi, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_mass", "FillMatchedSubjet(SubJet_mass, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_rawFactor", "FillMatchedSubjet(SubJet_rawFactor, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_pt_rawFactorCorrected", "FillMatchedSubjet((1 - SubJet_rawFactor) * SubJet_pt, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")

            // Corrections & Area
            .Define("fj_Subjet_area", "FillMatchedSubjet(SubJet_area, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")

            // Substructure & N-subjettiness
            .Define("fj_Subjet_tau1", "FillMatchedSubjet(SubJet_tau1, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_tau2", "FillMatchedSubjet(SubJet_tau2, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_tau3", "FillMatchedSubjet(SubJet_tau3, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_tau4", "FillMatchedSubjet(SubJet_tau4, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_n2b1", "FillMatchedSubjet(SubJet_n2b1, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_n3b1", "FillMatchedSubjet(SubJet_n3b1, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")

            // B-Tagging & Flavour
            .Define("fj_Subjet_btagDeepFlavB", "FillMatchedSubjet(SubJet_btagDeepFlavB, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_btagUParTAK4B", "FillMatchedSubjet(SubJet_btagUParTAK4B, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_hadronFlavour", "FillMatchedSubjet(SubJet_hadronFlavour, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_nBHadrons", "FillMatchedSubjet(SubJet_nBHadrons, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_nCHadrons", "FillMatchedSubjet(SubJet_nCHadrons, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")

            // UParT Regressions & Resolutions
            .Define("fj_Subjet_UParTAK4RegPtRawCorr", "FillMatchedSubjet(SubJet_UParTAK4RegPtRawCorr, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_UParTAK4RegPtRawCorrNeutrino", "FillMatchedSubjet(SubJet_UParTAK4RegPtRawCorrNeutrino, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_UParTAK4RegPtRawRes", "FillMatchedSubjet(SubJet_UParTAK4RegPtRawRes, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_UParTAK4V1RegPtRawCorr", "FillMatchedSubjet(SubJet_UParTAK4V1RegPtRawCorr, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_UParTAK4V1RegPtRawCorrNeutrino", "FillMatchedSubjet(SubJet_UParTAK4V1RegPtRawCorrNeutrino, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")
            .Define("fj_Subjet_UParTAK4V1RegPtRawRes", "FillMatchedSubjet(SubJet_UParTAK4V1RegPtRawRes, fj_matchedSubjet1_idx, fj_matchedSubjet2_idx)")

            // Generator sj variables
            .Define("dR_fj_Subjet_tau1",
                "fj_matchedSubjet1_idx >= 0 ? "
                "dR(fj_Subjet_eta[0], fj_Subjet_phi[0], "
                "GenPart_eta[fj_matchedSubjet1_tauIdx], GenPart_phi[fj_matchedSubjet1_tauIdx]) : "
                "kMissing")

            .Define("dR_fj_Subjet_tau2",
                "fj_matchedSubjet2_idx >= 0 ? "
                "dR(fj_Subjet_eta[1], fj_Subjet_phi[1], "
                "GenPart_eta[fj_matchedSubjet2_tauIdx], GenPart_phi[fj_matchedSubjet2_tauIdx]) : "
                "kMissing");
            
    }

    if (include_AK15) {
        df_AK15 = df_truth
            .Define("goodJets", Form("MakeGoodJetMask(AK15Puppi_pt, AK15Puppi_eta, %f, %f)", AK15_min_pt, AK15_eta_max))
            .Define("AK15Match", "MatchOneJetToHTauTau(AK15Puppi_eta, AK15Puppi_phi, goodJets, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 1.5f)")
            .Filter("AK15Match[0] >= 0", "Valid AK15 match")
            .Define("matchedAK15JetIdx", "AK15Match[0]")
            .Define("matchedTau1Idx", "AK15Match[1]")
            .Define("matchedTau2Idx", "AK15Match[2]")
            .Define("matchedHiggsIdx", "AK15Match[3]")
            .Define("target_mass", "GenPart_mass[matchedHiggsIdx]")
            .Define("nTauMatchedGoodAK15Jet", "1")

            // Kinematics
            .Define("ak15_pt", "AK15Puppi_pt[matchedAK15JetIdx]")
            .Define("ak15_eta", "AK15Puppi_eta[matchedAK15JetIdx]") 
            .Define("ak15_phi", "AK15Puppi_phi[matchedAK15JetIdx]")
            .Define("ak15_mass", "AK15Puppi_mass[matchedAK15JetIdx]")
            .Define("ak15_msoftdrop", "AK15Puppi_msoftdrop[matchedAK15JetIdx]")
            .Define("ak15_rawFactor", "AK15Puppi_rawFactor[matchedAK15JetIdx]")
            .Define("ak15_mass_rawFactorCorrected", "ak15_mass * (1.f - ak15_rawFactor)")

            // Generator variables
            .Define("genH_pt", "GenPart_pt[matchedHiggsIdx]")
            .Define("genH_eta", "GenPart_eta[matchedHiggsIdx]")
            .Define("genH_phi", "GenPart_phi[matchedHiggsIdx]")
            .Define("genTau1_pt", "GenPart_pt[matchedTau1Idx]")
            .Define("genTau1_eta", "GenPart_eta[matchedTau1Idx]")
            .Define("genTau1_phi", "GenPart_phi[matchedTau1Idx]")
            .Define("genTau2_pt", "GenPart_pt[matchedTau2Idx]")
            .Define("genTau2_eta", "GenPart_eta[matchedTau2Idx]")
            .Define("genTau2_phi", "GenPart_phi[matchedTau2Idx]")
            .Define("genTau_pt_asym", "abs(genTau1_pt - genTau2_pt) / (genTau1_pt + genTau2_pt)")

            // MET and geometry
            .Define("dphi_ak15_pfmet", "dphi(ak15_phi, RawPFMET_phi)")
            .Define("dphi_ak15_puppimet", "dphi(ak15_phi,  PuppiMET_phi)")
            .Define("ak15_pt_minus_pfmet_pt", "(ak15_pt > kMissing && RawPFMET_pt > kMissing) ? ak15_pt - RawPFMET_pt : kMissing")
            .Define("ak15_pt_minus_puppimet_pt", "(ak15_pt > kMissing && PuppiMET_pt > kMissing) ? ak15_pt - PuppiMET_pt : kMissing")
            .Define("pfmet_over_ak15_pt", "safeRatio(RawPFMET_pt, ak15_pt)")
            .Define("puppimet_over_ak15_pt", "safeRatio(PuppiMET_pt, ak15_pt)")
            .Define("dR_ak15_tau1", "dR(ak15_eta, ak15_phi, genTau1_eta, genTau1_phi)")
            .Define("dR_ak15_tau2", "dR(ak15_eta, ak15_phi, genTau2_eta, genTau2_phi)")
            .Define("dR_ak15_H", "dR(ak15_eta, ak15_phi, genH_eta, genH_phi)")
            .Define("dR_tau1_tau2", "dR(genTau1_eta, genTau1_phi, genTau2_eta, genTau2_phi)")

            // Flavour & Area
            .Define("ak15_area", "AK15Puppi_area[matchedAK15JetIdx]")
            .Define("ak15_nBHadrons", "AK15Puppi_nBHadrons[matchedAK15JetIdx]")
            .Define("ak15_nCHadrons", "AK15Puppi_nCHadrons[matchedAK15JetIdx]")

            // Substructure & N-subjettiness
            .Define("ak15_tau1", "AK15Puppi_tau1[matchedAK15JetIdx]")
            .Define("ak15_tau2", "AK15Puppi_tau2[matchedAK15JetIdx]")
            .Define("ak15_tau3", "AK15Puppi_tau3[matchedAK15JetIdx]")

            // ParTv3 Scores & Corrections
            .Define("ak15_ParTv3_massCorrGeneric", "AK15Puppi_ParTv3_massCorrGeneric[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_massCorrResonance", "AK15Puppi_ParTv3_massCorrResonance[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_massCorrX2p", "AK15Puppi_ParTv3_massCorrX2p[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probQCD", "AK15Puppi_ParTv3_probQCD[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probTopbWev", "AK15Puppi_ParTv3_probTopbWev[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probTopbWmv", "AK15Puppi_ParTv3_probTopbWmv[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probTopbWq", "AK15Puppi_ParTv3_probTopbWq[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probTopbWqq", "AK15Puppi_ParTv3_probTopbWqq[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probTopbWtauhv", "AK15Puppi_ParTv3_probTopbWtauhv[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXWW3q", "AK15Puppi_ParTv3_probXWW3q[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXWW4q", "AK15Puppi_ParTv3_probXWW4q[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXWWqqev", "AK15Puppi_ParTv3_probXWWqqev[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXWWqqmv", "AK15Puppi_ParTv3_probXWWqqmv[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXbb", "AK15Puppi_ParTv3_probXbb[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXcc", "AK15Puppi_ParTv3_probXcc[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXcs", "AK15Puppi_ParTv3_probXcs[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXqq", "AK15Puppi_ParTv3_probXqq[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXtauhtaue", "AK15Puppi_ParTv3_probXtauhtaue[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXtauhtauh", "AK15Puppi_ParTv3_probXtauhtauh[matchedAK15JetIdx]")
            .Define("ak15_ParTv3_probXtauhtaum", "AK15Puppi_ParTv3_probXtauhtaum[matchedAK15JetIdx]")

            // ParticleNetMD Scores & Corrections
            .Define("ak15_ParticleNetMD_mass", "AK15Puppi_ParticleNetMD_mass[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probQCDb", "AK15Puppi_ParticleNetMD_probQCDb[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probQCDbb", "AK15Puppi_ParticleNetMD_probQCDbb[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probQCDc", "AK15Puppi_ParticleNetMD_probQCDc[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probQCDcc", "AK15Puppi_ParticleNetMD_probQCDcc[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probQCDothers", "AK15Puppi_ParticleNetMD_probQCDothers[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probXbb", "AK15Puppi_ParticleNetMD_probXbb[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probXcc", "AK15Puppi_ParticleNetMD_probXcc[matchedAK15JetIdx]")
            .Define("ak15_ParticleNetMD_probXqq", "AK15Puppi_ParticleNetMD_probXqq[matchedAK15JetIdx]")


            // Subjets
            .Define("ak15_nSubjetsPerEventTotal", "nAK15PuppiSubJet")
            .Define("ak15_goodSubjets", "MakeGoodSubjetMask(AK15PuppiSubJet_eta, AK15PuppiSubJet_phi, ak15_eta, ak15_phi, 1.5f, 0.f)")
            .Define("ak15_nSubjets", "ROOT::VecOps::Sum(ak15_goodSubjets)")
            .Define("ak15_matchedSubjets", "MatchTwoJetsToHTauTau(AK15PuppiSubJet_pt, AK15PuppiSubJet_eta, AK15PuppiSubJet_phi, ak15_goodSubjets, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 0.4f)")
            .Define("ak15_matchedSubjet1_idx", "ak15_matchedSubjets[0]")
            .Define("ak15_matchedSubjet2_idx", "ak15_matchedSubjets[1]")
            .Define("ak15_matchedSubjet1_tauIdx", "ak15_matchedSubjets[2]")
            .Define("ak15_matchedSubjet2_tauIdx", "ak15_matchedSubjets[3]")
            .Define("ak15_nMatchedSubjets", "(int)(ak15_matchedSubjet1_idx >= 0) + (int)(ak15_matchedSubjet2_idx >= 0)")

            // Kinematic variables
            .Define("ak15_Subjet_mass", "FillMatchedSubjet(AK15PuppiSubJet_mass, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_eta", "FillMatchedSubjet(AK15PuppiSubJet_eta, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_phi", "FillMatchedSubjet(AK15PuppiSubJet_phi, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_pt", "FillMatchedSubjet(AK15PuppiSubJet_pt, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_rawFactor", "FillMatchedSubjet(AK15PuppiSubJet_rawFactor, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_pt_rawFactorCorrected", "FillMatchedSubjet((1 - AK15PuppiSubJet_rawFactor) * AK15PuppiSubJet_pt, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")

            // Area
            .Define("ak15_Subjet_area", "FillMatchedSubjet(AK15PuppiSubJet_area, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_radius", "FillMatchedSubjet(sqrt(AK15PuppiSubJet_area / 3.14159265), ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")

            // Flavours
            .Define("ak15_Subjet_nBHadrons", "FillMatchedSubjet(AK15PuppiSubJet_nBHadrons, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")
            .Define("ak15_Subjet_nCHadrons", "FillMatchedSubjet(AK15PuppiSubJet_nCHadrons, ak15_matchedSubjet1_idx, ak15_matchedSubjet2_idx)")

            .Define("dR_ak15_Subjet_tau1",
                "ak15_matchedSubjet1_idx >= 0 ? "
                "dR(ak15_Subjet_eta[0], ak15_Subjet_phi[0], "
                "GenPart_eta[ak15_matchedSubjet1_tauIdx], GenPart_phi[ak15_matchedSubjet1_tauIdx]) : "
                "kMissing")

            .Define("dR_ak15_Subjet_tau2",
                "ak15_matchedSubjet2_idx >= 0 ? "
                "dR(ak15_Subjet_eta[1], ak15_Subjet_phi[1], "
                "GenPart_eta[ak15_matchedSubjet2_tauIdx], GenPart_phi[ak15_matchedSubjet2_tauIdx]) : "
                "kMissing");
    }

    std::string hadhad = require_hadhad ? "_hadhad" : "";
    
    std::vector<ROOT::RDF::RResultHandle> snapshots;
    ROOT::RDF::RSnapshotOptions opts;
    opts.fLazy = true;


    if (include_RawData) {
        std::cout << "Skimming Raw Data... " <<std::endl;
        std::string out_dir = std::string(save_directory) + "/RawEventInfo" + hadhad + ".root";
        snapshots.push_back(df_raw_event_data.Snapshot("Events", out_dir, raw_data_dict, opts));
    }

    if (include_Tau) {
        std::cout << "Skimming Tau... " << std::endl;
        std::string out_dir = std::string(save_directory) + "/Tau" + hadhad + ".root";
        snapshots.push_back(df_Tau.Snapshot("Events", out_dir, tau_dict, opts));
    }

    if (include_AK4) {
        std::cout << "Skimming AK4... " <<std::endl;
        std::string out_dir = std::string(save_directory) + "/Jet" + hadhad + ".root";
        snapshots.push_back(df_AK4.Snapshot("Events", out_dir, AK4_dict, opts));
    }

    if (include_AK8) {
        std::cout << "Skimming AK8" <<std::endl;
        std::string out_dir = std::string(save_directory) + "/fatJet" + hadhad + ".root";
        snapshots.push_back(df_AK8.Snapshot("Events", out_dir, AK8_dict, opts));
    }

    if (include_AK15) {
        std::cout << "Skimming AK15" <<std::endl;
        std::string out_dir = std::string(save_directory) + "/AK15" + hadhad + ".root";
        snapshots.push_back(df_AK15.Snapshot("Events", out_dir, AK15_dict, opts));
    }

    if (!snapshots.empty()) {
        ROOT::RDF::RunGraphs(snapshots);
    }
    return;
}
