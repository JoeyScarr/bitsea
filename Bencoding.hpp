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
private:
	std::string encodedString;
	int position;
	boost::any decodedObject;
	std::pair<int,int> infoDictPosition;

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
	std::string getRawInfoDict();
	static bool isString(const boost::any &inputDict, const char input[]);
	
	template<typename T>
	static T get(const boost::any &inputDict, const char input[]) {
		const std::string strInput(input);
		std::unordered_map<std::string,boost::any> dictionary = boost::any_cast<std::unordered_map<std::string,boost::any>>(inputDict);
		std::unordered_map<std::string,boost::any>::const_iterator lookup = dictionary.find(strInput);
			
		if(lookup == dictionary.end())
			throw 0;
					
		return boost::any_cast<T>(lookup->second);
	}
	
	
};

#endif
