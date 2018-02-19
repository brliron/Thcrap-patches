#include <Squirrel tracer.h>
#include <vector>
#include <list>
#include <map>
#include <functional>

enum	ArgType
{
	ARG_NONE = 0x0000,
	ARG_STACK = 0x0001,
	ARG_LITERAL = 0x0002,
	ARG_IMMEDIATE = 0x0004,
	ARG_UNKNOWN = 0x0008, // Just display its value

	MASK_FLOAT = 0x0010,
	MASK_CMP = 0x0020,
	MASK_BITWISE = 0x0040,
	MASK_SIGNED = 0x0080,
	MASK_TARGET = 0x0100,
	MASK_STACKBASE = 0x0200, // Base address for a range of values on the stack. The next argument contains the number of stack elements to use.

	ARG_FLOAT = ARG_IMMEDIATE | MASK_FLOAT,
	ARG_CMP = ARG_IMMEDIATE | MASK_CMP,
	ARG_BITWISE = ARG_IMMEDIATE | MASK_BITWISE,
	ARG_SIGNED = ARG_IMMEDIATE | MASK_SIGNED,
	ARG_TARGET = ARG_STACK | MASK_TARGET,
	ARG_TARGET_OFF = ARG_STACK | MASK_TARGET | MASK_SIGNED, // Offset to a stackbase
	ARG_STACKBASE = ARG_STACK | MASK_STACKBASE,
};

struct	OpcodeDescriptor
{
	const char*	name;
	ArgType	arg0;
	ArgType	arg1;
	ArgType	arg2;
	ArgType	arg3;
};

