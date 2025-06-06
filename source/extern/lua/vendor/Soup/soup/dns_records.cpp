#include "dns_records.hpp"

#include "dnsName.hpp"
#include "Exception.hpp"
#include "string.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	dnsRecordFactory dnsRecord::getFactory(dnsType type)
	{
		switch (type)
		{
		case DNS_A:
			return [](std::string&& name, uint32_t ttl, std::string&& str) -> UniquePtr<dnsRecord>
			{
				IpAddr addr;
				addr.fromString(str);
				return soup::make_unique<dnsARecord>(std::move(name), ttl, addr.getV4());
			};

		case DNS_AAAA:
			return [](std::string&& name, uint32_t ttl, std::string&& str) -> UniquePtr<dnsRecord>
			{
				IpAddr addr;
				addr.fromString(str);
				return soup::make_unique<dnsAaaaRecord>(std::move(name), ttl, addr);
			};

		case DNS_CNAME:
			return [](std::string&& name, uint32_t ttl, std::string&& str) -> UniquePtr<dnsRecord>
			{
				return soup::make_unique<dnsCnameRecord>(std::move(name), ttl, std::move(str));
			};

		case DNS_PTR:
			return [](std::string&& name, uint32_t ttl, std::string&& str) -> UniquePtr<dnsRecord>
			{
				return soup::make_unique<dnsPtrRecord>(std::move(name), ttl, std::move(str));
			};

		case DNS_TXT:
			return [](std::string&& name, uint32_t ttl, std::string&& str) -> UniquePtr<dnsRecord>
			{
				return soup::make_unique<dnsTxtRecord>(std::move(name), ttl, std::move(str));
			};

		case DNS_NS:
			return [](std::string&& name, uint32_t ttl, std::string&& str) -> UniquePtr<dnsRecord>
			{
				return soup::make_unique<dnsNsRecord>(std::move(name), ttl, std::move(str));
			};

		default:;
		}
		return nullptr;
	}

	UniquePtr<dnsRecord> dnsRecord::fromString(dnsType type, std::string&& name, uint32_t ttl, std::string&& str)
	{
		auto f = getFactory(type);
		if (f)
		{
			return f(std::move(name), ttl, std::move(str));
		}
		return {};
	}

	UniquePtr<dnsRecord> dnsRecord::copy() const
	{
		return fromString(type, std::string(name), ttl, toString());
	}

	std::string dnsRecordName::toRdata() const
	{
		dnsName value;
		value.name = string::explode(data, '.');

		StringWriter sw;
		value.write(sw);
		SOUP_MOVE_RETURN(sw.data);
	}

	std::string dnsMxRecord::toString() const
	{
		std::string str = std::to_string(priority);
		str.push_back(' ');
		str.append(target);
		return str;
	}

	std::string dnsMxRecord::toRdata() const
	{
		StringWriter sw;
		{
			uint16_t priority = this->priority;
			sw.u16_be(priority);
		}
		{
			dnsName target;
			target.name = string::explode(this->target, '.');
			target.write(sw);
		}
		SOUP_MOVE_RETURN(sw.data);
	}

	std::string dnsSrvRecord::toString() const
	{
		std::string str = std::to_string(priority);
		str.push_back(' ');
		str.append(std::to_string(weight));
		str.push_back(' ');
		str.append(std::to_string(port));
		str.push_back(' ');
		str.append(target);
		return str;
	}

	std::string dnsSrvRecord::toRdata() const
	{
		Exception::purecall();
	}
}
