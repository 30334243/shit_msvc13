#ifndef SHIT_MSVC13
#define SHIT_MSVC13

#include <functional>
#include <cstdint>
#include <vector>
#include <fstream>

namespace Shit {
	static int const kSig{2};
	static int const kLSig{4};

	template<uint8_t SZ>
		std::vector<uint8_t> Read(std::ifstream& src){
			size_t sz{};
			src.read((char*)&sz, SZ);
			std::vector<uint8_t> ret(sz);
			src.read((char*)ret.data(), ret.size());
			return ret;
		}

	template<uint8_t SZ>
		void Write(std::ofstream& dst, uint8_t const* pbeg, size_t const sz) {
			dst.write((char*)&sz, SZ);
			dst.write((char*)pbeg, sz);
		}

	static auto Crop(size_t const skip_beg, size_t const skip_end = 0) ->
		std::function<bool(uint8_t**, uint8_t**)> {
			return [skip_beg, skip_end] (uint8_t** ppbeg, uint8_t** ppend) {
				uint8_t* pbeg{*ppbeg};
				pbeg += skip_beg;
				*ppbeg = pbeg;
				uint8_t* pend{*ppend};
				pend -= skip_end;
				*ppend = pend;
				return true;
			};
		}
	static void SaveToLid(uint8_t const mask,
								 uint64_t& lid,
								 uint8_t const offset) {
		*(((uint8_t*)&lid)+offset) = mask;
	}
	template<class T>
		static auto SaveToLid(uint64_t& lid, uint8_t& offset) ->
		std::function<void(T const)> {
			return [&lid, &offset] (T const mask) {
				SaveToLid(mask, lid, offset);
				offset += sizeof(T);
			};
		}
	template<class Save>
		static auto Mask(std::vector<uint8_t> const& masks,
							  uint8_t const val,
							  Save save) ->
		std::function<bool(uint8_t**,uint8_t**)> {
			return [masks, val, save] (uint8_t** ppbeg, uint8_t** ppend) {
				uint8_t const cur{*(*ppbeg)};
				for (auto const& mask : masks) {
					if ((mask & cur) == val) {
						save(mask);
					}
				}
				return true;
			};
		}
	static auto CleanOffset(uint8_t& offset) ->
		std::function<bool(uint8_t**,uint8_t**)> {
			return [&offset] (...) {
				offset = 0;
				return true;
			};
		}
	static bool OutOfRange(uint8_t** ppbeg, uint8_t** ppend) {
		uint8_t* pbeg{*ppbeg};
		uint8_t* pend{*ppend};
		return pbeg < pend;
	}
}

#endif
