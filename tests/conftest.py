import os
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRAINING_WORKFLOW_DIR = os.path.join(REPO_ROOT, "training_workflow")
if TRAINING_WORKFLOW_DIR not in sys.path:
    sys.path.insert(0, TRAINING_WORKFLOW_DIR)
