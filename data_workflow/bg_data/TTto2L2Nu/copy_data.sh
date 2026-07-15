
DATASET="/TTtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8/CustomNanoAODv15-NanoTuples-uParTv3-parTlepID-NanoAODv15_RunIII2024Summer24MiniAODv6-150X_v2-v2-00000000000000000000000000000000/USER"

echo "Querying DAS for 150 files..."
FILES=$(dasgoclient -query="file dataset=$DATASET instance=prod/phys03" -limit=150)

if [ -z "$FILES" ]; then
    echo "No files found. Check your grid proxy or dataset name."   
    exit 1
fi

COUNT=1
for FILE in $FILES; do
    echo "Copying file $COUNT of 150: $FILE"
    
    xrdcp "root://cms-xrd-global.cern.ch/$FILE" .
    
    ((COUNT++))
done

echo "Done!"
