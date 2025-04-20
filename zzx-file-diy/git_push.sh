#!/bin/bash

# copy the log.
# cp ./log_file/debug_output.txt ./src/ndnSIM/logs/debug_output.txt

TARGET_DIR=src/ndnSIM/zzx-file-diy
if [ ! -d "$TARGET_DIR" ]; then
  mkdir "$TARGET_DIR"
fi
cp ./*.sh "$TARGET_DIR"
cp -r ./log_file "$TARGET_DIR"

# Navigate to the project directory
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
echo "pwd: ${SCRIPT_DIR}"
cd "${SCRIPT_DIR}/src/ndnSIM"

# Initialize a Git repository if it doesn't already exist
if [ ! -d .git ]; then
    git init
fi

# Add the remote repository
# git remote add origin https://github.com/zzhang154/Decentralized-Aggregation.git

# Add all files to the staging area
git add .

# Commit the changes
git commit -m "ndnCFNAgg-v1.1
Three test project file, under "src/ndnSIM/examples" and "src/ndnSIM/apps".
(1) agg-mini
(2) cfnagg-lite
(3) cfnagg

Comments:
'agg-mini' is can successfully run a mini test, but without many components. Both 'cfnagg-lite' and 'cfnagg' have BUGs on message forwarding from ROOT to AGG.
"

# Rename the branch to main
git branch -M main

# Force push the changes to the remote repository
git push -u origin main
# git push -u origin main --force