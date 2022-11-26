#include <iostream>
#include <set>

int GetWordLen(const std::string_view& target, const char* it)
{
	int wordLen = 0;
	for (; it != target.end() &&  std::isalpha(*it); ++wordLen, ++it);

	return wordLen;
}

std::size_t GetNumberOfUniqueWords(const std::string_view& target)
{
	auto* occurrence = new std::set<std::string_view>();
	auto it = target.begin();

	for (int i = 0; it != target.end(); ++it, ++i)
	{
		//skipping whitespaces
		for (; it != target.end() && isspace(*it); ++it, ++i);

		if (it == target.end())
		{
			break;
		}

		int wordLen = GetWordLen(target, it);

		//unique filtering
		auto word = target.substr(i, wordLen);
		i += wordLen;
		occurrence->insert(word);
	}

	return occurrence->size();
}


int main()
{
	const char* text = " Hello, Wo rlda !";
	std::cout << GetNumberOfUniqueWords(text) << std::endl;
//TODO: if empty insert
	return 0;
}
