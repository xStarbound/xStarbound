-- Taken straight from OpenStarbound's
-- assets/opensb/interface/windowconfig/songbook_search_patch.lua
function patch(config)
  local scrollBG = config.paneLayout.scrollBG
  local scrollSize = assets.image(scrollBG.file):size()
  config.paneLayout.search = {
    type = "textbox",
    position = {scrollBG.position[1] + 3,
                scrollBG.position[2] + scrollSize[2] - 10},
    hint = "^#999;Type here to search for a song",
    maxWidth = scrollSize[1] - 6
  }
  return config
end
