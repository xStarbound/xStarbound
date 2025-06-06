#include "SocketTlsEncrypter.hpp"

#include "aes.hpp"
#include "rand.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "TlsMac.hpp"

NAMESPACE_SOUP
{
	std::string SocketTlsEncrypter::calculateMacBytes(TlsContentType_t content_type, size_t content_length) SOUP_EXCAL
	{
		TlsMac mac{};
		mac.seq_num = seq_num++;
		mac.record.content_type = content_type;
		mac.record.length = static_cast<uint16_t>(content_length);
		return mac.toBinaryString();
	}

	std::string SocketTlsEncrypter::calculateMac(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL
	{
		auto msg = calculateMacBytes(content_type, size);
		if (mac_key_len == 20)
		{
			sha1::HmacState st(mac_key, mac_key_len);
			st.append(msg.data(), msg.size());
			st.append(data, size);
			st.finalise();
			return st.getDigest();
		}
		//else if (mac_key_len == 32)
		{
			sha256::HmacState st(mac_key, mac_key_len);
			st.append(msg.data(), msg.size());
			st.append(data, size);
			st.finalise();
			return st.getDigest();
		}
	}

	Buffer SocketTlsEncrypter::encrypt(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL
	{
		constexpr auto cipher_bytes = 16;

		if (!isAead()) // AES-CBC
		{
			constexpr auto record_iv_length = 16;

			auto mac = calculateMac(content_type, data, size);
			auto cont_with_mac_size = (size + mac.size());
			auto aligned_in_len = ((((cont_with_mac_size + 1) / cipher_bytes) + 1) * cipher_bytes);
			auto pad_len = static_cast<char>(aligned_in_len - cont_with_mac_size);

			Buffer buf(size + mac.size() + pad_len);
			buf.append(data, size);
			buf.append(mac.data(), mac.size());
			buf.insert_back((size_t)pad_len, (pad_len - 1));

			auto iv = rand.vec_u8(record_iv_length);
			aes::cbcEncrypt(
				buf.data(), buf.size(),
				cipher_key, cipher_key_len,
				iv.data()
			);

			buf.prepend(iv.data(), iv.size());
			return buf;
		}
		else // AES-GCM
		{
			constexpr auto implicit_iv_len = 4;
			constexpr auto explicit_iv_len = 8;

			uint8_t iv[implicit_iv_len + explicit_iv_len];
			memcpy(iv, implicit_iv, implicit_iv_len);
			for (auto i = implicit_iv_len; i != implicit_iv_len + explicit_iv_len; ++i)
			{
				iv[i] = rand.byte();
			}

			auto ad = calculateMacBytes(content_type, size);

			Buffer buf(size + cipher_bytes + explicit_iv_len);
			buf.append(&iv[implicit_iv_len], explicit_iv_len);
			buf.append(data, size);

			uint8_t tag[cipher_bytes];
			aes::gcmEncrypt(
				buf.data() + explicit_iv_len, buf.size() - explicit_iv_len,
				(const uint8_t*)ad.data(), ad.size(),
				cipher_key, cipher_key_len,
				iv, sizeof(iv),
				tag
			);

			buf.append(tag, cipher_bytes);
			return buf;
		}
	}

	void SocketTlsEncrypter::reset() noexcept
	{
		seq_num = 0;
		cipher_key_len = 0;
		mac_key_len = 0;
	}
}
