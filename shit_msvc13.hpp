#ifndef SHIT_MSVC13
#define SHIT_MSVC13

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

static auto Crop(size_t const skip_beg, size_t const skip_end = 0) -> std::function<bool(uint8_t** ppbeg, uint8_t** ppend)> {
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
#endif
