// Bencoding.hpp
#ifndef BENCODING_H
#define BENCODING_H

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <vector>
#include <memory>
#include <cstring>

enum BCODE {BCODE_INTEGER, BCODE_STRING, BCODE_LIST, BCODE_DICTIONARY };

struct Bcode {
	BCODE type;
	std::shared_ptr<void> object;
};

#define shared_Bcode_vec std::static_pointer_cast<std::vector<Bcode>>
#define shared_Bcode std::static_pointer_cast<Bcode>
#define shared_int std::static_pointer_cast<int>
#define shared_string std::static_pointer_cast<std::string>
#define make_Bcode_vec std::make_shared<std::vector<Bcode>>
#define make_Bcode std::make_shared<Bcode>
#define make_string std::make_shared<std::string>
#define make_int std::make_shared<int>

template <typename T> using s_ptr = std::shared_ptr<T>; // remember for future
typedef std::shared_ptr<std::vector<Bcode>> Bcode_vec_ptr;
typedef std::shared_ptr<Bcode> Bcode_ptr;
typedef std::shared_ptr<std::string> string_ptr;
typedef std::shared_ptr<int> int_ptr;

class BDecoder {
	std::shared_ptr<std::string> input;
	int position;
	int inputLength;
	
	std::vector<std::string> split(const std::string &s, const char delim);
	std::vector<std::string> &split(const std::string &s, const char delim, std::vector<std::string> &elems);
	bool decodeInteger(Bcode_ptr localOutput);
	bool decodeString(Bcode_ptr localOutput);
	bool decodeList(Bcode_ptr localOutput);
	bool decodeDictionary(Bcode_ptr localOutput);

public:
	std::shared_ptr<Bcode> output;

	BDecoder( string_ptr input, int position) {
		this->input = input;
		this->position = position;
		inputLength = input->length();
		output = Bcode_ptr(make_Bcode());

	}
	bool decode();
	bool decode(Bcode_ptr localOutput);
	bool isEmpty();

};


#endif
