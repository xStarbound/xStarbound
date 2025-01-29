#include "StarModsMenu.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarListWidget.hpp"

namespace Star {

ModsMenu::ModsMenu() {
  auto assets = Root::singleton().assets();

  GuiReader reader;
  reader.registerCallback("linkbutton", bind(&ModsMenu::openLink, this));
  reader.registerCallback("workshopbutton", bind(&ModsMenu::openWorkshop, this));
  reader.construct(assets->json("/interface/modsmenu/modsmenu.config:paneLayout"), this);

  m_assetsSources = assets->assetSources();
  m_modList = fetchChild<ListWidget>("mods.list");
  for (auto const& assetsSource : m_assetsSources) {
    auto modName = m_modList->addItem()->fetchChild<LabelWidget>("name");
    modName->setText(bestModName(assets->assetSourceMetadata(assetsSource), assetsSource));
  }

  m_modName = findChild<LabelWidget>("modname");
  m_modAuthor = findChild<LabelWidget>("modauthor");
  m_modVersion = findChild<LabelWidget>("modversion");
  m_modPath = findChild<LabelWidget>("modpath");
  m_modDescription = findChild<LabelWidget>("moddescription");

  m_linkButton = fetchChild<ButtonWidget>("linkbutton");
  m_copyLinkButton = fetchChild<ButtonWidget>("copylinkbutton");

  auto linkLabel = fetchChild<LabelWidget>("linklabel");
  auto copyLinkLabel = fetchChild<LabelWidget>("copylinklabel");
  auto workshopLinkButton = fetchChild<ButtonWidget>("workshopbutton");

  auto& guiContext = GuiContext::singleton();
  bool hasDesktopService = (bool)guiContext.applicationController()->desktopService();

  workshopLinkButton->setEnabled(hasDesktopService);

  m_linkButton->setVisibility(hasDesktopService);
  m_copyLinkButton->setVisibility(!hasDesktopService);

  m_linkButton->setEnabled(false);
  m_copyLinkButton->setEnabled(false);

  linkLabel->setVisibility(hasDesktopService);
  copyLinkLabel->setVisibility(!hasDesktopService);
}

void ModsMenu::update(float dt) {
  Pane::update(dt);

  size_t selectedItem = m_modList->selectedItem();
  if (selectedItem == NPos) {
    m_modName->setText("");
    m_modAuthor->setText("");
    m_modVersion->setText("");
    m_modPath->setText("");
    m_modDescription->setText("");

  } else {
    String assetsSource = m_assetsSources.at(selectedItem);
    JsonObject assetsSourceMetadata = Root::singleton().assets()->assetSourceMetadata(assetsSource);

    String modName = bestModName(assetsSourceMetadata, assetsSource);
    m_modName->setText(modName);
  
    auto modAuthor = assetsSourceMetadata.value("author", "<n/a>");
    bool authorIsString = modAuthor.isType(Json::Type::String);
    String modAuthorStr = authorIsString ? modAuthor.toString() : "<n/a>";
    m_modAuthor->setText(modAuthorStr);
    // if (!authorIsString)
    //   Logger::warn("ModsMenu: Specified author name for mod '{}' at '{}' is not a string; discarding invalid author name", modName, assetsSource);
  
    auto modVersion = assetsSourceMetadata.value("version", "<n/a>");
    String modVersionStr = modVersion.isType(Json::Type::String) ? modVersion.toString() : 
      ((modVersion.isType(Json::Type::Int) || modVersion.isType(Json::Type::Float)) ? modVersion.repr() : "<n/a>");
    m_modVersion->setText(modVersionStr);
    // if (!(modVersion.isType(Json::Type::Int) || modVersion.isType(Json::Type::Float) || modVersion.isType(Json::Type::String)))
    //   Logger::warn("ModsMenu: Specified version for mod '{}' at '{}' is not a string or number; discarding invalid version", modName, assetsSource);
  
    m_modPath->setText(assetsSource);
  
    auto modDescription = assetsSourceMetadata.value("description", "<n/a>");
    bool descriptionIsString = modDescription.isType(Json::Type::String);
    String modDescriptionStr = descriptionIsString ? modDescription.toString() : "<n/a>";
    m_modDescription->setText(modDescriptionStr);
    // if (!descriptionIsString)
    //   Logger::warn("ModsMenu: Specified description for mod '{}' at '{}' is not a string; discarding invalid description", modName, assetsSource);

    auto linkJson = assetsSourceMetadata.value("link", "");
    bool linkIsString = linkJson.isType(Json::Type::String);
    String link = linkIsString ? linkJson.toString() : "";
    // if (!linkIsString)
    //   Logger::warn("ModsMenu: Specified link for mod '{}' at '{}' is not a string; discarding invalid link", modName, assetsSource);

    m_linkButton->setEnabled(!link.empty());
    m_copyLinkButton->setEnabled(!link.empty());
  }
}

String ModsMenu::bestModName(JsonObject const& metadata, String const& sourcePath) {
  if (auto ptr = metadata.ptr("friendlyName"))
    return ptr->isType(Json::Type::String) ? ptr->toString() : ptr->repr();
  if (auto ptr = metadata.ptr("name"))
    return ptr->isType(Json::Type::String) ? ptr->toString() : ptr->repr();
  String baseName = File::baseName(sourcePath);
  if (baseName.contains("."))
    baseName.rextract(".");
  return baseName;
}

void ModsMenu::openLink() {
  size_t selectedItem = m_modList->selectedItem();
  if (selectedItem == NPos)
    return;

  String assetsSource = m_assetsSources.at(selectedItem);
  JsonObject assetsSourceMetadata = Root::singleton().assets()->assetSourceMetadata(assetsSource);
  String link = assetsSourceMetadata.value("link", "").toString();

  if (link.empty())
    return;

  auto& guiContext = GuiContext::singleton();
  if (auto desktopService = guiContext.applicationController()->desktopService())
    desktopService->openUrl(link);
  else
    guiContext.setClipboard(link);
}

void ModsMenu::openWorkshop() {
  auto assets = Root::singleton().assets();
  auto& guiContext = GuiContext::singleton();
  if (auto desktopService = guiContext.applicationController()->desktopService())
    desktopService->openUrl(assets->json("/interface/modsmenu/modsmenu.config:workshopLink").toString());
}

}
