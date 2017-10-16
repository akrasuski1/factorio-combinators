#include "entity.h"

Entity::Entity(Simulation& simulation, json11::Json json): 
	simulation(simulation) {
	
	const auto& cond = json["control_behavior"]["circuit_condition"];
	if (cond.type() == json11::Json::OBJECT) {
		decider = std::make_unique<Decider>(simulation, cond);
	}

	name = json["name"].string_value();
	
	for (const auto& kv: json["connections"].object_items()) {
		int cid = std::stoi(kv.first);
		for (const auto& cv: kv.second.object_items()) {
			Simulation::Color col = Simulation::string_to_color(cv.first);
			for (const auto& conn_json: cv.second.array_items()) {
				auto conn = conn_json.object_items();
				size_t eid_to = conn["entity_id"].number_value();
				int cid_to = 1;
				if (conn.count("circuit_id")) {
					cid_to = conn["circuit_id"].number_value();
				}
				edges[{cid, col}].push_back({eid_to, cid_to});
			}
		}
	}
}

void Entity::update(){}

void Entity::print(std::ostream& os) {
	os << "[generic: ";
	if (decider) {
		decider->print(os);
		os << " ";
	}
	os << name << "]";
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
	const auto& cond = json["control_behavior"]["arithmetic_conditions"];
	oper = cond["operation"].string_value();

	if (oper == "+") {
		operation = [](int32_t a, int32_t b){ return a + b; }; 
	}
	else {
		operation = [this](int32_t, int32_t) -> int32_t {
		   	throw std::runtime_error("Unknown operation" + oper); 
		};
	}

	if (cond.object_items().count("first_signal")) {
		left = simulation.get_resource_id(cond["first_signal"]["name"]
				.string_value());
	}
	else {
		left = simulation.get_resource_id("none");
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
	}

	if (cond.object_items().count("output_signal")) {
		output = simulation.get_resource_id(cond["output_signal"]["name"]
				.string_value());
	}
	else {
		output = simulation.get_resource_id("none");
	}
}

void ArithmeticCombinator::print(std::ostream& os) {
	os 
		<< "[arithmetic: " 
		<< simulation.get_resource_name(left) << " " << oper
		<< " " << simulation.get_resource_name(right);
	if (right == simulation.CONSTANT) {
		os << "(" << constant << ")";
	}
	os
		<< " -> "
		<< simulation.get_resource_name(output) 
		<< "]";
}

Decider::Decider(Simulation& simulation, json11::Json json): 
	simulation(simulation) {

	oper = json["comparator"].string_value();

	if (oper == ">") {
		comparator = [](int32_t a, int32_t b){ return a > b; }; 
	}
	else {
		comparator = [this](int32_t, int32_t) -> bool {
		   	throw std::runtime_error("Unknown comparator" + oper); 
		};
	}

	if (json.object_items().count("first_signal")) {
		left = simulation.get_resource_id(json["first_signal"]["name"]
				.string_value());
	}
	else {
		left = simulation.get_resource_id("none");
	}

	if (json.object_items().count("second_signal")) {
		right = simulation.get_resource_id(json["second_signal"]["name"]
				.string_value());
	}
	else if (json.object_items().count("constant")) {
		right = simulation.get_resource_id("constant");
		constant = json["constant"].number_value();
	}
	else {
		right = simulation.get_resource_id("none");
	}
}

void Decider::print(std::ostream& os) {
	os 
		<< "[decision: " 
		<< simulation.get_resource_name(left) << " " 
		<< oper << " " 
		<< simulation.get_resource_name(right);
	if (right == simulation.CONSTANT) {
		os << "(" << constant << ")";
	}
	os << "]";
}


DeciderCombinator::DeciderCombinator(Simulation& simulation, json11::Json json):
	Entity(simulation, json), 
	decider(simulation, json["control_behavior"]["decider_conditions"]) {

	const auto& cond = json["control_behavior"]["decider_conditions"];
	copy = cond["copy_count_from_input"].bool_value();

	if (cond.object_items().count("output_signal")) {
		output = simulation.get_resource_id(cond["output_signal"]["name"]
				.string_value());
	}
	else {
		output = simulation.get_resource_id("none");
	}
}

void DeciderCombinator::print(std::ostream& os) {
	os << "[decider: ";
	decider.print(os);
	os << " -> " << simulation.get_resource_name(output) << " ";
	if (copy) {
		os << "(copy)";
	}
	else {
		os << "(1)";
	}
	os << "]";
}
