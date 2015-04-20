// Bencoding.cpp
#include "Bencoding.hpp"

bool BDecoder::isEmpty() {
	return encodedString.length() == 0;
}

bool BDecoder::decode() {
	return decode(decodedObject);
}

bool BDecoder::decode(boost::any &localOutput) {
	char command = encodedString[position];

	if(isEmpty()) {
		return false;
	}
	else if( isdigit(command) ) {
		command = 's';
	}
	else if( !isalpha(command) )
		return false;

	bool success;
	switch(command) {
		case 's':
			success = decodeString(localOutput);
			break;
		case 'i':
			success = decodeInteger(localOutput);
			break;
		case 'l':
			success = decodeList(localOutput);
			break;
		case 'd':
			success = decodeDictionary(localOutput);
			break;
		default:
			success = false;
			break;
	}
	
	return success;
}

bool BDecoder::decodeString(boost::any & localOutput) {
	int terminalCharacterPosition = encodedString.find_first_of(":", position);
	const char *cLength;
	int stringLength;
	
	try {
		cLength = (encodedString.substr(position, terminalCharacterPosition-position)).c_str();
		stringLength = std::stoi(cLength);
	}
	catch(int e) {
		return false;
	}
	
	localOutput = encodedString.substr(terminalCharacterPosition+1, stringLength);
	position = terminalCharacterPosition+1+stringLength;
	return true;
}

bool BDecoder::decodeInteger(boost::any & localOutput) {
	position++;
	int terminalCharacterPosition = encodedString.find_first_of("e", position);
	
	int number, numStringLength;
	try {
		const char *stringNumber = (encodedString.substr(position, terminalCharacterPosition-position)).c_str();
		number = std::stoi(stringNumber);
		numStringLength = strlen(stringNumber);
	}
	catch(int e) {
		return false;
	}
	
	if(numStringLength > 1 && number == 0)
		return false;
		
	localOutput = number;
	position = terminalCharacterPosition+1;
	
	return true;
}

bool BDecoder::decodeList(boost::any & localOutput) {
	position++;
	std::vector<boost::any> list;
	while(encodedString[position] != 'e') {
		boost::any currentBcode;
		int success = decode(currentBcode);
		if(!success)
			return false;
		list.push_back(currentBcode);
	}
	localOutput = list;
	position++;
	return true;
}

bool BDecoder::decodeDictionary(boost::any & localOutput) {
	position++;
	std::unordered_map<std::string, boost::any> dict;
	
	while(encodedString[position] != 'e') {
		boost::any key;
		boost::any value;
		int startPos, endPos;

		bool status1 = decodeString(key);
		startPos = position;
		bool status2 = decode(value);
		endPos = position;
		
		if(boost::any_cast<std::string>(key).compare("info") == 0 ) {
			infoDictPosition = std::make_pair(startPos, endPos);
		}
		
		if(!status1 || !status2)
			return false;
		
		dict.insert(std::pair<std::string,boost::any>(boost::any_cast<std::string>(key),value));
	}
	localOutput = dict;
	position++;
	return true;
}

boost::any BDecoder::get() {
	return decodedObject;
}

std::string BDecoder::getRawInfoDict() {
	int start = infoDictPosition.first;
	int end = infoDictPosition.second;

	return (encodedString.substr(start, end-start));
}

bool BDecoder::isString(const boost::any &inputDict, const char input[]) {
	const std::string strInput(input);
	std::unordered_map<std::string,boost::any> dictionary = boost::any_cast<std::unordered_map<std::string,boost::any>>(inputDict);
	std::unordered_map<std::string,boost::any>::const_iterator lookup = dictionary.find(strInput);
			
	if(lookup == dictionary.end())
		throw 0;
	if(lookup->second.type() == typeid(std::string))
		return true;
	else
		return false;
}
