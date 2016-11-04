

#ifndef _DEBUG
#define NDEBUG
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "src/FileCoder.hpp"
using namespace ErrorCorrectingCodes;

#include <mutex>
std::mutex m;

volatile uint64_t process_length;


uint32_t __stdcall encode_callback(uint64_t offset,uint32_t length,uint64_t total_length,uint32_t result)
{
	uint32_t ret = 0;
	m.lock();
	process_length += length;
	uint32_t percent = (uint32_t)(process_length * 100 / total_length);
	if (percent > 100) percent = 100;
	printf("%3d%%", percent);
	fflush(stdout);
	printf("\b\b\b\b");
	m.unlock();
	return ret;
}

uint32_t __stdcall decode_callback(uint64_t offset,uint32_t length,uint64_t total_length,uint32_t result)
{
	uint32_t ret = 0;
	m.lock();
	process_length += length; 
	uint32_t percent = (uint32_t)(process_length * 100 / total_length);
	if (percent > 100) percent = 100;
	printf("%3d%%", percent);
	fflush(stdout);
	printf("\b\b\b\b");

	if (result == IReedSolomonCoder::ECC_FAILED) {
		printf("fix failed\n");
		ret = CEccFileCoder::CODER_BREAK;
	}
	m.unlock();
	return ret;
}

int main(int argc, char *argv[])
{
	/*printf("%d\n", argc);
	for (int i = 0; i < argc;i++){
		printf("%s\n", argv[i]);
	}*/

	/*argc = 5;
	argv[1] = "-e";
	argv[2] = "xxx";
	argv[3] = "yyy";
	argv[4] = "3";*/
	//ecc.exe - e xxx  yyy 3

	/*argc = 5;
	argv[1] = "-d";
	argv[2] = "xxx2";
	argv[3] = "yyy";
	argv[4] = "zzz";*/
	//ecc.exe - d xxx2 yyy zzz

	if (argc==5 && strcmp(argv[1], "-e") == 0) {

		std::string raw_file = argv[2];
		std::string ecc_file = argv[3];
		uint32_t percent = atoi(argv[4]);

		CEccFileCoder fc;
		CEccFileCoder::ECC_PARAM param;
		if (!fc.CreateEccParam(param, percent)) {
			printf("incorrect input...\n");
			return 1;
		}

		process_length = 0;
		if (!fc.CreateEccFile(raw_file, ecc_file, param, &encode_callback)) {
			printf("runtime error...\n");
			return 1;
		}
		return 0;
	}

	if (argc==5 && strcmp(argv[1], "-d") == 0) {
		std::string raw_file = argv[2];
		std::string ecc_file = argv[3];
		std::string fix_file = argv[4];

		CEccFileCoder fc;

		process_length = 0;
		if (!fc.CheckEccFile(raw_file, ecc_file, fix_file, &decode_callback)) {
			printf("runtime error...\n");
			return 1;
		}
		return 0;
	}

	if (argc==2 && strcmp(argv[1], "-h") == 0) {
		printf("encode example: -e raw_file ecc_file percent\n");
		printf("decode example: -d raw_file ecc_file fix_file\n");
		return 0;
	}
	
	printf("command not supported, please use \"-h\" to get more information...\n");
	return 1;
}