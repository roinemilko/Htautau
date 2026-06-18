#ifndef HTAUTAU_STUDENT_HELPERS_H
#define HTAUTAU_STUDENT_HELPERS_H

// Modified from Onni Salmi's "helpers.h"

#include <ROOT/RVec.hxx>
#include <cmath>
#include <vector>

using namespace ROOT::VecOps;

// NanoAOD GenPart_statusFlags bit positions used here. 
// Refernece: https://pdg.lbl.gov/2025/reviews/rpp2025-rev-monte-carlo-numbering.pdf

constexpr int kFromHardProcess = 8; // is it from the hard scatter?
constexpr int kIsLastCopy      = 13; // is it the last copy in the decay chain?
constexpr int tauPdgId         = 15; // is a tau?
constexpr int muonPdgId        = 13; // is it a muon?
constexpr int electronPdgId    = 11; // is it an electron?
constexpr int higgsPdgId       = 25; // is it Higgs?
constexpr int safetyThreshold  = 200;

// Signed delta-phi in (-pi, pi].
inline float DeltaPhi(float phi1, float phi2) {
    constexpr float pi = 3.14159265358979323846f;
    float dphi = std::fmod(phi1 - phi2, 2.0f * pi);
    if (dphi > pi) dphi -= 2.0f * pi;
    if (dphi <= -pi) dphi += 2.0f * pi;
    return dphi;
}

// Delta-R in (eta, phi) space.
inline float DeltaR(float eta1, float phi1, float eta2, float phi2) {
    const float dphi = DeltaPhi(phi1, phi2);
    const float deta = eta1 - eta2;
    return std::sqrt(deta * deta + dphi * dphi);
}

// JET PRESELECTION 

// Per-jet mask: 1 if pt > pt_min and |eta| < abs_eta_max, else 0.
inline ROOT::VecOps::RVec<int> MakeGoodJetMask(
    const ROOT::VecOps::RVec<float>& jet_pt,
    const ROOT::VecOps::RVec<float>& jet_eta,
    float pt_min,
    float abs_eta_max
) {
    ROOT::VecOps::RVec<int> out(jet_pt.size(), 0);
    for (size_t i = 0; i < jet_pt.size(); ++i) {
        out[i] = (jet_pt[i] > pt_min && std::abs(jet_eta[i]) < abs_eta_max) ? 1 : 0;
    }
    return out;
}

// GROUND-TRUTH INFORMATION

inline bool HasStatusFlag(int statusFlags, int bit) {
    return (statusFlags >> bit) & 1;
}

// Finds the closest ancestor of a particle with the type targetAncestorPdgId and returns its index or -1 if none exist.
// Walks mother links until |pdgId| == targetAncestorPdgId; 
inline int FindAncestorIndex(
    int idx,
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx,
    int targetAncestorPdgId
) {
    int current = idx;
    int safety = 0;

    while (current >= 0 && current < static_cast<int>(gen_motherIdx.size()) && safety < safetyThreshold) {
        const int mom = gen_motherIdx[current];
        if (mom < 0 || mom >= static_cast<int>(gen_pdgId.size())) return -1;
        if (std::abs(gen_pdgId[mom]) == targetAncestorPdgId) return mom;
        current = mom;
        ++safety;
    }
    return -1;
}

// True if is hadron
inline bool IsHadronPdgId(int pdgId) {
    return std::abs(pdgId) > 100;
}


// True if is the original tau from the hard H decay - That is:
// - is a tau (page id == \pm 15)
// - from the hard process (kFromHardProcess == True)
// - is the last copy in the decay chain (kIsLastCopy == True)
inline bool IsHardProcLastCopyTauFromHiggs(
    int idx,
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx,
    const ROOT::VecOps::RVec<int>& gen_statusFlags
) {
    if (idx < 0 || idx >= static_cast<int>(gen_pdgId.size())) return false;
    if (std::abs(gen_pdgId[idx]) != tauPdgId) return false;
    if (!HasStatusFlag(gen_statusFlags[idx], kFromHardProcess)) return false;
    if (!HasStatusFlag(gen_statusFlags[idx], kIsLastCopy)) return false;
    return FindAncestorIndex(idx, gen_pdgId, gen_motherIdx, higgsPdgId) >= 0;
}