static std::vector<OpcodeDescriptor>	opcodes = {
  { "line",		ARG_NONE,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE },
  { "load",		ARG_TARGET,	ARG_LITERAL,	ARG_NONE,	ARG_NONE },
  { "loadint",		ARG_TARGET,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE },
  { "loadfloat",	ARG_TARGET,	ARG_FLOAT,	ARG_NONE,	ARG_NONE },
  { "dload",		ARG_TARGET,	ARG_LITERAL,	ARG_TARGET,	ARG_LITERAL },
  { "tailcall",		ARG_NONE,	ARG_STACK,	ARG_STACKBASE,	ARG_IMMEDIATE },
  { "call",		ARG_TARGET_OFF,	ARG_STACK,	ARG_STACKBASE,	ARG_IMMEDIATE },
  { "prepcall",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_TARGET },
  { "prepcallk",	ARG_TARGET,	ARG_LITERAL,	ARG_STACK,	ARG_TARGET },
  { "getk",		ARG_TARGET,	ARG_LITERAL,	ARG_STACK,	ARG_NONE },
  { "move",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "newslot",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_TARGET },
  { "delete",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "set",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_STACK },
  { "get",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "eq",		ARG_TARGET,	ARG_UNKNOWN,	ARG_STACK,	ARG_IMMEDIATE }, // arg1 type: arg3 != 0 ? LITERAL : STACK
  { "ne",		ARG_TARGET,	ARG_UNKNOWN,	ARG_STACK,	ARG_IMMEDIATE }, // arg1 type: arg3 != 0 ? LITERAL : STACK
  { "add",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "sub",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "mul",	        ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "div",	        ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "mod",	        ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "bitw",	        ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_BITWISE },
  { "return",		ARG_IMMEDIATE,	ARG_UNKNOWN,	ARG_NONE,	ARG_NONE }, // If arg0 is not 0xFF, arg1 is a STACK. Else, it isn't used.
  { "loadnulls",	ARG_TARGET,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE }, // arg0 is a target and a stackbase.
  { "loadroot",		ARG_TARGET,	ARG_NONE,	ARG_NONE,	ARG_NONE },
  { "loadbool",		ARG_TARGET,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE },
  { "dmove",		ARG_TARGET,	ARG_STACK,	ARG_TARGET,	ARG_STACK },
  { "jmp",		ARG_NONE,	ARG_SIGNED,	ARG_NONE,	ARG_NONE },
  { "jcmp",		ARG_STACK,	ARG_SIGNED,	ARG_STACK,	ARG_CMP },
  { "jz",		ARG_STACK,	ARG_SIGNED,	ARG_NONE,	ARG_NONE },
  { "setouter",		ARG_TARGET,	ARG_IMMEDIATE,	ARG_STACK,	ARG_NONE }, // arg1 is an outer value
  { "getouter",		ARG_TARGET,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE }, // arg1 is an outer value
  { "newobj",		ARG_TARGET,	ARG_UNKNOWN,	ARG_UNKNOWN,	ARG_IMMEDIATE /* enum NewObjectType */ }, // arg1 and arg2 depends on arg3
  { "appendarray",	ARG_STACK,	ARG_UNKNOWN,	ARG_IMMEDIATE,	ARG_NONE }, // The type of arg2 depends on arg1
  { "comparith",	ARG_STACK,	ARG_UNKNOWN,	ARG_STACK,	ARG_UNKNOWN },
  { "inc",		ARG_STACK,	ARG_STACK,	ARG_STACK,	ARG_SIGNED },
  { "incl",		ARG_NONE,	ARG_STACK,	ARG_NONE,	ARG_SIGNED },
  { "pinc",		ARG_STACK,	ARG_STACK,	ARG_STACK,	ARG_SIGNED },
  { "pincl",		ARG_NONE,	ARG_STACK,	ARG_NONE,	ARG_SIGNED },
  { "cmp",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_CMP },
  { "exists",		ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "instanceof",	ARG_TARGET,	ARG_STACK,	ARG_STACK,	ARG_NONE },
  { "and",		ARG_TARGET,	ARG_SIGNED,	ARG_STACK,	ARG_NONE },
  { "or",		ARG_TARGET,	ARG_SIGNED,	ARG_STACK,	ARG_NONE },
  { "neg",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "not",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "bwnot",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "closure",		ARG_TARGET,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE }, // arg1 is a function
  { "yield",		ARG_UNKNOWN,	ARG_STACK,	ARG_IMMEDIATE,	ARG_NONE }, // arg2 is a stack offset. If arg0 is not 0xFF, arg1 is a STACK. Else, it isn't used.
  { "resume",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "foreach",		ARG_STACK,	ARG_SIGNED,	ARG_STACK,	ARG_NONE },
  { "postforeach",	ARG_STACK,	ARG_SIGNED,	ARG_NONE,	ARG_NONE },
  { "clone",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "typeof",		ARG_TARGET,	ARG_STACK,	ARG_NONE,	ARG_NONE },
  { "pushtrap",		ARG_IMMEDIATE,	ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE },
  { "poptrap",		ARG_IMMEDIATE,	ARG_NONE,	ARG_NONE,	ARG_NONE },
  { "throw",		ARG_STACK,	ARG_NONE,	ARG_NONE,	ARG_NONE },
  { "newslota",		ARG_IMMEDIATE,	ARG_STACK,	ARG_STACK,	ARG_STACK },
  { "getbase",		ARG_TARGET,	ARG_NONE,	ARG_NONE,	ARG_NONE },
  { "close",		ARG_NONE,	ARG_STACK,	ARG_NONE,	ARG_NONE }
};

struct ObjectDump
{
	char *mem_dump;
	size_t mem_dump_size;
	json_t *json;
};

