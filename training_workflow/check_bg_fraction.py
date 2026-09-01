import uproot
import os

def main():
    bg_dir = "/eos/user/m/mroine/NanoTuples/Htautau/data_workflow/jets/MADGRAPH"
    
    raw_file = os.path.join(bg_dir, "RawEventInfo.root")
    tau_file = os.path.join(bg_dir, "Tau.root")
    ak8_file = os.path.join(bg_dir, "fatJet.root")
    ak15_file = os.path.join(bg_dir, "AK15.root")

    print("Reading tree metadata from TTto4Q background files...\n")
    
    # 1. Read Raw total generated background events
    if not os.path.exists(raw_file):
        print(f"Error: Raw file not found at {raw_file}")
        return

    with uproot.open(f"{raw_file}:Events") as tree_raw:
        n_raw = tree_raw.num_entries

    print(f"Total Generated Raw Background Events: {n_raw:,}\n")
    print(f"{'Jet / Mode Type':<18} | {'Passing Events':<15} | {'Fraction Remaining':<18}")
    print("-" * 58)


    files = [
        ("Tau", tau_file),
        ("AK8", ak8_file),
        ("AK15", ak15_file),
    ]

    for name, filepath in files:
        if not os.path.exists(filepath):
            print(f"{name:<18} | File Not Found")
            continue

        with uproot.open(f"{filepath}:Events") as tree:
            n_events = tree.num_entries
            
        frac = (n_events / n_raw) * 100 if n_raw > 0 else 0.0
        print(f"{name:<18} | {n_events:>15,} | {frac:>17.2f}%")

    print("-" * 58 + "\n")

if __name__ == "__main__":
    main()