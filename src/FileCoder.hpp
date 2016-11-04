#pragma once

#ifndef _FILECODER_HPP_
#define _FILECODER_HPP_

#include "ReedSolomonCoder.hpp"

#include <tuple>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <fstream>
#include <memory.h>

class CEccFileCoder
{
public:

	struct ECC_PARAM {
		uint32_t ui32CodeWordBits;//8 or 16
		uint32_t ui32ChunkSize;
		uint32_t ui32ChunkCount;	// ChunkSize * ChunkCount = 256b(8bit) or 128kb(16bit)
		uint32_t ui32EccCount;	// EccCount < ChunkCount, T = (EccCount-1)/2
		uint32_t ui32Intertwine;  // I/O once time = (ChunkCount - EccCount)*Intertwine
	};

	typedef uint32_t(__stdcall *ECC_CALLBACK_FUNC)(uint64_t offset, uint32_t length, uint64_t total_length, uint32_t result);

	enum :uint32_t {
		CODER_CONTIONUE = 0,
		CODER_BREAK = 1,
	};
protected:
	//	typedef ErrorDetectingCodes::CCrc32 CCrc32;
	typedef ErrorCorrectingCodes::IReedSolomonCoder				IReedSolomonCoder;
	typedef ErrorCorrectingCodes::CReedSolomonCoder<uint8_t>	CReedSolomonCoder8;
	typedef ErrorCorrectingCodes::CReedSolomonCoder<uint16_t>	CReedSolomonCoder16;

	struct ECC_HEADER {
		char		szSign[4];	//"ecc"
		char		szCoder[4];	//"rs\x01\x00"
		ECC_PARAM	param;
		uint64_t	ui64FileLength;
		//uint32_t ui32Crc32;
	};

	enum :uint32_t {
		ECC_HEADER_CODER_T = (CReedSolomonCoder8::N - sizeof(ECC_HEADER) - 1) / 2,
	};

	IReedSolomonCoder* CreateEccCoder(const ECC_PARAM& ecc_param) {

		assert(ecc_param.ui32CodeWordBits == 8 || ecc_param.ui32CodeWordBits == 16);
		assert(ecc_param.ui32EccCount & 1);

		uint32_t ecc_codeword_size = ecc_param.ui32CodeWordBits / Bit::BITS_PER_UINT8;
		uint32_t T = ((ecc_param.ui32EccCount*ecc_param.ui32ChunkSize / ecc_codeword_size) - 2) / 2;
		if (T == 0) T = 1;

		if (ecc_param.ui32CodeWordBits == 8) {
			return new CReedSolomonCoder8(T);
		}
		if (ecc_param.ui32CodeWordBits == 16) {
			return new CReedSolomonCoder16(T);
		}
		return nullptr;
	}

	bool WriteEccHeader(
		std::ofstream& ecc_stream,
		const ECC_PARAM& ecc_param,
		uint64_t ui64FileLength)
	{

		ECC_HEADER ecc_header;
		strncpy(ecc_header.szSign, "ecc", 4);
		strncpy(ecc_header.szCoder, "rs10", 4);
		ecc_header.param = ecc_param;
		ecc_header.ui64FileLength = ui64FileLength;
		//ecc_header.ui32Crc32=CCrc32::Calc((uint8_t*)&ecc_header,offsetof(ECC_HEADER, ui32Crc32));

		CReedSolomonCoder8 coder(ECC_HEADER_CODER_T);
		uint8_t buff[CReedSolomonCoder8::N];
		memcpy(buff, &ecc_header, sizeof(ecc_header));
		for (uint32_t i = sizeof(ecc_header); i<coder.K; i++) {
			buff[i] = 0;
		}
		coder.EncodeT2(buff, buff + coder.K);
		ecc_stream.write((char*)&buff, sizeof(buff));
		return true;
	}

	bool ReadEccHeader(
		std::ifstream& ecc_stream,
		ECC_PARAM& ecc_param,
		uint64_t& ui64FileLength)
	{
		CReedSolomonCoder8 coder(ECC_HEADER_CODER_T);
		uint8_t buff[CReedSolomonCoder8::N];
		ecc_stream.read((char*)&buff, sizeof(buff));

		if (coder.DecodeT2(buff, buff + coder.K) == IReedSolomonCoder::ECC_FAILED) {
			return false;
		}

		ECC_HEADER ecc_header;
		memcpy(&ecc_header, buff, sizeof(ecc_header));

		ecc_param = ecc_header.param;
		ui64FileLength = ecc_header.ui64FileLength;
		return true;
	}

