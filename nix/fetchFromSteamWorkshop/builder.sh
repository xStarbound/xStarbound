#!/bin/bash
if [ -e .attrs.sh ]; then source .attrs.sh; fi
source "${stdenv:?}/setup"

# hack to stop crash
# ~/.local/share/
export HOME
HOME=$(mktemp -d)

mkdir -p "${out:?}"

# export LD_LIBRARY_PATH
# echo "hi"
# echo "$LD_LIBRARY_PATH"

steamcmd \
  "+login anonymous" \
  "+workshop_download_item $appId $workshopId" \
  "+quit"


mv "$HOME/.local/share/Steam/Steamapps/workshop/content/$appId/$workshopId/"* "$out"

# mv 

# DepotDownloader \
# 	"${args[@]}" \
# 	-dir "${out:?}"
# rm -rf "${out:?}/.DepotDownloader"