/*
static void printObj(SQObject &o);
static void printObj(SQTable &o,
	std::function<bool(SQTable::_HashNode &node)> print_elem  = [](SQTable::_HashNode &) { return true; },
	std::function<void(SQTable::_HashNode &node)> print_value = [](SQTable::_HashNode &node) { printObj(node.val); });
static void printObj(SQFunctionProto &o);
static void printObj(SQClass &o);

static void printObj(SQObject &o)
{
	static std::list<SQObject*> stack;
	if (std::count(stack.begin(), stack.end(), &o) > 0) {
		log_print("<infinite recursion detected>");
		return;
	}
	stack.push_back(&o);
	switch (o._type) {
	case OT_NULL:
		log_print("null");
		break;
	case OT_INTEGER:
		log_printf("%d", o._unVal.nInteger);
		break;
	case OT_FLOAT:
		log_printf("%f", o._unVal.fFloat);
		break;
	case OT_BOOL:
		log_print(o._unVal.nInteger ? "true" : "false");
		break;
	case OT_STRING: {
		SQString *s = o._unVal.pString;
		log_print("\"");
		log_nprint(s->_val, s->_len);
		log_print("\"");
		break;
	}
	case OT_TABLE:
		printObj(*o._unVal.pTable);
		break;
	case OT_ARRAY: {
		sqvector<SQObjectPtr> &array = o._unVal.pArray->_values;
		log_print("[ ");
		for (unsigned int i = 0; i < array.size(); i++) {
			if (i != 0) {
				log_print(", ");
			}
			printObj(array._vals[i]);
		}
		log_print(" ]");
		break;
	}
	case OT_USERDATA:
		log_printf("<user data: %p (size %d)>", o._unVal.pUserData + 1, o._unVal.pUserData->_size);
		break;
	case OT_CLOSURE:
		log_print("<closure: ");
		printObj(*o._unVal.pClosure->_function);
		// We may also want to print some things from SQWeakRef *_root and from SQClass *_base.
		log_print(">");
		break;
	case OT_NATIVECLOSURE:
		log_print("<native closure ");
		printObj(o._unVal.pNativeClosure->_name);
		log_printf(" : %p>", o._unVal.pNativeClosure->_function);
		break;
	case OT_GENERATOR:
		log_print("<generator: ");
		printObj(o._unVal.pGenerator->_closure);
		// We may also want to print some things from SQVM::CallInfo _ci and from SQGeneratorState _state
		log_print(">");
		break;
	case OT_USERPOINTER:
		log_printf("<user pointer: %p>", o._unVal.pUserPointer);
		break;
	case OT_THREAD:
		log_print("<thread>"); // Type: SQVM. I don't think we're interested by its content.
		break;
	case OT_FUNCPROTO:
		printObj(*o._unVal.pFunctionProto);
		break;
	case OT_CLASS:
		printObj(*o._unVal.pClass);
		break;
	case OT_INSTANCE: {
		SQInstance *i = o._unVal.pInstance;
		log_print("<instance: class=");
		printObj(*i->_class);
		log_print(", values=");

		// Test of extracting the actual values of members
		log_print(", values=");
		printObj(*i->_class->_members,
			[i](SQTable::_HashNode &node) { return _isfield(node.val) != 0; },
			[i](SQTable::_HashNode &node) { printObj(i->_values[_member_idx(node.val)]); }
		);

		log_printf(">");
		break;
	}
	case OT_WEAKREF:
		log_print("<weakref: ");
		printObj(o._unVal.pWeakRef->_obj);
		log_print(">");
		break;
	case OT_OUTER:
		log_print("<outer: valptr=");
		printObj(*o._unVal.pOuter->_valptr);
		log_print(", value=");
		printObj(o._unVal.pOuter->_value);
		log_print(">");
		break;
	default:
		log_print("<unknown type>");
		break;
	}
	stack.pop_back();
}

static void printObj(SQTable &o,
	std::function<bool(SQTable::_HashNode &node)> print_elem,
	std::function<void(SQTable::_HashNode &node)> print_value)
{
	bool print_comma = false;
	log_print("{ ");
	for (int i = 0; i < o._numofnodes; i++) {
		if (!print_elem(o._nodes[i])) {
			continue;
		}
		if (print_comma) {
			log_print(", ");
		}
		print_comma = true;
		printObj(o._nodes[i].key);
		log_print(" : ");
		print_value(o._nodes[i]);
	}
	log_print(" }");
}

static void printObj(SQFunctionProto &o)
{
	log_print("<function proto: sourcename=");
	printObj(o._sourcename);
	log_print(", name=");
	printObj(o._name);
	log_print(">");
}

static void printObj(SQClass &o)
{
	log_printf("<class: ");
	if (o._base) {
		log_print("base=");
		printObj(*o._base);
		log_print(", ");
	}
	log_print("members=");
	printObj(*o._members);

	// Test of extracting the actual values of members
	log_print(", default values=");
	printObj(*o._members,
		[&o](SQTable::_HashNode &node) { return _isfield(node.val) != 0 && (unsigned int)_member_idx(node.val) < o._defaultvalues._size; },
		[&o](SQTable::_HashNode &node) { printObj(o._defaultvalues[_member_idx(node.val)].val); }
	);

	log_print(", methods=");
	printObj(*o._members,
		[&o](SQTable::_HashNode &node) { return _isfield(node.val) == 0 && (unsigned int)_member_idx(node.val) < o._methods._size; },
		[&o](SQTable::_HashNode &node) { printObj(o._methods[_member_idx(node.val)].val); }
	);

	log_printf(">");
}

static void printArg(SQVM *self, ArgType type, uint32_t arg, bool& print_coma)
{
	if (type != ARG_NONE) {
		if (print_coma) {
			log_print(", ");
		}
		print_coma = true;
	}

	switch (type) {
	case ARG_NONE:
		break;

	case ARG_STACK:
		printObj(self->_stack._vals[self->_stackbase + arg]);
		break;
	
	case ARG_LITERAL:
		printObj(self->ci->_literals[arg]);
		break;
	
	case ARG_IMMEDIATE:
		log_printf("%u", arg);
		break;

	case ARG_UNKNOWN:
		log_print("(arg_type not supported yet)");
		break;

	case ARG_FLOAT:
		log_printf("%f", arg);
		break;

	case ARG_CMP: {
		const char *names[] = {
			"CMP_G",
			nullptr,
			"CMP_GE",
			"CMP_L",
			"CMP_LE",
			"CMP_3W"
		};
		const char* name = "UNKNOWN";
		if (arg >= 0 && arg <= 5 && names[arg])
			name = names[arg];
		log_print(name);
		break;
	}

	case ARG_BITWISE: {
		const char *names[] = {
			"BW_AND",
			nullptr,
			"BW_OR",
			"BW_XOR",
			"BW_SHIFTL",
			"BW_SHIFTR",
			"BW_USHIFTR"
		};
		const char* name = "UNKNOWN";
		if (arg >= 0 && arg <= 6 && names[arg])
			name = names[arg];
		log_print(name);
		break;
	}

	case ARG_SIGNED: {
		int32_t sarg;
		memcpy(&sarg, &arg, 4);
		log_printf("%d", sarg);
		break;
	}

	case ARG_TARGET:
	case ARG_TARGET_OFF:
		log_print("TARGET");
		break;

	case ARG_STACKBASE:
		log_print("(ARG_STACKBASE not supported yet)");
		break;

	default:
		log_print("(error in the instructions table)");
		break;
	}
}
*/

