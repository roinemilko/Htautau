# Htautau
General use repo for my 2026 CERN summer project. May fork some parts for a more defined product later. Details in documentation of source code.

# Overview
In standard low energy ($< 250$ GeV) topologies $H\to \tau^- \tau^+$ -processes from di-Higgs -events are often reconstructed with two AK4 jets, one for each tau. In highly boosted
topologies ($> 400$ GeV) a single AK8 jet is used. It turns out reconstructing the event with a single big jet and analysing the jet substructure has some algorithmic
advantages, but is only available in rare, highly collimated event topologies. Furthermore, the "grey area" of $p_T \sim 300 - 400$ GeV remains underutilised in event reconstruction.

Theoretically, much larger AK15 jets could be used to reconstruct $H\to \tau^- \tau^+$ -events at energy scales from $p_T \geq \frac{2m_H}{\Delta R} \approx 167$ GeV, providing an
alternative tool for analysing events with semi-boosted topologies. My goal is to find out if these jets provide improved performance in a large enough range of transverse momenta
to be a useful addition to the $H\to \tau^-\tau^+$ reconstruction toolset. I will do this by training mass regression for the three types of jets against skimmed MC -data for a range
of momenta and comparing the results. Current challenges:
- How to define "good" performance? Fraction of correctly identified events? ROC -curve on signal vs. data?
- Dealing with the significantly larger amounts of pileup in AK15 jets
