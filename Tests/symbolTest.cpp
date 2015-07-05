
//
// symbolTest
// a unit test made using the Catch framework in catch.hpp / tests.cpp.
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "catch.hpp"
#include "../include/madronalib.h"

const char letters[24] = "abcdefghjklmnopqrstuvw";

TEST_CASE("madronalib/core/symbol/maps", "[symbol]")
{
	const int kTableSize = 100;	
	const int kTestLength = 1000000;
	
	// main maps for testing
	std::map<MLSymbol, float> testMap1;
	std::map<std::string, float> testMap2;
	std::unordered_map<MLSymbol, float> testMap3;
	std::unordered_map<std::string, float> testMap4;
	
	// make dictionaries of symbols, strings and chars for testing
	MLNameMaker nameMaker;
	std::vector<MLSymbol> symbolDict;
	std::vector<std::string> stringDict;
	std::vector<const char *> charDict;
	int p = 0;
	for(int i=0; i<kTableSize; ++i)
	{
		// make procedural gibberish
		std::string newString;
		int length = 3 + (p%8);
		for(int j=0; j<length; ++j)
		{
			p += (i*j + 1);
			p += i%37;
			p += j%23;
			p = abs(p);
			newString += letters[p % 22];
		}		
		
		stringDict.push_back(newString);
		
		// add it to symbol table
		MLSymbol newSym(newString.c_str());
		symbolDict.push_back(newSym);
		
		// add an entry to each map
		float val = i;
		testMap1[newSym] = val;
		testMap2[newString] = val;
		testMap3[newSym] = val;
		testMap4[newString] = val;
	}
	
	// make char dict after string dict is complete, otherwise ptrs may change!
	for(int i=0; i<stringDict.size(); ++i)
	{
		charDict.push_back(stringDict[i].c_str());
	}
	
	SECTION("test maps")
	{
		std::chrono::time_point<std::chrono::system_clock> start, end;
		std::chrono::duration<double> elapsed;
		double symbolSum, stringSum;
		int idx;
		
		// lookup from existing std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMap2[stringDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing strings, elapsed time: " << elapsed.count() << "s\n";
		
		// lookup from existing MLSymbols
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMap1[symbolDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing symbols, elapsed time: " << elapsed.count() << "s\n";
		
		REQUIRE(stringSum == symbolSum);
		
		// lookup from existing std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMap4[stringDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing strings, unordered, elapsed time: " << elapsed.count() << "s\n";
		
		// lookup from existing MLSymbols
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMap3[symbolDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing symbols, unordered, elapsed time: " << elapsed.count() << "s\n";

		REQUIRE(stringSum == symbolSum);
		
		// lookup from newly made std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMap2[charDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing strings, elapsed time: " << elapsed.count() << "s\n";
		
		// lookup from new MLSymbols made from char * 
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMap1[charDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing symbols, elapsed time: " << elapsed.count() << "s\n";

		REQUIRE(stringSum == symbolSum);
				
		// lookup from newly made std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMap4[charDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing strings, unordered, elapsed time: " << elapsed.count() << "s\n";
		
		// unordered lookup from new MLSymbols made from char * 
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMap3[charDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing symbols, unordered, elapsed time: " << elapsed.count() << "s\n";

		REQUIRE(stringSum == symbolSum);
		
		REQUIRE(theSymbolTable().audit());
	}
}

TEST_CASE("madronalib/core/symbol/numbers", "[symbol]")
{
	MLNameMaker namer;
	for(int i=0; i<10; ++i)
	{
		MLSymbol testSym = namer.nextNameAsString();
		MLSymbol testSymWithNum = testSym.withFinalNumber(i);
		MLSymbol testSymWithoutNum = testSymWithNum.withoutFinalNumber();
		REQUIRE(testSym == testSymWithoutNum);
	}
	REQUIRE(theSymbolTable().audit());
}

static const int kThreadTestSize = 100;

void threadTest(int threadID)
{
	MLNameMaker namer;
	for(int i=0; i<kThreadTestSize; ++i)
	{
		MLSymbol sym(namer.nextNameAsString());
		std::this_thread::yield();
	}
}

TEST_CASE("madronalib/core/symbol/threads", "[symbol][threads]")
{
	// multithreaded test. multiple nameMakers will try to make duplicate names at about the same time,
	// which will almost certainly lead to problems unless the symbol library is properly thread-safe.
	
	theSymbolTable().clear();
	int nThreads = 10;
	std::vector< std::thread > threads;
	for(int i=0; i < nThreads; ++i)
	{
		threads.push_back(std::thread(threadTest, i));
	}
	for(int i=0; i < nThreads; ++i)
	{
		threads[i].join();
	}
	
	REQUIRE(theSymbolTable().audit());
	REQUIRE(theSymbolTable().getSize() == kThreadTestSize + 1);
}

