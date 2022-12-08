//
// Created by Dentair on 07.12.2022.
//

#include <fstream>

#include "testFileGenerator.h"

/// NOTE:
/// randomBlockSize should be 1-3 for normal
void testFileGenerator()
{
	static const char PossibleSymbols[] = "   a   b   c   ";
//		" \n"
//		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//		"abcdefghijklmnopqrstuvwxyz";
	const size_t      randomBlockSize   = 512'000'000; // 512Mb
	const size_t      desiredFileSize   = 32'000'000'000; // 32 GB
	srand(time(nullptr));

	std::string testsFileName = std::string("_only_for_testing_big_file_32gb_");
	std::ofstream testsFile("./", std::ios::out);
	testsFile.open(testsFileName);

	for (size_t i = 0; i < desiredFileSize; ++i)
	{
		std::string tmp_s;
		tmp_s.reserve(randomBlockSize);
		size_t j = 0;
		for (; j < randomBlockSize && i + j < desiredFileSize; ++j)
		{
			tmp_s += PossibleSymbols[rand() % (sizeof(PossibleSymbols) - 1)];
		}
		i += j;
		testsFile << tmp_s << std::endl;
	}

	testsFile.close();
}
