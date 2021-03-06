#include <Squirrel tracer.h>
#include <vector>
#include <list>
#include <map>

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

json_t *SquirrelTracer::arg_to_json(ArgType type, uint32_t arg)
{
	switch (type) {
	case ARG_NONE:
		return json_null();

	case ARG_STACK:
		return add_STK(arg);

	case ARG_LITERAL:
		return add_obj(&this->vm->ci->_literals[arg]);

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

void SquirrelTracer::add_instruction(SQInstruction *_i_)
{
	if (!this->enabled) {
		return;
	}

	OpcodeDescriptor *desc = &opcodes[_i_->op];
	json_t *instruction = json_object();

	json_object_set_new(instruction, "type", json_string("instruction"));
	json_object_set_new(instruction, "fn", json_string(this->fn.c_str()));
	json_object_set_new(instruction, "op", json_string(desc->name));

	switch (_i_->op) {
	case _OP_TAILCALL:
	case _OP_CALL: {
		json_object_set_new(instruction, "arg0", arg_to_json(desc->arg0, _i_->_arg0));
		json_object_set_new(instruction, "arg1", arg_to_json(desc->arg1, _i_->_arg1));
		json_t *callstack = json_array();
		for (int i = _i_->_arg2; i < _i_->_arg2 + _i_->_arg3; i++)
			json_array_append(callstack, add_STK(i));
		json_object_set_new(instruction, "arg2", callstack);
		json_object_set_new(instruction, "arg3", json_null());
		break;
	}

	case _OP_EQ:
	case _OP_NE:
		json_object_set_new(instruction, "arg0", arg_to_json(desc->arg0, _i_->_arg0));
		if (_i_->_arg3) {
			json_object_set_new(instruction, "arg1", add_obj(&this->vm->ci->_literals[_i_->_arg1]));
		}
		else {
			json_object_set_new(instruction, "arg1", add_STK(_i_->_arg1));
		}
		json_object_set_new(instruction, "arg2", arg_to_json(desc->arg2, _i_->_arg2));
		json_object_set_new(instruction, "arg3", json_null());
		break;

	case _OP_RETURN:
	case _OP_YIELD:
		json_object_set_new(instruction, "arg0", arg_to_json(desc->arg0, _i_->_arg0));
		if (_i_->_arg0 != 0xFF) {
			json_object_set_new(instruction, "arg1", add_STK(_i_->_arg1));
		}
		else {
			json_object_set_new(instruction, "arg1", json_null());
		}
		json_object_set_new(instruction, "arg2", arg_to_json(desc->arg2, _i_->_arg2));
		json_object_set_new(instruction, "arg3", arg_to_json(desc->arg3, _i_->_arg3));
		break;

	case _OP_GETOUTER:
	case _OP_SETOUTER:
		json_object_set_new(instruction, "arg0", arg_to_json(desc->arg0, _i_->_arg0));
		json_object_set_new(instruction, "arg1", add_obj(&this->vm->ci->_closure._unVal.pClosure->_outervalues[_i_->_arg1]));
		json_object_set_new(instruction, "arg2", arg_to_json(desc->arg2, _i_->_arg2));
		json_object_set_new(instruction, "arg3", arg_to_json(desc->arg3, _i_->_arg3));
		break;

	/**
	  * OP_CLOSURE will often be called with self->ci (or an object in it) being garbage.
	  * Even with all these NULL checks, if self->ci is 0x00000003 (it happened), well... I can't do anything but crash.
	  * So I'll just fallback to the default case that says "arg1 is an integer".

	case _OP_CLOSURE: {
		json_object_set_new(instruction, "arg0", arg_to_json(self, file, desc->arg0, _i_->_arg0));
		SQObjectPtr *functions = nullptr;
		if (self->ci) {
			if (self->ci->_closure._type == OT_CLOSURE) {
				SQClosure *closure = self->ci->_closure._unVal.pClosure;
				if (closure->_function) {
					functions = closure->_function->_functions;
				}
			}
		}
		if (functions) {
			json_object_set_new(instruction, "arg1", add_obj(file, &functions[_i_->_arg1]));
		}
		else {
			json_object_set_new(instruction, "arg2", json_string("<null>"));
		}
		json_object_set_new(instruction, "arg2", arg_to_json(self, file, desc->arg2, _i_->_arg2));
		json_object_set_new(instruction, "arg3", arg_to_json(self, file, desc->arg3, _i_->_arg3));
		break;
	}
	*/

	// TODO: _OP_NEWOBJ, _OP_APPENDARRAY, _OP_COMPARITH

	default:
		json_object_set_new(instruction, "arg0", arg_to_json(desc->arg0, _i_->_arg0));
		json_object_set_new(instruction, "arg1", arg_to_json(desc->arg1, _i_->_arg1));
		json_object_set_new(instruction, "arg2", arg_to_json(desc->arg2, _i_->_arg2));
		json_object_set_new(instruction, "arg3", arg_to_json(desc->arg3, _i_->_arg3));
		break;
	}

	json_dumpf(instruction, this->file, JSON_COMPACT);
	fwrite(",\n", 2, 1, this->file);
	fflush(this->file);
	json_decref(instruction);
}

SquirrelTracer::SquirrelTracer()
	: enabled(true)
{
	InitializeCriticalSection(&this->cs);
	this->file = fopen("trace.json", "w");
	fwrite("[\n", 2, 1, this->file);
}

SquirrelTracer::~SquirrelTracer()
{
	fclose(this->file);
	DeleteCriticalSection(&this->cs);
}

void SquirrelTracer::enter(SQVM *vm)
{
	if (GetAsyncKeyState('O') & 0x8000) {
		this->enabled = !this->enabled;
		log_mboxf(NULL, MB_OK, "SquirrelTracer is now %s.\n"
			"To toggle the SquirrelTracer state, press the 'O' key.",
			this->enabled ? "enabled" : "disabled");
	}
	if (!this->enabled) {
		return;
	}
	EnterCriticalSection(&this->cs);
	this->vm = vm;
	this->fn = this->closureDB.get(vm->ci->_closure._unVal.pClosure).c_str();
}

void SquirrelTracer::leave()
{
	if (!this->enabled) {
		return;
	}
	this->objs_list.unmapAll();
	this->fn.clear();
	this->vm = nullptr;
	LeaveCriticalSection(&this->cs);
}

static SquirrelTracer *tracer = nullptr;

/**
  * switch instruction in SQVM::execute
  * The breakpoint should cover the jmp [opcode*4+jump_table_addr]
  */
extern "C" int BP_SQVM_execute_switch(x86_reg_t *regs, json_t *bp_info)
{
	// Parameters
	// ----------
	SQVM *vm = (SQVM*)json_object_get_immediate(bp_info, regs, "this");
	SQInstruction *_i_ = (SQInstruction*)json_object_get_immediate(bp_info, regs, "instruction");
	// ----------

	if (!tracer) {
		tracer = new SquirrelTracer();
	}

	tracer->enter(vm);
	tracer->add_instruction(_i_);
	tracer->leave();

	return 1;
}

/**
  * sq_readclosure
  * It doesn't make sense to put this breakpoint at the beginning of the function,
  * SQClosure::Load must have been called.
  * Over the call to v->Push(closure) seems a good place.
  */
extern "C" int BP_sq_readclosure(x86_reg_t *regs, json_t *bp_info)
{
	// Parameters
	// ----------
	SQObjectPtr *closure = (SQObjectPtr*)json_object_get_immediate(bp_info, regs, "closure");
	// ----------

	if (tracer && closure) {
		tracer->closureDB[closure->_unVal.pClosure] = tracer->closureDB.lastFileName;
	}
	return 1;
}

/**
  * Copy of BP_th135_file_name from base_tasofro.
  * But thcrap doesn't support multiple breakpoints functions for a single breakpoint.
  */
int BP_file_name_for_squirrel(x86_reg_t *regs, json_t *bp_info)
{
	// Parameters
	// ----------
	const char *filename = (const char*)json_object_get_immediate(bp_info, regs, "file_name");
	// ----------

	if (tracer && filename && strcmp(PathFindExtension(filename), ".nut") == 0) {
		tracer->closureDB.lastFileName = filename;
	}
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
