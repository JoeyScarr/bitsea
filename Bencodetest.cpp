#include "Bencoding.hpp"

int main() {
	std::string stringtest = "3:hogAB";
	std::string numbertest = "i30eA";
	std::string numbertest2 = "i00e";
	std::string listtest= "l3:hogi30el2:bi3:bioi1eee";
	std::string ltest = "l3:hog2:go5:abcdei10ee";
	
	std::shared_ptr<std::string> test1 (std::make_shared<std::string>(stringtest));
	std::shared_ptr<std::string> test2 (std::make_shared<std::string>(numbertest));
	std::shared_ptr<std::string> test3 (std::make_shared<std::string>(listtest));
	std::shared_ptr<std::string> test4 (std::make_shared<std::string>(ltest));

	
	BDecoder t1(test1, 0);
	t1.decode();
	std::cout << *shared_string(t1.output->object) << std::endl;
	
	//BDecoder t2(test2, 0);
	//t2.decode();
	//std::cout << *shared_int(t2.output->object) << std::endl;

	//BDecoder t3(test3, 0);
	//t3.decode();
	//Bcode_vec_ptr blah = (shared_Bcode_vec (t3.output->object));
	//Bcode tt1 = (*blah)[2];
	//std::cout << "Type: " << tt1.type << std::endl;
	//Bcode_vec_ptr blah2 = (shared_Bcode_vec (tt1.object));
	//std::cout << blah2->size() << std::endl;
	
	//BDecoder t4(test4,0);
	//t4.decode();
	//Bcode_vec_ptr blah = (shared_Bcode_vec(t4.output->object));
	//std::cout << blah->size() << std::endl;
	//Bcode bing = (*blah)[3];
	//std::cout << *shared_int(bing.object) << std::endl;
}
