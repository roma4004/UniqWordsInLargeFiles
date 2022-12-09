#include <iostream>
#include <fstream>
#include <filesystem> // Compile with /std:c++17 or higher
#include <thread>
#include <mutex>
#include <unordered_set>
#include <deque>

#include "testFileGenerator.h"

void FileParser(std::deque<std::string> *parsingQueue, std::ifstream &file,
                std::mutex &queueMtx,
                std::atomic<bool> &endOfRead, std::atomic<size_t> &queueSize);

void ParseWords(std::deque<std::string> *parsingQueue, std::unordered_set<std::string> *uniqueWords,
                std::mutex &queueMtx, std::mutex &wordMtx,
                const std::atomic<bool> &endOfRead, std::atomic<size_t> &queueSize);

char *BackupBlock(const std::unique_ptr<char[]> &buffer, const size_t startPos, const size_t length);

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 2) {
		std::cout << "please pass as argument full file name, like \"D://bigFile.txt\"" << std::endl;
		return 0;
	}

	const std::string command1{"./run_test"};
	bool isCommand = true;
	size_t commandLength = command1.length();
	for (size_t i = 0; argv[1][i] != '\0' && i < commandLength; ++i) {
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

	const size_t threadsAmount = std::thread::hardware_concurrency();
	std::mutex blockMtx, wordMtx;
	std::vector<std::thread> threads(threadsAmount);
	std::atomic<bool> endOfRead = false;
	std::atomic<size_t> queueSize = 0;
	auto *allUniqueWords = new std::unordered_set<std::string>();
	auto *strBlocks = new std::deque<std::string>();

	if (threadsAmount == 1) {
		std::thread t1 = std::thread{FileParser, strBlocks, std::ref(file),
		                             std::ref(blockMtx),
		                             std::ref(endOfRead), std::ref(queueSize)};
		t1.join();
		std::thread t2 = std::thread{ParseWords, strBlocks, allUniqueWords,
		                             std::ref(blockMtx), std::ref(wordMtx),
		                             std::ref(endOfRead), std::ref(queueSize)};
		t2.join();
	} else {
		for (size_t id = 0; id < threadsAmount; ++id) {
			if (id == 0) {
				threads[id] = std::thread{FileParser, strBlocks, std::ref(file),
				                          std::ref(blockMtx),
				                          std::ref(endOfRead), std::ref(queueSize)};
			} else {
				threads[id] = std::thread{ParseWords, strBlocks, allUniqueWords,
				                          std::ref(blockMtx), std::ref(wordMtx),
				                          std::ref(endOfRead), std::ref(queueSize)};
			}
		}

		for (auto &thread: threads) thread.join();
	}

	delete strBlocks;

	file.close();

	std::cout << allUniqueWords->size() << std::endl;

	// DEBUG: output allUniqueWords
//	std::cout << "occurrence:" << std::endl;
//	for (const auto &it: *allUniqueWords)
//		std::cout << it << std::endl;

	delete allUniqueWords;

	return 0;
}

void ParseWords(std::deque<std::string> *parsingQueue, std::unordered_set<std::string> *uniqueWords,
                std::mutex &queueMtx, std::mutex &wordMtx,
                const std::atomic<bool> &endOfRead, std::atomic<size_t> &queueSize) {
	auto *uniqueWordsLocal = new std::unordered_set<std::string>();
	while (true) {
		if (queueSize.load() == 0) {
			if (endOfRead.load()) {
				break;
			} else {
				std::this_thread::yield(); // other threads can push work to the queue now
				continue;
			}
		}

		queueMtx.lock();
		if (!parsingQueue->empty()) {
			auto *str = new std::string(parsingQueue->front());
			parsingQueue->pop_front();
			--queueSize;
			queueMtx.unlock();

			const size_t strLen = str->length();
			for (size_t i = 0; i < strLen; ++i) {
				for (; i < strLen && isspace((*str)[i]); ++i); // skipping all next spaces

				if (i == strLen) break; // if contain only spaces, so nothing to store

				const size_t wordStart = i;
				size_t wordLen = 0;
				for (; i < strLen && isalpha((*str)[i]); ++i, ++wordLen); // measuring word length

				uniqueWordsLocal->emplace(*str, wordStart, wordLen); // savin found word
			}

			delete str;
		} else {
			queueMtx.unlock();
		}
	}

	wordMtx.lock();
	uniqueWords->insert(uniqueWordsLocal->begin(), uniqueWordsLocal->end()); // merging all local unique word occurrence
	wordMtx.unlock();

	delete uniqueWordsLocal;
}

void FileParser(std::deque<std::string> *parsingQueue, std::ifstream &file,
                std::mutex &queueMtx,
                std::atomic<bool> &endOfRead, std::atomic<size_t> &queueSize) {
	constexpr ssize_t bufferSize = 1'000'000; // 1 Mb
	const unsigned int maxQueueSize = std::thread::hardware_concurrency();

	size_t prevBufferSize = 0;
	char *prevBuffer = nullptr;
	file.seekg(0, std::ios::beg); // rewind to the beginning of fileBlocks

	while (file) {
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(prevBufferSize + bufferSize);
		if (prevBuffer != nullptr) {
			for (size_t i = 0; i < prevBufferSize; ++i) {
				buffer[i] = prevBuffer[i]; // restoring remaining word part to buffer
			}
			free(prevBuffer);
		}

		file.read(buffer.get() + prevBufferSize, bufferSize); // reading text portion
		ssize_t bytesRead = file.gcount();
		if (bytesRead == 0 && prevBufferSize == 0) break;

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
			while (queueSize.load() > maxQueueSize) {
				std::this_thread::yield();
			}

			queueMtx.lock();
			parsingQueue->emplace_back(buffer.get(), 0, splitSize); // split and save block till last complete word
			++queueSize;
			queueMtx.unlock();

			prevBufferSize = actualBufferSize - splitSize;
			prevBuffer = BackupBlock(buffer, splitSize, prevBufferSize); // backup remain parts
		}
	}

	endOfRead.store(true); // reached EOF flag
}

char *BackupBlock(const std::unique_ptr<char[]> &buffer, const size_t startPos, const size_t length) {
	char *prevBuffer = (char *) malloc(length);
	for (size_t i = 0; i < length; ++i) {
		prevBuffer[i] = buffer[startPos + i];
	}

	return prevBuffer;
}
