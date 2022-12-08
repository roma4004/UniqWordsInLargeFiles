#include <iostream>
#include <fstream>
#include <filesystem> // Compile with /std:c++17 or higher
#include <thread>
#include <mutex>
#include <chrono>
#include <unordered_set>
#include <deque>

#include "testFileGenerator.h"

void FileParser(std::deque<std::string> *parsingQueue, std::ifstream &file, std::mutex &write_mtx);

void ParseWords(std::deque<std::string> *strBlocks, std::unordered_set<std::string> *uniqWords,
				std::mutex &blocksMtx, std::mutex &wordMtx);

char *BackupBlock(const std::unique_ptr<char[]> &buffer, size_t startPos, size_t prevBufferSize);

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 2) {
		std::cout << "please pass as argument full file name, like \"D://bigFile.txt\"" << std::endl;
		return 0;
	}

	const std::string command1{"./run_test"};
	bool isCommand = true;
	size_t commandLength = command1.length();
	for (unsigned int i = 0; argv[1][i] != '\0' && i < commandLength; ++i) {
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

	const unsigned int threadsAmount = std::thread::hardware_concurrency();
	std::mutex blockMtx, wordMtx;
	std::vector<std::thread> threads(threadsAmount);
	auto *uniqWords = new std::unordered_set<std::string>(); //TODO: check it can be just char* ?
	auto *strBlocks = new std::deque<std::string>();
//TODO: add atomic bool to display for all thread that queue adding in progress, then if false thread end their work if see empty queue;
	if (threadsAmount == 1) {
		std::thread t1 = std::thread{FileParser, strBlocks, std::ref(file), std::ref(blockMtx)};
		t1.join();
		std::thread t2 = std::thread{ParseWords, strBlocks, uniqWords, std::ref(blockMtx), std::ref(wordMtx)};
		t2.join();
	} else {
		for (unsigned int id = 0; id < threadsAmount; ++id) {
			if (id == 0) {
				threads[id] = std::thread{FileParser, strBlocks, std::ref(file), std::ref(blockMtx)};
			} else {
				threads[id] = std::thread{ParseWords, strBlocks, uniqWords, std::ref(blockMtx), std::ref(wordMtx)};
			}
		}

		for (auto &thread: threads) thread.join();
	}

	file.close();

	//DEBUG: output queue
//	std::cout << "queue:" << std::endl;
//	for (const auto& it: *strBlocks)
//		std::cout << '/' << it << '/' << std::endl;

	//DEBUG: output uniqWords
//	std::cout << "occurrence:" << std::endl;
//	for (const auto &it: *uniqWords)
//		std::cout << '|' << it << '|' << std::endl;

	std::cout << uniqWords->size() << std::endl;

	return 0;
}

void ParseWords(std::deque<std::string> *strBlocks, std::unordered_set<std::string> *uniqWords, std::mutex &blocksMtx, std::mutex &wordMtx) {
	using namespace std::chrono_literals;
	while (strBlocks->empty()) std::this_thread::sleep_for(500ms); //TODO: implement conditional waits

	blocksMtx.lock();
	auto *str = new std::string(strBlocks->front());
	strBlocks->pop_front();
	blocksMtx.unlock();

	const size_t strLen = str->length();
	for (size_t i = 0; i < strLen; ++i) {
		for (; i < strLen && isspace((*str)[i]); ++i); // skipping all next spaces

		if (i == strLen) break; // if contain only spaces, so nothing to store

		size_t wordStart = i, wordLen = 0;
		for (; i < strLen && isalpha((*str)[i]); ++i, ++wordLen); // measuring word length

		wordMtx.lock();
		uniqWords->insert(std::string(*str, wordStart, wordLen)); // savin found word
		wordMtx.unlock();
	}
}

void FileParser(std::deque<std::string> *parsingQueue, std::ifstream &file, std::mutex &write_mtx) {
	constexpr ssize_t bufferSize = 1'000; // 1 Mb
	size_t prevBufferSize = 0;
	char *prevBuffer = nullptr;
	file.seekg(0, std::ios::beg); // rewind to the beginning of fileBlocks

	while (file) {
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(prevBufferSize + bufferSize);
		if (prevBuffer != nullptr) {
			for (int i = 0; i < prevBufferSize; ++i) {
				buffer[i] = prevBuffer[i]; // restoring remaining word part to buffer
			}
			free(prevBuffer);
		}

		file.read(buffer.get() + prevBufferSize, bufferSize); // reading text portion
		ssize_t bytesRead = file.gcount();
		if (bytesRead == 0) break;

		size_t actualBufferSize = prevBufferSize + bytesRead;
		size_t splitSize = actualBufferSize;
		for (; splitSize > 0; --splitSize) {
			if (isspace(buffer[splitSize - 1])) {
				break; // finding last completed word
			}
		}

		if (splitSize == 0) {
			prevBufferSize = actualBufferSize;
			prevBuffer = BackupBlock(buffer, 0, prevBufferSize); // backup whole block
		} else {
			write_mtx.lock();
			parsingQueue->emplace_back(buffer.get(), 0, splitSize); // save block till last complete word
			write_mtx.unlock();

			prevBufferSize = actualBufferSize - splitSize;
			prevBuffer = BackupBlock(buffer, splitSize, prevBufferSize); // backup remain parts
		}
	}
}

char *BackupBlock(const std::unique_ptr<char[]> &buffer, size_t startPos, size_t prevBufferSize) {
	char *prevBuffer = (char *) malloc(prevBufferSize);
	for (unsigned int i = 0; i < prevBufferSize; ++i) {
		prevBuffer[i] = buffer[startPos + i];
	}

	return prevBuffer;
}
