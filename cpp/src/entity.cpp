#include "entity.h"

Entity::Entity(Simulation& simulation, json11::Json json): 
	simulation(simulation) {
	
	const auto& cond = json["control_behavior"]["circuit_condition"];
	if (cond.type() == json11::Json::OBJECT) {
		decider = std::make_unique<Decider>(simulation, cond);
	}

	name = json["name"].string_value();
	eid = json["entity_number"].number_value();
	
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

void Entity::update() {}

void Entity::print(std::ostream& os) {
	os << "[generic: ";
	if (decider) {
		decider->print(os);
		os << " ";
	}
	os << name << "]";
}

bool Entity::is_input(int cid) {
	return cid == 1;
}

bool Entity::is_fulfilled() {
	if (!decider) {
		return true;
	}
	signal_t in = combine_signals(
		simulation.network_to_signal[
			simulation.get_network(eid, 1, Simulation::RED)], 
		simulation.network_to_signal[
			simulation.get_network(eid, 1, Simulation::GREEN)]
	);
	return decider->is_fulfilled(in);
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

bool ConstantCombinator::is_input(int) {
	return false;
}

void ConstantCombinator::update() {
	for (const auto& kv: constants) {
		for (int col = Simulation::MIN_COLOR; col < Simulation::MAX_COLOR; col++) {
			size_t nid = simulation.get_network(eid, 1, Simulation::Color(col));
			simulation.new_network_signal[nid][kv.first] += kv.second;
		}
	}
}


static int32_t power(int32_t a, int32_t b) {
	if (a == 0) { return 0; }
	if (a == 1) { return 1; }
	if (b < 0) { return 0; }
	if (b == 0) { return 1; }
	int32_t half = power(a, b / 2);
	half *= half;
	if (b % 2 == 1) {
		half *= a;
	}
	return half;
}

ArithmeticCombinator::ArithmeticCombinator(Simulation& simulation, json11::Json json):
	Entity(simulation, json) {
	const auto& cond = json["control_behavior"]["arithmetic_conditions"];
	oper = cond["operation"].string_value();

	if (oper == "*") {
		operation = [](int32_t a, int32_t b){ return a * b; }; 
	}
	else if (oper == "/") {
		operation = [](int32_t a, int32_t b){ return a / b; }; 
	}
	else if (oper == "+") {
		operation = [](int32_t a, int32_t b){ return a + b; }; 
	}
	else if (oper == "-") {
		operation = [](int32_t a, int32_t b){ return a - b; }; 
	}
	else if (oper == "%") {
		operation = [](int32_t a, int32_t b){ return a % b; }; 
	}
	else if (oper == "^") {
		operation = [](int32_t a, int32_t b){ return power(a, b); }; 
	}
	else if (oper == "<<") {
		operation = [](int32_t a, int32_t b){ return a << b; }; 
	}
	else if (oper == ">>") {
		operation = [](int32_t a, int32_t b){ return a >> b; }; 
	}
	else if (oper == "AND") {
		operation = [](int32_t a, int32_t b){ return a & b; }; 
	}
	else if (oper == "OR") {
		operation = [](int32_t a, int32_t b){ return a | b; }; 
	}
	else if (oper == "XOR") {
		operation = [](int32_t a, int32_t b){ return a ^ b; }; 
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

bool ArithmeticCombinator::is_input(int cid) {
	return cid == 1;
}

void ArithmeticCombinator::update() {
	if (left == Simulation::NONE) { return; }
	if (right == Simulation::NONE) { return; }
	if (output == Simulation::NONE) { return; }

	signal_t in = combine_signals(
		simulation.network_to_signal[
			simulation.get_network(eid, 1, Simulation::RED)], 
		simulation.network_to_signal[
			simulation.get_network(eid, 1, Simulation::GREEN)]
	);
	int32_t val2;
	if (right == Simulation::CONSTANT) {
		val2 = constant;
	}
	else {
		val2 = in[right];
	}
	if (left == Simulation::EACH) {
		for (const auto& kv: in) {
			int32_t result = operation(kv.second, val2);
			for (int col = Simulation::MIN_COLOR; col < Simulation::MAX_COLOR; col++) {
				size_t nid = simulation.get_network(eid, 2, Simulation::Color(col));
				if (output == Simulation::EACH) {
					simulation.new_network_signal[nid][kv.first] += result;
				}
				else {
					simulation.new_network_signal[nid][output] += result;
				}
			}
		}
	}
	else {
		int32_t result = operation(in[left], val2);
		for (int col = Simulation::MIN_COLOR; col < Simulation::MAX_COLOR; col++) {
			size_t nid = simulation.get_network(eid, 2, Simulation::Color(col));
			simulation.new_network_signal[nid][output] += result;
		}
	}
}


Decider::Decider(Simulation& simulation, json11::Json json): 
	simulation(simulation) {

	oper = json["comparator"].string_value();

	if (oper == "<") {
		comparator = [](int32_t a, int32_t b){ return a < b; }; 
	}
	else if (oper == ">") {
		comparator = [](int32_t a, int32_t b){ return a > b; }; 
	}
	else if (oper == "=") {
		comparator = [](int32_t a, int32_t b){ return a == b; }; 
	}
	else if (oper == "≥") {
		comparator = [](int32_t a, int32_t b){ return a >= b; }; 
	}
	else if (oper == "≤") {
		comparator = [](int32_t a, int32_t b){ return a <= b; }; 
	}
	else if (oper == "≠") {
		comparator = [](int32_t a, int32_t b){ return a != b; }; 
	}
	else {
		throw std::runtime_error("Unknown comparator " + oper); 
		comparator = [this](int32_t, int32_t){ return false; };
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

bool Decider::is_fulfilled(signal_t signal, resource_t res) {
	if (left == Simulation::NONE) { return false; }
	if (right == Simulation::NONE) { return false; }

	int32_t val2;
	if (right == Simulation::CONSTANT) {
		val2 = constant;
	}
	else {
		val2 = signal[right];
	}

	if (left == Simulation::ANY) {
		for (const auto& kv: signal) {
			if(comparator(kv.second, val2)) {
				return true;
			}
		}
		return false;
	}
	else if (left == Simulation::EVERY) {
		for (const auto& kv: signal) {
			if(!comparator(kv.second, val2)) {
				return false;
			}
		}
		return true;
	}
	else if (left == Simulation::EACH) {
		return comparator(signal[res], val2);
	}
	else {
		return comparator(signal[left], val2);
	}
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

bool DeciderCombinator::is_input(int cid) {
	return cid == 1;
}

void DeciderCombinator::update() {
	if (output == Simulation::NONE) { return; }

	std::cout << "Decider " << eid << std::endl;

	signal_t in = combine_signals(
		simulation.network_to_signal[
			simulation.get_network(eid, 1, Simulation::RED)], 
		simulation.network_to_signal[
			simulation.get_network(eid, 1, Simulation::GREEN)]
	);

	if (decider.left == Simulation::EACH) {
		for (const auto& kv: in) {
			if (decider.is_fulfilled(in, kv.first)) {
				for (int col = Simulation::MIN_COLOR; col < Simulation::MAX_COLOR; col++) {
					size_t nid = simulation.get_network(eid, 2, Simulation::Color(col));
					int32_t result = 1;
					if (copy) {
						result = kv.second;
					}
					if (output == Simulation::EACH) {
						simulation.new_network_signal[nid][kv.first] += result;
					}
					else {
						simulation.new_network_signal[nid][output] += result;
					}
				}
			}
		}
	}
	else {
		std::cout << "  condition: " << decider.is_fulfilled(in) << std::endl;
		if (decider.is_fulfilled(in)) {
			for (int col = Simulation::MIN_COLOR; col < Simulation::MAX_COLOR; col++) {
				size_t nid = simulation.get_network(eid, 2, Simulation::Color(col));
				if (output == Simulation::EVERY) {
					for (const auto& kv: in) {
						int32_t result = 1;
						if (copy) {
							result = kv.second;
						}
						simulation.new_network_signal[nid][kv.first] += result;
					}
				}
				else {
					int32_t result = 1;
					if (copy) {
						result = in[output];
					}
					simulation.new_network_signal[nid][output] += result;
				}
			}
		}
	}
}