// Mask that forces IsHardProcLastCopyTauFromHiggs; 1 if is last copy and 0 otherwise
inline ROOT::VecOps::RVec<int> MakeIsHardProcLastCopyTauFromHiggsMask(
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx,
    const ROOT::VecOps::RVec<int>& gen_statusFlags
) {
    ROOT::VecOps::RVec<int> out(gen_pdgId.size(), 0);
    for (int i = 0; i < static_cast<int>(gen_pdgId.size()); ++i) {
        out[i] = IsHardProcLastCopyTauFromHiggs(i, gen_pdgId, gen_motherIdx, gen_statusFlags) ? 1 : 0;
    }
    return out;
}

// Follow mothers from idx until a tau or a hadron is reached;
// return tau idx if decayed from a tau or -1 if decayed from a hadron
inline int FindTauAncestorBeforeHadron(
    int idx,
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx
) {
    int current = idx;
    int safety = 0;

    while (current >= 0 && current < static_cast<int>(gen_motherIdx.size()) && safety < safetyThreshold) {
        const int mom = gen_motherIdx[current];
        if (mom < 0 || mom >= static_cast<int>(gen_pdgId.size())) return -1;

        const int apdg = std::abs(gen_pdgId[mom]);
        if (apdg == tauPdgId) return mom;
        if (IsHadronPdgId(gen_pdgId[mom])) return -1;

        current = mom;
        ++safety;
    }
    return -1;
}

// Masks taus that decay directly to mus;
// 1 if has at least one mu decendant linked by FindTauAncestorBeforeHadron
inline ROOT::VecOps::RVec<int> TauIsSemileptonicMuMask_Clean(
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx
) {
    ROOT::VecOps::RVec<int> out(gen_pdgId.size(), 0);
    for (int tau = 0; tau < static_cast<int>(gen_pdgId.size()); ++tau) {
        if (std::abs(gen_pdgId[tau]) != tauPdgId) continue;
        for (int i = 0; i < static_cast<int>(gen_pdgId.size()); ++i) {
            if (std::abs(gen_pdgId[i]) != muonPdgId) continue;
            if (FindTauAncestorBeforeHadron(i, gen_pdgId, gen_motherIdx) == tau) {
                out[tau] = 1;
                break;
            }
        }
    }
    return out;
}

// Masks taus that decay directly to electrons;
// 1 if has at least one electron decendant linked by FindTauAncestorBeforeHadron
inline ROOT::VecOps::RVec<int> TauIsSemileptonicEMask_Clean(
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx
) {
    ROOT::VecOps::RVec<int> out(gen_pdgId.size(), 0);
    for (int tau = 0; tau < static_cast<int>(gen_pdgId.size()); ++tau) {
        if (std::abs(gen_pdgId[tau]) != tauPdgId) continue;
        for (int i = 0; i < static_cast<int>(gen_pdgId.size()); ++i) {
            if (std::abs(gen_pdgId[i]) != electronPdgId) continue;
            if (FindTauAncestorBeforeHadron(i, gen_pdgId, gen_motherIdx) == tau) {
                out[tau] = 1;
                break;
            }
        }
    }
    return out;
}

// Masks taus that are not either of the above and then by exclusion decayed hadronically
// 1 if !TauIsSemileptonicEMask_Clean and !TauIsSemileptonicMuMask_Clean
inline ROOT::VecOps::RVec<int> TauIsHadronicMask_Clean(
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& tauMuMask,
    const ROOT::VecOps::RVec<int>& tauEMask
) {
    ROOT::VecOps::RVec<int> out(gen_pdgId.size(), 0);
    for (int tau = 0; tau < static_cast<int>(gen_pdgId.size()); ++tau) {
        if (std::abs(gen_pdgId[tau]) != tauPdgId) continue;
        if (tauMuMask[tau]) continue;
        if (tauEMask[tau]) continue;
        out[tau] = 1;
    }
    return out;
}

// FIND H->TAUTAU PROCESS

// Struct for tau candidate
struct TauCandidateInfo {
    int idx;
    int higgsIdx;
    int pdgId;
};

