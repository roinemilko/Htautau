#ifndef BUILDDATA_H
#define BUILDDATA_H

#include "RtypesCore.h"
#include "ROOT/RVec.hxx"
#include <TChain.h>
#include <TTree.h>
#include <vector>

#define AK4_min_pt 30.0f
#define AK4_eta_max 2.5f

#define AK8_min_pt 200.0f
#define AK8_eta_max 2.5f

#define AK15_min_pt 0.f
#define AK15_eta_max 2.5f

constexpr Float_t kMissing = -998.f;

const std::vector<std::string> AK4_dict = {
    "target_mass",
    "matchedHiggsIdx",
    "matchedTau1Idx",
    "matchedTau2Idx",
    "nLastCopyTauFromHiggs",
    "nHiggsTauMu",
    "nHiggsTauE",
    "nHiggsTauHad",
    "is_truth_ehad",
    "is_truth_muhad",
    "is_truth_hadhad",
    "genH_pt",
    "genH_eta",
    "genH_phi",
    "PV_npvsGood",
    "Pileup_nTrueInt",
    "PV_npvs",

    "genTau1_pt",
    "genTau2_pt",

    "nTauMatchedGoodAK4Jet",
    "matchedAK4JetIdx",
    "ak4_pt",
    "ak4_eta",
    "ak4_phi",
    "ak4_mass",
    "ak4_rawFactor",
    "ak4_mass_rawFactorCorrected",
    "dphi_ak4_pfmet",
    "dphi_ak4_puppimet",
    "ak4_pt_minus_pfmet_pt",
    "ak4_pt_minus_puppimet_pt",
    "pfmet_over_ak4_pt",
    "puppimet_over_ak4_pt",
    "dR_ak4_tau1",
    "dR_ak4_tau2",
    "dR_ak4_H",
    "dR_tau1_tau2",
    "genTau_pt_asym",

    "RawPFMET_pt",
    "RawPFMET_phi",
    "PuppiMET_significance",
    "PuppiMET_sumEt",
    "PuppiMET_sumPtUnclustered",
    "PuppiMET_pt",
    "PuppiMET_phi"
};

const std::vector<std::string> AK8_dict = {
    // Event info
    "target_mass",
    "matchedHiggsIdx",
    "matchedTau1Idx",
    "matchedTau2Idx",
    "nLastCopyTauFromHiggs",
    "nHiggsTauMu",
    "nHiggsTauE",
    "nHiggsTauHad",
    "is_truth_ehad",
    "is_truth_muhad",
    "is_truth_hadhad",
    "PV_npvsGood",
    "Pileup_nTrueInt",
    "PV_npvs",
    
    // Generator
    "genH_pt",
    "genH_eta",
    "genH_phi",
    "genTau1_pt",
    "genTau1_eta",
    "genTau1_phi",
    "genTau2_pt",
    "genTau2_eta",
    "genTau2_phi",
    "genTau_pt_asym",

    // Kinematics + matching
    "matchedFatJetIdx",
    "nTauMatchedGoodFatJet",
    "fj_pt",
    "fj_eta",
    "fj_phi",
    "fj_mass",
    "fj_msoftdrop",
    "fj_rawFactor",
    "fj_mass_rawFactorCorrected",

    // more properties
    "fj_area",
    "fj_chEmEF",
    "fj_chHEF",
    "fj_chMultiplicity",
    "fj_hadronFlavour",
    "fj_hfEmEF",
    "fj_hfHEF",
    "fj_lsf3",
    "fj_muEF",
    "fj_nConstituents",
    "fj_neEmEF",
    "fj_neHEF",
    "fj_neMultiplicity",
    "fj_tau1",
    "fj_tau2",
    "fj_tau3",
    "fj_tau4",

    // GlobalParT3
    "fj_globalParT3_QCD",
    "fj_globalParT3_TopbWev",
    "fj_globalParT3_TopbWmv",
    "fj_globalParT3_TopbWq",
    "fj_globalParT3_TopbWqq",
    "fj_globalParT3_TopbWtauhv",
    "fj_globalParT3_WvsQCD",
    "fj_globalParT3_XWW3q",
    "fj_globalParT3_XWW4q",
    "fj_globalParT3_XWWqqev",
    "fj_globalParT3_XWWqqmv",
    "fj_globalParT3_Xbb",
    "fj_globalParT3_Xcc",
    "fj_globalParT3_Xcs",
    "fj_globalParT3_Xqq",
    "fj_globalParT3_Xtauhtaue",
    "fj_globalParT3_Xtauhtauh",
    "fj_globalParT3_Xtauhtaum",
    "fj_globalParT3_massCorrGeneric",
    "fj_globalParT3_massCorrX2p",
    "fj_globalParT3_withMassTopvsQCD",
    "fj_globalParT3_withMassWvsQCD",
    "fj_globalParT3_withMassZvsQCD",

    // MET & dR
    "RawPFMET_pt",
    "RawPFMET_phi",
    "PuppiMET_significance",
    "PuppiMET_sumEt",
    "PuppiMET_sumPtUnclustered",
    "PuppiMET_pt",
    "PuppiMET_phi",
    "dphi_fj_pfmet",
    "dphi_fj_puppimet",
    "fj_pt_minus_pfmet_pt",
    "fj_pt_minus_puppimet_pt",
    "pfmet_over_fj_pt",
    "puppimet_over_fj_pt",
    "dR_fj_tau1",
    "dR_fj_tau2",
    "dR_fj_H",
    "dR_tau1_tau2",

    // Subjets
    "fj_nSubjetsPerEventTotal",
    "fj_nSubjets",
    "fj_nMatchedSubjets",

    // kinematics
    "fj_Subjet_mass",
    "fj_Subjet_eta",
    "fj_Subjet_phi",
    "fj_Subjet_pt",
    "fj_Subjet_rawFactor",
    "fj_Subjet_pt_rawFactorCorrected",
    "fj_Subjet_area",

    // more properties
    "fj_Subjet_tau1",
    "fj_Subjet_tau2",
    "fj_Subjet_tau3",
    "fj_Subjet_tau4",
    "fj_Subjet_n2b1",
    "fj_Subjet_n3b1",
    "fj_Subjet_btagDeepFlavB",
    "fj_Subjet_btagUParTAK4B",
    "fj_Subjet_hadronFlavour",
    "fj_Subjet_nBHadrons",
    "fj_Subjet_nCHadrons",

    // ParT
    "fj_Subjet_UParTAK4RegPtRawCorr",
    "fj_Subjet_UParTAK4RegPtRawCorrNeutrino",
    "fj_Subjet_UParTAK4RegPtRawRes",
    "fj_Subjet_UParTAK4V1RegPtRawCorr",
    "fj_Subjet_UParTAK4V1RegPtRawCorrNeutrino",
    "fj_Subjet_UParTAK4V1RegPtRawRes",

    // gen dRtau
    "dR_fj_Subjet_tau1",
    "dR_fj_Subjet_tau2"
};

