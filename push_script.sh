#!/bin/bash

# --- EDIT THIS LINE BEFORE RUNNING ---
COMMIT_MESSAGE="Opt-debug-v1.1: successfully locate the bug in optimized mode, which is in the 'ndnSIM-opt-test/src/ndnSIM/NFD/daemon/fw/strategy.cpp', in the 'Strategy::sendData' function."
# ------------------------------------

# Navigate to the script's directory (optional, but safer if run from elsewhere)
# cd "$(dirname "$0")"

# Check if the commit message is the default placeholder
if [ "$COMMIT_MESSAGE" = "Your commit message goes here" ]; then
  echo "Error: Please edit the COMMIT_MESSAGE variable in the script before running."
  exit 1
fi

echo "--- Staging all changes ---"
git add .
echo "Changes staged."
echo ""

# Check if there's anything to commit
if git diff --staged --quiet; then
  echo "No changes staged to commit. Checking if there are unpushed commits..."
else
  # Commit changes using the message from the variable
  echo ""
  echo "--- Committing changes with message: '$COMMIT_MESSAGE' ---"
  git commit -m "$COMMIT_MESSAGE"
  if [ $? -ne 0 ]; then
    echo "Commit failed. Aborting."
    exit 1
  fi
  echo "Commit successful."
  echo ""
fi

# Get current branch name
current_branch=$(git branch --show-current)
if [ -z "$current_branch" ]; then
  echo "Error: Could not determine current branch. Are you in a detached HEAD state?"
  exit 1
fi

# Push to remote
echo "--- Pushing branch '$current_branch' to remote 'origin' ---"
git push origin "$current_branch"
if [ $? -ne 0 ]; then
  echo "Push failed."
  exit 1
fi

echo ""
echo "--- Push successful! ---"
exit 0