	static void ecc_encode(
		std::tuple<IReedSolomonCoder*, uint32_t> coder_info,
		std::tuple<uint8_t*, uint32_t, uint32_t, uint32_t> buffer_info,
		std::tuple<ECC_CALLBACK_FUNC, uint64_t, uint64_t> callback_info,
		std::promise<uint32_t>&& thread_exitcode)
	{
		IReedSolomonCoder* pCoder = std::get<0>(coder_info);
		uint32_t ecc_offset = std::get<1>(coder_info);

		uint8_t* code_buff = std::get<0>(buffer_info);
		uint32_t line_size = std::get<1>(buffer_info);
		uint32_t begin_line = std::get<2>(buffer_info);
		uint32_t end_line = std::get<3>(buffer_info);

		ECC_CALLBACK_FUNC func = std::get<0>(callback_info);
		uint64_t read_offset = std::get<1>(callback_info);
		uint64_t total_length = std::get<2>(callback_info);

		for (uint32_t i = begin_line; i != end_line; i++) {
			uint8_t* pData = &code_buff[i*line_size];
			uint8_t* pEcc = &code_buff[i*line_size + ecc_offset];
			pCoder->EncodeT(pData, pEcc);
			//uint32_t coder_result=pCoder->DecodeT(pData,pEcc);

			if (func != nullptr) {
				uint32_t result = (*func)(read_offset + i*ecc_offset, ecc_offset, total_length, pCoder->ECC_NOERROR);
				if (result == CODER_BREAK) {
					thread_exitcode.set_value(CODER_BREAK);
					return;
				}
			}
		}
		thread_exitcode.set_value(CODER_CONTIONUE);

		//printf("--end %d\n",
		//    ErrorCorrectingCodes::CPoly<65535,ErrorCorrectingCodes::CFNT<4>>::GetMemPool()->m_dlSegment.m_pDummy);
	}


	static void ecc_decode(
		std::tuple<IReedSolomonCoder*, uint32_t> coder_info,
		std::tuple<uint8_t*, uint32_t, uint32_t, uint32_t> buffer_info,
		std::tuple<ECC_CALLBACK_FUNC, uint64_t, uint64_t> callback_info,
		std::promise<uint32_t>&& thread_exitcode)
	{
		IReedSolomonCoder* pCoder = std::get<0>(coder_info);
		uint32_t ecc_offset = std::get<1>(coder_info);

		uint8_t* code_buff = std::get<0>(buffer_info);
		uint32_t line_size = std::get<1>(buffer_info);
		uint32_t begin_line = std::get<2>(buffer_info);
		uint32_t end_line = std::get<3>(buffer_info);

		ECC_CALLBACK_FUNC func = std::get<0>(callback_info);
		uint64_t read_offset = std::get<1>(callback_info);
		uint64_t total_length = std::get<2>(callback_info);

		for (uint32_t i = begin_line; i != end_line; i++) {
			uint8_t* pData = &code_buff[i*line_size];
			uint8_t* pEcc = &code_buff[i*line_size + ecc_offset];
			uint32_t coder_result = pCoder->DecodeT(pData, pEcc);

			if (func != nullptr) {
				uint32_t result = (*func)(read_offset + i*ecc_offset, ecc_offset, total_length, coder_result);
				if (result == CODER_BREAK) {
					thread_exitcode.set_value(CODER_BREAK);
					return;
				}
			}
		}
		thread_exitcode.set_value(CODER_CONTIONUE);
	}
public:
	CEccFileCoder() {
	}
	~CEccFileCoder() {
	}

