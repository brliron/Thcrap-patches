#include <Squirrel tracer.h>

/*
** First version of the code to dump instructions.
** If I don't dump objects, I don't have enough informations. If I dump all objects used by every instruction, my log files are *way* too big.
** So I dropped this code and went to a different solution.

#include <list>
#include <functional>

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
