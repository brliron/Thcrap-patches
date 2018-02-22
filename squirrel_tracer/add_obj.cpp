#include <Squirrel tracer.h>
#include <vector>
#include <list>

template<typename T> static T *add_refcounted(FILE *file, T *o);
template<typename T> static json_t *obj_to_json(T *o);
template<typename T> static void recurse_add_obj(FILE *file, T *o);


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
template<> static void recurse_add_obj(FILE *file, SQString *o) {}

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
template<> static void recurse_add_obj(FILE *file, SQTable *o)
{
	for (int i = 0; i < o->_numofnodes; i++) {
		json_decref(add_obj(file, &o->_nodes[i].key));
		json_decref(add_obj(file, &o->_nodes[i].val));
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
template<> static void recurse_add_obj(FILE *file, SQArray *o)
{
	for (unsigned int i = 0; i < o->_values.size(); i++) {
		json_decref(add_obj(file, &o->_values._vals[i]));
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
template<> static void recurse_add_obj(FILE *file, SQUserData *o) {}

template<> static json_t *obj_to_json(SQClosure *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQClosure"));
	json_object_set_new(res, "_function", addr_to_json(o->_function));
	return res;
}
template<> static void recurse_add_obj(FILE *file, SQClosure *o)
{
	add_refcounted<SQFunctionProto>(file, o->_function);
}

template<> static json_t *obj_to_json(SQNativeClosure *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQNativeClosure"));
	json_object_set_new(res, "_name", addr_to_json(&o->_name));
	json_object_set_new(res, "_function", hex_to_json((uint32_t)o->_function));
	return res;
}
template<> static void recurse_add_obj(FILE *file, SQNativeClosure *o)
{
	json_decref(add_obj(file, &o->_name));
}

template<> static json_t *obj_to_json(SQGenerator *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQGenerator"));
	json_object_set_new(res, "_closure", addr_to_json(&o->_closure));
	// We may also want to print some things from SQVM::CallInfo _ci and from SQGeneratorState _state
	return res;
}
template<> static void recurse_add_obj(FILE *file, SQGenerator *o)
{
	json_decref(add_obj(file, &o->_closure));
}

template<> static json_t *obj_to_json(SQFunctionProto *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQFunctionProto"));
	json_object_set_new(res, "_sourcename", addr_to_json(&o->_sourcename));
	json_object_set_new(res, "_name", addr_to_json(&o->_name));
	return res;
}
template<> static void recurse_add_obj(FILE *file, SQFunctionProto *o)
{
	json_decref(add_obj(file, &o->_sourcename));
	json_decref(add_obj(file, &o->_name));
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
template<> static void recurse_add_obj(FILE *file, SQClassMemberVec *o) {}

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
template<> static void recurse_add_obj(FILE *file, SQClass *o)
{
	if (o->_base) {
		add_refcounted<SQClass>(file, o->_base);
	}
	add_refcounted<SQTable>(file, o->_members);
	add_refcounted<SQClassMemberVec>(file, &o->_defaultvalues);
	add_refcounted<SQClassMemberVec>(file, &o->_methods);
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
template<> static void recurse_add_obj(FILE *file, SQInstance *o)
{
	add_refcounted<SQClass>(file, o->_class);
	for (size_t i = 0; i < o->_class->_defaultvalues.size(); i++) {
		json_decref(add_obj(file, &o->_values[i]));
	}
}

template<> static json_t *obj_to_json(SQWeakRef *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQWeakRef"));
	json_object_set_new(res, "_obj", addr_to_json(&o->_obj));
	return res;
}
template<> static void recurse_add_obj(FILE *file, SQWeakRef *o)
{
	json_decref(add_obj(file, &o->_obj));
}

template<> static json_t *obj_to_json(SQOuter *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQOuter"));
	json_object_set_new(res, "_valptr", addr_to_json(o->_valptr));
	json_object_set_new(res, "_value", addr_to_json(&o->_value));
	return res;
}
template<> static void recurse_add_obj(FILE *file, SQOuter *o)
{
	json_decref(add_obj(file, o->_valptr));
	json_decref(add_obj(file, &o->_value));
}

static std::list<void*> *stack = nullptr;
static ObjectDumpCollection *objs_list = nullptr;

template<typename T>
static T *add_refcounted(FILE *file, T *o)
{
	if (!stack) {
		stack = new std::list<void*>;
	}
	if (!objs_list) {
		objs_list = new ObjectDumpCollection();
	}

	if (std::count(stack->begin(), stack->end(), o) > 0) {
		log_print("<Squirrel tracer - infinite recursion detected>\n");
		return o;
	}
	stack->push_back(o);

	ObjectDump& dump = (*objs_list)[o];
	if (dump.equal(o, sizeof(T)) == false) {
		json_t *obj_json = obj_to_json<T>(o);

		// Even when the objects differ, if the JSON dump is identical, we don't need to replace it.
		if (dump.equal(obj_json) == 0) {
			dump.set(o, sizeof(T), obj_json);

			json_t *out = json_object();
			json_object_set_new(out, "type", json_string("object"));
			json_object_set_new(out, "address", addr_to_json(o));
			json_object_set(out, "content", obj_json);
			json_dumpf(out, file, JSON_COMPACT);
			fwrite(",\n", 2, 1, file);
			json_decref(out);
		}
		json_decref(obj_json);
	}

	recurse_add_obj<T>(file, o);

	stack->pop_back();
	return o;
}

json_t *add_obj(FILE *file, SQObject *o)
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
			return addr_to_json(add_refcounted<SQString>(file, o->_unVal.pString));

		case OT_TABLE:
			return addr_to_json(add_refcounted<SQTable>(file, o->_unVal.pTable));

		case OT_ARRAY:
			return addr_to_json(add_refcounted<SQArray>(file, o->_unVal.pArray));

		case OT_USERDATA:
			return addr_to_json(add_refcounted<SQUserData>(file, o->_unVal.pUserData));

		case OT_CLOSURE:
			return addr_to_json(add_refcounted<SQClosure>(file, o->_unVal.pClosure));

		case OT_NATIVECLOSURE:
			return addr_to_json(add_refcounted<SQNativeClosure>(file, o->_unVal.pNativeClosure));

		case OT_GENERATOR:
			return addr_to_json(add_refcounted<SQGenerator>(file, o->_unVal.pGenerator));

		case OT_THREAD:
			return json_string("<thread>"); // Type: SQVM. I don't think we're interested by its content.

		case OT_FUNCPROTO:
			return addr_to_json(add_refcounted<SQFunctionProto>(file, o->_unVal.pFunctionProto));

		case OT_CLASS:
			return addr_to_json(add_refcounted<SQClass>(file, o->_unVal.pClass));

		case OT_INSTANCE:
			return addr_to_json(add_refcounted<SQInstance>(file, o->_unVal.pInstance));

		case OT_WEAKREF:
			return addr_to_json(add_refcounted<SQWeakRef>(file, o->_unVal.pWeakRef));

		case OT_OUTER:
			return addr_to_json(add_refcounted<SQOuter>(file, o->_unVal.pOuter));

		default:
			char string[] = "<unknown refcounted type 0000000000>";
			sprintf(string, "<unknown refcounted type %d>", o->_type);
			return json_string(string);
		}
	}
}