// Mask: For each preselected jet, select the ones that contain opposite charge taus from hard processes (indicating H->tautau jet). Forces
// only one matched jet
// returns {jet, tau1_index, tau2_index, H_index}
inline ROOT::VecOps::RVec<int> MatchOneJetToHTauTau(
    const ROOT::VecOps::RVec<float>& jet_eta,
    const ROOT::VecOps::RVec<float>& jet_phi,
    const ROOT::VecOps::RVec<int>& goodJet,
    const ROOT::VecOps::RVec<float>& gen_eta,
    const ROOT::VecOps::RVec<float>& gen_phi,
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx,
    const ROOT::VecOps::RVec<int>& gen_statusFlags,
    float dRmax
) {
    const ROOT::VecOps::RVec<int> fail = {-1, -1, -1, -1};
    int matchedJet = -1, tau1 = -1, tau2 = -1, higgsIdx = -1, nMatched = 0;

    for (size_t jet = 0; jet < jet_eta.size(); ++jet) {
        if (!goodJet[jet]) continue;

        for (int a = 0; a < static_cast<int>(gen_eta.size()); ++a) {
            // find tau that is hard ancestor from higgs and in cone
            if (!IsHardProcLastCopyTauFromHiggs(a, gen_pdgId, gen_motherIdx, gen_statusFlags)) continue;
            if (DeltaR(jet_eta[jet], jet_phi[jet], gen_eta[a], gen_phi[a]) >= dRmax) continue;

            // save ancestor higgs idx
            const int hA = FindAncestorIndex(a, gen_pdgId, gen_motherIdx, higgsPdgId);
            if (hA < 0) continue;

            // find a pair and save its idx
            for (int b = a + 1; b < static_cast<int>(gen_eta.size()); ++b) {
                if (!IsHardProcLastCopyTauFromHiggs(b, gen_pdgId, gen_motherIdx, gen_statusFlags)) continue;
                if (gen_pdgId[a] != -gen_pdgId[b]) continue;
                if (FindAncestorIndex(b, gen_pdgId, gen_motherIdx, higgsPdgId) != hA) continue;
                if (DeltaR(jet_eta[jet], jet_phi[jet], gen_eta[b], gen_phi[b]) >= dRmax) continue;

                matchedJet = static_cast<int>(jet);
                tau1 = a;
                tau2 = b;
                higgsIdx = hA;
                ++nMatched;
                goto done;
            }
        }
    }
    done:
    // make sure only one jet is matched
    if (nMatched != 1) return fail;
    return {matchedJet, tau1, tau2, higgsIdx};
}

// Similar to MatchJetToHTauTau but matches 2 (AK4) jets with each Jet containing one tau
// Returns {jetForTau1, jetForTau2, tau1Idx, tau2Idx, higgsIdx}; all -1 on failure.
inline ROOT::VecOps::RVec<int> MatchTwoJetsToHTauTau(
    const ROOT::VecOps::RVec<float>& jet_eta,
    const ROOT::VecOps::RVec<float>& jet_phi,
    const ROOT::VecOps::RVec<int>& goodJet,
    const ROOT::VecOps::RVec<float>& gen_eta,
    const ROOT::VecOps::RVec<float>& gen_phi,
    const ROOT::VecOps::RVec<int>& gen_pdgId,
    const ROOT::VecOps::RVec<int>& gen_motherIdx,
    const ROOT::VecOps::RVec<int>& gen_statusFlags,
    float dRmax
) {
    const ROOT::VecOps::RVec<int> fail = {-1, -1, -1, -1, -1};
    int tau1 = -1, tau2 = -1, higgsIdx = -1, nPairs = 0;

    for (int a = 0; a < static_cast<int>(gen_pdgId.size()); ++a) {

        // find a tau that is from hard scatter 
        if (!IsHardProcLastCopyTauFromHiggs(a, gen_pdgId, gen_motherIdx, gen_statusFlags)) continue;

        // find a pair
        for (int b = a + 1; b < static_cast<int>(gen_pdgId.size()); ++b) {
            if (!IsHardProcLastCopyTauFromHiggs(b, gen_pdgId, gen_motherIdx, gen_statusFlags)) continue;
            const int h = FindAncestorIndex(a, gen_pdgId, gen_motherIdx, higgsPdgId);
            if (h < 0 || h != FindAncestorIndex(b, gen_pdgId, gen_motherIdx, higgsPdgId)) continue;
            if (gen_pdgId[a] != -gen_pdgId[b]) continue;
            tau1 = a; tau2 = b; higgsIdx = h; ++nPairs;
        }
    }
    // force only one pair found
    if (nPairs != 1) return fail;

    // lambda fn to find a jet for a tau
    auto jetForTau = [&](int tau) {
        int jet = -1, n = 0;
        for (size_t j = 0; j < jet_eta.size(); ++j) {
            if (!goodJet[j]) continue;
            if (DeltaR(jet_eta[j], jet_phi[j], gen_eta[tau], gen_phi[tau]) >= dRmax) continue;
            jet = static_cast<int>(j);
            ++n;
        }
        return (n == 1) ? jet : -1;
    };

    // find AK4 jets for each tau
    const int jet1 = jetForTau(tau1);
    const int jet2 = jetForTau(tau2);
    if (jet1 < 0 || jet2 < 0 || jet1 == jet2) return fail;
    return {jet1, jet2, tau1, tau2, higgsIdx};
}

#endif
