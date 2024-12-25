#!/bin/bash
if [ -e .attrs.sh ]; then source .attrs.sh; fi
source "${stdenv:?}/setup"

# hack to stop crash
export HOME
HOME=$(mktemp -d)

mkdir -p "${out:?}"

steamcmd \
  "+login anonymous" \
  "+workshop_download_item $appId $workshopId" \
  "+quit"

mv "$HOME/.local/share/Steam/Steamapps/workshop/content/$appId/$workshopId/"* "$out"
