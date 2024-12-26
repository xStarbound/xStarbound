{ runCommandLocal
}:

# helper function for turning a file derivation into a directory derivation
drv:
runCommandLocal "${drv.name}-dirwrapped" { } ''
  mkdir -p "$out"
  ln -s '${drv}' "$out/${drv.name}"
''
