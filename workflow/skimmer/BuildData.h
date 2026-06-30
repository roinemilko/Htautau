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


    "matchedFatJetIdx",
    "nTauMatchedGoodFatJet",
    "fj_pt",
    "fj_eta",
    "fj_phi",
    "fj_mass",
    "fj_msoftdrop",
    "fj_rawFactor",
    "fj_mass_rawFactorCorrected",

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
    "genTau_pt_asym",

    "RawPFMET_pt",
    "RawPFMET_phi",
    "PuppiMET_significance",
    "PuppiMET_sumEt",
    "PuppiMET_sumPtUnclustered",
    "PuppiMET_pt",
    "PuppiMET_phi"
};


const std::vector<std::string> AK15_dict = {
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


    "matchedAK15JetIdx",
    "nTauMatchedGoodAK15Jet",
    "ak15_pt",
    "ak15_eta",
    "ak15_phi",
    "ak15_mass",
    "ak15_msoftdrop",
    "ak15_rawFactor",
    "ak15_mass_rawFactorCorrected",

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
    "genTau_pt_asym",

    "RawPFMET_pt",
    "RawPFMET_phi",
    "PuppiMET_significance",
    "PuppiMET_sumEt",
    "PuppiMET_sumPtUnclustered",
    "PuppiMET_pt",
    "PuppiMET_phi"
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
    "dR_AK15_nolim"
};

#endif