const std::vector<std::string> AK15_dict = {
    // event info
    "target_mass",
    "matchedHiggsIdx",
    "matchedTau1Idx",
    "matchedTau2Idx",
    "nLastCopyTauFromHiggs",
    "nHiggsTauMu",
    "nHiggsTauE",
    "nHiggsTauHad",
    "is_truth_ehad",
    "is_truth_muhad",
    "is_truth_hadhad",
    "PV_npvsGood",
    "Pileup_nTrueInt",
    "PV_npvs",
    
    // gen info
    "genH_pt",
    "genH_eta",
    "genH_phi",
    "genTau1_pt",
    "genTau1_eta",
    "genTau1_phi",
    "genTau2_pt",
    "genTau2_eta",
    "genTau2_phi",
    "genTau_pt_asym",

    // kinematics
    "matchedAK15JetIdx",
    "nTauMatchedGoodAK15Jet",
    "ak15_pt",
    "ak15_eta",
    "ak15_phi",
    "ak15_mass",
    "ak15_msoftdrop",
    "ak15_rawFactor",
    "ak15_mass_rawFactorCorrected",

    // more propetries
    "ak15_area",
    "ak15_nBHadrons",
    "ak15_nCHadrons",
    "ak15_tau1",
    "ak15_tau2",
    "ak15_tau3",

    // ParTv3
    "ak15_ParTv3_massCorrGeneric",
    "ak15_ParTv3_massCorrResonance",
    "ak15_ParTv3_massCorrX2p",
    "ak15_ParTv3_probQCD",
    "ak15_ParTv3_probTopbWev",
    "ak15_ParTv3_probTopbWmv",
    "ak15_ParTv3_probTopbWq",
    "ak15_ParTv3_probTopbWqq",
    "ak15_ParTv3_probTopbWtauhv",
    "ak15_ParTv3_probXWW3q",
    "ak15_ParTv3_probXWW4q",
    "ak15_ParTv3_probXWWqqev",
    "ak15_ParTv3_probXWWqqmv",
    "ak15_ParTv3_probXbb",
    "ak15_ParTv3_probXcc",
    "ak15_ParTv3_probXcs",
    "ak15_ParTv3_probXqq",
    "ak15_ParTv3_probXtauhtaue",
    "ak15_ParTv3_probXtauhtauh",
    "ak15_ParTv3_probXtauhtaum",

    // ParticleNetMD
    "ak15_ParticleNetMD_mass",
    "ak15_ParticleNetMD_probQCDb",
    "ak15_ParticleNetMD_probQCDbb",
    "ak15_ParticleNetMD_probQCDc",
    "ak15_ParticleNetMD_probQCDcc",
    "ak15_ParticleNetMD_probQCDothers",
    "ak15_ParticleNetMD_probXbb",
    "ak15_ParticleNetMD_probXcc",
    "ak15_ParticleNetMD_probXqq",

    // MET & dR
    "RawPFMET_pt",
    "RawPFMET_phi",
    "PuppiMET_significance",
    "PuppiMET_sumEt",
    "PuppiMET_sumPtUnclustered",
    "PuppiMET_pt",
    "PuppiMET_phi",
    "dphi_ak15_pfmet",
    "dphi_ak15_puppimet",
    "ak15_pt_minus_pfmet_pt",
    "ak15_pt_minus_puppimet_pt",
    "pfmet_over_ak15_pt",
    "puppimet_over_ak15_pt",
    "dR_ak15_tau1",
    "dR_ak15_tau2",
    "dR_ak15_H",
    "dR_tau1_tau2",

    // subjets
    "ak15_nSubjetsPerEventTotal",
    "ak15_nSubjets",
    "ak15_nMatchedSubjets",
    "ak15_Subjet_mass",
    "ak15_Subjet_eta",
    "ak15_Subjet_phi",
    "ak15_Subjet_pt",
    "ak15_Subjet_rawFactor",
    "ak15_Subjet_pt_rawFactorCorrected",
    "ak15_Subjet_area",

    // flavour & dR
    "ak15_Subjet_nBHadrons",
    "ak15_Subjet_nCHadrons",
    "dR_ak15_Subjet_tau1",
    "dR_ak15_Subjet_tau2"
};

