#include "JsonObject.hpp"

#include "Exception.hpp"
#include "json.hpp"
#include "JsonBool.hpp"
#include "JsonFloat.hpp"
#include "JsonInt.hpp"
#include "JsonString.hpp"
#include "string.hpp"
#include "Writer.hpp"

NAMESPACE_SOUP
{
	using Container = JsonObject::Container;

	void JsonObject::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.push_back('{');
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			i->first->encodeAndAppendTo(str);
			str.push_back(':');
			i->second->encodeAndAppendTo(str);
			if (i != children.end() - 1)
			{
				str.push_back(',');
			}
		}
		str.push_back('}');
	}

	void JsonObject::encodePrettyAndAppendTo(std::string& str, unsigned depth) const SOUP_EXCAL
	{
		if (children.empty())
		{
			str.append("{}");
		}
		else
		{
			const auto child_depth = (depth + 1);
			str.append("{\n");
			for (auto i = children.begin(); i != children.end(); ++i)
			{
				str.append(child_depth * 4, ' ');
				i->first->encodeAndAppendTo(str);
				str.append(": ");
				i->second->encodePrettyAndAppendTo(str, child_depth);
				if (i != children.end() - 1)
				{
					str.push_back(',');
				}
				str.push_back('\n');
			}
			str.append(depth * 4, ' ');
			str.push_back('}');
		}
	}

	bool JsonObject::msgpackEncode(Writer& w) const
	{
		if (children.size() <= 0b1111)
		{
			uint8_t b = 0b1000'0000 | (uint8_t)children.size();
			SOUP_RETHROW_FALSE(w.u8(b));
		}
		else if (children.size() <= 0xffff)
		{
			uint8_t b = 0xde;
			SOUP_RETHROW_FALSE(w.u8(b));
			auto len = (uint16_t)children.size();
			SOUP_RETHROW_FALSE(w.u16_be(len));
		}
		else if (children.size() <= 0xffff'ffff)
		{
			uint8_t b = 0xdf;
			SOUP_RETHROW_FALSE(w.u8(b));
			auto len = (uint32_t)children.size();
			SOUP_RETHROW_FALSE(w.u32_be(len));
		}
		else
		{
			SOUP_ASSERT_UNREACHABLE;
		}

		for (const auto& child : children)
		{
			SOUP_RETHROW_FALSE(child.first->msgpackEncode(w));
			SOUP_RETHROW_FALSE(child.second->msgpackEncode(w));
		}

		return true;
	}

	JsonNode* JsonObject::find(std::string k) const noexcept
	{
		return find(JsonString(std::move(k)));
	}

	JsonNode* JsonObject::find(const JsonNode& k) const noexcept
	{
		for (const auto& child : children)
		{
			if (*child.first == k)
			{
				return child.second.get();
			}
		}
		return nullptr;
	}

	UniquePtr<JsonNode>* JsonObject::findUp(std::string k) noexcept
	{
		return findUp(JsonString(std::move(k)));
	}

	UniquePtr<JsonNode>* JsonObject::findUp(const JsonNode& k) noexcept
	{
		for (auto& child : children)
		{
			if (*child.first == k)
			{
				return &child.second;
			}
		}
		return nullptr;
	}

	Container::iterator JsonObject::findIt(std::string k) noexcept
	{
		return findIt(JsonString(std::move(k)));
	}

	Container::iterator JsonObject::findIt(const JsonNode& k) noexcept
	{
		auto it = children.begin();
		for (; it != children.end(); ++it)
		{
			if (*it->first == k)
			{
				break;
			}
		}
		return it;
	}

	bool JsonObject::contains(const JsonNode& k) const noexcept
	{
		return find(k) != nullptr;
	}

	bool JsonObject::contains(std::string k) const noexcept
	{
		return contains(JsonString(std::move(k)));
	}

	JsonNode& JsonObject::at(const JsonNode& k) const
	{
		if (auto e = find(k))
		{
			return *e;
		}
		std::string err = "JsonObject has no member with key ";
		err.append(k.encode());
		SOUP_THROW(Exception(std::move(err)));
	}

	JsonNode& JsonObject::at(std::string k) const
	{
		return at(JsonString(std::move(k)));
	}

	void JsonObject::erase(const JsonNode& k) noexcept
	{
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			if (*i->first == k)
			{
				children.erase(i);
				break;
			}
		}
	}

	void JsonObject::erase(std::string k) noexcept
	{
		return erase(JsonString(std::move(k)));
	}

	void JsonObject::erase(Container::const_iterator it) noexcept
	{
		children.erase(it);
	}

	void JsonObject::clear() noexcept
	{
		children.clear();
	}

	void JsonObject::add(UniquePtr<JsonNode>&& k, UniquePtr<JsonNode>&& v)
	{
		children.emplace_back(std::move(k), std::move(v));
	}

	void JsonObject::add(std::string k, std::string v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonString>(std::move(v)));
	}

	void JsonObject::add(std::string k, const char* v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonString>(v));
	}

	void JsonObject::add(std::string k, int8_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, int16_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, int32_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, uint32_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, int64_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, bool v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonBool>(v));
	}

	void JsonObject::add(std::string k, double v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonFloat>(v));
	}
}
