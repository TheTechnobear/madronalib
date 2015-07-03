
// MLSymbol.h
// ----------
// Madrona Labs C++ framework for DSP applications.
// Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// MLSymbol is designed to be an efficient key in STL containers such as map and
// unordered_map, that is quick to convert to and from a unique string.  
//
// requirements
// ---------
//
// The value of an MLSymbol must remain valid even after more MLSymbols are created.  
// This allows MLSymbols to function as correct keys.
//
// Accessing an MLSymbol must not cause any heap to be allocated if the symbol already exists. 
// This allows use in DSP code, assuming that the signal graph or whatever has already been parsed.

#ifndef _ML_SYMBOL_H
#define _ML_SYMBOL_H

#include <set>
#include <vector>
#include <string>
#include <mutex>

#define debug() std::cout

// With USE_ALPHA_SORT on, a std::map<MLSymbol, ...> will be in alphabetical order.
// With it off, the symbols will sort into the order they were created, and symbol creation 
// as well as map lookups will be significantly faster. 
#define USE_ALPHA_SORT	0

// TODO document with doxygen!!

static const int kMLMaxSymbolLength = 56;
static const int kMLMaxNumberLength = 8;

const int kHashTableBits = 16;
const int kHashTableSize = (1 << kHashTableBits);
const int kHashTableMask = kHashTableSize - 1;

// symbols are allocated in chunks of this size when needed. 
const int kTableChunkSize = 1024;

class MLSymbolTable
{
public:
	MLSymbolTable();
	~MLSymbolTable();
	void clear();
	void allocateChunk();
	
#if USE_ALPHA_SORT	
	int getSymbolAlphaOrder(const int symID);
#endif
	int addEntry(const char * sym, int len);
	
	// look up a symbol by name and return its ID. Used in MLSymbol constructors.
	// if the symbol already exists, this routine must not allocate any heap memory.
	int getSymbolID(const char * sym);
	
	const std::string& getSymbolByID(int symID);
	
	int getSize() { return mSize; }	
	void dump(void);
	int audit(void);
	
private:
	
	// very simple hash function from Kernighan & Ritchie.
	inline unsigned KRhash(const char *s)
	{
		const unsigned char *p;
		unsigned hashval = 0;
		for (p = (const unsigned char *) s; *p; p++)
		{
			hashval = *p + 31u * hashval;
		}
		return hashval & kHashTableMask;
	}
	
	// 2^32 unique symbols are possible. There is no checking for overflow.
	int mSize;
	int mCapacity;
	
	std::mutex mMutex;
	
	// vector of symbols in ID/creation order
	std::vector<std::string> mSymbolsByID;	
	
#if USE_ALPHA_SORT	
	// vector of alphabetically sorted indexes into symbol vector, in ID order
	std::vector<int> mAlphaOrderByID;	
	
	// TEMP set used for sorting.
	std::set<std::string> mSymbolsByAlphaOrder;
#endif
	
	// hash table containing indexes to strings
	std::vector< std::vector<int> > mHashTable;
};

inline MLSymbolTable& theSymbolTable()
{
	static std::unique_ptr<MLSymbolTable> t (new MLSymbolTable());
	return *t;
}

// ----------------------------------------------------------------
#pragma mark MLSymbol

class MLSymbol
{
	friend std::ostream& operator<< (std::ostream& out, const MLSymbol r);
public:
	
	// creating symbols:
	// Must be reasonably fast.  We will often want to be
	// lazy and write code like getParam("gain"), even in a DSP method. 
	// So the constructor must not allocate any heap memory when looking up
	// a symbol, after the first time a symbol is used.  
	//
	// Where the best possible performance is needed, symbols can be
	// cached like so:
	//
	// void myDSPMethod() {
	//		static const MLSymbol gainSym("gain");
	//		...
	//		getParam(gainSym);
	//
	MLSymbol();
	MLSymbol(const char *sym);
	MLSymbol(const std::string& str);
	
	// compare two symbols:
	// Must be the fastest. used in std:map all over the place.
	// bool operator< (const MLSymbol b) const;
	inline bool operator< (const MLSymbol b) const
	{
#if USE_ALPHA_SORT			
		return (theSymbolTable().getSymbolAlphaOrder(mID) < theSymbolTable().getSymbolAlphaOrder(b.mID));
#else
		return(mID < b.mID);
#endif
	}
	
	inline bool operator== (const MLSymbol b) const
	{
		return (mID == b.mID);
	}	
	
	operator bool() const { return mID != 0; }
	int getID() const { return mID; }
	const std::string& getString() const;
	
	bool beginsWith (const MLSymbol b) const;
	bool endsWith (const MLSymbol b) const;
	bool hasWildCard() const;
	MLSymbol withWildCardNumber(int n) const;
	MLSymbol withFinalNumber(int n) const;
	MLSymbol withoutFinalNumber() const;
	int getFinalNumber() const;	
	int compare(const char *str) const;
	
private:
	
	// the ID equals the order in which the symbol was created.
	int mID;
};

std::ostream& operator<< (std::ostream& out, const MLSymbol r);

// hashing function for MLSymbol use in unordered STL containers. simply return the ID,
// which will give each MLSymbol a unique hash.
namespace std
{
	template<>
	struct hash<MLSymbol>
	{
		std::size_t operator()(MLSymbol const& s) const
		{
			return s.getID();
		}
	};
}

// ----------------------------------------------------------------
#pragma mark MLNameMaker
// a utility to make many short, unique, human-readable names when they are needed. 

class MLNameMaker
{
public:
	MLNameMaker() : index(0) {};
	~MLNameMaker() {};
	
	// return the next name in the sequence as a string. 
	std::string nextNameAsString();
	
	// return the next name as a symbol, having added it to the symbol table. 
	const MLSymbol nextName();
	
private:
	int index;
};



#endif // _ML_SYMBOL_H

