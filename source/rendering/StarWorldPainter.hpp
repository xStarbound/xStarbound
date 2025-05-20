#ifndef STAR_WORLD_PAINTER_HPP
#define STAR_WORLD_PAINTER_HPP

#include "StarDirectives.hpp"
#include "StarDrawablePainter.hpp"
#include "StarEnvironmentPainter.hpp"
#include "StarRenderer.hpp"
#include "StarTextPainter.hpp"
#include "StarTilePainter.hpp"
#include "StarWorldRenderData.hpp"

namespace Star {

STAR_CLASS(WorldPainter);

// Will update client rendering window internally
class WorldPainter {
public:
  struct EnabledLayers {
    bool stars = true;
    bool debrisFields = true;
    bool backOrbiters = true;
    bool planetHorizon = true;
    bool sky = true;
    bool frontOrbiters = true;
    bool parallax = true;
    bool entities = true;
    bool backgroundOverlays = true;
    bool backgroundTiles = true;
    bool midgroundTiles = true;
    bool particles = true;
    bool liquids = true;
    bool foregroundTiles = true;
    bool overlays = true;
    bool nametags = true;
    bool bars = true;
  };

  WorldPainter();

  void renderInit(RendererPtr renderer);

  void setEnabledRenderLayers(Json const& renderLayerChanges);
  JsonObject enabledRenderLayers() const;
  void fullbrightOverride(bool override);
  bool isFullbright() const;
  void setTileRenderDirectives(Json const& newDirectives);
  JsonObject tileRenderDirectives() const;

  void setCameraPosition(WorldGeometry const& worldGeometry, Vec2F const& position);

  WorldCamera& camera();

  void update(float dt);
  void render(WorldRenderData& renderData, function<void()> lightWaiter, Maybe<Vec3F> const& lightMultiplier = {},
      Array<Vec3F, 6> const& shaderParameters = Array<Vec3F, 6>::filled(Vec3F::filled(0.0f)));
  void adjustLighting(WorldRenderData& renderData, Maybe<Vec3F> const& lightMultiplier = {});

private:
  void renderParticles(WorldRenderData& renderData, Particle::Layer layer);
  void renderBars(WorldRenderData& renderData);

  void drawEntityLayer(List<Drawable> drawables, EntityHighlightEffect highlightEffect = EntityHighlightEffect());

  void drawDrawable(Drawable drawable);
  void drawDrawableSet(List<Drawable>& drawable);

  WorldCamera m_camera;

  RendererPtr m_renderer;

  EnabledLayers m_enabledLayers;

  bool m_fullbrightOverride;

  TilePainter::TilePainterDirectives m_tileRenderDirectives;

  TextPainterPtr m_textPainter;
  DrawablePainterPtr m_drawablePainter;
  EnvironmentPainterPtr m_environmentPainter;
  TilePainterPtr m_tilePainter;

  Json m_highlightConfig;
  Map<EntityHighlightEffectType, pair<Directives, Directives>> m_highlightDirectives;

  Vec2F m_entityBarOffset;
  Vec2F m_entityBarSpacing;
  Vec2F m_entityBarSize;
  Vec2F m_entityBarIconOffset;

  // Updated every frame

  AssetsConstPtr m_assets;
  RectF m_worldScreenRect;

  Vec2F m_previousCameraCenter;
  Vec2F m_parallaxWorldPosition;

  float m_preloadTextureChance;
};

} // namespace Star

#endif
