import os
import sys

# training_workflow's modules are flat scripts (no package/__init__.py), meant
# to be imported by running python from inside training_workflow/ itself
# (that's how the Snakefile invokes them). Add it to sys.path so tests can
# import Helpers/plot_helpers the same way those scripts import each other.
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRAINING_WORKFLOW_DIR = os.path.join(REPO_ROOT, "training_workflow")
if TRAINING_WORKFLOW_DIR not in sys.path:
    sys.path.insert(0, TRAINING_WORKFLOW_DIR)
