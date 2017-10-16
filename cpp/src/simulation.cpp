#include "simulation.h"

#include "entity.h"

#include "lib/base64.h"

#include <zlib.h>

#include <iostream>
#include <cstring>

Simulation::Simulation(const std::string& blueprint_string) {
	get_resource_id("constant");
	get_resource_id("signal-each");
	get_resource_id("signal-anything");
	get_resource_id("signal-everything");
	get_resource_id("none");

	auto json = read_blueprint(blueprint_string);
	const auto& json_entities = json["blueprint"]["entities"].array_items();
	for (const auto& e: json_entities) {
		size_t eid = e["entity_number"].number_value();
		if (entities.size() <= eid) {
			entities.resize(eid + 1);
		}
		std::string name = e["name"].string_value();
		if (name == "constant-combinator") {
			entities[eid] = std::make_unique<ConstantCombinator>(*this, e);
		}
		else if (name == "arithmetic-combinator") {
			entities[eid] = std::make_unique<ArithmeticCombinator>(*this, e);
		}
		else {
			entities[eid] = std::make_unique<Entity>(*this, e);
		}
	}
	for (size_t i = 0; i < entities.size(); i++) {
		if (entities[i]) {
			std::cout << i << ":\t" << *entities[i] << std::endl;
		}
	}
}

Simulation::~Simulation() {}

resource_t Simulation::get_resource_id(std::string name) {
	if (resource_to_id.count(name)) {
		return resource_to_id[name];
	}
	resource_t id = resource_names.size();
	resource_to_id[name] = id;
	resource_names.push_back(name);
	std::cout << id << " : " << name << std::endl;
	return id;
}

std::string Simulation::get_resource_name(resource_t id) {
	return resource_names.at(id);
}

json11::Json Simulation::read_blueprint(const std::string& b64) {
	std::string zlibbed = base64_decode(b64.substr(1));

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	inflateInit(&stream);

	std::vector<byte> bufin(zlibbed.begin(), zlibbed.end());
	std::vector<byte> bufout(1 << 16);
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
