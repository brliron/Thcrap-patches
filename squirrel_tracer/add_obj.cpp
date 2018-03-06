#include <Squirrel tracer.h>
#include <vector>
#include <list>

static json_t *hex_to_json(uint32_t hex)
{
	char string[] = "0x00000000";
	sprintf(string, "0x%.8x", hex);
	return json_string(string);
}

template<> json_t *SquirrelTracer::obj_to_json(SQString *o) { return json_stringn(o->_val, o->_len); }

template<> json_t *SquirrelTracer::obj_to_json(SQTable *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQTable"));

	json_t *nodes = json_array();
	for (int i = 0; i < o->_numofnodes; i++) {
		json_t *node = json_object();
		json_object_set_new(node, "key", add_obj(&o->_nodes[i].key));
		json_object_set_new(node, "val", add_obj(&o->_nodes[i].val));
		json_array_append_new(nodes, node);
	}
	json_object_set_new(res, "_nodes", nodes);
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQArray *o)
{
	json_t *res = json_array();
	for (unsigned int i = 0; i < o->_values.size(); i++) {
		json_array_append_new(res, add_obj(&o->_values._vals[i]));
	}
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQUserData *o)
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

template<> json_t *SquirrelTracer::obj_to_json(SQClosure *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQClosure"));
	json_object_set_new(res, "_function", add_refcounted<SQFunctionProto>(o->_function));
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQNativeClosure *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQNativeClosure"));
	json_object_set_new(res, "_name", add_obj(&o->_name));
	json_object_set_new(res, "_function", hex_to_json((uint32_t)o->_function));
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQGenerator *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQGenerator"));
	json_object_set_new(res, "_closure", add_obj(&o->_closure));
	// We may also want to print some things from SQVM::CallInfo _ci and from SQGeneratorState _state
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQFunctionProto *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQFunctionProto"));
	json_object_set_new(res, "_sourcename", add_obj(&o->_sourcename));
	json_object_set_new(res, "_name", add_obj(&o->_name));
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQClassMemberVec *o)
{
	json_t *res = json_array();
	for (unsigned int i = 0; i < o->size(); i++) {
		SQObjectPtr &it = o->_vals[i].val;
		json_t *json = json_object();
		json_object_set_new(json, "_ismethod", _ismethod(it) ? json_true() : json_false());
		json_object_set_new(json, "_isfield", _isfield(it) ? json_true() : json_false());
		json_object_set_new(json, "_member_idx", json_integer(_member_idx(it)));
		json_array_append_new(res, json);
	}
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQClass *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQClass"));
	if (o->_base) {
		json_object_set_new(res, "_base", add_refcounted<SQClass>(o->_base));
	}
	json_object_set_new(res, "_members", add_refcounted<SQTable>(o->_members));
	json_object_set_new(res, "_defaultvalues", add_refcounted<SQClassMemberVec>(&o->_defaultvalues));
	json_object_set_new(res, "_methods", add_refcounted<SQClassMemberVec>(&o->_methods));
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQInstance *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQInstance"));
	json_object_set_new(res, "_class", add_refcounted<SQClass>(o->_class));

	json_t *_values = json_array();
	size_t _values_size = o->_class->_defaultvalues.size(); // ... I guess?

	for (size_t i = 0; i < _values_size; i++) {
		json_array_append_new(_values, add_obj(&o->_values[i]));
	}

	json_object_set_new(res, "_values", _values);
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQWeakRef *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQWeakRef"));
	json_object_set_new(res, "_obj", add_obj(&o->_obj));
	return res;
}

template<> json_t *SquirrelTracer::obj_to_json(SQOuter *o)
{
	json_t *res = json_object();
	json_object_set_new(res, "ObjectType", json_string("SQOuter"));
	json_object_set_new(res, "_valptr", add_obj(o->_valptr));
	json_object_set_new(res, "_value", add_obj(&o->_value));
	return res;
}

static std::list<void*> *stack = nullptr;

template<typename T>
json_t *SquirrelTracer::add_refcounted(T *o)
{
	// TODO: use something based on std::vector to reduce allocations
	if (!stack) {
		stack = new std::list<void*>;
	}

	if (!o) {
		return json_null();
	}

	json_t *res;
	char string[] = "POINTER:0x00000000";
	sprintf(string, "POINTER:%p", o);
	res = json_string(string);

	if (std::count(stack->begin(), stack->end(), o) > 0) {
		log_print("<Squirrel tracer - infinite recursion detected>\n");
		return res;
	}
	stack->push_back(o);

	ObjectDump& dump = objs_list[o];
	// TODO: create the json object only when dump.equal(o, sizeof(T)) == false
	json_t *obj_json = this->obj_to_json<T>(o);
	if (dump.equal(o, sizeof(T)) == false) {
		// Even when the objects differ, if the JSON dump is identical, we don't need to replace it.
		if (dump.equal(obj_json) == false) {
			dump.set(o, sizeof(T), obj_json);
			dump.writeToFile(this->file);
		}
	}
	json_decref(obj_json);

	stack->pop_back();
	return res;
}

json_t *SquirrelTracer::add_obj(SQObject *o)
{
	if (!o) {
		return json_null();
	}

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
			char string[1024] = "<user pointer: 0x00000000>";
			sprintf(string, "<user pointer: %p>", o->_unVal.pUserPointer);
			return json_string(string);
		}

		default: {
			char string[1024] = "<unknown non-refcounted type 0000000000>";
			sprintf(string, "<unknown non-refcounted type %d>", o->_type);
			return json_string(string);
		}
		}
	}
	else {
		switch (o->_type) {
		case OT_STRING:
			return add_refcounted<SQString>(o->_unVal.pString);

		case OT_TABLE:
			return add_refcounted<SQTable>(o->_unVal.pTable);

		case OT_ARRAY:
			return add_refcounted<SQArray>(o->_unVal.pArray);

		case OT_USERDATA:
			return add_refcounted<SQUserData>(o->_unVal.pUserData);

		case OT_CLOSURE:
			return add_refcounted<SQClosure>(o->_unVal.pClosure);

		case OT_NATIVECLOSURE:
			return add_refcounted<SQNativeClosure>(o->_unVal.pNativeClosure);

		case OT_GENERATOR:
			return add_refcounted<SQGenerator>(o->_unVal.pGenerator);

		case OT_THREAD:
			return json_string("<thread>"); // Type: SQVM. I don't think we're interested by its content.

		case OT_FUNCPROTO:
			return add_refcounted<SQFunctionProto>(o->_unVal.pFunctionProto);

		case OT_CLASS:
			return add_refcounted<SQClass>(o->_unVal.pClass);

		case OT_INSTANCE:
			return add_refcounted<SQInstance>(o->_unVal.pInstance);

		case OT_WEAKREF:
			return add_refcounted<SQWeakRef>(o->_unVal.pWeakRef);

		case OT_OUTER:
			return add_refcounted<SQOuter>(o->_unVal.pOuter);

		default:
			char string[1024] = "<unknown refcounted type 0000000000>";
			sprintf(string, "<unknown refcounted type %d>", o->_type);
			return json_string(string);
		}
	}
}
