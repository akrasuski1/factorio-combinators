#include "lib/base64.h"
#include "lib/json11.hpp"

#include <zlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>


typedef unsigned char byte;

json11::Json read_blueprint(std::string b64){
	std::string zlibbed = base64_decode(b64.substr(1));

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	inflateInit(&stream);

	std::vector<byte> bufin(zlibbed.begin(), zlibbed.end());
	std::vector<byte> bufout(1024);
	std::vector<byte> result;

	stream.avail_in = bufin.size();
	stream.next_in = bufin.data();
	do {
		stream.avail_out = bufout.size();
		stream.next_out = bufout.data();
		int ret = inflate(&stream, Z_NO_FLUSH);
		if(ret < 0) {
			std::cerr << ret << std::endl;
			throw std::runtime_error("Compression error");
		}
		size_t have = bufout.size() - stream.avail_out;
		result.insert(result.end(), bufout.begin(), bufout.begin() + have);
	} while (stream.avail_out == 0);
	std::string err;
	return json11::Json::parse(
		std::string(result.begin(), result.end()), err);
}


int main(){
	std::string blueprint_string;
	std::cin >> blueprint_string;
	auto json = read_blueprint(blueprint_string);
	std::cout << json.type() <<std::endl;
}
