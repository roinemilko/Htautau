# Htautau
General use repo for my 2026 CERN summer project. May fork some parts for a more defined product later. Details in documentation of source code.

# Overview
At the Large Hadron Collider (LHC), proton-proton collisions of center-of-mass energies up to $13.6\,\text{TeV}$ are measured and the underlying processes reconstructed as a way to test predictions of the Standard Model and to probe new physics beyond it. In these studies, events in which a Higgs boson decays into two tau leptons provide a sensitive channel for measuring how the Higgs boson couples with fermions, allowing further insight into the properties of one of the most significant scientific discoveries of this decade.

The Compact Muon Solenoid (CMS) detector at the LHC consists of hadronic and electromagnetic calorimeters, muon chambers and superconducting magnets paired with silicon trackers to measure the masses, energies and momenta of the stable particles showering from the collisions produced at the LHC. Using this information the particles and their trajectories are reconstructed using Particle Flow algorithms and then clustered into jets, generally using the anti-$k_T$ -algorithm. The properties and substructure of these jets are then analyzed using tagging algorithms and machine learning techniques to identify the original particles and processes that happened at collision-level.

The goal of this project is to experiment with using anti-$k_T$ -jets of an exceptionally large radius parameter of $R=1.5$ (AK15 -jets) to reconstruct $H\to \tau^+\tau^-$ -events in the CMS -experiment. These jets have potential to replace the traditional choice of $R = 0.8$ (FatJets) as more efficient and reliable in collimated collision topologies.

In the first part of the projecrt properties and clustering kinematics of AK15 -jets, FatJets and jets constructed with $R=0.4$ (Jets) are studied using Monte Carlo -simulated collision data. The next goal is to compare tagging performance of these jets is compared using ParticleNet and Unified ParT 2024, combined with a BDT and an MLP network.
