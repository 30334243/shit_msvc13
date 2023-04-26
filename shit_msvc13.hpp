#ifndef SHIT_MSVC13
#define SHIT_MSVC13

#include <functional>
#include <cstdint>
#include <vector>
#include <fstream>
#include <memory>
#include <cmath>
#include <iostream>
#include <string>

// SHIT
namespace Shit {
	uint8_t* gpbeg = nullptr;
	uint8_t* gpend = nullptr;
	uint8_t gbeg{};
	uint8_t gend{};
	// INIT
	void Init(uint8_t* pbeg, uint8_t* pend) {
		gpbeg = pbeg;
		gpend = pend;
		gbeg = 0;
		gend = 0;
	}
	// CONSTANTS
	static uint8_t const kSig{2};
	static uint8_t const kLSig{4};
	static const std::vector<uint8_t> bits{
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
	// TEST
	static auto Test(std::vector<uint8_t>& vec) -> Func {
		return [&vec] (uint8_t** ppbeg, uint8_t** ppend) {
			vec.resize(*ppend - *ppbeg);
			std::copy(*ppbeg, *ppend, vec.data());
			return true;
		};
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
	uint8_t Rol(size_t const offset, uint8_t const val){
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
			uint8_t* tmp_pbeg{(*ppbeg)-1};
			uint8_t old{};
			while (tmp_pbeg < (*ppend)+1) {
				uint8_t tmp{(uint8_t)(old | ((*tmp_pbeg) >> offset))};
				uint8_t last{(uint8_t)((*tmp_pbeg) & bits[offset])};
				old = Ror(offset, last);
				*tmp_pbeg = tmp;
				++tmp_pbeg;
			}
			return true;
		};
	}
	// SHIFT LEFT IN BITS
	static auto ShlInBits(size_t const offset) -> Func {
		return [offset] (uint8_t** ppbeg, uint8_t** ppend) {
			uint8_t* tmp_pend{(*ppend)+1};
			uint8_t old{};
			std::copy(*ppbeg, *ppend, (*ppbeg)+1);
			while (*ppbeg <= tmp_pend) {
				uint8_t tmp{(uint8_t)(old | ((*tmp_pend) << offset))};
				old = Rol(offset, *tmp_pend);
				old &= bits[offset];
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
			gpbeg = pbeg;
			*ppbeg = pbeg;
			uint8_t* pend{*ppend};
			pend -= skip_end;
			gpend -= skip_end;
			*ppend = pend;
			return true;
		};
	}
	// SAVE TO LID
	template<class T>
		static void SaveToLid(T const mask,
									 uint64_t& lid,
									 size_t const offset) {
			*(((T*)&lid)+offset) = mask;
		}
	// SAVE TO LID
	template<class T>
		static auto SaveToLid(uint64_t& lid, size_t& offset) ->
		std::function<void(T const)> {
			return [&lid, &offset] (T const mask) {
				SaveToLid<T>(mask, lid, offset);
				offset += sizeof(T);
			};
		}
	// FILTER
	template<class T, class Save>
		static auto Filter(T const mask, T const val, Save save) -> Func {
			return [mask, val, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				auto d = (T)(*(T*)pbeg);
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
	// SPLIT
	template<class T, class Save>
		static auto Split(T const mask, Save save) -> Func {
			return [mask, save] (uint8_t** ppbeg, uint8_t** ppend) {
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				save(cur);
				return true;
			};
		}
	// SPLIT NOT
	template<class T, class Save>
		static auto SplitNot(T const mask, Save save) -> Func {
			return [mask, save] (uint8_t** ppbeg, uint8_t** ppend) {
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				save(~cur);
				return true;
			};
		}
	// AND
	template<class T, class Save>
		static auto And(T const mask, Save save) -> Func {
			return [mask, save] (uint8_t** ppbeg, uint8_t** ppend) {
				bool ret{};
				uint8_t* pbeg{*ppbeg};
				T const cur{(T)((*(T*)pbeg) & mask)};
				if (cur) {
					save(0xff);
					ret = true;
				}
				return ret;
			};
		}
	// AND NOT
	template<class T, class Save>
		static auto AndNot(T const mask, Save save) -> Func {
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
	// INSERT
	static auto Insert(uint64_t const sz,
							 std::vector<uint8_t> const& data) -> Func {
		return [sz, data] (uint8_t** ppbeg, uint8_t** ppend) {
			uint8_t* pbeg{*ppbeg};
			uint8_t* pend{*ppend};
			std::copy(pbeg, pend, pbeg + data.size());
			std::copy(data.begin(), data.end(), pbeg);
			pend += data.size();
			*ppend = pend;
			return true;
		};
	}
	// REPLACE
	static auto Replace(std::vector<uint8_t> const& data) -> Func {
		return [data] (uint8_t** ppbeg, uint8_t** ppend) {
			std::copy(data.begin(), data.end(), *ppbeg);
			return true;
		};
	}
	// CHECK
	namespace Check {
		// LID
		static auto Lid(size_t& offset, std::string& msg) -> Func {
			return [&offset, &msg] (...) {
				bool ret{};
				if (offset < sizeof(uint64_t)) {
					ret = true;
				} else {
					msg = "LID overflow";
				}
				return ret;
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
		static auto OutOfRange(size_t const beg,
									  size_t const end,
									  size_t& counter,
									  std::string& msg) -> Func {
			return [beg, end, &counter, &msg] (uint8_t**, uint8_t**) {
				bool ret{(gpbeg + beg) < (gpend - end)};
				if (!ret) {
					msg = "Out of range packet: " + std::to_string(counter);
				}
				return ret;
			};
		}
		// OUT OR RANGE RIGHT
		static auto OutOfRangeRight(size_t const offset, std::string& msg) -> Func {
			return [offset, &msg] (uint8_t** ppbeg, uint8_t**) {
				bool ret{};
				if (((*ppbeg) + offset) < gpend) {
					ret = true;
				} else {
					msg = "Out of range: " + std::to_string(std::distance(((*ppbeg) + offset), gpend));
				}
				return ret;
			};
		}
		// OUT OR RANGE LEFT
		static auto OutOfRangeLeft(size_t const offset, std::string& msg) -> Func {
			return [offset, &msg] (uint8_t** ppbeg, uint8_t**) {
				bool ret{};
				if (gpbeg <= ((*ppbeg) - offset)) {
					ret = true;
				} else {
					msg = "Out of range: " + std::to_string(std::distance(gpbeg, ((*ppbeg) - offset)));
				}
				return ret;
			};
		}
	}
}

#endif
