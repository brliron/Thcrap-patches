#include <Squirrel tracer.h>
#include <vector>
#include <list>
#include <map>

template<typename T> static T *add_refcounted(json_t *new_objs, T *o);
template<typename T> static json_t *obj_to_json(T *o);
template<typename T> static void recurse_add_obj(json_t *new_objs, T *o);


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
		json_object_set_new(json, "_ismethod", _ismethod(it) ? json_true() : json_false());
		json_object_set_new(json, "_isfield", _isfield(it) ? json_true() : json_false());
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

struct ObjectDump
{
	char *mem_dump;
	size_t mem_dump_size;
	json_t *json;
};

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

json_t *add_obj(json_t *new_objs, SQObject *o)
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