static json_t *add_obj(json_t *new_objs, SQObject *o);
template<typename T> static T *add_refcounted(json_t *new_objs, T *o);

static json_t *addr_to_json(void *addr)
{
	char string[] = "POINTER:0x00000000";
	sprintf(string, "POINTER:%p", addr);
	return json_string(string);
}

static json_t *hex_to_json(uint32_t hex)
{
	char string[] = "0x00000000";
	sprintf(string, "0x%.8x", hex);
	return json_string(string);
}

template<typename T> json_t *obj_to_json(T *o);
template<typename T> void recurse_add_obj(json_t *new_objs, T *o);

template<> static json_t *obj_to_json(SQString *o) { return json_stringn(o->_val, o->_len); }
template<> static void recurse_add_obj(json_t *new_objs, SQString *o) {}

template<> static json_t *obj_to_json(SQTable *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQTable"));

	for (int i = 0; i < o->_numofnodes; i++) {
		char key[] = "POINTER:0x00000000";
		sprintf(key, "POINTER:%p", &o->_nodes[i].key);
		json_object_set_new(res, key, addr_to_json(&o->_nodes[i].val));
	}
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQTable *o)
{
	for (int i = 0; i < o->_numofnodes; i++) {
		add_obj(new_objs, &o->_nodes[i].key);
		add_obj(new_objs, &o->_nodes[i].val);
	}
}

template<> static json_t *obj_to_json(SQArray *o)
{
	json_t *res = json_array();
	for (unsigned int i = 0; i < o->_values.size(); i++) {
		json_array_append_new(res, addr_to_json(&o->_values._vals[i]));
	}
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQArray *o)
{
	for (unsigned int i = 0; i < o->_values.size(); i++) {
		add_obj(new_objs, &o->_values._vals[i]);
	}
}

template<> static json_t *obj_to_json(SQUserData *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQUserData"));

	char *string = (char*)malloc(o->_size * 2 + 3);
	strcpy(string, "0x");

	uint8_t *data = (uint8_t*)(o + 1);
	for (int i = 0; i < o->_size; i++) {
		sprintf(string + 2 + i * 2, "%.2x", data[i]);
	}

	json_object_set_new(res, "data", json_string(string));
	free(string);
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQUserData *o) {}

