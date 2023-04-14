#ifndef SHIT_MSVC13
#define SHIT_MSVC13

#include <functional>
#include <cstdint>
#include <vector>
#include <fstream>

namespace Shit {
	static uint8_t const kSig{2};
	static uint8_t const kLSig{4};
	using Func = std::function<bool(uint8_t**,uint8_t**)>;

	template<uint8_t SZ>
		std::vector<uint8_t> Read(std::ifstream& src){
			size_t sz{};
			src.read((char*)&sz, SZ);
			std::vector<uint8_t> ret(sz);
			src.read((char*)ret.data(), ret.size());
			return ret;
		}

	template<uint8_t SZ>
		static void Write(std::ofstream& dst, uint8_t const* pbeg, size_t const sz) {
			dst.write((char*)&sz, SZ);
			dst.write((char*)pbeg, sz);
		}
	template<uint8_t SZ>
		static auto Write(std::ofstream& dst) -> Func {
			return [&dst] (uint8_t** ppbeg, uint8_t** ppend) {
				uint8_t const* pbeg{*ppbeg};
				uint8_t const* pend{*ppend};
				size_t const sz{(size_t)(pend - pbeg)};
				Write<SZ>(dst, pbeg, sz);
				return true;
			};
		}

	static auto Shr(size_t const offset) -> Func {
			return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
				(*ppbeg) += offset;
				return true;
			};
		}
	static auto Shl(size_t const offset) -> Func {
			return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
				(*ppbeg) -= offset;
				return true;
			};
		}

	static auto Crop(size_t const skip_beg, size_t const skip_end = 0) -> Func {
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
	template<class T, class Save>
		static auto Filter(T const mask, T const val, Save save) -> Func {
			return [mask, val, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				if (cur == val) {
					save(cur);
					ret = true;
				} else {
					ret = false;
				}
				return ret;
			};
		}
	template<class T, class Save>
		static auto Filter(T const mask, std::vector<T> const& vec, Save save) -> Func {
			return [mask, vec, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				for (auto const& val : vec) {
					ret |= Filter<T>(mask, val, save);
				}
				return ret;
			};
		}

	static auto Empty() -> std::function<bool(uint8_t**,uint8_t**)> {
		return [] (uint8_t**,uint8_t**) {
			return true;
		};
	}
	template<class T, class Save>
		static auto Mask(T const mask, Save save) -> Func {
			return [mask, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				if (cur) {
					save(0xFF);
					ret = true;
				} else {
					save(0x00);
					ret = false;
				}
				return ret;
			};
		}
	static auto CleanOffset(uint8_t& offset) -> Func {
		return [&offset] (...) {
			offset = 0;
			return true;
		};
	}
	namespace Check {
		static auto CheckOffset(uint8_t& offset) -> Func {
			return [&offset] (...) {
				return offset <= sizeof(uint64_t);
			};
		}
		static bool OutOfRange(uint8_t** ppbeg, uint8_t** ppend) {
			uint8_t* pbeg{*ppbeg};
			uint8_t* pend{*ppend};
			return pbeg < pend;
		}
		static auto OutOfRange() -> Func {
			return [] (uint8_t** ppbeg, uint8_t** ppend) {
				return OutOfRange(ppbeg, ppend);
			};
		}
		static auto OutOfRange(uint8_t* pbeg) -> Func {
			return [pbeg] (uint8_t** ppbeg, uint8_t** ppend) {
				return pbeg < *ppbeg;
			};
		}
	}
}

#endif