	static bool CreateEccParam(
		ECC_PARAM& ecc_param,
		uint32_t ecc_size_percent = 3,
		uint32_t ecc_codeword_bits = 16,
		uint32_t ecc_intertwine_mb = 32)
	{
		if (!(ecc_size_percent > 0 && ecc_size_percent <= 100))
			return false;
		//if (!(ecc_codeword_bits==8 || ecc_codeword_bits==16))
		//	return false;
		if (!(ecc_intertwine_mb > 0 && ecc_intertwine_mb < 4096))
			return false;

		if (ecc_codeword_bits == 8) {
			ecc_param.ui32CodeWordBits = 8;
			ecc_param.ui32ChunkSize = 1;
			ecc_param.ui32ChunkCount = 256;
			ecc_param.ui32EccCount = ecc_param.ui32ChunkCount*ecc_size_percent / (100 + ecc_size_percent);
			ecc_param.ui32Intertwine = (ecc_intertwine_mb << 20) / (ecc_param.ui32ChunkSize*(ecc_param.ui32ChunkCount - ecc_param.ui32EccCount));
			return true;
		}
		if (ecc_codeword_bits == 16) {
			ecc_param.ui32CodeWordBits = 16;
			ecc_param.ui32ChunkSize = 512;
			ecc_param.ui32ChunkCount = 256;
			ecc_param.ui32EccCount = ecc_param.ui32ChunkCount*ecc_size_percent / (100 + ecc_size_percent);
			ecc_param.ui32Intertwine = (ecc_intertwine_mb << 20) / ((ecc_param.ui32ChunkCount - ecc_param.ui32EccCount)*ecc_param.ui32ChunkSize);
			return true;
		}
		return false;
	}

	bool CreateEccFile(
		const std::string& raw_file,
		const std::string& ecc_file,
		const ECC_PARAM& ecc_param,
		ECC_CALLBACK_FUNC func = nullptr,
		uint32_t thread_count = 0/* 0 for all cpu */)
	{
		std::ifstream raw_stream;
		raw_stream.open(raw_file, std::ios::in | std::ios::binary);
		if (!raw_stream.is_open()) {
			return false;
		}

		std::ofstream ecc_stream;
		ecc_stream.open(ecc_file, std::ios::out | std::ios::binary);
		if (!ecc_stream.is_open()) {
			return false;
		}

		raw_stream.seekg(0, std::ios::end);
		uint64_t ui64FileLength = raw_stream.tellg();
		raw_stream.seekg(0, std::ios::beg);

		if (!WriteEccHeader(ecc_stream, ecc_param, ui64FileLength)) {
			return false;
		}

		uint32_t max_thread_count = std::thread::hardware_concurrency();
		if (thread_count == 0 || thread_count>max_thread_count) {
			thread_count = max_thread_count;
		}

		std::vector<
			std::tuple<
			bool,
			std::thread,
			std::future<uint32_t>
			>
		>  vthread(thread_count);

		IReedSolomonCoder* pCoder = CreateEccCoder(ecc_param);
		assert(pCoder);
		pCoder->Init();

		uint8_t* read_buff = new uint8_t[ecc_param.ui32Intertwine*ecc_param.ui32ChunkSize*(ecc_param.ui32ChunkCount - ecc_param.ui32EccCount)];
		//[ecc_param.ui32ChunkCount-ecc_param.ui32EccCount][ecc_param.ui32Intertwine][ecc_param.ui32ChunkSize]
		uint8_t* code_buff = new uint8_t[ecc_param.ui32Intertwine*ecc_param.ui32ChunkSize*ecc_param.ui32ChunkCount];
		//[ecc_param.ui32Intertwine][ecc_param.ui32ChunkCount][ecc_param.ui32ChunkSize]

		uint32_t ecc_codeword_size = ecc_param.ui32CodeWordBits / Bit::BITS_PER_UINT8;
		uint32_t ecc_chunk_size = ecc_param.ui32ChunkSize;
		uint32_t ecc_chunk_count = ecc_param.ui32ChunkCount;
		uint32_t ecc_code_count = ecc_param.ui32EccCount;
		uint32_t ecc_data_count = ecc_chunk_count - ecc_code_count;
		uint64_t read_length = ecc_param.ui32Intertwine*ecc_data_count*ecc_chunk_size;

		for (uint64_t read_offset = 0; read_offset<ui64FileLength; read_offset += read_length) {

			uint32_t read_real_length = (uint32_t)std::min(ui64FileLength - read_offset, (uint64_t)read_length);
			uint32_t read_chunk_count = (read_real_length + ecc_chunk_size - 1) / ecc_chunk_size;
			uint32_t read_intertwinet = (read_chunk_count + ecc_data_count - 1) / ecc_data_count;

			uint32_t buff_line_size = read_intertwinet*ecc_chunk_size;
			uint32_t code_line_size = ecc_chunk_count*ecc_chunk_size;
			uint32_t code_ecc_size = ecc_code_count*ecc_chunk_size - ecc_codeword_size;
			uint32_t code_ecc_offset = ecc_data_count*ecc_chunk_size + ecc_codeword_size;

			raw_stream.read((char*)read_buff, read_real_length);
			for (uint32_t i = read_real_length; i<buff_line_size*ecc_data_count; i++) {
				read_buff[i] = 0;
			}
			//transpose
			for (uint32_t i = 0; i<read_intertwinet; i++) {
				for (uint32_t j = 0; j<ecc_data_count; j++) {
					//code_buff[i][j]=read_buff[j][i]
					memcpy(
						&code_buff[(i*ecc_chunk_count + j)*ecc_chunk_size],
						&read_buff[(j*read_intertwinet + i)*ecc_chunk_size],
						ecc_chunk_size);
				}
				code_buff[(i*ecc_chunk_count + ecc_data_count)*ecc_chunk_size] = 0;
			}

			for (uint32_t i = 0, j = 0; i<thread_count; i++) {
				uint32_t begin_line = j;
				uint32_t end_line = read_intertwinet*(i + 1) / thread_count;

				std::get<0>(vthread[i]) = false;
				if (end_line>begin_line) {
					j = end_line;

					std::promise<uint32_t> thread_exitcode;
					std::get<0>(vthread[i]) = true;
					std::get<2>(vthread[i]) = thread_exitcode.get_future();
					std::get<1>(vthread[i]) = std::thread(&ecc_encode,
						std::make_tuple(pCoder, code_ecc_offset),
						std::make_tuple(code_buff, code_line_size, begin_line, end_line),
						std::make_tuple(func, read_offset, ui64FileLength),
						std::move(thread_exitcode));
				}
			}

			bool bBreak = false;
			for (uint32_t i = 0; i<thread_count; i++) {
				if (std::get<0>(vthread[i])) {
					std::get<1>(vthread[i]).join();
					uint32_t exit_code = std::get<2>(vthread[i]).get();
					if (exit_code == CODER_BREAK) {
						bBreak = true;
					}
				}
			}
			for (uint32_t i = 0; i<read_intertwinet; i++) {
				ecc_stream.write((char*)&code_buff[i*code_line_size + code_ecc_offset], code_ecc_size);
			}
			if (bBreak) {
				break;
			}
		}

		delete[] read_buff;
		delete[] code_buff;

		delete pCoder;

		return true;
	}

