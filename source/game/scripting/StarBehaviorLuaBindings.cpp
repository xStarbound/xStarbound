#include "StarBehaviorLuaBindings.hpp"
#include "StarEntity.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarRoot.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeBehaviorLuaCallbacks(List<BehaviorStatePtr>* list, Entity* entityPtr) {
  LuaCallbacks callbacks;

  auto entity = GameObjectRegistry::smuggleWrap(entityPtr);

  callbacks.registerCallback("behavior", [list, entity](Json const& config, JsonObject const& parameters, LuaTable context, Maybe<LuaUserData> blackboard) -> BehaviorStateWeakPtr {
    // FezzedOne: Need to check the existence of the entity that is implicitly referenced in the passed behaviour state list pointer.
    entity.checkSmuggle();

    auto behaviorDatabase = Root::singleton().behaviorDatabase();
    Maybe<BlackboardWeakPtr> board = {};
    if (blackboard && blackboard->is<BlackboardWeakPtr>())
      board = blackboard->get<BlackboardWeakPtr>();

    BehaviorTreeConstPtr tree;
    if (config.isType(Json::Type::String)) {
      if (parameters.empty()) {
        tree = behaviorDatabase->behaviorTree(config.toString());
      } else {
        JsonObject treeConfig = behaviorDatabase->behaviorConfig(config.toString()).toObject();
        treeConfig.set("parameters", jsonMerge(treeConfig.get("parameters"), parameters));
        tree = behaviorDatabase->buildTree(treeConfig);
      }
    } else {
      tree = behaviorDatabase->buildTree(config.set("parameters", jsonMerge(config.getObject("parameters", {}), parameters)));
    }

    BehaviorStatePtr state = make_shared<BehaviorState>(tree, context, board);
    list->append(state);
    return weak_ptr<BehaviorState>(state);
  });

  return callbacks;
}

} // namespace Star
