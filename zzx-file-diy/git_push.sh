#!/bin/bash

# copy the log.
# cp ./log_file/debug_output.txt ./src/ndnSIM/logs/debug_output.txt

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
git commit -m "ndnCFNAgg-v1.0.1-copy
A back up version of the original ndn-CFNAgg project.
Add the 'src/ndnSIM/README.md' and 'src/ndnSIM/prompt/CFNAgg-Prompt.md' files.
(1) Initial and re-construct the whole CFNAgg project code.
(2) The readme file of this project is in 'src/ndnSIM/README.md'.
(3) The sketch of the CFNAgg is in 'src/ndnSIM/prompt/CFNAgg-Prompt.md'.

BUGS:
Agg1 cannot receive the packet from Root. It seems to have problem in name registration. Lots of methods tried, but no success.
"

# Rename the branch to main
git branch -M main

# Force push the changes to the remote repository
git push -u origin main
# git push -u origin main --force