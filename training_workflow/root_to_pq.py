import argparse
import os
import pandas as pd

# Adjust these imports to match whatever you named the files containing your loader functions!
from uproot_data import load_mixed_tau_data
from uproot_fat import load_mixed_fatjet_data

def main():
    parser = argparse.ArgumentParser(description="Convert ROOT files to a fast-reading Parquet cache.")
    
    parser.add_argument("--inputs", required=True, help="Comma-separated list of input ROOT files")
    parser.add_argument("--out", required=True, help="Output Parquet file path")
    parser.add_argument("--mode", required=True, choices=["Tau", "AK8", "AK15"], help="Object type")
    parser.add_argument("--label", type=int, default=0, help="Class label (0 for bg, 1 for sig)")
    parser.add_argument("--num_taus", type=int, default=2, help="Number of taus (Tau mode only)")
    parser.add_argument("--use_subjets", action="store_true", help="Load subjet variables (AK8/AK15 only)")
    parser.add_argument("--use_weights", action="store_true", help="Calculate and apply event weights")
    
    args = parser.parse_args()

    # Parse the comma-separated input string into a list
    paths = [p.strip() for p in args.inputs.split(",") if p.strip()]

    print(f"Converting {len(paths)} ROOT file(s) for mode {args.mode}...")

    # Load the data using your memory-safe accumulator functions
    if args.mode == "Tau":
        df = load_mixed_tau_data(
            paths=paths,
            label=args.label,
            num_taus=args.num_taus,
            variables=None,  # Falls back to your default variable lists
            apply_weights=args.use_weights
        )
    elif args.mode in ["AK8", "AK15"]:
        df = load_mixed_fatjet_data(
            paths=paths,
            label=args.label,
            jet_type=args.mode,
            use_subjets=args.use_subjets,
            variables=None,
            apply_weights=args.use_weights
        )
    else:
        raise ValueError(f"Unknown mode: {args.mode}")

    # Ensure the target directory exists
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)

    # Save to Parquet
    print(f"Saving {len(df)} events to {args.out}...")
    df.to_parquet(args.out, index=False)
    print("Done!")

if __name__ == "__main__":
    main()
