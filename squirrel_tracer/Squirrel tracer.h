/**
  * Touhou Community Reliant Automatic Patcher
  * Squirrel tracing plugin
  *
  * ----
  *
  * Main include file.
  */

#pragma once

#include <thcrap.h>

#ifdef Yield
# undef Yield
#endif
#include <new>

#define private public
#include "squirrel\include\squirrel.h"
#include "squirrel\squirrel\sqvm.h"
#include "squirrel\squirrel\sqstate.h"
#include "squirrel\squirrel\sqobject.h"
#include "squirrel\squirrel\sqarray.h"
#include "squirrel\squirrel\sqtable.h"
#include "squirrel\squirrel\sqclass.h"
#include "squirrel\squirrel\sqfuncproto.h"
#include "squirrel\squirrel\sqclosure.h"
#include "squirrel\squirrel\sqstring.h"
#include "squirrel\squirrel\squserdata.h"
#undef private

#ifdef __cplusplus
extern "C" {
#endif

int __stdcall thcrap_plugin_init();

#ifdef __cplusplus
}
#endif