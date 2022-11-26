#include <iostream>
#include <set>
#include <fstream>
#include <memory>
#include <string_view>
#include <algorithm>

int main()
{
	auto occurrence = new std::set<std::string>(); //TODO: it can be just char* ?
//TODO: if empty insert

	std::ifstream file(R"(D:\Desktop\SubStrA\bigfile.txt)", std::ios::app | std::ios::ate);
	std::cout << file.tellg() << "Of Total Bytes" << "\n";
	file.seekg(0, std::ios::beg); // rewind to the beginning of file

	std::string             prevPart;
	constexpr size_t        bufferSize = 1; //16kb

	while (file)
	{
		//reserve space for prev word part + new buffer size to concat word parts
		size_t                  prevLength = prevPart.length();
		std::unique_ptr<char[]> buffer     = std::make_unique<char[]>(prevLength + bufferSize);

		//copy prev part to buffer
		if (prevLength != 0)
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
			//TODO: save last word part to store
			break; // EOF
		}

		//parsing unique words
		const std::string_view& target = std::string_view(buffer.get(), bytesRead + prevLength);
		// output buffer
		std::cout << "buffer:" << target << std::endl;
		auto it = target.begin();
		int i = 0;
		for (; it != target.end(); ++it, ++i)
		{
			//skipping whitespaces
			for (; it != target.end() && isspace(*it); ++it, ++i);

			//if part contain only spaces, so nothing to store at this time, moving to the next round
			if (it == target.end())
			{
				break;
			}

			//measuring word length
			int wordLen = 0;
			for (; it != target.end() && std::isalpha(*it); ++wordLen, ++it);

			//in case reaching the end, word remain is unfinished, we need to backup this part
			if (it == target.end())
			{
				if (bytesRead == 0)
				{
					auto word = std::string(target, i, prevLength);
					occurrence->insert(word);
				}
				else
				{
					prevPart = std::string(target, i, wordLen);
				}
				break;
			}
			else // or just add this to unique word storage
			{
				//unique filtering
				auto word = std::string(target, i, prevLength);
				occurrence->insert(word);
				i += wordLen;
			}
		}
	}

	// output buffer
	std::cout << "occurrence:" << std::endl;
	for (auto it : *occurrence)
		std::cout << it << '|' << std::endl;
	std::cout << "total words count is: " << occurrence->size() << std::endl;
	std::cout << "the end" << std::endl;
	return 0;
}
