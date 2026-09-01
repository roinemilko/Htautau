import xgboost as xgb
import os
import random
import argparse

def visualize():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model_path", required=True, help="Path of the model to be visualized")
    parser.add_argument("--model_name", required=False, help="Name of the model for dir name")
    parser.add_argument("--out_dir", required=True, help="Output directory")
    args = parser.parse_args()

    model = xgb.Booster()
    model.load_model(args.model_path)

    total_trees = len(model.get_dump())
    print(f"Model contains {total_trees} trees in total.")

    num_to_plot = min(10, total_trees)


    random_indices = range(num_to_plot)
    
    for i in random_indices:
        dot = xgb.to_graphviz(model, num_trees=i, rankdir='UT')

        dot.format = 'svg'
        
        file_path = os.path.join(args.out_dir, f'tree_{i:03d}')
        dot.render(file_path)
        
        print(f"Saved: {file_path}.svg")

    print("Done!")

if __name__ == "__main__":
    visualize()