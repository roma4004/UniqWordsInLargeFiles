#include <iostream>
#include <fstream>
#include <set>
#include <memory>
#include <filesystem> // Compile with /std:c++17 or higher

std::unique_ptr<char[]> InitializeBuffer(std::string& initWith, size_t len, const size_t bufferSize)
{
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(len + bufferSize);

	if (!initWith.empty())
	{
		for (int i = 0; i < len; ++i) // restoring remaining word part to buffer
		{
			buffer[i] = initWith[i];
		}
		initWith.clear();
	}

	return buffer;
}

void parseWords(const std::string_view& str, std::set<std::string>* uniqWords, std::string& prevPart, size_t bytesRead)
{
	for (auto it = str.cbegin(); it != str.cend(); ++it)
	{
		for (; it != str.cend() && isspace(*it); ++it); // skipping whitespaces

		if (it == str.cend()) // if contain only spaces, so nothing to store then moving to the next round
		{
			break;
		}

		size_t wordLen = 0;
		auto wordStart = it;
		for (; it != str.cend() && isalpha(*it); ++wordLen, ++it); // measuring word length

		if (it == str.cend()) // if text block reach end, word is unfinished, so we need to backup this part
		{

			if (bytesRead == 0) // in case we reach end file last word counting as separate word
			{
				uniqWords->insert(std::string(wordStart, wordLen));
			}
			else // reaching end of text block, backup will be stored
			{
				prevPart = std::string(wordStart, wordLen);
			}
			break;
		}
		else // we found space, so add word to storage
		{
			uniqWords->insert(std::string(wordStart, wordLen));
		}
	}
}

std::set<std::string>* ParseUniqWordsFromFile(std::ifstream& file)
{
	constexpr size_t bufferSize = 512'000'000; // 512 Mb
	file.seekg(0, std::ios::beg); // rewind to the beginning of file
	auto occurrence = new std::set<std::string>(); //TODO: check it can be just char* ?
	std::string prevPart;

	while (file)
	{
		size_t prevLength = prevPart.length();
		std::unique_ptr<char[]> buffer = InitializeBuffer(prevPart, prevLength, bufferSize);

		char *bufferStart = buffer.get() + prevLength;
		file.read(bufferStart, bufferSize);
		auto bytesRead = file.gcount();

		if (bytesRead == 0 && prevLength == 0) //if file end and prev parts too, we stop reading algo
		{
			break;
		}

		const std::string_view& target = std::string_view(buffer.get(), prevLength + bytesRead);
		parseWords(target, occurrence, prevPart, bytesRead);
	}

	return occurrence;
}

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

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 2)
	{
		std::cout << "please pass as argument full file name, like \"D://bigFile.txt\"" << std::endl;
		return 0;
	}

	std::string command1  = {"./run_test"};
	bool isCommand = true;
	unsigned long long int commandLength = command1.length();

	for (int i = 0; argv[1][i] != '\0' && i < commandLength; ++i)
	{
		if (argv[1][i] != command1[i])
		{
			isCommand = false;
			break;
		}
	}

	if (argv[1][0] != '\0' && isCommand)
	{
		testFileGenerator();

		return 0;
	}

	// NOTE:
	// .generic_string() in Windows: "c:/temp/test.txt"
	// ios_base::in - Open file for reading
	std::filesystem::path p(argv[1]);
	std::ifstream file(p.generic_string(), std::ios::in);

	if (file.fail())
	{
		perror("error due file opening, details");
		exit(1);
	}

	auto* occurrence = ParseUniqWordsFromFile(file);
	file.close();

	//DEBUG: output buffer
//	std::cout << "occurrence:" << std::endl;
//	for (const auto& it : *occurrence)
//		std::cout << it << '|' << std::endl;

	std::cout << occurrence->size() << std::endl;

	return 0;
}