const std::vector<std::string> tau_dict = {
    // note: gentruth
    "target_mass",
    "nMatchedGoodTau",
    "matchedTausIdx",
    "matchedGenTau1Idx",
    "matchedGenTau2Idx",
    "matchedHiggsIdx",

    "is_truth_hadhad",
    "is_truth_ehad",
    "is_truth_muhad",

    // note: genTruthh
    "genH_pt",
    "genH_eta",
    "genH_phi",
    "genTau1_pt",
    "genTau1_eta",
    "genTau1_phi",
    "genTau2_pt",
    "genTau2_eta",
    "genTau2_phi",
    "genTau_pt_asym",

    "tau_pt",
    "tau_eta",
    "tau_phi",
    "tau_mass",
    "tau_charge",
    "tau_dxy",
    "tau_dz",
    "tau_ipLengthSig",
    "tau_chargedIso",
    "tau_neutralIso",
    "tau_rawIso",
    "tau_rawIsodR03",
    "tau_puCorr",

    "tau_decayMode",
    "tau_genPartFlav",
    "tau_genPartIdx",

    // Deeptau
    "tau_idDeepTauVSjet",
    "tau_idDeepTauVSe",
    "tau_idDeepTauVSmu",
    "tau_rawDeepTauVSjet",
    "tau_rawDeepTauVSe",  
    "tau_rawDeepTauVSmu",

    // ParticleNet 2023
    "tau_decayModePNet",
    "tau_probDM0PNet",
    "tau_probDM1PNet",
    "tau_probDM2PNet",
    "tau_probDM10PNet",
    "tau_probDM11PNet",
    "tau_ptCorrPNet",
    "tau_qConfPNet",
    "tau_rawPNetVSe",
    "tau_rawPNetVSjet",
    "tau_rawPNetVSmu",

    // ParT 2024
    "tau_decayModeUParT",
    "tau_probDM0UParT",
    "tau_probDM1UParT",
    "tau_probDM2UParT",
    "tau_probDM10UParT",
    "tau_probDM11UParT",
    "tau_ptCorrUParT",
    "tau_qConfUParT",
    "tau_rawUParTVSe",
    "tau_rawUParTVSjet",
    "tau_rawUParTVSmu",

    // note: Gentruth
    "dR_tau_tau1",
    "dR_tau_tau2",
    "dR_tau1_tau2",
    "dR_tau_H",

    "PV_npvsGood",
    "Pileup_nTrueInt",
    "PV_npvs"
};

const std::vector<std::string> raw_data_dict = {
    "RawPFMET_pt",
    "RawPFMET_phi",   
    "PuppiMET_pt",
    "PuppiMET_phi",
    "truthHiggsIdx_raw",
    "genH_pt_raw",
    "genH_eta_raw",
    "genH_phi_raw",
    "dR_tau1_tau2_raw",
    "PV_npvsGood",
    "Pileup_nTrueInt",
    "PV_npvs",
    "genTau1_pt_raw",
    "genTau2_pt_raw",
    "genTau_pt_asym_raw",
    "dR_fJ_nolim",
    "dR_Jet_nolim",
    "dR_AK15_nolim",
    "dR_Tau_nolim",
    "is_truth_ehad",
    "is_truth_muhad",
    "is_truth_hadhad",
    "DecayProds_absid",
    "DecayProds_ptfrac",
    "DecayProds_parentTauIdx"
};

#endif