#include "RefPack.h"

namespace refpack {

	// TODO: refpack compression

	std::vector<mco::byte> Decompress(mco::Span<mco::byte> compressed) {
		const mco::byte* in = compressed.get();

		// Command variables
		mco::byte first;
		mco::byte second;
		mco::byte third;
		mco::byte fourth;

		// output buffer
		std::vector<mco::byte> out;

		mco::uint32 proc_len;
		mco::uint32 ref_run;
		mco::byte* ref_ptr;

		// perform null & 0 size check
		if(!in || compressed.size() == 0)
			return {};

		mco::uint16 signature = ((in[0] << 8) | in[1]);
		in += sizeof(mco::uint16);

		// skip uint24 compressed size field
		if(signature & 0x0100)
			in += mco::threebyte_size;

		// read the uint24 decompressed size
		mco::uint32 decompressed_size = ((in[0] << 16) | (in[1] << 8) | in[2]);
		in += mco::threebyte_size;

		// then resize output buffer to decompressed size
		// and retrive pointer to that data

		out.resize(decompressed_size);
		mco::byte* outptr = out.data();

		while(true) {
			// Retrive the first command byte
			first = *in++;

			if(!(first & 0x80)) {
				// 2-byte command: 0DDRRRPP DDDDDDDD
				second = *in++;

				proc_len = first & 0x03;

				for(mco::uint32 i = 0; i < proc_len; i++)
					*outptr++ = *in++;

				ref_ptr = outptr - ((first & 0x60) << 3) - second - 1;
				ref_run = ((first >> 2) & 0x07) + 3;

				for(mco::uint32 i = 0; i < ref_run; ++i)
					*outptr++ = *ref_ptr++;

			} else if(!(first & 0x40)) {
				// 3-byte command: 10RRRRRR PPDDDDDD DDDDDDDD
				second = *in++;
				third = *in++;

				proc_len = second >> 6;

				for(mco::uint32 i = 0; i < proc_len; ++i)
					*outptr++ = *in++;

				ref_ptr = outptr - ((second & 0x3f) << 8) - third - 1;
				ref_run = (first & 0x3f) + 4;

				for(mco::uint32 i = 0; i < ref_run; ++i)
					*outptr++ = *ref_ptr++;

			} else if(!(first & 0x20)) {
				// 4-byte command: 110DRRPP DDDDDDDD DDDDDDDD RRRRRRRR
				second = *in++;
				third = *in++;
				fourth = *in++;

				proc_len = first & 0x03;

				for(mco::uint32 i = 0; i < proc_len; ++i)
					*outptr++ = *in++;

				ref_ptr = outptr - ((first & 0x10) << 12) - (second << 8) - third - 1;
				ref_run = ((first & 0x0c) << 6) + fourth + 5;

				for(mco::uint32 i = 0; i < ref_run; ++i)
					*outptr++ = *ref_ptr++;
			} else {
				// 1-byte command: 111PPPPP

				proc_len = (first & 0x1f) * 4 + 4;

				if(proc_len <= 0x70) {
					// no stop flag

					for(mco::uint32 i = 0; i < proc_len; ++i)
						*outptr++ = *in++;

				} else {
					// has a stop flag
					proc_len = first & 0x3;

					for(mco::uint32 i = 0; i < proc_len; ++i)
						*outptr++ = *in++;

					break;
				}
			}
		}

		return out;
	}

} // namespace refpack