	bool CheckEccFile(
		const std::string& raw_file,
		const std::string& ecc_file,
		const std::string& fix_file,
		ECC_CALLBACK_FUNC func = nullptr,
		uint32_t thread_count = 0/* 0 for all cpu */)
	{
		std::ifstream raw_stream;
		raw_stream.open(raw_file, std::ios::in | std::ios::binary);
		if (!raw_stream.is_open()) {
			return false;
		}

		std::ifstream ecc_stream;
		ecc_stream.open(ecc_file, std::ios::in | std::ios::binary);
		if (!ecc_stream.is_open()) {
			return false;
		}

		std::ofstream fix_stream;
		fix_stream.open(fix_file, std::ios::out | std::ios::binary);
		if (!fix_stream.is_open()) {
			return false;
		}

		ECC_PARAM ecc_param;
		uint64_t ui64FileLength = 0;
		if (!ReadEccHeader(ecc_stream, ecc_param, ui64FileLength)) {
			return false;
		}


		uint32_t max_thread_count = std::thread::hardware_concurrency();
		if (thread_count == 0 || thread_count>max_thread_count) {
			thread_count = max_thread_count;
		}

		std::vector<
			std::tuple<
			bool,
			std::thread,
			std::future<uint32_t>
			>
		> vthread(thread_count);

		IReedSolomonCoder* pCoder = CreateEccCoder(ecc_param);
		assert(pCoder);
		pCoder->Init();

		uint8_t* read_buff = new uint8_t[ecc_param.ui32Intertwine*ecc_param.ui32ChunkSize*(ecc_param.ui32ChunkCount - ecc_param.ui32EccCount)];
		//[ecc_param.ui32ChunkCount-ecc_param.ui32EccCount][ecc_param.ui32Intertwine][ecc_param.ui32ChunkSize]
		uint8_t* ecc_buff = new uint8_t[ecc_param.ui32Intertwine*ecc_param.ui32ChunkSize*ecc_param.ui32EccCount];
		//[ecc_param.ui32Intertwine][ecc_param.ui32EccCount][ecc_param.ui32ChunkSize]
		uint8_t* code_buff = new uint8_t[ecc_param.ui32Intertwine*ecc_param.ui32ChunkSize*ecc_param.ui32ChunkCount];
		//[ecc_param.ui32Intertwine][ecc_param.ui32ChunkCount][ecc_param.ui32ChunkSize]

		uint32_t ecc_codeword_size = ecc_param.ui32CodeWordBits / Bit::BITS_PER_UINT8;
		uint32_t ecc_chunk_size = ecc_param.ui32ChunkSize;
		uint32_t ecc_chunk_count = ecc_param.ui32ChunkCount;
		uint32_t ecc_code_count = ecc_param.ui32EccCount;
		uint32_t ecc_data_count = ecc_chunk_count - ecc_code_count;
		uint64_t read_length = ecc_param.ui32Intertwine*ecc_data_count*ecc_chunk_size;

		for (uint64_t read_offset = 0; read_offset<ui64FileLength; read_offset += read_length) {

			uint32_t read_real_length = (uint32_t)std::min(ui64FileLength - read_offset, (uint64_t)read_length);
			uint32_t read_chunk_count = (read_real_length + ecc_chunk_size - 1) / ecc_chunk_size;
			uint32_t read_intertwinet = (read_chunk_count + ecc_data_count - 1) / ecc_data_count;

			uint32_t buff_line_size = read_intertwinet*ecc_chunk_size;
			uint32_t code_line_size = ecc_chunk_count*ecc_chunk_size;
			uint32_t code_ecc_size = ecc_code_count*ecc_chunk_size - ecc_codeword_size;
			uint32_t code_ecc_offset = ecc_data_count*ecc_chunk_size + ecc_codeword_size;

			raw_stream.read((char*)read_buff, read_real_length);
			for (uint32_t i = read_real_length; i<buff_line_size*ecc_data_count; i++) {
				read_buff[i] = 0;
			}

			//transpose
			for (uint32_t i = 0; i<read_intertwinet; i++) {
				for (uint32_t j = 0; j<ecc_data_count; j++) {
					//code_buff[i][j]=read_buff[j][i]
					memcpy(
						&code_buff[(i*ecc_chunk_count + j)*ecc_chunk_size],
						&read_buff[(j*read_intertwinet + i)*ecc_chunk_size],
						ecc_chunk_size);
				}
				code_buff[(i*ecc_chunk_count + ecc_data_count)*ecc_chunk_size] = 0;
			}

			for (uint32_t i = 0; i<read_intertwinet; i++) {
				ecc_stream.read((char*)&code_buff[i*code_line_size + code_ecc_offset], code_ecc_size);
			}

			for (uint32_t i = 0, j = 0; i<thread_count; i++) {
				uint32_t begin_line = j;
				uint32_t end_line = read_intertwinet*(i + 1) / thread_count;

				std::get<0>(vthread[i]) = false;
				if (end_line>begin_line) {
					j = end_line;

					std::promise<uint32_t> thread_exitcode;
					std::get<0>(vthread[i]) = true;
					std::get<2>(vthread[i]) = thread_exitcode.get_future();
					std::get<1>(vthread[i]) = std::thread(&ecc_decode,
						std::make_tuple(pCoder, code_ecc_offset),
						std::make_tuple(code_buff, code_line_size, begin_line, end_line),
						std::make_tuple(func, read_offset, ui64FileLength),
						std::move(thread_exitcode));
				}
			}

			bool bBreak = false;
			for (uint32_t i = 0; i<thread_count; i++) {
				if (std::get<0>(vthread[i])) {
					std::get<1>(vthread[i]).join();
					uint32_t exit_code = std::get<2>(vthread[i]).get();
					if (exit_code == CODER_BREAK) {
						bBreak = true;
					}
				}
			}

			//transpose
			for (uint32_t i = 0; i<read_intertwinet; i++) {
				for (uint32_t j = 0; j<ecc_data_count; j++) {
					//code_buff[i][j]=read_buff[j][i]
					memcpy(
						&read_buff[(j*read_intertwinet + i)*ecc_chunk_size],
						&code_buff[(i*ecc_chunk_count + j)*ecc_chunk_size],
						ecc_chunk_size);
				}
			}
			fix_stream.write((char*)read_buff, read_real_length);

			if (bBreak) {
				break;
			}
		}

		delete[] read_buff;
		delete[] ecc_buff;
		delete[] code_buff;

		delete pCoder;

		return true;
	}
};


#endif
