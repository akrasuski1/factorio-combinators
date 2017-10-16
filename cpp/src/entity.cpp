#include "entity.h"

Entity::Entity(Simulation& simulation, json11::Json json): 
	simulation(simulation), json(json) {}

void Entity::update(){}

void Entity::print(std::ostream& os) {
	os << "[generic: " << json["name"].string_value() << "]";
}

std::ostream& operator<<(std::ostream& os, Entity& e) {
	e.print(os);
	return os;
}


ConstantCombinator::ConstantCombinator(Simulation& simulation, json11::Json json):
	Entity(simulation, json) {
	for (const auto& sig: json["control_behavior"]["filters"].array_items()) {
		resource_t id = simulation.get_resource_id(
				sig["signal"]["name"].string_value());
		constants[id] += sig["count"].number_value();
	}
}

void ConstantCombinator::print(std::ostream& os) {
	os << "[constant: ";
	print_signal(os, constants, simulation);
	os << "]";
}


ArithmeticCombinator::ArithmeticCombinator(Simulation& simulation, json11::Json json):
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

void ArithmeticCombinator::print(std::ostream& os) {
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
