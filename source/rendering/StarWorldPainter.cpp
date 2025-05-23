#include "StarWorldPainter.hpp"
#include "StarAnimation.hpp"
#include "StarAssets.hpp"
#include "StarConfiguration.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"

namespace Star {

WorldPainter::WorldPainter() {
  m_assets = Root::singleton().assets();

  m_camera.setScreenSize({800, 600});
  m_camera.setCenterWorldPosition(Vec2F());
  m_camera.setPixelRatio(clamp(Root::singleton().configuration()->get("zoomLevel").toFloat(), 0.25f, 100.0f));

  m_enabledLayers = WorldPainter::EnabledLayers();
  m_fullbrightOverride = false;
  m_tileRenderDirectives = TilePainter::TilePainterDirectives();
  m_overlayRenderDirectives = {Directives(), Directives(), Directives()};

  m_highlightConfig = m_assets->json("/highlights.config");
  for (auto p : m_highlightConfig.get("highlightDirectives").iterateObject())
    m_highlightDirectives.set(EntityHighlightEffectTypeNames.getLeft(p.first), {p.second.getString("underlay", ""), p.second.getString("overlay", "")});

  m_entityBarOffset = jsonToVec2F(m_assets->json("/rendering.config:entityBarOffset"));
  m_entityBarSpacing = jsonToVec2F(m_assets->json("/rendering.config:entityBarSpacing"));
  m_entityBarSize = jsonToVec2F(m_assets->json("/rendering.config:entityBarSize"));
  m_entityBarIconOffset = jsonToVec2F(m_assets->json("/rendering.config:entityBarIconOffset"));
  m_preloadTextureChance = m_assets->json("/rendering.config:preloadTextureChance").toFloat();
}

void WorldPainter::renderInit(RendererPtr renderer) {
  m_assets = Root::singleton().assets();

  m_enabledLayers = WorldPainter::EnabledLayers();
  m_fullbrightOverride = false;
  m_tileRenderDirectives = TilePainter::TilePainterDirectives();
  m_overlayRenderDirectives = {Directives(), Directives(), Directives()};

  m_renderer = std::move(renderer);
  auto textureGroup = m_renderer->createTextureGroup(TextureGroupSize::Large);
  m_textPainter = make_shared<TextPainter>(m_renderer, textureGroup);
  m_tilePainter = make_shared<TilePainter>(m_renderer);
  m_drawablePainter = make_shared<DrawablePainter>(m_renderer, make_shared<AssetTextureGroup>(textureGroup));
  m_environmentPainter = make_shared<EnvironmentPainter>(m_renderer);
}

void WorldPainter::setCameraPosition(WorldGeometry const& geometry, Vec2F const& position) {
  m_camera.setWorldGeometry(geometry);
  m_camera.setCenterWorldPosition(position);
}

WorldCamera& WorldPainter::camera() {
  return m_camera;
}

void WorldPainter::update(float dt) {
  m_environmentPainter->update(dt);
}

void WorldPainter::setEnabledRenderLayers(Json const& renderLayerChanges) {
  auto& el = m_enabledLayers;
  if (renderLayerChanges.isType(Json::Type::Object)) {
    el.stars = renderLayerChanges.optBool("stars").value(el.stars);
    el.debrisFields = renderLayerChanges.optBool("debrisFields").value(el.debrisFields);
    el.backOrbiters = renderLayerChanges.optBool("backOrbiters").value(el.backOrbiters);
    el.planetHorizon = renderLayerChanges.optBool("planetHorizon").value(el.planetHorizon);
    el.sky = renderLayerChanges.optBool("sky").value(el.sky);
    el.frontOrbiters = renderLayerChanges.optBool("frontOrbiters").value(el.frontOrbiters);
    el.parallax = renderLayerChanges.optBool("parallax").value(el.parallax);
    el.entities = renderLayerChanges.optBool("entities").value(el.entities);
    el.backgroundOverlays = renderLayerChanges.optBool("backgroundOverlays").value(el.backgroundOverlays);
    el.backgroundTiles = renderLayerChanges.optBool("backgroundTiles").value(el.backgroundTiles);
    el.midgroundTiles = renderLayerChanges.optBool("midgroundTiles").value(el.midgroundTiles);
    el.particles = renderLayerChanges.optBool("particles").value(el.particles);
    el.liquids = renderLayerChanges.optBool("liquids").value(el.liquids);
    el.foregroundTiles = renderLayerChanges.optBool("foregroundTiles").value(el.foregroundTiles);
    el.overlays = renderLayerChanges.optBool("overlays").value(el.overlays);
    el.nametags = renderLayerChanges.optBool("nametags").value(el.nametags);
    el.bars = renderLayerChanges.optBool("bars").value(el.bars);
  } else if (renderLayerChanges.isNull()) {
    el = EnabledLayers(); // FezzedOne: Should set everything back to defaults.
  }
}

JsonObject WorldPainter::enabledRenderLayers() const {
  return JsonObject{
      {"stars", m_enabledLayers.stars},
      {"debrisFields", m_enabledLayers.debrisFields},
      {"backOrbiters", m_enabledLayers.backOrbiters},
      {"planetHorizon", m_enabledLayers.planetHorizon},
      {"sky", m_enabledLayers.sky},
      {"frontOrbiters", m_enabledLayers.frontOrbiters},
      {"parallax", m_enabledLayers.parallax},
      {"entities", m_enabledLayers.entities},
      {"backgroundOverlays", m_enabledLayers.backgroundOverlays},
      {"backgroundTiles", m_enabledLayers.backgroundTiles},
      {"midgroundTiles", m_enabledLayers.midgroundTiles},
      {"particles", m_enabledLayers.particles},
      {"liquids", m_enabledLayers.liquids},
      {"foregroundTiles", m_enabledLayers.foregroundTiles},
      {"overlays", m_enabledLayers.overlays},
      {"nametags", m_enabledLayers.nametags},
      {"bars", m_enabledLayers.bars},
  };
}

void WorldPainter::fullbrightOverride(bool override) {
  m_fullbrightOverride = override;
}

bool WorldPainter::isFullbright() const {
  return m_fullbrightOverride;
}

void WorldPainter::setTileRenderDirectives(Json const& newDirectives) {
  if (newDirectives.isType(Json::Type::Object)) {
    bool shouldFlush = false;
    String liquids = newDirectives.optString("liquids").value(m_tileRenderDirectives.liquids);
    shouldFlush |= liquids != m_tileRenderDirectives.liquids;
    m_tileRenderDirectives.liquids = std::move(liquids);
    String backgroundOverlays = newDirectives.optString("backgroundOverlays").value(m_overlayRenderDirectives.backgroundOverlays.string());
    m_overlayRenderDirectives.backgroundOverlays = std::move(Directives(backgroundOverlays));
    String background = newDirectives.optString("background").value(m_tileRenderDirectives.terrainLayers.background);
    shouldFlush |= background != m_tileRenderDirectives.terrainLayers.background;
    m_tileRenderDirectives.terrainLayers.background = std::move(background);
    String midground = newDirectives.optString("midground").value(m_tileRenderDirectives.terrainLayers.midground);
    shouldFlush |= midground != m_tileRenderDirectives.terrainLayers.midground;
    m_tileRenderDirectives.terrainLayers.midground = std::move(midground);
    String foreground = newDirectives.optString("foreground").value(m_tileRenderDirectives.terrainLayers.foreground);
    shouldFlush |= foreground != m_tileRenderDirectives.terrainLayers.foreground;
    m_tileRenderDirectives.terrainLayers.foreground = std::move(foreground);
    String foregroundOverlays = newDirectives.optString("foregroundOverlays").value(m_overlayRenderDirectives.foregroundOverlays.string());
    m_overlayRenderDirectives.foregroundOverlays = std::move(Directives(foregroundOverlays));
    String bars = newDirectives.optString("bars").value(m_overlayRenderDirectives.bars.string());
    m_overlayRenderDirectives.bars = std::move(Directives(bars));
    String particles = newDirectives.optString("particles").value(m_overlayRenderDirectives.particles.string());
    m_overlayRenderDirectives.particles = std::move(Directives(particles));
    if (m_tilePainter && shouldFlush)
      m_tilePainter->flushCaches(m_tileRenderDirectives.liquids);
  } else if (newDirectives.isNull()) {
    m_tileRenderDirectives = TilePainter::TilePainterDirectives();
    if (m_tilePainter) m_tilePainter->flushCaches(m_tileRenderDirectives.liquids);
  }
}

JsonObject WorldPainter::tileRenderDirectives() const {
  return JsonObject{
      {"liquids", m_tileRenderDirectives.liquids},
      {"backgroundOverlays", m_overlayRenderDirectives.backgroundOverlays.string()},
      {"background", m_tileRenderDirectives.terrainLayers.background},
      {"midground", m_tileRenderDirectives.terrainLayers.midground},
      {"foreground", m_tileRenderDirectives.terrainLayers.foreground},
      {"foregroundOverlays", m_overlayRenderDirectives.foregroundOverlays.string()},
      {"bars", m_overlayRenderDirectives.bars.string()},
      {"particles", m_overlayRenderDirectives.particles.string()},
  };
}

void WorldPainter::render(WorldRenderData& renderData, function<void()> lightWaiter, Maybe<Vec3F> const& lightMultiplier, Array<Vec3F, 6> const& shaderParameters) {
  auto& el = m_enabledLayers;

  m_camera.setScreenSize(m_renderer->screenSize());
  m_camera.setTargetPixelRatio(clamp(Root::singleton().configuration()->get("zoomLevel").toFloat(), 0.25f, 100.0f));

  m_assets = Root::singleton().assets();

  m_tilePainter->setup(m_camera, renderData, m_tileRenderDirectives);

  // Stars, Debris Fields, Sky, and Orbiters

  // Use a fixed pixel ratio for certain things.
  float pixelRatioBasis = m_camera.screenSize()[1] / 1080.0f;
  float starAndDebrisRatio = lerp(0.0625f, pixelRatioBasis * 2.0f, m_camera.pixelRatio());
  float orbiterAndPlanetRatio = lerp(0.125f, pixelRatioBasis * 3.0f, m_camera.pixelRatio());

  if (el.stars)
    m_environmentPainter->renderStars(starAndDebrisRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  if (el.debrisFields)
    m_environmentPainter->renderDebrisFields(starAndDebrisRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  if (el.backOrbiters)
    m_environmentPainter->renderBackOrbiters(orbiterAndPlanetRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  if (el.planetHorizon)
    m_environmentPainter->renderPlanetHorizon(orbiterAndPlanetRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  if (el.sky)
    m_environmentPainter->renderSky(Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  if (el.frontOrbiters)
    m_environmentPainter->renderFrontOrbiters(orbiterAndPlanetRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);

  m_renderer->flush();

  if (lightWaiter) {
    auto start = Time::monotonicMicroseconds();
    lightWaiter();
    LogMap::set("async_light_wait", strf(u8"{:05d}\u00b5s", Time::monotonicMicroseconds() - start));
  }

  {
    m_renderer->setEffectParameter("param1", shaderParameters[0]);
    m_renderer->setEffectParameter("param2", shaderParameters[1]);
    m_renderer->setEffectParameter("param3", shaderParameters[2]);
    m_renderer->setEffectParameter("param4", shaderParameters[3]);
    m_renderer->setEffectParameter("param5", shaderParameters[4]);
    m_renderer->setEffectParameter("param6", shaderParameters[5]);
  }

  if (renderData.isFullbright || m_fullbrightOverride) {
    m_renderer->setEffectTexture("lightMap", Image::filled(Vec2U(1, 1), {255, 255, 255, 255}, PixelFormat::RGB24));
    m_renderer->setEffectTexture("tileLightMap", Image::filled(Vec2U(1, 1), {0, 0, 0, 0}, PixelFormat::RGBA32));
    m_renderer->setEffectParameter("lightMapMultiplier", 1.0f);
  } else {
    adjustLighting(renderData, lightMultiplier);
    m_renderer->setEffectParameter("lightMapMultiplier", m_assets->json("/rendering.config:lightMapMultiplier").toFloat());
    m_renderer->setEffectParameter("lightMapScale", Vec2F::filled(TilePixels * m_camera.pixelRatio()));
    m_renderer->setEffectParameter("lightMapOffset", m_camera.worldToScreen(Vec2F(renderData.lightMinPosition)));
    m_renderer->setEffectTexture("lightMap", renderData.lightMap);
    m_renderer->setEffectTexture("tileLightMap", renderData.tileLightMap);
  }

  // Parallax layers

  if (el.parallax) {
    auto parallaxDelta = m_camera.worldGeometry().diff(m_camera.centerWorldPosition(), m_previousCameraCenter);
    if (parallaxDelta.magnitude() > 10)
      m_parallaxWorldPosition = m_camera.centerWorldPosition();
    else
      m_parallaxWorldPosition += parallaxDelta;
    m_parallaxWorldPosition[1] = m_camera.centerWorldPosition()[1];
  }
  m_previousCameraCenter = m_camera.centerWorldPosition();

  if (el.parallax && !renderData.parallaxLayers.empty())
    m_environmentPainter->renderParallaxLayers(m_parallaxWorldPosition, m_camera, renderData.parallaxLayers, renderData.skyRenderData);

  // Main world layers

  Map<EntityRenderLayer, List<pair<EntityHighlightEffect, List<Drawable>>>> entityDrawables;
  for (auto& ed : renderData.entityDrawables) {
    for (auto& p : ed.layers)
      entityDrawables[p.first].append({ed.highlightEffect, std::move(p.second)});
  }

  auto entityDrawableIterator = entityDrawables.begin();
  auto renderEntitiesUntil = [this, &entityDrawables, &entityDrawableIterator](Maybe<EntityRenderLayer> until) {
    while (true) {
      if (!m_enabledLayers.entities)
        break;
      if (entityDrawableIterator == entityDrawables.end())
        break;
      if (until && entityDrawableIterator->first >= *until)
        break;
      for (auto& edl : entityDrawableIterator->second)
        drawEntityLayer(std::move(edl.second), edl.first);
      ++entityDrawableIterator;
    }

    m_renderer->flush();
  };

  renderEntitiesUntil(RenderLayerBackgroundOverlay);
  if (el.backgroundOverlays) drawDrawableSet(renderData.backgroundOverlays, m_overlayRenderDirectives.backgroundOverlays);
  renderEntitiesUntil(RenderLayerBackgroundTile);
  if (el.backgroundTiles) m_tilePainter->renderBackground(m_camera);
  renderEntitiesUntil(RenderLayerPlatform);
  if (el.midgroundTiles) m_tilePainter->renderMidground(m_camera);
  renderEntitiesUntil(RenderLayerBackParticle);
  if (el.particles) renderParticles(renderData, Particle::Layer::Back, m_overlayRenderDirectives.particles);
  renderEntitiesUntil(RenderLayerLiquid);
  if (el.liquids) m_tilePainter->renderLiquid(m_camera);
  renderEntitiesUntil(RenderLayerMiddleParticle);
  if (el.particles) renderParticles(renderData, Particle::Layer::Middle, m_overlayRenderDirectives.particles);
  renderEntitiesUntil(RenderLayerForegroundTile);
  if (el.foregroundTiles) m_tilePainter->renderForeground(m_camera);
  renderEntitiesUntil(RenderLayerForegroundOverlay);
  if (el.overlays) drawDrawableSet(renderData.foregroundOverlays, m_overlayRenderDirectives.foregroundOverlays);
  renderEntitiesUntil(RenderLayerFrontParticle);
  if (el.particles) renderParticles(renderData, Particle::Layer::Front, m_overlayRenderDirectives.particles);
  renderEntitiesUntil(RenderLayerOverlay);
  if (el.nametags) drawDrawableSet(renderData.nametags, m_overlayRenderDirectives.nametags);
  if (el.bars) renderBars(renderData, m_overlayRenderDirectives.bars);
  renderEntitiesUntil({});

  auto dimLevel = round(renderData.dimLevel * 255);
  if (dimLevel != 0)
    m_renderer->render(renderFlatRect(RectF::withSize({}, Vec2F(m_camera.screenSize())), Vec4B(renderData.dimColor, dimLevel), 0.0f));

  int64_t textureTimeout = m_assets->json("/rendering.config:textureTimeout").toInt();
  m_textPainter->cleanup(textureTimeout);
  m_drawablePainter->cleanup(textureTimeout);
  m_environmentPainter->cleanup(textureTimeout);
  m_tilePainter->cleanup();
}

void WorldPainter::adjustLighting(WorldRenderData& renderData, Maybe<Vec3F> const& lightMultiplier) {
  m_tilePainter->adjustLighting(renderData, lightMultiplier);
}

void WorldPainter::renderParticles(WorldRenderData& renderData, Particle::Layer layer, Directives const& renderDirectives) {
  const int textParticleFontSize = m_assets->json("/rendering.config:textParticleFontSize").toInt();
  const RectF particleRenderWindow = RectF::withSize(Vec2F(), Vec2F(m_camera.screenSize())).padded(m_assets->json("/rendering.config:particleRenderWindowPadding").toInt());

  if (!renderData.particles)
    return;

  for (Particle const& particle : *renderData.particles) {
    if (layer != particle.layer)
      continue;

    Vec2F position = m_camera.worldToScreen(particle.position);

    if (!particleRenderWindow.contains(position))
      continue;

    Vec2F size = Vec2F::filled(particle.size * m_camera.pixelRatio());

    if (particle.type == Particle::Type::Ember) {
      m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(),
          RectF(position - size / 2, position + size / 2),
          particle.color.toRgba(),
          particle.fullbright ? 0.0f : 1.0f);

    } else if (particle.type == Particle::Type::Streak) {
      // Draw a rotated quad streaking in the direction the particle is coming from.
      // Sadly this looks awful.
      Vec2F dir = particle.velocity.normalized();
      Vec2F sideHalf = dir.rot90() * m_camera.pixelRatio() * particle.size / 2;
      float length = particle.length * m_camera.pixelRatio();
      Vec4B color = particle.color.toRgba();
      float lightMapMultiplier = particle.fullbright ? 0.0f : 1.0f;
      m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(),
          position - sideHalf,
          position + sideHalf,
          position - dir * length + sideHalf,
          position - dir * length - sideHalf,
          color, lightMapMultiplier);

    } else if (particle.type == Particle::Type::Textured || particle.type == Particle::Type::Animated) {
      Drawable drawable;
      if (particle.type == Particle::Type::Textured)
        drawable = Drawable::makeImage(particle.image, 1.0f / TilePixels, true, Vec2F(0, 0));
      else
        drawable = particle.animation->drawable(1.0f / TilePixels);

      if (particle.flip && particle.flippable)
        drawable.scale(Vec2F(-1, 1));
      if (drawable.isImage()) {
        drawable.imagePart().addDirectivesGroup(particle.directives, true);
        drawable.imagePart().addDirectives(renderDirectives, true);
      }
      drawable.fullbright = particle.fullbright;
      drawable.color = particle.color;
      drawable.rotate(particle.rotation);
      drawable.scale(particle.size);
      drawable.translate(particle.position);
      drawDrawable(std::move(drawable));

    } else if (particle.type == Particle::Type::Text) {
      Vec2F position = m_camera.worldToScreen(particle.position);
      int size = min(128.0f, round((float)textParticleFontSize * m_camera.pixelRatio() * particle.size));
      if (size > 0) {
        m_textPainter->setFontSize(size);
        m_textPainter->setFontColor(particle.color.toRgba());
        m_textPainter->setProcessingDirectives("");
        m_textPainter->setFont("");
        m_textPainter->renderText(particle.string, {position, HorizontalAnchor::HMidAnchor, VerticalAnchor::VMidAnchor});
      }
    }
  }

  m_renderer->flush();
}

void WorldPainter::renderBars(WorldRenderData& renderData, Directives const& renderDirectives) {
  auto offset = m_entityBarOffset;
  for (auto const& bar : renderData.overheadBars) {
    auto position = bar.entityPosition + offset;
    offset += m_entityBarSpacing;
    if (bar.icon) {
      auto iconDrawPosition = position - (m_entityBarSize / 2).round() + m_entityBarIconOffset;
      auto drawable = Drawable::makeImage(*bar.icon, 1.0f / TilePixels, true, iconDrawPosition);
      if (!renderDirectives.empty())
        drawable.imagePart().addDirectives(renderDirectives, true);
      drawDrawable(std::move(drawable));
    }

    if (!bar.detailOnly) {
      auto fullBar = RectF({}, {m_entityBarSize.x() * bar.percentage, m_entityBarSize.y()});
      auto emptyBar = RectF({m_entityBarSize.x() * bar.percentage, 0.0f}, m_entityBarSize);
      auto fullColor = bar.color;
      auto emptyColor = Color::Black;

      drawDrawable(Drawable::makePoly(PolyF(emptyBar), emptyColor, position));
      drawDrawable(Drawable::makePoly(PolyF(fullBar), fullColor, position));
    }
  }

  m_renderer->flush();
}

void WorldPainter::drawEntityLayer(List<Drawable> drawables, EntityHighlightEffect highlightEffect) {
  highlightEffect.level *= m_highlightConfig.getFloat("maxHighlightLevel", 1.0);
  if (highlightEffect.overrideOverlayDirectives || highlightEffect.overrideUnderlayDirectives) {
    auto underlayDirectives = highlightEffect.overrideUnderlayDirectives ? *(highlightEffect.overrideUnderlayDirectives) : Directives();
    if (!underlayDirectives.empty()) {
      for (auto& d : drawables) {
        if (d.isImage()) {
          auto underlayDrawable = Drawable(d);
          underlayDrawable.imagePart().addDirectives(underlayDirectives, true);
          drawDrawable(std::move(underlayDrawable));
        }
      }
    }

    // second pass, draw main drawables and overlays
    auto overlayDirectives = highlightEffect.overrideOverlayDirectives ? *(highlightEffect.overrideOverlayDirectives) : Directives();
    for (auto& d : drawables) {
      drawDrawable(d);
      if (!overlayDirectives.empty() && d.isImage()) {
        auto overlayDrawable = Drawable(d);
        overlayDrawable.imagePart().addDirectives(overlayDirectives, true);
        drawDrawable(std::move(overlayDrawable));
      }
    }
  } else if (m_highlightDirectives.contains(highlightEffect.type) && highlightEffect.level > 0) {
    // first pass, draw underlay
    auto underlayDirectives = m_highlightDirectives[highlightEffect.type].first;
    if (!underlayDirectives.empty()) {
      for (auto& d : drawables) {
        if (d.isImage()) {
          auto underlayDrawable = Drawable(d);
          underlayDrawable.fullbright = true;
          underlayDrawable.color = Color::rgbaf(1, 1, 1, highlightEffect.level);
          underlayDrawable.imagePart().addDirectives(underlayDirectives, true);
          drawDrawable(std::move(underlayDrawable));
        }
      }
    }

    // second pass, draw main drawables and overlays
    auto overlayDirectives = m_highlightDirectives[highlightEffect.type].second;
    for (auto& d : drawables) {
      drawDrawable(d);
      if (!overlayDirectives.empty() && d.isImage()) {
        auto overlayDrawable = Drawable(d);
        overlayDrawable.fullbright = true;
        overlayDrawable.color = Color::rgbaf(1, 1, 1, highlightEffect.level);
        overlayDrawable.imagePart().addDirectives(overlayDirectives, true);
        drawDrawable(std::move(overlayDrawable));
      }
    }
  } else {
    for (auto& d : drawables)
      drawDrawable(std::move(d));
  }
}

void WorldPainter::drawDrawable(Drawable drawable, Directives const& renderDirectives) {
  drawable.position = m_camera.worldToScreen(drawable.position);
  drawable.scale(m_camera.pixelRatio() * TilePixels, drawable.position);

  if (drawable.isLine())
    drawable.linePart().width *= m_camera.pixelRatio();
  else if (drawable.isImage() && !renderDirectives.empty())
    drawable.imagePart().addDirectives(renderDirectives, true);

  // draw the drawable if it's on screen
  // if it's not on screen, there's a random chance to pre-load
  // pre-load is not done on every tick because it's expensive to look up images with long paths
  if (RectF::withSize(Vec2F(), Vec2F(m_camera.screenSize())).intersects(drawable.boundBox(false)))
    m_drawablePainter->drawDrawable(drawable);
  else if (drawable.isImage() && Random::randf() < m_preloadTextureChance)
    m_assets->tryImage(drawable.imagePart().image);
}

void WorldPainter::drawDrawableSet(List<Drawable>& drawables, Directives const& renderDirectives) {
  for (Drawable& drawable : drawables)
    drawDrawable(std::move(drawable), renderDirectives);

  m_renderer->flush();
}

} // namespace Star
