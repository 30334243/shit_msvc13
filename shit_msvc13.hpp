#ifndef SHIT_MSVC13
#define SHIT_MSVC13

#include <functional>
#include <cstdint>
#include <vector>
#include <fstream>
#include <memory>
#include <cmath>

// SHIT
namespace Shit {
	uint8_t* gpbeg = nullptr;
	uint8_t* gpend = nullptr;
	// CONSTANTS
	static uint8_t const kSig{2};
	static uint8_t const kLSig{4};
	static std::vector<uint8_t> bits{
		0,
			0b00000001,
			0b00000011,
			0b00000111,
			0b00001111,
			0b00011111,
			0b00111111,
			0b01111111
	};
	static const std::vector<uint8_t> table_fill{
			0,
			1,		3,		7,
			0xF,	0x1F,	0x3F, 0x7F,
			0xFF
		};
	// USING
	using Func = std::function<bool(uint8_t**,uint8_t**)>;
	// READ
	template<uint8_t SZ>
		std::vector<uint8_t> Read(std::ifstream& src){
			size_t sz{};
			src.read((char*)&sz, SZ);
			std::vector<uint8_t> ret(sz);
			src.read((char*)ret.data(), ret.size());
			return ret;
		}
	// WRITE
	template<uint8_t SZ>
		static void Write(std::ofstream& dst, uint8_t const* pbeg, size_t const sz) {
			dst.write((char*)&sz, SZ);
			dst.write((char*)pbeg, sz);
		}
	// WRITE
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
	// SHIFT RIGHT
	static auto Shr(size_t const offset) -> Func {
		return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
			(*ppbeg) += offset;
			return true;
		};
	}
	// SHIFT LEFT
	static auto Shl(size_t const offset) -> Func {
		return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
			(*ppbeg) -= offset;
			return true;
		};
	}
	// ROR
	uint8_t Ror(uint8_t const offset, uint8_t const val){
		uint8_t ret{};
		if (offset < 8) {
			uint8_t const hi{(uint8_t)(8 - offset)};
			ret = (val >> offset) | (val << hi);
		}
		return ret;
	}
	// ROL
	uint8_t Rol(uint8_t const offset, uint8_t const val){
		uint8_t ret{};
		if (offset < 8) {
			uint8_t const hi{(uint8_t)(8 - offset)};
			ret = (val << offset) | (val >> hi);
		}
		return ret;
	}
	// SHIFT RIGHT IN BITS
	static auto ShrInBits(size_t const offset) -> Func {
		return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
			*ppbeg = (*ppbeg) + (offset < 8 ? 0 : (offset / 8));
			uint8_t* tmp_pbeg{*ppbeg};
			size_t const offset_in_bits{offset%8};
			uint8_t old{};
			while (tmp_pbeg < *ppend) {
				uint8_t tmp{(uint8_t)(old | ((*tmp_pbeg) >> offset_in_bits))};
				uint8_t last{(uint8_t)((*tmp_pbeg) & bits[offset_in_bits])};
				old = Ror(offset_in_bits, last);
				*tmp_pbeg = tmp;
				++tmp_pbeg;
			}
			return true;
		};
	}
	// SHIFT LEFT IN BITS
	static auto ShlInBits(size_t const offset) -> Func {
		return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
			*ppbeg = (*ppbeg) - (offset < 8 ? 0 : (offset / 8));
			uint8_t* tmp_pend{*ppend-1};
			size_t const offset_in_bits{offset%8};
			uint8_t old{};
			while (*ppbeg <= tmp_pend) {
				uint8_t tmp{(uint8_t)(old | ((*tmp_pend) << offset_in_bits))};
				old = Rol(offset_in_bits, *tmp_pend);
				old &= bits[offset_in_bits];
				*tmp_pend = tmp;
				--tmp_pend;
			}
			return true;
		};
	}
	// CROP
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
	// SAVE TO LID
	static void SaveToLid(uint8_t const mask,
								 uint64_t& lid,
								 uint8_t const offset) {
		*(((uint8_t*)&lid)+offset) = mask;
	}
	// SAVE TO LID
	template<class T>
		static auto SaveToLid(uint64_t& lid, uint8_t& offset) ->
		std::function<void(T const)> {
			return [&lid, &offset] (T const mask) {
				SaveToLid(mask, lid, offset);
				offset += sizeof(T);
			};
		}
	// FILTER
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
	// FILTER
	template<class T, class Save>
		static auto Filter(T const mask,
								 std::vector<uint64_t> const& vec,
								 Save save) -> Func {
			return [mask, vec, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				for (auto& val : vec) {
					ret |= Filter<T>(mask, (T)val, save)(ppbeg, ppend);
				}
				return ret;
			};
		}
	// FILTER NOT
	template<class T, class Save>
		static auto FilterNot(T const mask, T const val, Save save) -> Func {
			return [mask, val, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				if (cur != val) {
					save(cur);
					ret = true;
				} else {
					ret = false;
				}
				return ret;
			};
		}
	// FILTER NOT
	template<class T, class Save>
		static auto FilterNot(T const mask,
									 std::vector<uint64_t> const& vec,
									 Save save) -> Func {
			return [mask, vec, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{true};
				for (auto& val : vec) {
					ret &= FilterNot<T>(mask, (T)val, save)(ppbeg, ppend);
				}
				return ret;
			};
		}
	// EQUAL
	template<class T, class Save>
		static auto Eq(T const val, Save save) -> Func {
			return [val, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg))};
				if (cur == val) {
					save(cur);
					ret = true;
				} else {
					ret = false;
				}
				return ret;
			};
		}
	// EQUAL
	template<class T, class Save>
		static auto Eq(std::vector<uint64_t> const& vec,
							Save save) -> Func {
			return [vec, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				for (auto& val : vec) {
					ret |= Eq<T>((T)val, save)(ppbeg, ppend);
				}
				return ret;
			};
		}
	// EQUAL NOT
	template<class T, class Save>
		static auto EqNot(T const val, Save save) -> Func {
			return [val, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg))};
				if (cur != val) {
					save(cur);
					ret = true;
				} else {
					ret = false;
				}
				return ret;
			};
		}
	// EQUAL NOT
	template<class T, class Save>
		static auto EqNot(std::vector<uint64_t> const& vec,
								Save save) -> Func {
			return [vec, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{true};
				for (auto& val : vec) {
					ret &= EqNot<T>((T)val, save)(ppbeg, ppend);
				}
				return ret;
			};
		}
	// EMPTY
	static auto Empty() -> std::function<bool(uint8_t**,uint8_t**)> {
		return [] (uint8_t**,uint8_t**) {
			return true;
		};
	}
	// MASK
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
	// MASK NOT
	template<class T, class Save>
		static auto MaskNot(T const mask, Save save) -> Func {
			return [mask, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				if (!cur) {
					save(0xFF);
					ret = true;
				} else {
					save(0x00);
					ret = false;
				}
				return ret;
			};
		}
	// CLEAN OFFSET
	static auto CleanOffset(uint8_t& offset) -> Func {
		return [&offset] (...) {
			offset = 0;
			return true;
		};
	}
	// CHECK OFFSET
	namespace Check {
		static auto CheckOffset(uint8_t& offset) -> Func {
			return [&offset] (...) {
				return offset <= sizeof(uint64_t);
			};
		}
		// OUT OF RANGE
		static bool OutOfRange(uint8_t** ppbeg, uint8_t** ppend) {
			uint8_t* pbeg{*ppbeg};
			uint8_t* pend{*ppend};
			return pbeg < pend;
		}
		// OUT OF RANGE
		static auto OutOfRange() -> Func {
			return [] (uint8_t** ppbeg, uint8_t** ppend) {
				return OutOfRange(ppbeg, ppend);
			};
		}
		// OUT OF RANGE
		static auto OutOfRange(size_t const beg, size_t const end) -> Func {
			return [beg, end] (uint8_t**, uint8_t**) {
				return (gpbeg + beg) < (gpend - end);
			};
		}
		// OUT OR RANGE RIGHT IN BITS
		static auto OutOfRangeRightInBits(size_t const offset) -> Func {
			return [offset] (uint8_t** ppbeg, uint8_t**) {
				return ((*ppbeg) + (size_t)(std::ceil(offset/8))) < gpend;
			};
		}
		// OUT OR RANGE RIGHT
		static auto OutOfRangeRight(size_t const offset) -> Func {
			return [offset] (uint8_t** ppbeg, uint8_t**) {
				return ((*ppbeg) + offset) < gpend;
			};
		}
		// OUT OR RANGE LEFT
		static auto OutOfRangeLeft(size_t const offset) -> Func {
			return [offset] (uint8_t** ppbeg, uint8_t**) {
				return gpbeg <= ((*ppbeg) - offset);
			};
		}
	}
}

#endif
