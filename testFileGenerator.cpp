//
// Created by Dentair on 07.12.2022.
//

#include <chrono>
#include <mutex>
#include <thread>
#include <filesystem> // Compile with /std:c++17 or higher
#include <unordered_set>
#include <fstream>
#include <iostream>
#include "testFileGenerator.h"

void testFileGenerator()
{
	static const char PossibleSymbols[] =
		" \n"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	const size_t      randomBlockSize   = 512'000; // 512Mb
	const size_t      desiredFileSize   = 1'073'741'824; // 1 GB
	srand(time(nullptr));

	std::string testsFileName = std::string("_only_for_testing_big_file_32gb_");
	std::ofstream testsFile("./", std::ios::out);
	testsFile.open(testsFileName);

	for (int i = 0; i < desiredFileSize; ++i)
	{

		std::string tmp_s;
		tmp_s.reserve(randomBlockSize);
		int j = 0;
		for (; j < randomBlockSize && i + j < desiredFileSize; ++j)
		{
			tmp_s += PossibleSymbols[rand() % (sizeof(PossibleSymbols) - 1)];
		}
		i += j;
		testsFile << tmp_s << std::endl;
	}

	testsFile.close();
}