template<> static json_t *obj_to_json(SQClosure *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQClosure"));
	json_object_set_new(res, "_function", addr_to_json(o->_function));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQClosure *o)
{
	add_refcounted<SQFunctionProto>(new_objs, o->_function);
}

template<> static json_t *obj_to_json(SQNativeClosure *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQNativeClosure"));
	json_object_set_new(res, "_name", addr_to_json(&o->_name));
	json_object_set_new(res, "_function", hex_to_json((uint32_t)o->_function));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQNativeClosure *o)
{
	add_obj(new_objs, &o->_name);
}

template<> static json_t *obj_to_json(SQGenerator *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQGenerator"));
	json_object_set_new(res, "_closure", addr_to_json(&o->_closure));
	// We may also want to print some things from SQVM::CallInfo _ci and from SQGeneratorState _state
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQGenerator *o)
{
	add_obj(new_objs, &o->_closure);
}

template<> static json_t *obj_to_json(SQFunctionProto *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQFunctionProto"));
	json_object_set_new(res, "_sourcename", addr_to_json(&o->_sourcename));
	json_object_set_new(res, "_name", addr_to_json(&o->_name));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQFunctionProto *o)
{
	add_obj(new_objs, &o->_sourcename);
	add_obj(new_objs, &o->_name);
}

template<> static json_t *obj_to_json(SQClassMemberVec *o)
{
	json_t *res = json_array();
	for (unsigned int i = 0; i < o->size(); i++) {
		SQObjectPtr &it = o->_vals[i].val;
		json_t *json = json_object();
		json_object_set_new(json, "_ismethod",   _ismethod(it) ? json_true() : json_false());
		json_object_set_new(json, "_isfield",    _isfield(it) ? json_true() : json_false());
		json_object_set_new(json, "_member_idx", json_integer(_member_idx(it)));
	}
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQClassMemberVec *o) {}

template<> static json_t *obj_to_json(SQClass *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQClass"));
	if (o->_base) {
		json_object_set_new(res, "_base", addr_to_json(o->_base));
	}
	json_object_set_new(res, "_members", addr_to_json(&o->_members));
	json_object_set_new(res, "_defaultvalues", addr_to_json(&o->_defaultvalues));
	json_object_set_new(res, "_methods", addr_to_json(&o->_methods));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQClass *o)
{
	if (o->_base) {
		add_refcounted<SQClass>(new_objs, o->_base);
	}
	add_refcounted<SQTable>(new_objs, o->_members);
	add_refcounted<SQClassMemberVec>(new_objs, &o->_defaultvalues);
	add_refcounted<SQClassMemberVec>(new_objs, &o->_methods);
}

template<> static json_t *obj_to_json(SQInstance *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQInstance"));
	json_object_set_new(res, "_class", addr_to_json(o->_class));

	json_t *_values = json_array();
	size_t _values_size = o->_class->_defaultvalues.size(); // ... I guess?

	for (size_t i = 0; i < _values_size; i++) {
		json_array_append_new(_values, addr_to_json(&o->_values[i]));
	}

	json_object_set_new(res, "_values", addr_to_json(&o->_values));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQInstance *o)
{
	add_refcounted<SQClass>(new_objs, o->_class);
	for (size_t i = 0; i < o->_class->_defaultvalues.size(); i++) {
		add_obj(new_objs, &o->_values[i]);
	}
}

template<> static json_t *obj_to_json(SQWeakRef *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQWeakRef"));
	json_object_set_new(res, "_obj", addr_to_json(&o->_obj));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQWeakRef *o)
{
	add_obj(new_objs, &o->_obj);
}

template<> static json_t *obj_to_json(SQOuter *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQOuter"));
	json_object_set_new(res, "_valptr", addr_to_json(o->_valptr));
	json_object_set_new(res, "_value", addr_to_json(&o->_value));
	return res;
}
template<> static void recurse_add_obj(json_t *new_objs, SQOuter *o)
{
	add_obj(new_objs, o->_valptr);
	add_obj(new_objs, &o->_value);
}

