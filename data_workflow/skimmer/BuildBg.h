#ifndef BUILDBG_H
#define BUILDBG_H

#include "RtypesCore.h"
#include "ROOT/RVec.hxx"
#include <TChain.h>
#include <TTree.h>
#include <vector>

#define AK8_min_pt 200.0f
#define AK8_eta_max 2.5f

#define AK15_min_pt 0.f
#define AK15_eta_max 2.5f

const std::vector<std::string> TauBG_dict = {
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
    "tau_rawUParTVSmu"
};

const std::vector<std::string> AK8BG_dict = {
    // kinematics
    "fj_pt",
    "fj_eta",
    "fj_phi",
    "fj_mass",
    "fj_msoftdrop",
    "fj_rawFactor",

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

    // subjets
    "fj_nMatchedSubjets",

    // kinematics
    "fj_Subjet_pt",
    "fj_Subjet_eta",
    "fj_Subjet_phi",
    "fj_Subjet_mass",
    "fj_Subjet_area",
    "fj_Subjet_rawFactor",
    "fj_Subjet_pt_rawFactorCorrected",

    // substructure
    "fj_Subjet_tau1",
    "fj_Subjet_tau2",
    "fj_Subjet_tau3",
    "fj_Subjet_tau4",
    "fj_Subjet_n2b1",
    "fj_Subjet_n3b1",

    // B-Tagging & Flavour
    "fj_Subjet_btagDeepFlavB",
    "fj_Subjet_btagUParTAK4B",
    "fj_Subjet_hadronFlavour",
    "fj_Subjet_nBHadrons",
    "fj_Subjet_nCHadrons",

    // UParT
    "fj_Subjet_UParTAK4RegPtRawCorr",
    "fj_Subjet_UParTAK4RegPtRawCorrNeutrino",
    "fj_Subjet_UParTAK4RegPtRawRes",
    "fj_Subjet_UParTAK4V1RegPtRawCorr",
    "fj_Subjet_UParTAK4V1RegPtRawCorrNeutrino",
    "fj_Subjet_UParTAK4V1RegPtRawRes"
};

const std::vector<std::string> AK15BG_dict = {
    // kinematics 
    "ak15_pt",
    "ak15_eta",
    "ak15_phi",
    "ak15_mass",
    "ak15_msoftdrop",
    "ak15_rawFactor",
    "ak15_area",

    // flavour
    "ak15_nBHadrons",
    "ak15_nCHadrons",

    // substructure
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

    // subjets
    "ak15_nMatchedSubjets",

    // kinematics
    "ak15_Subjet_pt",
    "ak15_Subjet_eta",
    "ak15_Subjet_phi",
    "ak15_Subjet_mass",
    "ak15_Subjet_rawFactor",
    "ak15_Subjet_pt_rawFactorCorrected",
    "ak15_Subjet_area",

    // flavour
    "ak15_Subjet_nBHadrons",
    "ak15_Subjet_nCHadrons"
};

#endif