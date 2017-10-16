#include "lib/base64.h"
#include "lib/json11.hpp"

#include <zlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <unordered_map>


typedef uint16_t resource_t;
typedef std::unordered_map<resource_t, int32_t> signal_t;

class Entity;
class Simulation {
public:
	enum Special {
		CONSTANT, EACH, ANY, EVERY, NONE,
	};
	Simulation(const std::string& blueprint_string);
	resource_t get_resource_id(std::string name);
	std::string get_resource_name(resource_t id);
private:
	json11::Json read_blueprint(const std::string& b64);

	typedef uint8_t byte;

	std::unordered_map<std::string, resource_t> resource_to_id;
	std::vector<std::string> resource_names;

	std::vector<std::unique_ptr<Entity>> entities;
};


class Entity {
public:
	Entity(Simulation& simulation, json11::Json json): 
		simulation(simulation), json(json) {}
	virtual void update(){}
	virtual void print(std::ostream& os) {
		os << "[generic: " << json["name"].string_value() << "]";
	}
protected:
	Simulation& simulation;
	json11::Json json;
};


std::ostream& operator<<(std::ostream& os, Entity& e) {
	e.print(os);
	return os;
}


void print_signal(std::ostream& os, const signal_t& sig, Simulation& simulation) {
	os << "{";
	for (const auto& kv: sig) {
		os << simulation.get_resource_name(kv.first) << ": " << kv.second << ", ";
	}
	os << "}";
}


class ConstantCombinator: public Entity {
public:
	ConstantCombinator(Simulation& simulation, json11::Json json):
	   	Entity(simulation, json) {
		for (const auto& sig: json["control_behavior"]["filters"].array_items()) {
			resource_t id = simulation.get_resource_id(
					sig["signal"]["name"].string_value());
			constants[id] += sig["count"].number_value();
		}
	}
	virtual void print(std::ostream& os) {
		os << "[constant: ";
		print_signal(os, constants, simulation);
		os << "]";
	}
private:
	signal_t constants;
};


class ArithmeticCombinator: public Entity {
public:
	ArithmeticCombinator(Simulation& simulation, json11::Json json):
	   	Entity(simulation, json) {
		valid = true;
		const auto& cond = json["control_behavior"]["arithmetic_conditions"];
		std::string oper = cond["operation"].string_value();

		if (oper == "+") {
		   	operation = [](int32_t a, int32_t b){ return a + b; }; 
		}
		else {
			valid = false;
		}

		if (cond.object_items().count("first_signal")) {
			left = simulation.get_resource_id(cond["first_signal"]["name"]
					.string_value());
		}
		else {
			left = simulation.get_resource_id("none");
			valid = false;
		}

		if (cond.object_items().count("second_signal")) {
			right = simulation.get_resource_id(cond["second_signal"]["name"]
					.string_value());
		}
		else if (cond.object_items().count("constant")) {
			right = simulation.get_resource_id("constant");
			constant = cond["constant"].number_value();
		}
		else {
			right = simulation.get_resource_id("none");
			valid = false;
		}

		if (cond.object_items().count("output_signal")) {
			output = simulation.get_resource_id(cond["output_signal"]["name"]
					.string_value());
		}
		else {
			output = simulation.get_resource_id("none");
			valid = false;
		}
	}
	virtual void print(std::ostream& os) {
		os 
			<< "[arithmetic: " 
			<< simulation.get_resource_name(left) << " " 
			<< json["control_behavior"]["arithmetic_conditions"]["operation"]
				.string_value()
			<< " " << simulation.get_resource_name(right);
		if (right == simulation.CONSTANT) {
			os << "(" << constant << ")";
		}
		os
			<< " -> "
			<< simulation.get_resource_name(output) 
			<< "]";

		if (!valid) {
			os << " (invalid)";
		}
	}
private:
	bool valid;
	resource_t left, right, output;
	int32_t constant;
	std::function<int32_t(int32_t, int32_t)> operation;
};


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

int main() {
	std::string blueprint_string;
	std::cin >> blueprint_string;
	Simulation simulation(blueprint_string);
}
