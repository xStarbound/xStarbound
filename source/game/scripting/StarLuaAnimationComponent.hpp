#ifndef STAR_LUA_ANIMATION_COMPONENT_HPP
#define STAR_LUA_ANIMATION_COMPONENT_HPP

#include "StarAssets.hpp"
#include "StarDrawable.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarLightSource.hpp"
#include "StarLuaComponents.hpp"
#include "StarLuaConverters.hpp"
#include "StarMixer.hpp"
#include "StarParticle.hpp"
#include "StarParticleDatabase.hpp"
#include "StarRoot.hpp"

namespace Star {

STAR_EXCEPTION(LuaAnimationComponentException, LuaComponentException);

// Lua component that allows lua to directly produce drawables, light sources,
// audios, and particles.  Adds a "localAnimation" callback table.
template <typename Base>
class LuaAnimationComponent : public Base {
public:
  LuaAnimationComponent();

  List<pair<Drawable, Maybe<EntityRenderLayer>>> const& drawables();
  List<LightSource> const& lightSources();

  List<Particle> pullNewParticles();
  List<AudioInstancePtr> pullNewAudios();

  // FezzedOne: Needs to have its lifetime specified before initialising any Lua bindings.
  template <typename Parent>
  void initAnimationBindings(Parent* lifetimePtr) {
    LuaCallbacks animationCallbacks;

    auto parent = GameObjectRegistry::smuggleWrap(lifetimePtr);

    animationCallbacks.registerCallback("playAudio", [this, parent](String const& sound, Maybe<int> loops, Maybe<float> volume) {
      parent.checkSmuggle();
      auto audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(sound));
      audio->setLoops(loops.value(0));
      audio->setVolume(volume.value(1.0f));
      m_pendingAudios.append(audio);
      m_activeAudio.append(audio);
    });
    animationCallbacks.registerCallback("spawnParticle", [this, parent](Json const& particleConfig, Maybe<Vec2F> const& position) {
      parent.checkSmuggle();
      auto particle = Root::singleton().particleDatabase()->particle(particleConfig);
      particle.translate(position.value());
      m_pendingParticles.append(particle);
    });
    animationCallbacks.registerCallback("clearDrawables", [this, parent]() {
      parent.checkSmuggle();
      m_drawables.clear();
    });
    animationCallbacks.registerCallback("addDrawable", [this, parent](Drawable drawable, Maybe<String> renderLayerName, Maybe<bool> dontScale) {
      parent.checkSmuggle();
      Maybe<EntityRenderLayer> renderLayer;
      if (renderLayerName)
        renderLayer = parseRenderLayer(*renderLayerName);

      // FezzedOne: Scaling can fuck up embedded transformations, so add the option to turn it off.
      bool shouldScale = true;
      if (dontScale)
        shouldScale = !(*dontScale);
      if (shouldScale) {
        if (auto image = drawable.part.ptr<Drawable::ImagePart>())
          image->transformation.scale(0.125f);
      }

      m_drawables.append({std::move(drawable), renderLayer});
    });
    animationCallbacks.registerCallback("clearLightSources", [this, parent]() {
      parent.checkSmuggle();
      m_lightSources.clear();
    });
    animationCallbacks.registerCallback("addLightSource", [this, parent](LuaTable const& lightSourceTable) {
      parent.checkSmuggle();
      m_lightSources.append({lightSourceTable.get<Vec2F>("position"),
          lightSourceTable.get<Color>("color").toRgb(),
          lightSourceTable.get<Maybe<bool>>("pointLight").value(),
          lightSourceTable.get<Maybe<float>>("pointBeam").value(),
          lightSourceTable.get<Maybe<float>>("beamAngle").value(),
          lightSourceTable.get<Maybe<float>>("beamAmbience").value()});
    });
    Base::addCallbacks("localAnimator", std::move(animationCallbacks));
  }

protected:
  // Clears looping audio on context shutdown
  void contextShutdown() override;

private:
  List<Particle> m_pendingParticles;
  List<AudioInstancePtr> m_pendingAudios;
  List<AudioInstancePtr> m_activeAudio;

  List<pair<Drawable, Maybe<EntityRenderLayer>>> m_drawables;
  List<LightSource> m_lightSources;
};

template <typename Base>
LuaAnimationComponent<Base>::LuaAnimationComponent() {
}

template <typename Base>
List<pair<Drawable, Maybe<EntityRenderLayer>>> const& LuaAnimationComponent<Base>::drawables() {
  return m_drawables;
}

template <typename Base>
List<LightSource> const& LuaAnimationComponent<Base>::lightSources() {
  return m_lightSources;
}

template <typename Base>
List<Particle> LuaAnimationComponent<Base>::pullNewParticles() {
  return take(m_pendingParticles);
}

template <typename Base>
List<AudioInstancePtr> LuaAnimationComponent<Base>::pullNewAudios() {
  eraseWhere(m_activeAudio, [](AudioInstancePtr const& audio) {
    return audio->finished();
  });
  return take(m_pendingAudios);
}

template <typename Base>
void LuaAnimationComponent<Base>::contextShutdown() {
  for (auto const& audio : m_activeAudio)
    audio->setLoops(0);
  m_activeAudio.clear();
  Base::contextShutdown();
}

} // namespace Star

#endif
