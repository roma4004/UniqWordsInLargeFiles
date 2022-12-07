#include <iostream>
#include <fstream>
#include <unordered_set>
//#include <memory>
#include <filesystem> // Compile with /std:c++17 or higher
#include <thread>
#include <mutex>
#include <chrono>
#include <deque>

#include "testFileGenerator.h"

void FileParser(std::deque<std::string> *parsingQueue, std::ifstream &file, std::mutex &write_mtx);
void ParseWords(std::deque<std::string>* textBlocks, std::unordered_set<std::string>* uniqWords, std::mutex& writeDequeMtx, std::mutex& writeUniqMtx);

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 2) {
		std::cout << "please pass as argument full file name, like \"D://bigFile.txt\"" << std::endl;
		return 0;
	}

	std::string command1 = {"./run_test"};
	bool isCommand = true;
	unsigned long long int commandLength = command1.length();

	for (int i = 0; argv[1][i] != '\0' && i < commandLength; ++i) {
		if (argv[1][i] != command1[i]) {
			isCommand = false;
			break;
		}
	}

	if (argv[1][0] != '\0' && isCommand) {
		testFileGenerator();

		return 0;
	}

	// NOTE:
	// .generic_string() in Windows: "c:/temp/test.txt"
	// ios_base::in - Open file for reading
	std::filesystem::path p(argv[1]);
	std::ifstream file(p.generic_string(), std::ios::in);

	if (file.fail()) {
		perror("error due file opening, details");
		exit(1);
	}

	auto* splitBlocks = new std::deque<std::string>();
	std::mutex writeDequeMtx;
	std::mutex writeUniqMtx;

	auto *uniqWords = new std::unordered_set<std::string>(); //TODO: check it can be just char* ?

	const unsigned int threadsAmount = std::thread::hardware_concurrency();
	std::vector<std::thread> threads(threadsAmount);
//TODO: add atomic bool to display for all thread that queue adding in progress, then if false thread end their work if see empty queue;
	if (threadsAmount == 1)
	{
		std::thread t1 = std::thread{FileParser, splitBlocks, std::ref(file), std::ref(writeDequeMtx)};
		t1.join();
		std::thread t2 = std::thread{ParseWords, splitBlocks, uniqWords, std::ref(writeDequeMtx), std::ref(writeUniqMtx)};
		t2.join();
	}
	else
	{
		for (unsigned int threadId = 0; threadId < threadsAmount; ++threadId)
		{
			if (threadId == 0)
			{
				threads[threadId] = std::thread{FileParser, splitBlocks, std::ref(file), std::ref(writeDequeMtx)};
			}
			else
			{
				threads[threadId] = std::thread{ParseWords, splitBlocks, uniqWords, std::ref(writeDequeMtx), std::ref(writeUniqMtx)};
			}
		}

		for (auto& thread : threads) thread.join();
	}


	file.close();

//	std::cout << "queue:" << std::endl;
//	for (const auto& it: *splitBlocks)
//		std::cout << '/' << it << '/' << std::endl;

	//DEBUG: output buffer
	std::cout << "occurrence:" << std::endl;
	for (const auto &it: *uniqWords)
		std::cout << '|' << it << '|' << std::endl;

	std::cout << "result is:" << std::endl;
	std::cout << uniqWords->size() << std::endl;

	return 0;
}

void ParseWords(std::deque<std::string>* textBlocks, std::unordered_set<std::string>* uniqWords, std::mutex& writeDequeMtx, std::mutex& writeUniqMtx) {
	using namespace std::chrono_literals;
	while (textBlocks->empty())
		std::this_thread::sleep_for(500ms); //TODO: implement conditional waits

	writeDequeMtx.lock();
	auto* str = new std::string(textBlocks->front());
	textBlocks->pop_front();
	writeDequeMtx.unlock();

	size_t strLen = str->length();
	for (size_t i = 0; i < strLen; ++i) {
		for (; i < strLen && isspace((*str)[i]); ++i); // skipping all next whitespaces

		if (i == strLen) break; // if contain only spaces, so nothing to store

		size_t wordStart{i}, wordLen{0};
		for (; i < strLen && isalpha((*str)[i]); ++i, ++wordLen); // measuring word length

		if (wordLen == 0) continue;

		if (i == strLen) // if reach str end, word is unfinished, so we need backup this part
		{ // in case we reach end file last word counting as separate word,
			writeUniqMtx.lock();
			auto [it, status] = uniqWords->insert(std::string(*str, wordStart, wordLen));
//			if (status)
//				std::cout << "at str end newWord:" << ": " << *it << std::endl;
			writeUniqMtx.unlock();
			break;
		} else { // we found space, so add word to storage
			writeUniqMtx.lock();
			auto [it, status] = uniqWords->insert(std::string(*str, wordStart, wordLen));
//			if (status)
//				std::cout << "newWord:" << *it << std::endl;
			writeUniqMtx.unlock();
		}
	}
}

void FileParser(std::deque<std::string> *parsingQueue, std::ifstream &file, std::mutex &write_mtx) {
	constexpr ssize_t bufferSize = 10; // 1 Mb
	char *prevBuffer = nullptr;
	size_t prevBufferSize = 0;
	file.seekg(0, std::ios::beg); // rewind to the beginning of fileBlocks

	while (file) {
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(prevBufferSize + bufferSize);
		if (prevBuffer != nullptr) {
			for (int i = 0; i < prevBufferSize; ++i) { // restoring remaining word part to buffer
				buffer[i] = prevBuffer[i];
			}
			free(prevBuffer);
		}

		file.read(buffer.get() + prevBufferSize, bufferSize);
		ssize_t bytesRead = file.gcount();
		if (bytesRead == 0) break;

//		std::cout << "read:" << std::string (buffer.get(), 0, prevBufferSize + bytesRead) << std::endl;
		size_t actualBufferSize = prevBufferSize + bytesRead;
		size_t splitSize = actualBufferSize;
		for (; splitSize > 0; --splitSize) {
			if (isspace(buffer[splitSize - 1])) {
				break;
			}
		}

		if (splitSize == 0) { // backup whole block
			prevBufferSize = actualBufferSize;
			prevBuffer = (char *) malloc(actualBufferSize);
			for (int i = 0; i < actualBufferSize; ++i) {
				prevBuffer[i] = buffer[i];
			}
		} else { // split after last complete word
			write_mtx.lock();
			auto tmp = parsingQueue->emplace_back(buffer.get(), 0, splitSize);
//			std::cout << "queue add:" << tmp << std::endl;
			write_mtx.unlock();

			//and backup remain parts
			size_t remainsBlockLength = actualBufferSize - splitSize;
			prevBufferSize = remainsBlockLength;
			prevBuffer = (char *) malloc(prevBufferSize);
			for (int i = 0; i < prevBufferSize; ++i) {
				prevBuffer[i] = buffer[splitSize + i];
			}
			write_mtx.lock();
//			std::cout << "queue add:" << std::string(buffer.get(), splitSize - 1, prevBufferSize) << std::endl;
			write_mtx.unlock();
		}
	}
}
