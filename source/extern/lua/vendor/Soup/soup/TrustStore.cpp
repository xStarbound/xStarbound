#include "TrustStore.hpp"

#include "deflate.hpp"
#include "macros.hpp"
#include "MemoryRefReader.hpp"
#include "pem.hpp"

NAMESPACE_SOUP
{
#include "cacerts.hpp" // Generated by codegen/cacert.php based on the 2024-12-31 revision from https://curl.se/docs/caextract.html.

	const TrustStore& TrustStore::fromMozilla() SOUP_EXCAL
	{
		static TrustStore inst = fromMozillaImpl();
		return inst;
	}

	TrustStore TrustStore::fromMozillaImpl() SOUP_EXCAL
	{
		auto str = deflate::decompress(cacerts, sizeof(cacerts)).decompressed;
		MemoryRefReader sr(str);

		uint16_t num_ca_certs;
		sr.u16_le(num_ca_certs);

		TrustStore ts;
		ts.data.reserve(num_ca_certs);
		for (uint16_t i = 0; i != num_ca_certs; ++i)
		{
			uint16_t cert_len;
			sr.u16_le(cert_len);

			X509Certificate cert;
			cert.fromDer(str.data() + sr.getPosition(), cert_len);
			sr.skip(cert_len);

			ts.addCa(std::move(cert));
		}
		return ts;
	}

	void TrustStore::loadCaCerts(std::istream& is) SOUP_EXCAL
	{
		std::string ca_common_name{};
		std::string ca_pem{};
		for (std::string line{}; std::getline(is, line); )
		{
			if (line.empty()
				|| line.at(0) == '#'
				)
			{
				if (!ca_common_name.empty())
				{
					addCa(std::move(ca_common_name), std::move(ca_pem));
					ca_common_name.clear();
					ca_pem.clear();
				}
			}
			else if (line.at(0) != '=')
			{
				if (ca_common_name.empty())
				{
					ca_common_name = std::move(line);
				}
				else
				{
					ca_pem.append(std::move(line));
				}
				line.clear();
			}
		}
		if (!ca_common_name.empty())
		{
			addCa(std::move(ca_common_name), std::move(ca_pem));
		}
	}

	void TrustStore::addCa(X509Certificate&& cert) SOUP_EXCAL
	{
		addCa(cert.subject.getCommonName(), std::move(cert));
	}

	void TrustStore::addCa(std::string&& common_name, std::string&& pem) SOUP_EXCAL
	{
		X509Certificate xcert;
		if (xcert.fromDer(pem::decode(std::move(pem))))
		{
			addCa(std::move(common_name), std::move(xcert));
		}
	}

	void TrustStore::addCa(std::string&& common_name, X509Certificate&& cert) SOUP_EXCAL
	{
		if (!common_name.empty())
		{
			data.emplace(std::move(common_name), std::move(cert));
		}
	}

	bool TrustStore::contains(const X509Certificate& cert) const SOUP_EXCAL
	{
		if (auto entry = findCommonName(cert.subject.getCommonName()))
		{
			if (entry->isEc() == cert.isEc()
				&& entry->key.x == cert.key.x
				&& entry->key.y == cert.key.y
				)
			{
				return true;
			}
		}
		return false;
	}

	const X509Certificate* TrustStore::findCommonName(const std::string& cn) const noexcept
	{
		auto i = data.find(cn);
		if (i == data.end())
		{
			return nullptr;
		}
		return &i->second;
	}
}
