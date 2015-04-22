#include "Bencoding.hpp"
#include "TorrentFileParser.hpp"
#include "Tracker.hpp"

int main() {
	//std::string stringtest = "3:hogAB";
	//std::string numbertest = "i30eA";
	//std::string numbertest2 = "i00e";
	//std::string listtest= "l3:hogi30el2:bi3:bioi1eee";
	//std::string ltest = "l3:hog2:go5:abcdee";
	//std::string ltest2 = "l3:hog2:go5:abcdei10ee";
    //std::string dic1= "d3:cow3:moo4:spam4:eggse"; // represents the dictionary { "cow" => "moo", "spam" => "eggs" }
    //std::string dic2="d4:spaml1:a1:bee"; // represents the dictionary { "spam" => [ "a", "b" ] }
    //std::string dic3="d9:publisher3:bob17:publisher-webpage15:www.example.com18:publisher.location4:homee"; // represents { "publisher" => "bob", "publisher-webpage" => "www.example.com", "publisher.location" => "home" }
    //std::string dic4="de"; // represents an empty dictionary {} 
	
	// String test	
	//BDecoder t1(stringtest, 0);
	//t1.decode();
	//std::cout << boost::any_cast<std::string>(t1.get()) << std::endl;
	
	// Integer test
	//BDecoder t2(numbertest,0);
	//std::cout << t2.decode() << std::endl;
	//std::cout << boost::any_cast<int>(t2.get()) << std::endl;

	//BDecoder t3(numbertest2,0);
	//std::cout << t3.decode() << std::endl;

	// List test
	//BDecoder t4(listtest,0);
	//t4.decode();
	//std::vector<boost::any> list = boost::any_cast<std::vector<boost::any>>(t4.get());
	//std::cout << list.size() << std::endl;
	//std::cout << (list[0].type() == typeid(std::string)) << std::endl;
	
	// Dictionary test
	//BDecoder t5(dic1,0);
	//std::cout << t5.decode() << std::endl;
	//std::unordered_map<std::string,boost::any> dict = boost::any_cast<std::unordered_map<std::string,boost::any>>(t5.get());
	////std::cout << dict.size() << std::endl;
	//std::unordered_map<std::string,boost::any>::const_iterator got = dict.find ("cow");
	//std::cout << boost::any_cast<std::string>(got->second) << std::endl;
	
	//BDecoder t6(dic2,0);
	//std::cout << t6.decode() << std::endl;
	//std::unordered_map<std::string,boost::any> dict = boost::any_cast<std::unordered_map<std::string,boost::any>>(t6.get());
	////std::cout << dict.size() << std::endl;
	//std::unordered_map<std::string,boost::any>::const_iterator got = dict.find ("spam");
	//std::vector<boost::any> dictlist = boost::any_cast<std::vector<boost::any>>(got->second);
	//std::cout << boost::any_cast<std::string>(dictlist[1]) << std::endl;
	
	//BDecoder t7(dic4,0);
	//std::cout << t7.decode() << std::endl;
	//std::unordered_map<std::string,boost::any> dict = boost::any_cast<std::unordered_map<std::string,boost::any>>(t7.get());
	//std::cout << dict.size() << std::endl;
	
	std::string singleFile("test.torrent");
	TorrentFileParser single(singleFile);
	//std::cout << "getPieces: " << single.info.getPieces() << std::endl;
	//std::cout << "getPieceLength: " << single.info.getPieceLength() << std::endl;
	//std::cout << "getName: " << single.info.getName() << std::endl;
	//std::cout << "getPrivate: " << single.info.getPrivate() << std::endl;
	//std::cout << "getLength: " << single.info.getLength() << std::endl;
	//std::cout << "getMD5: " << single.info.getMD5() << std::endl;
	//std::cout << "getNumberOfFiles: " << single.info.getNumberOfFiles() << std::endl;
	//std::cout << "getRawInfoDict: " << single.info.string << std::endl;
	//std::cout << "Info: getHash: " << single.info.getHash() << " of length " << single.info.getHash().length() << std::endl;
	std::string testURL("test");
	std::string infoHash = single.info.getHash();
	Tracker trackHandler(testURL, infoHash);
	//std::cout << "URL encoded hash: " <<  trackHandler.urlEncode(single.info.getHash()) << std::endl;

	//std::string multipleFile("multi.torrent");
	//TorrentFileParser multi(multipleFile);
	//std::cout << "getPieces: " << multi.info.getPieces() << std::endl;
	//std::cout << "getPieceLength: " << multi.info.getPieceLength() << std::endl;
	//std::cout << "getName: " << multi.info.getName() << std::endl;
	//std::cout << "getPrivate: " << multi.info.getPrivate() << std::endl;
	//std::cout << "getLength: " << multi.info.getLength() << std::endl;
	//std::cout << "getMD5: " << multi.info.getMD5() << std::endl;
	//std::cout << "getNumberOfFiles: " << multi.info.getNumberOfFiles() << std::endl;

	//int i=1;
	//for(boost::any file : multi.info.files) {
		//std::cout << "File " << i++ << std::endl;
		//std::unordered_map<std::string, boost::any> dict = boost::any_cast<std::unordered_map<std::string, boost::any>>(file);
		//std::cout << "length=" << InfoParser::fileLength(dict) << std::endl;
		//std::cout << "path=" << InfoParser::filePath(dict) << std::endl;
		//std::cout << "md5=" << InfoParser::fileMD5(dict) << std::endl;
	//}
	
}
