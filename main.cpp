#include <iostream>
#include <set>
#include <fstream>
#include <memory>
#include <string_view>

int main()
{
	auto* occurrence = new std::set<std::string_view>();
//TODO: if empty insert

	std::ifstream file(R"(D:\Desktop\SubStrA\bigfile.txt)", std::ios::app | std::ios::ate);
	std::cout << file.tellg() << "Of Total Bytes" << "\n";
	file.seekg(0, std::ios::beg); // rewind to the beginning of file

	constexpr size_t bufferSize = 4; //16kb
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(bufferSize);
	while (file)
	{
		file.read(buffer.get(), bufferSize);
		auto bytesRead = file.gcount();
		if (bytesRead == 0)
		{
			break; // EOF
		}

		const std::string_view& target = std::string_view(buffer.get(), bytesRead);
		auto it = target.begin();

		for (int i = 0; it != target.end(); ++it, ++i)
		{
			//skipping whitespaces
			for (; it != target.end() && isspace(*it); ++it, ++i);

			if (it == target.end())
			{
				break;
			}

			//measuring word length
			int wordLen = 0;
			for (; it != target.end() && std::isalpha(*it); ++wordLen, ++it);

			if (it == target.end())
			{
//				if (lastBuffer is empty)
//				{
//					//save first part of word
//				}
//				else
//				{
//					//concat first part and next part
//				}
			}
			else
			{
				//unique filtering
				auto word = target.substr(i, wordLen);
				i += wordLen;
				occurrence->insert(word);
			}
		}

		//test test     //
		//test testtest //
		//testtest  test//
		//testtest      //

		// output buffer
//		std::cout << target << std::endl;
	}


	std::cout << "total words count is: " << occurrence->size() << std::endl;
	std::cout << "the end" << std::endl;
	return 0;
}
