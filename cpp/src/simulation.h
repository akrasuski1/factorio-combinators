#ifndef SIMULATION_H_
#define SIMULATION_H_

#include "common.h"

#include "lib/json11.hpp"


class Entity;
class Simulation {
public:
	enum Special {
		CONSTANT, EACH, ANY, EVERY, NONE,
	};
	Simulation(const std::string& blueprint_string);
	~Simulation();
	resource_t get_resource_id(std::string name);
	std::string get_resource_name(resource_t id);
private:
	json11::Json read_blueprint(const std::string& b64);

	typedef uint8_t byte;

	std::unordered_map<std::string, resource_t> resource_to_id;
	std::vector<std::string> resource_names;

	std::vector<std::unique_ptr<Entity>> entities;
};

#endif
