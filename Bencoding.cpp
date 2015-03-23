// Bencoding.cpp
#include "Bencoding.hpp"

std::vector<std::string> &BDecoder::split(const std::string &s, const char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    
    while (std::getline(ss, item, delim))
        elems.push_back(item);

    return elems;
}

std::vector<std::string> BDecoder::split(const std::string &s, const char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

bool BDecoder::isEmpty() {
	if(inputLength == 0) {
		return true;
	}
	return false;
}

bool BDecoder::decode(Bcode_ptr localOutput) {
	
	char command = (*input)[position];
	bool (BDecoder::*decoder)(Bcode_ptr localOutput); 
	
	if(isEmpty())
		return false;
		
	if( isdigit(command) )
		command = 's';
	else if(!isalpha(command))
		return false;

	switch(command) {
		case 's':
			decoder = &BDecoder::decodeString;
			break;
		case 'i':
			decoder = &BDecoder::decodeInteger;
			break;
		case 'd':
			decoder = &BDecoder::decodeDictionary;
			break;
		case 'l':
			decoder = &BDecoder::decodeList;
			break;
		default:
			return false;
			break;
	}

	bool success = (this->*decoder)(localOutput);
	if(!success)
		return false;
	else
		return true;
}

bool BDecoder::decode() {
	
	char command = (*input)[position];
	bool (BDecoder::*decoder)(Bcode_ptr localOutput); 
	
	if(isEmpty())
		return false;
		
	if( isdigit(command) )
		command = 's';
	else if(!isalpha(command))
		return false;

	switch(command) {
		case 's':
			decoder = &BDecoder::decodeString;
			break;
		case 'i':
			decoder = &BDecoder::decodeInteger;
			break;
		case 'd':
			decoder = &BDecoder::decodeDictionary;
			break;
		case 'l':
			decoder = &BDecoder::decodeList;
			break;
		default:
			return false;
			break;
	}
	
	bool success = (this->*decoder)(output);
	if(!success)
		return false;
	else
		return true;
}

bool BDecoder::decodeString(Bcode_ptr localOutput) {
	int terminalCharacterPosition = (*input).find_first_of(":", position);

	const char *cLength;
	int stringLength;
	try {
		cLength = (input->substr(position, terminalCharacterPosition-position)).c_str();
		stringLength = std::stoi(cLength);
	}
	catch(int e) {
		return false;
	}
	
	localOutput->type = BCODE_STRING;
	localOutput->object = make_string(input->substr(terminalCharacterPosition+1, stringLength));
	position = terminalCharacterPosition+1+stringLength;
	return true;
}

bool BDecoder::decodeInteger(Bcode_ptr localOutput) {
	position++;
	int terminalCharacterPosition = input->find_first_of("e", position);
	
	int number, numStringLength;
	try {
		const char *stringNumber = (input->substr(position, terminalCharacterPosition-position)).c_str();
		number = std::stoi(stringNumber);
		numStringLength = strlen(stringNumber);
	}
	catch(int e) {
		return false;
	}
	
	if(numStringLength > 1 && number == 0)
		return false;
		
	localOutput->type = BCODE_INTEGER;
	localOutput->object = make_int(number);
	position = terminalCharacterPosition+1;
	
	return true;
}

bool BDecoder::decodeList(Bcode_ptr localOutput) {
	position++;
	localOutput->type = BCODE_LIST;
	localOutput->object = make_Bcode_vec();
	while((*input)[position] != 'e') {
		Bcode_ptr currBcode(make_Bcode());
		int success = decode(currBcode);
		if(!success)
			return false;
			
		if( currBcode->type == BCODE_LIST )
			std::cout << "List size: " << shared_Bcode_vec(currBcode->object)->size() << std::endl;
			
		(shared_Bcode_vec(localOutput->object))->push_back(*currBcode);
	}
	position++;
	return true;
}

bool BDecoder::decodeDictionary(Bcode_ptr localOutput) {

	return true;
}
