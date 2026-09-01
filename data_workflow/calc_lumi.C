#include <iostream>
#include <string>
#include <TString.h>
#include <TChain.h>

void calc_lumi(std::string directory, double cross_section_fb) {
    // Create a TChain pointing to the "Runs" tree
    TChain chain("Runs");
    
    // Add all .root files in the target directory to the chain
    TString path = directory + "/*.root";
    int nFiles = chain.Add(path);
    
    if (nFiles == 0) {
        std::cerr << "Error: No ROOT files found in " << directory << "!" << std::endl;
        return;
    }

    // Set up the branch address to read genEventSumw
    Double_t genEventSumw = 0;
    chain.SetBranchAddress("genEventSumw", &genEventSumw);

    // Loop over all entries in the chain (across all files)
    Double_t totalSumw = 0;
    Long64_t nEntries = chain.GetEntries();
    
    for (Long64_t i = 0; i < nEntries; ++i) {
        chain.GetEntry(i);
        totalSumw += genEventSumw;
    }

    // Calculate the equivalent luminosity
    double lumi_fb = totalSumw / cross_section_fb;

    // Print the results
    std::cout << "\n==================================================" << std::endl;
    std::cout << "Directory      : " << directory << std::endl;
    std::cout << "Files chained  : " << nFiles << std::endl;
    std::cout << "Total SumW     : " << totalSumw << std::endl;
    std::cout << "Cross-section  : " << cross_section_fb << " fb" << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Eq. Luminosity : " << lumi_fb << " fb^-1" << std::endl;
    std::cout << "==================================================\n" << std::endl;
}
