#include "simulation.h"

#include "entity.h"

#include "lib/base64.h"

#include <zlib.h>

#include <iostream>
#include <cstring>
#include <queue>
#include <set>
#include <unordered_set>
#include <algorithm>


Simulation::Simulation(const std::string& blueprint_string):
	tick_number(0) {

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
		else if (name == "decider-combinator") {
			entities[eid] = std::make_unique<DeciderCombinator>(*this, e);
		}
		else {
			entities[eid] = std::make_unique<Entity>(*this, e);
		}
	}

	recalculate_networks();
	for (size_t nid = 0; nid < network_to_signal.size(); nid++) {
		triggered_networks.insert(nid);
	}

	for (size_t i = 0; i < entities.size(); i++) {
		if (entities[i]) {
			std::cout << i << ":\t" << *entities[i] << std::endl;
			for (const auto& kv: entities[i]->edges) {
				for (const auto& to: kv.second) {
					std::cout << "\t" << kv.first.first << " --";
					std::cout << color_to_string(kv.first.second) << "--> ";
					std::cout << to.first << ", " << to.second;
					std::cout << std::endl;
				}
			}
		}
	}

	for (size_t i = 0; i < network_to_endpoints.size(); i++) {
		if (network_to_endpoints[i].size() <= 1) {
			continue;
		}
		std::cout << "Network #" << i << " - " 
			<< color_to_string(std::get<2>(network_to_endpoints[i][0])) 
			<< ":" << std::endl;
		for (const auto& ep: network_to_endpoints[i]) {
			std::cout << "    " << std::get<0>(ep) << "-" << std::get<1>(ep) << std::endl;
		}
	}
}

void Simulation::recalculate_networks() {
	for (int cid = MIN_CID; cid < MAX_CID; cid++) {
		for (int col = MIN_COLOR; col < MAX_COLOR; col++) {
			endpoint_to_network[cid][col].clear();
			endpoint_to_network[cid][col].resize(entities.size());
		}
	}
	network_to_endpoints.clear();

	std::set<std::tuple<size_t, int, Color>> visited;
	for (size_t eid = 0; eid < entities.size(); eid++) {
		if (!entities[eid]) { continue; }
		for (int cid = MIN_CID; cid < MAX_CID; cid++) {
			for (int col = MIN_COLOR; col < MAX_COLOR; col++) {
				std::tuple<size_t, int, Color> endpoint(eid, cid, Color(col));
				if (visited.count(endpoint)) {
					continue;
				}
				size_t nid = network_to_endpoints.size();
				std::queue<std::tuple<size_t, int, Color>> q;
				q.push(endpoint);
				visited.insert(endpoint);
				network_to_endpoints.resize(nid + 1);
				while (!q.empty()) {
					auto cur = q.front();
					size_t eid_cur;
					int cid_cur;
					Color col_cur;
					std::tie(eid_cur, cid_cur, col_cur) = cur;
					q.pop();
					network_to_endpoints.back().push_back(cur);
					endpoint_to_network[cid_cur][col_cur][eid_cur] = nid;
					for (const auto& to: entities[eid_cur]->edges[{cid_cur, Color(col)}]) {
						size_t eid_to;
						int cid_to;
						std::tie(eid_to, cid_to) = to;
						std::tuple<size_t, int, Color> endpoint_to(
								eid_to, cid_to, Color(col));

						if (visited.count(endpoint_to)) {
							continue;
						}
						visited.insert(endpoint_to);
						q.push(endpoint_to);
					}
				}
			}
		}
	}

	network_to_signal.resize(network_to_endpoints.size());
}

size_t Simulation::get_network(size_t eid, int cid, Color color) {
	if (cid < MIN_CID || cid >= MAX_CID) {
		throw std::runtime_error("Connection id out of range");
	}
	if (color >= MAX_COLOR) {
		throw std::runtime_error("Color enum out of range");
	}
	const auto& vec = endpoint_to_network[cid][color];
	return vec.at(eid);
}


// std::vector<size_t> endpoint_to_network[MAX_CID][MAXCOLOR];
// std::vector<std::tuple<size_t, int, Color>> network_to_endpoint;

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

Simulation::Color Simulation::string_to_color(std::string str) {
	if (str == "red") { return RED; }
	if (str == "green") { return GREEN; }
	throw std::runtime_error("Bad color: " + str);
}

std::string Simulation::color_to_string(Color color) {
	switch (color) {
	case RED: return "red";
	case GREEN: return "green";
	default: throw std::runtime_error("Invalid color enum");
	}
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

void Simulation::tick() {
	std::set<size_t> triggered_entities;
	for (const auto& nid: triggered_networks) {
		for (auto endpoint: network_to_endpoints[nid]) {
			size_t eid = std::get<0>(endpoint);
			int cid = std::get<1>(endpoint);
			if (entities[eid]->is_input(cid)) {
				triggered_entities.insert(eid);
			}
		}
	}

	if (tick_number == 0) {
		for (size_t eid = 0; eid < entities.size(); eid++) {
			if (entities[eid]) {
				triggered_entities.insert(eid);
			}
		}
	}

	network_diff.clear();
	for (auto eid: triggered_entities) {
		for (int cid = MIN_CID; cid < MAX_CID; cid++) {
			for (int col = MIN_COLOR; col < MAX_COLOR; col++) {
				size_t nid = get_network(eid, cid, Color(col));
				signal_inplace_subtract(network_diff[nid], entities[eid]->outputs[cid]);
			}
		}
		entities[eid]->update();
		for (int cid = MIN_CID; cid < MAX_CID; cid++) {
			for (int col = MIN_COLOR; col < MAX_COLOR; col++) {
				size_t nid = get_network(eid, cid, Color(col));
				signal_inplace_add(network_diff[nid], entities[eid]->outputs[cid]);
			}
		}
	}

	triggered_networks.clear();
	for (const auto& nid_diff: network_diff) {
		bool any = false;
		size_t nid = nid_diff.first;
		signal_t& signal = network_to_signal[nid];
		std::vector<resource_t> todel;
		for (auto res_val: nid_diff.second) {
			if (res_val.second != 0) {
				any = 1;
				signal[res_val.first] += res_val.second;
				if (signal[res_val.first] == 0) {
					todel.push_back(res_val.first);
				}
			}
		}
		for (auto res: todel) {
			signal.erase(res);
		}
		if (any) {
			triggered_networks.insert(nid);
		}
	}

	/*
	for (size_t i = 0; i < network_to_signal.size(); i++) {
		if (network_to_signal[i].size() == 0) {
			continue;
		}
		std::cout << "Network #" << i << std::endl;
		for (const auto& sig_val: network_to_signal[i]) {
			std::cout << "  " << get_resource_name(sig_val.first) 
				<< ": " << sig_val.second << std::endl;
		}
	}
	*/

	std::vector<std::pair<double, bool>> lamp_x_state;
	static std::vector<std::pair<double, bool>> old_state;
	for (const auto& e: entities) {
		if (e && e->name == "small-lamp") {
			lamp_x_state.push_back({e->x, e->is_fulfilled()});
		}
	}
	std::sort(lamp_x_state.begin(), lamp_x_state.end());
	if (old_state != lamp_x_state || tick_number == 49999) {
		std::cout << "Tick " << (tick_number + 1) << ": \tlamps: ";
		for (const auto& xs: lamp_x_state) {
			std::cout << (xs.second ? "# " : ". ");
		}
		std::cout << std::endl;
		old_state = lamp_x_state;
	}

	tick_number++;
}
