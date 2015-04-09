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
#include <unordered_map>
#include <boost/any.hpp>

class BDecoder {
	std::string encodedString;
	int position;
	boost::any decodedObject;

	bool decodeInteger(boost::any & localOutput);
	bool decodeString(boost::any & localOutput);
	bool decodeList(boost::any & localOutput);
	bool decodeDictionary(boost::any & localOutput);

public:
	BDecoder( const std::string &input, int position) {
		this->encodedString = input;
		this->position = position;
	}
	bool decode(boost::any &localOutput);
	bool decode();
	bool isEmpty();
	boost::any get();
};

#endif