static std::list<void*> *stack = nullptr;
std::map<void*, ObjectDump> *objs_list = nullptr;

template<typename T>
static T *add_refcounted(json_t *new_objs, T *o)
{
	if (!stack) {
		stack = new std::list<void*>;
	}
	if (!objs_list) {
		objs_list = new std::map<void*, ObjectDump>;
	}

	if (std::count(stack->begin(), stack->end(), o) > 0) {
		log_print("<infinite recursion detected>");
		return o;
	}
	stack->push_back(o);

	const auto& it = objs_list->find(o);
	if (it == objs_list->end() || // We don't have any object with this pointer
		it->second.mem_dump_size != sizeof(T) || // The object we have doesn't have the same size
		memcmp(o, it->second.mem_dump, sizeof(T)) != 0 // The object changed
		) {
		json_t *dump = obj_to_json<T>(o);
		if (it == objs_list->end() ||
			json_equal(dump, it->second.json)) {
			ObjectDump &obj_dump = (*objs_list)[o];
			if (obj_dump.mem_dump) {
				free(obj_dump.mem_dump);
			}
			if (obj_dump.json) {
				json_decref(obj_dump.json);
			}

			obj_dump.mem_dump = (char*)malloc(sizeof(T));
			memcpy(obj_dump.mem_dump, o, sizeof(T));
			obj_dump.mem_dump_size = sizeof(T);
			obj_dump.json = dump;

			json_array_append(new_objs, dump);
		}
	}

	recurse_add_obj<T>(new_objs, o);

	stack->pop_back();
	return o;
}

static json_t *add_obj(json_t *new_objs, SQObject *o)
{
	if (!ISREFCOUNTED(o->_type)) {
		switch (o->_type) {
		case OT_NULL:
			return json_null();

		case OT_INTEGER:
			return json_integer(o->_unVal.nInteger);

		case OT_FLOAT:
			return json_real(o->_unVal.fFloat);

		case OT_BOOL:
			return o->_unVal.nInteger ? json_true() : json_false();

		case OT_USERPOINTER: {
			char string[] = "<user pointer: 0x00000000>";
			sprintf(string, "<user pointer: %p>", o->_unVal.pUserPointer);
			return json_string(string);
		}

		default: {
			char string[] = "<unknown non-refcounted type 0000000000>";
			sprintf(string, "<unknown non-refcounted type %d>", o->_type);
			return json_string(string);
		}
		}
	}
	else {
		switch (o->_type) {
		case OT_STRING:
			return addr_to_json(add_refcounted<SQString>(new_objs, o->_unVal.pString));

		case OT_TABLE:
			return addr_to_json(add_refcounted<SQTable>(new_objs, o->_unVal.pTable));

		case OT_ARRAY:
			return addr_to_json(add_refcounted<SQArray>(new_objs, o->_unVal.pArray));

		case OT_USERDATA:
			return addr_to_json(add_refcounted<SQUserData>(new_objs, o->_unVal.pUserData));

		case OT_CLOSURE:
			return addr_to_json(add_refcounted<SQClosure>(new_objs, o->_unVal.pClosure));

		case OT_NATIVECLOSURE:
			return addr_to_json(add_refcounted<SQNativeClosure>(new_objs, o->_unVal.pNativeClosure));

		case OT_GENERATOR:
			return addr_to_json(add_refcounted<SQGenerator>(new_objs, o->_unVal.pGenerator));

		case OT_THREAD:
			return json_string("<thread>"); // Type: SQVM. I don't think we're interested by its content.

		case OT_FUNCPROTO:
			return addr_to_json(add_refcounted<SQFunctionProto>(new_objs, o->_unVal.pFunctionProto));

		case OT_CLASS:
			return addr_to_json(add_refcounted<SQClass>(new_objs, o->_unVal.pClass));

		case OT_INSTANCE:
			return addr_to_json(add_refcounted<SQInstance>(new_objs, o->_unVal.pInstance));

		case OT_WEAKREF:
			return addr_to_json(add_refcounted<SQWeakRef>(new_objs, o->_unVal.pWeakRef));

		case OT_OUTER:
			return addr_to_json(add_refcounted<SQOuter>(new_objs, o->_unVal.pOuter));

		default:
			char string[] = "<unknown refcounted type 0000000000>";
			sprintf(string, "<unknown refcounted type %d>", o->_type);
			return json_string(string);
		}
	}
}

