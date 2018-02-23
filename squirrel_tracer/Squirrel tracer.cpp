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

static json_t *arg_to_json(SQVM *self, FILE *file, ArgType type, uint32_t arg)
{
	switch (type) {
	case ARG_NONE:
		return json_null();

	case ARG_STACK:
		return add_obj(file, &self->_stack._vals[self->_stackbase + arg]);
		break;

	case ARG_LITERAL:
		return add_obj(file, &self->ci->_literals[arg]);

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

ObjectDumpCollection *objs_list = nullptr;

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
		objs_list = new ObjectDumpCollection();
	}
	EnterCriticalSection(&cs);
	OpcodeDescriptor *desc = &opcodes[_i_->op];

	json_t *instruction = json_object();
	json_object_set_new(instruction, "type", json_string("instruction"));
	json_object_set_new(instruction, "op",     json_string(desc->name));
	json_object_set_new(instruction, "arg0",   arg_to_json(self, file, desc->arg0, _i_->_arg0));
	json_object_set_new(instruction, "arg1",   arg_to_json(self, file, desc->arg1, _i_->_arg1));
	json_object_set_new(instruction, "arg2",   arg_to_json(self, file, desc->arg2, _i_->_arg2));
	json_object_set_new(instruction, "arg3",   arg_to_json(self, file, desc->arg3, _i_->_arg3));

	json_dumpf(instruction, file, JSON_COMPACT);
	fwrite(",\n", 2, 1, file);
	fflush(file);

	json_decref(instruction);

	objs_list->unmapAll();
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
