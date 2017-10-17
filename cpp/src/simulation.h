#ifndef SIMULATION_H_
#define SIMULATION_H_

#include "common.h"

#include "lib/json11.hpp"

#include <set>


class Entity;
class Simulation {
public:
	enum Special {
		CONSTANT, EACH, ANY, EVERY, NONE,
	};
	enum Color {
		MIN_COLOR = 0,
		RED = MIN_COLOR, GREEN, 
		MAX_COLOR
	};
	enum Constants {
		MIN_CID = 1, MAX_CID = 3,
	};
	Simulation(const std::string& blueprint_string);
	~Simulation();
	resource_t get_resource_id(std::string name);
	std::string get_resource_name(resource_t id);
	void recalculate_networks();
	size_t get_network(size_t eid, int cid, Color color);

	static Color string_to_color(std::string str);
	static std::string color_to_string(Color col);

	std::vector<signal_t> network_to_signal;
	std::unordered_map<size_t, signal_t> new_network_signal;

	void tick();
private:
	json11::Json read_blueprint(const std::string& b64);

	typedef uint8_t byte;

	std::unordered_map<std::string, resource_t> resource_to_id;
	std::vector<std::string> resource_names;

	std::vector<std::unique_ptr<Entity>> entities;

	std::vector<size_t> endpoint_to_network[MAX_CID][MAX_COLOR];
	std::vector<std::vector<
		std::tuple<size_t, int, Color>>> network_to_endpoints;
	std::set<size_t> triggered_entities;
};

#endif
