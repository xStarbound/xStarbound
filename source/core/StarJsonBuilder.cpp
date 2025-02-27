#include "StarJsonBuilder.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

void JsonBuilderStream::beginObject() {
  pushSentry();
}

void JsonBuilderStream::objectKey(char32_t const* s, size_t len) {
  push(Json(s, len));
}

void JsonBuilderStream::endObject() {
  JsonObject object;

  // bmdhacks: Count how many key/value pairs until we hit the sentry
  size_t count = 0;
  // We know each pair is stored as a Key entry followed by a Value entry
  // in LIFO order (the Value is on top, then the Key).
  // So each pair = 2 consecutive entries in the stack.
  for (size_t i = m_stack.size(); i >= 1; i--) { // FezzedOne: Don't use ssize_t because Windows.
    if (m_stack[i - 1].isNothing()) // sentry
      break; // found boundary
    count++;
  }
  // 'count' is the number of entries after the sentry.
  // The number of key/value pairs = count/2
  // JsonObject is the primary user of FlatHashMap so reserving with a known
  // size improves memory fragmentation
  object.reserve(count / 2);

  while (true) {
    if (isSentry()) {
      set(Json(std::move(object)));
      return;
    } else {
      Json v = pop();
      String k = pop().toString();
      if (!object.insert(k, std::move(v)).second)
        throw JsonParsingException(strf("Json object contains a duplicate entry for key '{}'", k));
    }
  }
}

void JsonBuilderStream::beginArray() {
  pushSentry();
}

void JsonBuilderStream::endArray() {
  JsonArray array;

  // bmdhacks: Count how many Value entries until we hit the sentry
  size_t count = 0;
  for (size_t i = m_stack.size(); i >= 1; i--) {
    if (m_stack[i - 1].isNothing()) // sentry
      break;
    count++;
  }
  array.reserve(count); // pre-allocate the array

  while (true) {
    if (isSentry()) {
      array.reverse();
      set(Json(std::move(array)));
      return;
    } else {
      array.append(pop());
    }
  }
}

void JsonBuilderStream::putString(char32_t const* s, size_t len) {
  push(Json(s, len));
}

void JsonBuilderStream::putDouble(char32_t const* s, size_t len) {
  push(Json(lexicalCast<double>(String(s, len))));
}

void JsonBuilderStream::putInteger(char32_t const* s, size_t len) {
  push(Json(lexicalCast<long long>(String(s, len))));
}

void JsonBuilderStream::putBoolean(bool b) {
  push(Json(b));
}

void JsonBuilderStream::putNull() {
  push(Json());
}

void JsonBuilderStream::putWhitespace(char32_t const*, size_t) {}

void JsonBuilderStream::putColon() {}

void JsonBuilderStream::putComma() {}

size_t JsonBuilderStream::stackSize() {
  return m_stack.size();
}

Json JsonBuilderStream::takeTop() {
  if (m_stack.size())
    return m_stack.takeLast().take();
  else
    return Json();
}

void JsonBuilderStream::push(Json v) {
  m_stack.append(std::move(v));
}

Json JsonBuilderStream::pop() {
  return m_stack.takeLast().take();
}

void JsonBuilderStream::set(Json v) {
  m_stack.last() = std::move(v);
}

void JsonBuilderStream::pushSentry() {
  m_stack.append({});
}

bool JsonBuilderStream::isSentry() {
  return !m_stack.empty() && !m_stack.last();
}

void JsonStreamer<Json>::toJsonStream(Json const& val, JsonStream& stream, bool sort) {
  Json::Type type = val.type();
  if (type == Json::Type::Null) {
    stream.putNull();
  } else if (type == Json::Type::Float) {
    auto d = String(toString(val.toDouble())).wideString();
    stream.putDouble(d.c_str(), d.length());
  } else if (type == Json::Type::Bool) {
    stream.putBoolean(val.toBool());
  } else if (type == Json::Type::Int) {
    auto i = String(toString(val.toInt())).wideString();
    stream.putInteger(i.c_str(), i.length());
  } else if (type == Json::Type::String) {
    auto ws = val.toString().wideString();
    stream.putString(ws.c_str(), ws.length());
  } else if (type == Json::Type::Array) {
    stream.beginArray();
    bool first = true;
    for (auto const& elem : val.iterateArray()) {
      if (!first)
        stream.putComma();
      first = false;
      toJsonStream(elem, stream, sort);
    }
    stream.endArray();
  } else if (type == Json::Type::Object) {
    stream.beginObject();
    if (sort) {
      auto objectPtr = val.objectPtr();
      List<JsonObject::const_iterator> iterators;
      iterators.reserve(objectPtr->size());
      for (auto i = objectPtr->begin(); i != objectPtr->end(); ++i)
        iterators.append(i);
      iterators.sort([](JsonObject::const_iterator a, JsonObject::const_iterator b) {
          return a->first < b->first;
        });
      bool first = true;
      for (auto const& i : iterators) {
        if (!first)
          stream.putComma();
        first = false;
        auto ws = i->first.wideString();
        stream.objectKey(ws.c_str(), ws.length());
        stream.putColon();
        toJsonStream(i->second, stream, sort);
      }
    } else {
      bool first = true;
      for (auto const& pair : val.iterateObject()) {
        if (!first)
          stream.putComma();
        first = false;
        auto ws = pair.first.wideString();
        stream.objectKey(ws.c_str(), ws.length());
        stream.putColon();
        toJsonStream(pair.second, stream, sort);
      }
    }
    stream.endObject();
  }
}

}
