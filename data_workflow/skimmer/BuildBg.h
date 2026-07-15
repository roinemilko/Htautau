#ifndef BUILDBG_H
#define BUILDBG_H

#include "RtypesCore.h"
#include "ROOT/RVec.hxx"
#include <TChain.h>
#include <TTree.h>
#include <vector>

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

#endif