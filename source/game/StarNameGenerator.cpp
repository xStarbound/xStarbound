#include "StarNameGenerator.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"

namespace Star {

PatternedNameGenerator::PatternedNameGenerator() {
  auto assets = Root::singleton().assets();
  auto files = assets->scanExtension("namesource");
  assets->queueJsons(files);
  for (auto file : files) {
    try {
      auto sourceConfig = assets->json(file);

      if (m_markovSources.contains(sourceConfig.getString("name")))
        throw NameGeneratorException::format("Duplicate name source '{}', config file '{}'", sourceConfig.getString("name"), file);

      m_markovSources.insert(sourceConfig.getString("name"), makeMarkovSource(sourceConfig.getUInt("prefixSize", 1),
                                                                 sourceConfig.getUInt("endSize", 1), jsonToStringList(sourceConfig.get("sourceNames"))));
    } catch (std::exception const& e) {
      Logger::warn("NameGenerator: Error occurred while reading name source config file '{}', skipping: {}", file, outputException(e, true));
    }
  }

  auto profanityFilter = assets->json("/names/profanityfilter.config").toArray();
  for (auto naughtyWord : profanityFilter)
    m_profanityFilter.add(naughtyWord.toString().toLower());
}

String PatternedNameGenerator::generateName(String const& rulesAsset) const {
  RandomSource random;
  return generateName(rulesAsset, random);
}

String PatternedNameGenerator::generateName(String const& rulesAsset, uint64_t seed) const {
  RandomSource random = RandomSource(seed);
  return generateName(rulesAsset, random);
}

String PatternedNameGenerator::generateName(String const& rulesAsset, RandomSource& random) const {
  auto assets = Root::singleton().assets();
  if (rulesAsset.empty()) {
    Logger::warn("NameGenerator: No name generator rule file specified; skipping random name generation");
    return "";
  }
  if (!assets->assetExists(rulesAsset)) {
    Logger::warn("NameGenerator: Name generator rule file '{}' not found; skipping random name generation", rulesAsset);
    return "";
  }
  auto jRules = assets->json(rulesAsset);
  bool rulesAreArray = jRules.isType(Json::Type::Array);
  if (!rulesAreArray)
    Logger::warn("NameGenerator: Name generator rule file '{}' not a valid rules array; skipping random name generation", rulesAsset);
  auto rules = rulesAreArray ? jRules.toArray() : JsonArray{};
  String res = "";
  int tries = 100;
  while ((res.empty() || isProfane(res)) && tries > 0) {
    --tries;
    res = processRule(rules, random);
  }
  return res;
}

String PatternedNameGenerator::processRule(JsonArray const& rule, RandomSource& random) const {
  if (rule.empty())
    return "";
  Json meta;
  String result;
  String mode = "alts";
  size_t index = 0;
  bool titleCase = false;
  if (rule[0].type() == Json::Type::Object) {
    meta = rule[0];
    auto jMode = meta.get("mode", mode);
    mode = jMode.isType(Json::Type::String) ? jMode.toString() : mode;
    titleCase = meta.get("titleCase", false) == true;
    index++;
  }

  if (mode == "serie" || mode == "series") {
    for (; index < rule.size(); index++) {
      auto entry = rule[index];
      if (entry.type() == Json::Type::Array)
        result += processRule(entry.toArray(), random);
      else if (entry.type() == Json::Type::String)
        result += entry.toString();
      else
        Logger::warn("NameGenerator: Skipping non-string, non-array \"serie\" or \"series\" entry");
    }
  } else if (mode == "alts") {
    int i = index + random.randInt(rule.size() - 1 - index);
    auto entry = rule[i];
    if (entry.type() == Json::Type::Array)
      result += processRule(entry.toArray(), random);
    else if (entry.type() == Json::Type::String)
      result += entry.toString();
    else
      Logger::warn("NameGenerator: Skipping non-string, non-array \"alts\" entry");
  } else if (mode == "markov") {
    if (!m_markovSources.contains(meta.getString("source"))) {
      Logger::warn("Unknown Markov name source '{}', skipping \"markov\" rule", meta.getString("source"));
      return result;
    }

    auto jSourceName = meta.get("source", Json());
    if (!jSourceName.isType(Json::Type::String)) {
      Logger::warn("NameGenerator: Markov \"source\" not a string, ignoring");
      return result;
    }
    auto sourceName = jSourceName.toString();
    if (!m_markovSources.contains(sourceName)) {
      Logger::warn("NameGenerator: Markov source '{}' not found, ignoring", sourceName);
      return result;
    }
    auto source = m_markovSources.get(sourceName);
    auto jLengthRange = meta.get("targetLength", Json());
    if (!jLengthRange.isType(Json::Type::Array)) {
      Logger::warn("NameGenerator: Config for Markov source '{}' missing length range, ignoring", sourceName);
      return result;
    }
    auto lengthRange = jLengthRange.toArray();
    if (lengthRange.size() != 2) {
      Logger::warn("NameGenerator: Length range array for Markov source '{}' not two items in length, ignoring", sourceName);
      return result;
    }
    if (!(lengthRange[0].isType(Json::Type::Int) && lengthRange[1].isType(Json::Type::Int))) {
      Logger::warn("NameGenerator: Item(s) in length range array for Markov source '{}' not integer(s), ignoring", sourceName);
      return result;
    }
    int64_t shortest = lengthRange[0].toInt(), longest = lengthRange[1].toInt();
    if (shortest < 0 || longest < 0) {
      Logger::warn("NameGenerator: Negative integer(s) found in length range array, ignoring", sourceName);
      return result;
    }
    if (shortest > longest) {
      Logger::warn("NameGenerator: Lower bound is larger than upper bound, ignoring", sourceName);
      return result;
    }
    auto targetLength = random.randUInt(lengthRange[0].toUInt(), lengthRange[1].toUInt());

    size_t tries = 0;
    String piece;
    do {
      ++tries;
      piece = random.randFrom(source.starts);
      while (piece.length() < targetLength || !source.ends.contains(piece.slice(piece.length() - source.endSize, piece.length()))) {
        String link = piece.slice(piece.length() - source.prefixSize, piece.length());
        if (!source.chains.contains(link))
          break;
        piece += random.randFrom(source.chains.get(link));
      }
    } while (piece.length() > lengthRange[1].toUInt() && tries < 10);

    result += piece;
  } else {
    Logger::info("NameGenerator: Unknown mode '{}' specified; skipping rule", mode);
  }

  if (titleCase)
    result = result.titleCase();

  return result;
}

bool PatternedNameGenerator::isProfane(String const& name) const {
  auto matchNames = name.toLower().rot13().splitAny(" -");
  for (auto& naughtyWord : m_profanityFilter) {
    for (auto& matchName : matchNames) {
      auto find = matchName.find(naughtyWord);
      if (find != NPos && (find == 0 || naughtyWord.size() + 1 >= matchName.size()))
        return true;
    }
  }
  return false;
}

MarkovSource PatternedNameGenerator::makeMarkovSource(size_t prefixSize, size_t endSize, StringList sourceNames) {
  MarkovSource newSource;
  newSource.prefixSize = prefixSize;
  newSource.endSize = endSize;
  for (auto sourceName : sourceNames) {
    if (sourceName.length() < prefixSize || sourceName.length() < endSize)
      continue;

    sourceName = sourceName.toLower();
    newSource.ends.add(sourceName.slice(sourceName.length() - endSize, sourceName.length()));
    for (int i = 0; i + prefixSize <= sourceName.length(); ++i) {
      auto prefix = sourceName.slice(i, i + prefixSize);
      if (i == 0)
        newSource.starts.append(prefix);

      if (i + prefixSize < sourceName.length()) {
        if (!newSource.chains.contains(prefix))
          newSource.chains.insert(prefix, StringList());

        newSource.chains.get(prefix).append(sourceName.slice(i + prefixSize, i + prefixSize + 1));
      }
    }
  }
  return newSource;
}

} // namespace Star
