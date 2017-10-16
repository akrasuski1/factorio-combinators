import json


COLORS = ["red", "green"]


def parse_blueprint(bpstr):
    return json.loads(bpstr[1:].decode("base64").decode("zlib"))


def pprint(x):
    print json.dumps(x, indent=4)


def init_state(blueprint):
    entities = {} # "5": {whole entity dict}
    edges = {} # "5-1-green": ["2-1-green", "3-2-green"]
    point_to_network = {} # "2-1-green": 10

    for e in blueprint["blueprint"]["entities"]:
        if "connections" not in e:
            continue

        enum = str(e["entity_number"])
        entities[enum] = e

        for cid in e["connections"]:
            cv = e["connections"][cid]

            for color in cv:
                for conn_to in cv[color]:
                    conn_id = enum + '-' + cid + '-' + color
                    if conn_id not in edges:
                        edges[conn_id] = []

                    enum_to = str(conn_to["entity_id"])
                    cid_to = str(conn_to.get("circuit_id", 1))
                    edges[conn_id].append(enum_to + '-' + cid_to + "-" + color)

    netid = "0"
    for point in edges:
        if point in point_to_network:
            continue
        
        point_to_network[point] = netid
        queue = [point]

        while len(queue):
            cur = queue.pop()
            for to in edges[cur]:
                if to in point_to_network:
                    continue

                point_to_network[to] = netid
                queue.append(to)

        netid = str(int(netid) + 1)

    return {
        "entities": entities, 
        "point_to_network": point_to_network,
        "network_state": {},
    }


def add(future_state, nid, what, many):
    if nid is None:
        return

    if nid not in future_state:
        future_state[nid] = {}

    if what not in future_state[nid]:
        future_state[nid][what] = 0

    future_state[nid][what] += many


def get_network(point_to_network, eid, cid, color):
    id = eid + '-' + cid + '-' + color
    return point_to_network.get(id, None)


def get_signal(network_state, nid):
    return network_state.get(nid, {})


def combine_signals(siglist):
    sum = {}
    for sig in siglist:
        for signame in sig:
            if signame not in sum:
                sum[signame] = 0
            
            sum[signame] += sig[signame]

    return sum


def apply_operation(oper, left, right):
    if oper == "*":
        return left * right
    elif oper == "/":
        return left / right
    elif oper == "+":
        return left + right
    elif oper == "-":
        return left - right
    elif oper == "%":
        return left % right
    elif oper == "^":
        return left ** right
    elif oper == "<<":
        return left << right
    elif oper == ">>":
        return left >> right
    elif oper == "AND":
        return left & right
    elif oper == "OR":
        return left | right
    elif oper == "XOR":
        return left ^ right
    elif oper == "<":
        return int(left < right)
    elif oper == ">":
        return int(left > right)
    elif oper == "=":
        return int(left == right)
    elif oper == u"\u2264":
        return int(left <= right)
    elif oper == u"\u2265":
        return int(left >= right)
    elif oper == u"\u2260":
        return int(left != right)
    else:
        raise Exception("Unknown operation: ", oper)


def combine_input(network_state, point_to_network, eid):
    return combine_signals([
        get_signal(network_state, get_network(point_to_network, eid, "1", color))
        for color in COLORS])


def is_fulfilled(cond, signals):
    oper = cond["comparator"]
    first = cond["first_signal"]["name"]
    if "constant" in cond:
        second = cond["constant"]
    else:
        second = signals.get(cond["second_signal"]["name"], 0)

    if first in ["signal-each", "signal-anything"]:
        for sig in signals:
            if apply_operation(oper, signals.get(sig, 0), second):
                return True
        return False
    elif first == "signal-everything":
        for sig in signals:
            if not apply_operation(oper, signals.get(sig, 0), second):
                return False
        return True

    return apply_operation(oper, signals.get(first, 0), second)


def update(entity, point_to_network, network_state, future_state):
    eid = str(entity["entity_number"])
    name = entity["name"]

    to_add = {}
    endpoint = "1"
    if name == "constant-combinator":
        endpoint = "1"
        for filt in entity["control_behavior"]["filters"]:
            to_add[filt["signal"]["name"]] = filt["count"]
    elif name == "arithmetic-combinator":
        endpoint = "2"
        sum = combine_input(network_state, point_to_network, eid)
        cond = entity["control_behavior"]["arithmetic_conditions"]
        oper = cond["operation"]

        first = cond["first_signal"]["name"]
        if "constant" in cond:
            second = cond["constant"]
        else:
            second = sum.get(cond["second_signal"]["name"], 0)

        output_signal = cond["output_signal"]["name"]

        if first == "signal-each":
            siglist = list(sum)
        else:
            siglist = [first]

        for sig in siglist:
            if output_signal == "signal-each":
                out = sig
            else:
                out = output_signal

            if out not in to_add:
                to_add[out] = 0

            to_add[out] += apply_operation(oper, sum.get(sig, 0), second)
    elif name == "decider-combinator":
        endpoint = "2"
        signals = combine_input(network_state, point_to_network, eid)
        cond = entity["control_behavior"]["decider_conditions"]
        if is_fulfilled(cond, signals):
            output_copy = cond["copy_count_from_input"]
            output_signal = cond["output_signal"]["name"]
            if output_signal == "signal-everything":
                to_add = signals
            elif output_signal == "signal-each":
                oper = cond["comparator"]
                if "constant" in cond:
                    second = cond["constant"]
                else:
                    second = signals.get(cond["second_signal"]["name"], 0)
                for sig in signals:
                    if apply_operation(oper, signals.get(sig, 0), second):
                        to_add[sig] = signals.get(sig, 0)
            else:
                to_add = {output_signal: signals.get(output_signal, 0)}

            if not output_copy:
                for sig in to_add:
                    to_add[sig] = 1
            
    for sig in to_add:
        for color in COLORS:
            nid = get_network(point_to_network, eid, endpoint, color)
            add(future_state, nid, sig, to_add[sig])


def query_enabled(state, eid):
    e = state["entities"][eid]
    signals = combine_input(state["network_state"], state["point_to_network"], eid)
    return is_fulfilled(e["control_behavior"]["circuit_condition"], signals)


def tick(state):
    entities = state["entities"]
    point_to_network = state["point_to_network"]
    network_state = state["network_state"]
    future_state = {}
    
    for e in entities.values():
        update(e, point_to_network, network_state, future_state)

    state["network_state"] = future_state


def benchmark():
    import time
    TICKS = 3000
    s = open("shiftregister.bp").read()
    state = init_state(parse_blueprint(s))
    lamps = []
    for eid in state["entities"]:
        e = state["entities"][eid]
        if e["name"] == "small-lamp":
            lamps.append((e["position"]["x"], eid))

    lamps = [lamp[1] for lamp in sorted(lamps)]

    t0 = time.time()
    oldstates = tuple()
    for i in range(TICKS):
        lampstates = tuple([str(query_enabled(state, eid)) for eid in lamps])
        if lampstates != oldstates:
            oldstates = lampstates
            print "Tick %d: \tlamps: %s" % (i, " ".join(lampstates).replace("0", ".").replace("1", "#"))
        tick(state)
    print "Tick %d: \tlamps: %s" % (i, " ".join(lampstates).replace("0", ".").replace("1", "#"))

    t1 = time.time()
    print "#entities:", len(state["entities"])
    print "UPS:", TICKS / (t1 - t0)


if __name__ == "__main__":
    benchmark()
