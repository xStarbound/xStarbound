#include "StarSwingableItem.hpp"

namespace Star {

SwingableItem::SwingableItem() {
  m_swingAimFactor = 0;
  m_swingStart = 0;
  m_swingFinish = 0;
  m_swingIdleAngle = -25.0f * Constants::deg2rad;
}

SwingableItem::SwingableItem(Json const& params) : FireableItem(params) {
  setParams(params);
}

void SwingableItem::setParams(Json const& params) {
  if (!params.isType(Json::Type::Object)) return;
  m_swingStart = params.getFloat("swingStart", 60) * Constants::pi / 180;
  m_swingFinish = params.getFloat("swingFinish", -40) * Constants::pi / 180;
  m_swingAimFactor = params.getFloat("swingAimFactor", 1);
  auto coolingDownAngle = params.get("coolingDownAngle", Json());
  if (coolingDownAngle.isType(Json::Type::Float))
    m_coolingDownAngle = coolingDownAngle.toFloat() * Constants::deg2rad;
  else if (coolingDownAngle.isType(Json::Type::Int))
    m_coolingDownAngle = ((float)coolingDownAngle.toInt()) * Constants::deg2rad;
  else
    m_coolingDownAngle = {};
  auto idleAngle = params.get("idleAngle", Json());
  if (idleAngle.isType(Json::Type::Float))
    m_swingIdleAngle = idleAngle.toFloat() * Constants::deg2rad;
  else if (idleAngle.isType(Json::Type::Int))
    m_swingIdleAngle = ((float)idleAngle.toInt()) * Constants::deg2rad;
  FireableItem::setParams(params);
}

float SwingableItem::getAngleDir(float angle, Direction) {
  return getAngle(angle);
}

float SwingableItem::getAngle(float aimAngle) {
  if (!ready()) {
    if (coolingDown()) {
      if (m_coolingDownAngle)
        return *m_coolingDownAngle + aimAngle * m_swingAimFactor;
      else
        return m_swingIdleAngle; // -Constants::pi / 2;
    }

    if (m_timeFiring < windupTime())
      return m_swingStart + (m_swingFinish - m_swingStart) * m_timeFiring / windupTime() + aimAngle * m_swingAimFactor;

    return m_swingFinish + (m_swingStart - m_swingFinish) * fireTimer() / (cooldownTime() + windupTime()) + aimAngle * m_swingAimFactor;
  }

  return m_swingIdleAngle; // -Constants::pi / 2;
}

float SwingableItem::getItemAngle(float aimAngle) {
  return getAngle(aimAngle);
}

String SwingableItem::getArmFrame() {
  return "rotation";
}

} // namespace Star
