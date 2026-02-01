extends Node
## TokenDB - The core database kernel.
##
## Everything is a DB operation. Physics labels, scene tree queries,
## resource lookups - all operations on this structured data.
##
## Token addressing:
##   00.00.00.00.{value}    Byte codes (256, fixed)
##   00.00.00.01.{value}    NSM primitives (65, fixed)
##   01.00.00.{value}       Written characters (created on encounter)
##   J0.00.00.00.{count}    PBM storage records

# Token registry: address -> TokenRecord
var tokens: Dictionary = {}

# PBM registry: address -> PBMRecord
var pbm_records: Dictionary = {}

# PBM pairs: pbm_address -> Array of {from, to, count}
var pbm_pairs: Dictionary = {}

# LoD scoping: level -> Array of token addresses
var lod_levels: Dictionary = {}

# Encounter counter for auto-assigned character tokens
var _next_char_id: int = 0

# PBM counter
var _next_pbm_id: int = 0


class TokenRecord:
	var address: String
	var tree: String        # "linguistic" | "conceptual"
	var lod_level: int
	var created_by: String  # source PBM address, empty for fixed tokens
	var label: String

	func _init(p_address: String, p_tree: String, p_lod: int, p_label: String, p_source: String = ""):
		address = p_address
		tree = p_tree
		lod_level = p_lod
		label = p_label
		created_by = p_source


class PBMRecord:
	var address: String
	var name: String
	var source_type: String  # "encoding", "corpus", etc.
	var note: String

	func _init(p_address: String, p_name: String, p_type: String, p_note: String = ""):
		address = p_address
		name = p_name
		source_type = p_type
		note = p_note


class PBMPair:
	var from_token: String
	var to_token: String
	var count: int

	func _init(p_from: String, p_to: String, p_count: int = 1):
		from_token = p_from
		to_token = p_to
		count = p_count


func _ready():
	_init_byte_codes()
	_init_nsm_primitives()
	print("TokenDB: initialized with %d tokens" % tokens.size())


# --- Token Registration ---

func register_token(address: String, tree: String, lod_level: int, label: String, source: String = "") -> TokenRecord:
	if tokens.has(address):
		return tokens[address]

	var record = TokenRecord.new(address, tree, lod_level, label, source)
	tokens[address] = record

	# Add to LoD index
	if not lod_levels.has(lod_level):
		lod_levels[lod_level] = []
	lod_levels[lod_level].append(address)

	return record


func get_token(address: String) -> TokenRecord:
	return tokens.get(address)


func has_token(address: String) -> bool:
	return tokens.has(address)


func encounter_character(char_value: String, source_pbm: String = "") -> TokenRecord:
	## Create a character token on first encounter.
	## Returns existing token if character already known.

	# Check if already registered
	for addr in tokens:
		var record = tokens[addr] as TokenRecord
		if record.tree == "linguistic" and record.lod_level == 1 and record.label == char_value:
			return record

	# Create new character token
	var address = "01.00.00.%s" % _encode_base20(_next_char_id, 2)
	_next_char_id += 1

	return register_token(address, "linguistic", 1, char_value, source_pbm)


# --- PBM Operations ---

func create_pbm(name: String, source_type: String, note: String = "") -> PBMRecord:
	var address = "J0.00.00.00.%s" % _encode_base20(_next_pbm_id, 2)
	_next_pbm_id += 1

	var record = PBMRecord.new(address, name, source_type, note)
	pbm_records[address] = record
	pbm_pairs[address] = []

	return record


func add_pbm_pair(pbm_address: String, from_token: String, to_token: String, count: int = 1):
	if not pbm_pairs.has(pbm_address):
		pbm_pairs[pbm_address] = []

	# Check for existing pair
	for pair in pbm_pairs[pbm_address]:
		if pair.from_token == from_token and pair.to_token == to_token:
			pair.count += count
			return

	pbm_pairs[pbm_address].append(PBMPair.new(from_token, to_token, count))


func get_pbm_pairs(pbm_address: String) -> Array:
	return pbm_pairs.get(pbm_address, [])


func get_pbm(address: String) -> PBMRecord:
	return pbm_records.get(address)


# --- LoD Queries ---

func tokens_at_lod(level: int) -> Array:
	return lod_levels.get(level, [])


func token_count_at_lod(level: int) -> int:
	return tokens_at_lod(level).size()


# --- Initialization ---

func _init_byte_codes():
	## Register all 256 byte codes at LoD 0, linguistic tree.
	for i in range(256):
		var address = "00.00.00.00.%s" % _encode_base20(i, 2)
		var label = _byte_label(i)
		register_token(address, "linguistic", 0, label)


func _init_nsm_primitives():
	## Register NSM primitives at LoD 0, conceptual tree.
	var primitives = [
		"I", "you", "someone", "something", "people", "body",
		"this", "the same", "other",
		"one", "two", "some", "all", "much/many",
		"good", "bad",
		"big", "small",
		"think", "know", "want", "feel", "see", "hear",
		"say", "words",
		"do", "happen", "move", "touch",
		"there is", "have",
		"live", "die",
		"when/time", "now", "before", "after", "a long time", "a short time", "for some time",
		"where/place", "here", "above", "below", "far", "near", "side", "inside",
		"not", "maybe", "can", "because", "if", "like/way",
		"very",
		"like/as",
		"part", "kind", "true", "more",
		"be (somewhere)", "be (someone/something)", "mine", "moment",
	]

	for i in range(primitives.size()):
		var address = "00.00.00.01.%s" % _encode_base20(i, 2)
		register_token(address, "conceptual", 0, primitives[i])


# --- Base-20 Encoding ---

const BASE20_CHARS = "0123456789ABCDEFGHIJ"

func _encode_base20(value: int, min_length: int = 1) -> String:
	if value == 0:
		return BASE20_CHARS[0].repeat(min_length)

	var chars = ""
	var v = value
	while v > 0:
		chars = BASE20_CHARS[v % 20] + chars
		v = v / 20

	while chars.length() < min_length:
		chars = "0" + chars

	return chars


func _decode_base20(encoded: String) -> int:
	var result = 0
	for c in encoded:
		var idx = BASE20_CHARS.find(c.to_upper())
		if idx < 0:
			push_error("Invalid base-20 character: %s" % c)
			return -1
		result = result * 20 + idx
	return result


func _byte_label(value: int) -> String:
	if value >= 32 and value < 127:
		return char(value)
	var control_names = {
		0: "NUL", 1: "SOH", 2: "STX", 3: "ETX", 4: "EOT", 5: "ENQ",
		6: "ACK", 7: "BEL", 8: "BS", 9: "HT", 10: "LF", 11: "VT",
		12: "FF", 13: "CR", 14: "SO", 15: "SI", 16: "DLE", 17: "DC1",
		18: "DC2", 19: "DC3", 20: "DC4", 21: "NAK", 22: "SYN", 23: "ETB",
		24: "CAN", 25: "EM", 26: "SUB", 27: "ESC", 28: "FS", 29: "GS",
		30: "RS", 31: "US", 127: "DEL",
	}
	if control_names.has(value):
		return control_names[value]
	return "0x%02X" % value
