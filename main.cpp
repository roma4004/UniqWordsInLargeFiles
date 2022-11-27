#include <iostream>
#include <fstream>
#include <set>
#include <memory>

void parseUniqWord(const std::string_view& target, std::set<std::string>* occurrence, std::string& prevPartStorage, size_t bytesRead)
{
	// parsing unique words
	for (auto it = target.cbegin(); it != target.cend(); ++it)
	{
		// skipping whitespaces
		for (; it != target.cend() && isspace(*it); ++it);

		// if part contain only spaces, so nothing to store at this time, moving to the next round
		if (it == target.cend())
		{
			break;
		}

		// measuring word length
		size_t wordLen = 0;
		auto wordStart = it;
		for (; it != target.cend() && isalpha(*it); ++wordLen, ++it);

		if (it == target.cend()) // if text block reach end, word is unfinished, so we need to backup this part
		{

			if (bytesRead == 0) // in case we reach end file last word counting as separate word
			{
				occurrence->insert(std::string(wordStart, wordLen));
			}
			else // reaching end of text block, backup will be stored
			{
				prevPartStorage = std::string(wordStart, wordLen);
			}
			break;
		}
		else // we found space, so add word to storage
		{
			occurrence->insert(std::string(wordStart, wordLen));
		}
	}
}

int main()
{
	constexpr size_t bufferSize = 16000; //16kb
	auto occurrence = new std::set<std::string>(); //TODO: it can be just char* ?
//TODO: if empty insert

	std::ifstream file(R"(D:\Desktop\SubStrA\bigfile.txt)", std::ios::app | std::ios::ate);
	file.seekg(0, std::ios::beg); // rewind to the beginning of file

	// portion reading
	std::string prevPart;
	while (file)
	{
		//reserve space for prev word part + new buffer size to concat word parts
		size_t prevLength = prevPart.length();
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(prevLength + bufferSize);

		// restoring remaining word part to buffer
		if (prevPart.empty())
		{
			for (int i = 0; i < prevLength; ++i)
			{
				buffer[i] = prevPart[i];
			}
			prevPart.clear();
		}

		//reading new file part
		file.read(buffer.get() + prevLength, bufferSize);
		auto bytesRead = file.gcount();

		//if file end we stop reading algo
		if (bytesRead == 0 && prevLength == 0)
		{
			break; // EOF
		}

		const std::string_view& target = std::string_view(buffer.get(), prevLength + bytesRead);
		parseUniqWord(target, occurrence, prevPart, bytesRead);
	}

	//DEBUG: output buffer
//	std::cout << "occurrence:" << std::endl;
//	for (const auto& it : *occurrence)
//		std::cout << it << '|' << std::endl;

	//DEBUG: msg
//	std::cout << "total words count is: ";
	std::cout << occurrence->size() << std::endl;

	return 0;
}