static json_t *arg_to_json(SQVM *self, json_t *new_objs, ArgType type, uint32_t arg)
{
	switch (type) {
	case ARG_NONE:
		return json_null();

	case ARG_STACK:
		return add_obj(new_objs, &self->_stack._vals[self->_stackbase + arg]);
		break;

	case ARG_LITERAL:
		return add_obj(new_objs, &self->ci->_literals[arg]);

	case ARG_IMMEDIATE:
		return json_integer(arg);

	case ARG_UNKNOWN:
		return json_string("(arg_type not supported yet)");

	case ARG_FLOAT:
		float farg;
		memcpy(&farg, &arg, 4);
		return json_real(farg);

	case ARG_CMP: {
		const char *names[] = {
			"CMP_G",
			nullptr,
			"CMP_GE",
			"CMP_L",
			"CMP_LE",
			"CMP_3W"
		};
		const char* name = "UNKNOWN";
		if (arg >= 0 && arg <= 5 && names[arg])
			name = names[arg];
		return json_string(name);
	}

	case ARG_BITWISE: {
		const char *names[] = {
			"BW_AND",
			nullptr,
			"BW_OR",
			"BW_XOR",
			"BW_SHIFTL",
			"BW_SHIFTR",
			"BW_USHIFTR"
		};
		const char* name = "UNKNOWN";
		if (arg >= 0 && arg <= 6 && names[arg])
			name = names[arg];
		return json_string(name);
	}

	case ARG_SIGNED: {
		int32_t sarg;
		memcpy(&sarg, &arg, 4);
		return json_integer(sarg);
	}

	case ARG_TARGET:
	case ARG_TARGET_OFF:
		return json_string("TARGET");

	case ARG_STACKBASE:
		return json_string("(ARG_STACKBASE not supported yet)");

	default:
		return json_string("(error in the instructions table)");
	}
}

/**
  * switch instruction in SQVM::execute
  * The breakpoint should cover the jmp [opcode*4+jump_table_addr]
  */
extern "C" int BP_SQVM_execute_switch(x86_reg_t *regs, json_t *bp_info)
{
	// Parameters
	// ----------
	SQVM *self = (SQVM*)json_object_get_immediate(bp_info, regs, "this");
	SQInstruction *_i_ = (SQInstruction*)json_object_get_immediate(bp_info, regs, "instruction");
	// ----------

	static CRITICAL_SECTION cs;
	static FILE *file = nullptr;
	if (!file) {
		file = fopen("trace.json", "w");
		fwrite("[\n", 2, 1, file);
		InitializeCriticalSection(&cs);
	}
	EnterCriticalSection(&cs);
	OpcodeDescriptor *desc = &opcodes[_i_->op];

	json_t *new_objs = json_array();
	json_t *instruction = json_object();
	json_object_set_new(instruction, "__type", json_string("instruction"));
	json_object_set_new(instruction, "op",     json_string(desc->name));
	json_object_set_new(instruction, "arg0",   arg_to_json(self, new_objs, desc->arg0, _i_->_arg0));
	json_object_set_new(instruction, "arg1",   arg_to_json(self, new_objs, desc->arg1, _i_->_arg1));
	json_object_set_new(instruction, "arg2",   arg_to_json(self, new_objs, desc->arg2, _i_->_arg2));
	json_object_set_new(instruction, "arg3",   arg_to_json(self, new_objs, desc->arg3, _i_->_arg3));

	size_t i;
	json_t *it;
	json_array_foreach(new_objs, i, it) {
		json_dumpf(it, file, 0);
		fwrite(",\n", 2, 1, file);
	}
	json_dumpf(instruction, file, 0);
	fwrite(",\n", 2, 1, file);
	fflush(file);

	json_decref(instruction);
	json_decref(new_objs);

	LeaveCriticalSection(&cs);
	return 1;
}

int __stdcall thcrap_plugin_init()
{
	int squirrel_tracer_removed = stack_remove_if_unneeded("squirrel_tracer");
	if (squirrel_tracer_removed == 1) {
		return 1;
	}
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
