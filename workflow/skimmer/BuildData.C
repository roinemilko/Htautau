#include <TChain.h> 
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <string>
#include "BuildData.h"
#include "helpers_v2.h"
#include "ROOT/RDataFrame.hxx"


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
    bool include_AK4 = true, bool include_AK8 = true, bool include_AK15 = true,
    bool require_hadhad = false
) {
    
    if (!include_AK4 && !include_AK8 && !include_AK15) {
        std::cerr << "You have to include somehting" <<std::endl;
        return;
    }
    
    ROOT::EnableImplicitMT();
    ROOT::RDataFrame df("Events", file_pattern);


    // Sometimes MET is missing so safeguard against that
    ROOT::RDF::RNode df_safe = df;
    std::vector<std::string> check_missing = {
            "RawPFMET_pt", "RawPFMET_phi", "RawPFMET_sumEt", 
            "PuppiMET_pt", "PuppiMET_phi", "PuppiMET_sumEt"
    };

    for (const auto& col : check_missing) {
        if (!df_safe.HasColumn(col)) {
            df_safe = df_safe.Define(col, []() { return kMissing; });
            std::cout << col << " missing..." << ::std::endl;
        }
    }

    auto df_truth = df_safe
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


    ROOT::RDF::RNode df_AK4 = df_truth;
    ROOT::RDF::RNode df_AK8 = df_truth;
    ROOT::RDF::RNode df_AK15 = df_truth;


    if (include_AK4) {
        df_AK4 = df_truth
            .Define("goodJets", Form("MakeGoodJetMask(Jet_pt, Jet_eta, %f, %f)", AK4_min_pt, AK4_eta_max))
            .Define("AK4Match", "MatchTwoJetsToHTauTau(Jet_eta, Jet_phi, goodJets, GenPart_eta, GenPart_phi, GenPart_pdgId, GenPart_genPartIdxMother, GenPart_statusFlags, 0.4f)")
            .Filter("AK4Match[0] >= 0", "Valid AK4 match")
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
            
            .Define("fj_pt", "FatJet_pt[matchedFatJetIdx]")
            .Define("fj_eta", "FatJet_eta[matchedFatJetIdx]")
            .Define("fj_phi", "FatJet_phi[matchedFatJetIdx]")
            .Define("fj_mass", "FatJet_mass[matchedFatJetIdx]")
            .Define("fj_msoftdrop", "FatJet_msoftdrop[matchedFatJetIdx]")
            .Define("fj_rawFactor", "FatJet_rawFactor[matchedFatJetIdx]")
            .Define("fj_mass_rawFactorCorrected", "fj_mass * (1.f - fj_rawFactor)")

            .Define("genH_pt", "GenPart_pt[matchedHiggsIdx]")
            .Define("genH_eta", "GenPart_eta[matchedHiggsIdx]")
            .Define("genH_phi", "GenPart_phi[matchedHiggsIdx]")
            .Define("genTau1_pt", "GenPart_pt[matchedTau1Idx]")
            .Define("genTau1_eta", "GenPart_eta[matchedTau1Idx]")
            .Define("genTau1_phi", "GenPart_phi[matchedTau1Idx]")
            .Define("genTau2_pt", "GenPart_pt[matchedTau2Idx]")
            .Define("genTau2_eta", "GenPart_eta[matchedTau2Idx]")
            .Define("genTau2_phi", "GenPart_phi[matchedTau2Idx]")

            .Define("dphi_fj_pfmet", "dphi(fj_phi, RawPFMET_phi)")
            .Define("dphi_fj_puppimet", "dphi(fj_phi,  PuppiMET_phi)")
            .Define("fj_pt_minus_pfmet_pt", "(fj_pt > kMissing && RawPFMET_pt > kMissing) ? fj_pt - RawPFMET_pt : kMissing")
            .Define("fj_pt_minus_puppimet_pt", "(fj_pt > kMissing && PuppiMET_pt > kMissing) ? fj_pt - PuppiMET_pt : kMissing")
            .Define("pfmet_over_fj_pt", "safeRatio(RawPFMET_pt, fj_pt)")
            .Define("puppimet_over_fj_pt", "safeRatio(PuppiMET_pt, fj_pt)")
            .Define("dR_fj_tau1", "dR(fj_eta, fj_phi, genTau1_eta, genTau1_phi)")
            .Define("dR_fj_tau2", "dR(fj_eta, fj_phi, genTau2_eta, genTau2_phi)")
            .Define("dR_fj_H", "dR(fj_eta, fj_phi, genH_eta, genH_phi)")
            .Define("dR_tau1_tau2", "dR(genTau1_eta, genTau1_phi, genTau2_eta, genTau2_phi)");
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

            .Define("ak15_pt", "AK15Puppi_pt[matchedAK15JetIdx]")
            .Define("ak15_eta", "AK15Puppi_eta[matchedAK15JetIdx]") 
            .Define("ak15_phi", "AK15Puppi_phi[matchedAK15JetIdx]")
            .Define("ak15_mass", "AK15Puppi_mass[matchedAK15JetIdx]")
            .Define("ak15_msoftdrop", "AK15Puppi_msoftdrop[matchedAK15JetIdx]")
            .Define("ak15_rawFactor", "AK15Puppi_rawFactor[matchedAK15JetIdx]")
            .Define("ak15_mass_rawFactorCorrected", "ak15_mass * (1.f - ak15_rawFactor)")

            .Define("genH_pt", "GenPart_pt[matchedHiggsIdx]")
            .Define("genH_eta", "GenPart_eta[matchedHiggsIdx]")
            .Define("genH_phi", "GenPart_phi[matchedHiggsIdx]")
            .Define("genTau1_pt", "GenPart_pt[matchedTau1Idx]")
            .Define("genTau1_eta", "GenPart_eta[matchedTau1Idx]")
            .Define("genTau1_phi", "GenPart_phi[matchedTau1Idx]")
            .Define("genTau2_pt", "GenPart_pt[matchedTau2Idx]")
            .Define("genTau2_eta", "GenPart_eta[matchedTau2Idx]")
            .Define("genTau2_phi", "GenPart_phi[matchedTau2Idx]")

            .Define("dphi_ak15_pfmet", "dphi(ak15_phi, RawPFMET_phi)")
            .Define("dphi_ak15_puppimet", "dphi(ak15_phi,  PuppiMET_phi)")
            .Define("ak15_pt_minus_pfmet_pt", "(ak15_pt > kMissing && RawPFMET_pt > kMissing) ? ak15_pt - RawPFMET_pt : kMissing")
            .Define("ak15_pt_minus_puppimet_pt", "(ak15_pt > kMissing && PuppiMET_pt > kMissing) ? ak15_pt - PuppiMET_pt : kMissing")
            .Define("pfmet_over_ak15_pt", "safeRatio(RawPFMET_pt, ak15_pt)")
            .Define("puppimet_over_ak15_pt", "safeRatio(PuppiMET_pt, ak15_pt)")
            .Define("dR_ak15_tau1", "dR(ak15_eta, ak15_phi, genTau1_eta, genTau1_phi)")
            .Define("dR_ak15_tau2", "dR(ak15_eta, ak15_phi, genTau2_eta, genTau2_phi)")
            .Define("dR_ak15_H", "dR(ak15_eta, ak15_phi, genH_eta, genH_phi)")
            .Define("dR_tau1_tau2", "dR(genTau1_eta, genTau1_phi, genTau2_eta, genTau2_phi)");
    } 

    std::string hadhad = require_hadhad ? "_hadhad" : "";

    if (include_AK4) {
        std::cout << "Skimming AK4... " <<std::endl;
        df_AK4.Snapshot(
            "Events",
            "jets/Jet" + hadhad + ".root",
            AK4_dict
        );
    }

    if (include_AK8) {
        std::cout << "Skimming AK8" <<std::endl;
        df_AK8.Snapshot(
            "Events",
            "jets/fatJet" + hadhad + ".root",
            AK8_dict
        );
    }

    if (include_AK15) {
        std::cout << "Skimming AK15" <<std::endl;
        df_AK15.Snapshot(
            "Events",
            "jets/AK15" + hadhad + ".root",
            AK15_dict
        );

    }
    return;
}
