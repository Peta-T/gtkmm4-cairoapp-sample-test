#include <gio/gio.h>

#if defined (__ELF__) && ( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
# define SECTION __attribute__ ((section (".gresource.resources"), aligned (sizeof(void *) > 8 ? sizeof(void *) : 8)))
#else
# define SECTION
#endif

static const SECTION union { const guint8 data[2161]; const double alignment; void * const ptr;}  resources_resource_data = {
  "\107\126\141\162\151\141\156\164\000\000\000\000\000\000\000\000"
  "\030\000\000\000\164\000\000\000\000\000\000\050\003\000\000\000"
  "\000\000\000\000\000\000\000\000\001\000\000\000\126\164\020\316"
  "\001\000\000\000\164\000\000\000\010\000\114\000\174\000\000\000"
  "\200\000\000\000\324\265\002\000\377\377\377\377\200\000\000\000"
  "\001\000\114\000\204\000\000\000\210\000\000\000\226\305\011\347"
  "\000\000\000\000\210\000\000\000\030\000\166\000\240\000\000\000"
  "\160\010\000\000\164\157\157\154\142\141\162\057\002\000\000\000"
  "\057\000\000\000\000\000\000\000\141\143\164\151\157\156\055\154"
  "\151\156\145\055\163\171\155\142\157\154\151\143\056\163\166\147"
  "\300\007\000\000\000\000\000\000\074\077\170\155\154\040\166\145"
  "\162\163\151\157\156\075\042\061\056\060\042\040\145\156\143\157"
  "\144\151\156\147\075\042\125\124\106\055\070\042\040\163\164\141"
  "\156\144\141\154\157\156\145\075\042\156\157\042\077\076\012\074"
  "\163\166\147\040\170\155\154\156\163\072\151\156\153\163\143\141"
  "\160\145\075\042\150\164\164\160\072\057\057\167\167\167\056\151"
  "\156\153\163\143\141\160\145\056\157\162\147\057\156\141\155\145"
  "\163\160\141\143\145\163\057\151\156\153\163\143\141\160\145\042"
  "\040\170\155\154\156\163\072\163\157\144\151\160\157\144\151\075"
  "\042\150\164\164\160\072\057\057\163\157\144\151\160\157\144\151"
  "\056\163\157\165\162\143\145\146\157\162\147\145\056\156\145\164"
  "\057\104\124\104\057\163\157\144\151\160\157\144\151\055\060\056"
  "\144\164\144\042\040\170\155\154\156\163\075\042\150\164\164\160"
  "\072\057\057\167\167\167\056\167\063\056\157\162\147\057\062\060"
  "\060\060\057\163\166\147\042\040\170\155\154\156\163\072\163\166"
  "\147\075\042\150\164\164\160\072\057\057\167\167\167\056\167\063"
  "\056\157\162\147\057\062\060\060\060\057\163\166\147\042\040\167"
  "\151\144\164\150\075\042\061\067\056\064\061\064\042\040\150\145"
  "\151\147\150\164\075\042\061\066\042\040\166\145\162\163\151\157"
  "\156\075\042\061\056\061\042\040\151\144\075\042\163\166\147\066"
  "\042\040\163\157\144\151\160\157\144\151\072\144\157\143\156\141"
  "\155\145\075\042\141\143\164\151\157\156\055\154\151\156\145\055"
  "\163\171\155\142\157\154\151\143\056\163\166\147\042\040\151\156"
  "\153\163\143\141\160\145\072\166\145\162\163\151\157\156\075\042"
  "\061\056\064\040\050\145\067\143\063\146\145\142\061\060\060\054"
  "\040\062\060\062\064\055\061\060\055\060\071\051\042\076\074\144"
  "\145\146\163\040\151\144\075\042\144\145\146\163\066\042\057\076"
  "\074\163\157\144\151\160\157\144\151\072\156\141\155\145\144\166"
  "\151\145\167\040\151\144\075\042\156\141\155\145\144\166\151\145"
  "\167\066\042\040\160\141\147\145\143\157\154\157\162\075\042\043"
  "\146\146\146\146\146\146\042\040\142\157\162\144\145\162\143\157"
  "\154\157\162\075\042\043\060\060\060\060\060\060\042\040\142\157"
  "\162\144\145\162\157\160\141\143\151\164\171\075\042\060\056\062"
  "\065\042\040\151\156\153\163\143\141\160\145\072\163\150\157\167"
  "\160\141\147\145\163\150\141\144\157\167\075\042\062\042\040\151"
  "\156\153\163\143\141\160\145\072\160\141\147\145\157\160\141\143"
  "\151\164\171\075\042\060\056\060\042\040\151\156\153\163\143\141"
  "\160\145\072\160\141\147\145\143\150\145\143\153\145\162\142\157"
  "\141\162\144\075\042\060\042\040\151\156\153\163\143\141\160\145"
  "\072\144\145\163\153\143\157\154\157\162\075\042\043\144\061\144"
  "\061\144\061\042\040\151\156\153\163\143\141\160\145\072\172\157"
  "\157\155\075\042\064\065\056\062\065\064\070\063\064\042\040\151"
  "\156\153\163\143\141\160\145\072\143\170\075\042\065\056\064\060"
  "\062\067\063\067\067\042\040\151\156\153\163\143\141\160\145\072"
  "\143\171\075\042\070\056\066\061\067\070\066\063\071\042\040\151"
  "\156\153\163\143\141\160\145\072\167\151\156\144\157\167\055\167"
  "\151\144\164\150\075\042\061\070\064\064\042\040\151\156\153\163"
  "\143\141\160\145\072\167\151\156\144\157\167\055\150\145\151\147"
  "\150\164\075\042\061\060\061\061\042\040\151\156\153\163\143\141"
  "\160\145\072\167\151\156\144\157\167\055\170\075\042\060\042\040"
  "\151\156\153\163\143\141\160\145\072\167\151\156\144\157\167\055"
  "\171\075\042\060\042\040\151\156\153\163\143\141\160\145\072\167"
  "\151\156\144\157\167\055\155\141\170\151\155\151\172\145\144\075"
  "\042\061\042\040\151\156\153\163\143\141\160\145\072\143\165\162"
  "\162\145\156\164\055\154\141\171\145\162\075\042\163\166\147\066"
  "\042\057\076\074\160\141\164\150\040\163\164\171\154\145\075\042"
  "\146\151\154\154\072\043\060\060\060\060\060\060\042\040\144\075"
  "\042\115\040\062\056\063\066\054\061\060\056\060\066\040\061\062"
  "\056\067\066\054\064\056\061\062\042\040\151\144\075\042\160\141"
  "\164\150\066\042\057\076\074\160\141\164\150\040\163\164\171\154"
  "\145\075\042\146\151\154\154\072\043\060\060\060\060\060\060\042"
  "\040\144\075\042\115\040\063\056\061\062\054\071\056\070\040\061"
  "\065\056\062\064\054\064\056\067\066\042\040\151\144\075\042\160"
  "\141\164\150\061\063\042\057\076\074\160\141\164\150\040\163\164"
  "\171\154\145\075\042\146\151\154\154\072\043\060\060\060\060\060"
  "\060\042\040\144\075\042\115\040\062\056\070\070\054\061\061\056"
  "\061\062\040\061\064\056\067\062\054\064\056\063\062\042\040\151"
  "\144\075\042\160\141\164\150\061\064\042\057\076\074\160\141\164"
  "\150\040\163\164\171\154\145\075\042\146\151\154\154\072\043\060"
  "\060\060\060\060\060\073\163\164\162\157\153\145\055\167\151\144"
  "\164\150\072\061\056\063\063\063\065\063\042\040\144\075\042\115"
  "\040\061\056\070\063\067\062\064\062\067\054\061\062\056\070\062"
  "\063\066\063\040\061\065\056\060\067\070\067\067\066\054\062\056"
  "\064\070\063\062\063\070\063\040\061\065\056\071\071\061\071\063"
  "\054\063\056\062\066\065\060\063\064\064\040\062\056\066\066\064"
  "\064\063\067\054\061\063\056\066\060\060\060\063\071\040\132\042"
  "\040\151\144\075\042\160\141\164\150\061\065\042\040\163\157\144"
  "\151\160\157\144\151\072\156\157\144\145\164\171\160\145\163\075"
  "\042\143\143\143\143\143\042\057\076\074\160\141\164\150\040\163"
  "\164\171\154\145\075\042\146\151\154\154\072\043\060\060\060\060"
  "\060\060\073\163\164\162\157\153\145\055\167\151\144\164\150\072"
  "\064\056\065\070\063\066\066\042\040\144\075\042\155\040\062\056"
  "\063\064\065\062\070\060\065\054\071\056\060\071\071\062\062\063"
  "\071\040\055\060\056\060\060\063\070\065\054\063\056\066\065\060"
  "\065\070\064\061\040\055\060\056\060\060\063\064\071\054\063\056"
  "\063\060\070\071\066\071\040\062\056\061\060\064\063\067\066\071"
  "\054\060\056\060\061\066\061\064\040\055\060\056\060\061\061\070"
  "\071\054\055\066\056\071\064\064\063\060\060\064\040\172\042\040"
  "\151\144\075\042\160\141\164\150\061\066\055\063\042\040\163\157"
  "\144\151\160\157\144\151\072\156\157\144\145\164\171\160\145\163"
  "\075\042\143\143\143\143\143\143\042\057\076\074\160\141\164\150"
  "\040\163\164\171\154\145\075\042\146\151\154\154\072\043\060\060"
  "\060\060\060\060\073\163\164\162\157\153\145\055\167\151\144\164"
  "\150\072\064\056\065\064\061\064\066\042\040\144\075\042\155\040"
  "\060\056\060\066\067\061\070\071\060\071\054\061\061\056\067\061"
  "\071\062\061\065\040\066\056\067\062\067\071\061\070\061\061\054"
  "\055\060\056\060\062\063\063\070\040\055\060\056\060\062\065\064"
  "\060\063\054\062\056\061\066\071\071\066\062\040\110\040\060\056"
  "\060\066\067\061\064\065\070\040\132\042\040\151\144\075\042\160"
  "\141\164\150\061\067\055\066\042\040\163\157\144\151\160\157\144"
  "\151\072\156\157\144\145\164\171\160\145\163\075\042\143\143\143"
  "\143\143\042\057\076\074\160\141\164\150\040\163\164\171\154\145"
  "\075\042\146\151\154\154\072\043\060\060\060\060\060\060\073\163"
  "\164\162\157\153\145\055\167\151\144\164\150\072\064\056\065\070"
  "\063\066\066\042\040\144\075\042\155\040\061\063\056\062\067\062"
  "\065\060\065\054\060\056\061\065\065\061\060\066\070\070\040\055"
  "\060\056\060\060\063\070\054\063\056\066\065\060\065\070\064\061"
  "\062\040\055\060\056\060\060\063\065\054\063\056\063\060\070\071"
  "\066\070\071\040\062\056\061\060\064\063\067\066\054\060\056\060"
  "\061\066\061\064\040\055\060\056\060\061\061\070\071\054\055\066"
  "\056\071\064\064\063\060\060\063\062\040\172\042\040\151\144\075"
  "\042\160\141\164\150\061\066\055\063\055\065\042\040\163\157\144"
  "\151\160\157\144\151\072\156\157\144\145\164\171\160\145\163\075"
  "\042\143\143\143\143\143\143\042\057\076\074\160\141\164\150\040"
  "\163\164\171\154\145\075\042\146\151\154\154\072\043\060\060\060"
  "\060\060\060\073\163\164\162\157\153\145\055\167\151\144\164\150"
  "\072\064\056\065\064\061\064\066\042\040\144\075\042\155\040\061"
  "\060\056\067\062\071\062\064\070\054\062\056\064\070\067\070\063"
  "\066\063\040\066\056\067\062\067\071\061\070\054\055\060\056\060"
  "\062\063\063\070\040\055\060\056\060\062\065\064\054\062\056\061"
  "\066\071\071\066\062\040\150\040\055\066\056\067\060\062\065\066"
  "\062\040\172\042\040\151\144\075\042\160\141\164\150\061\067\055"
  "\066\055\066\042\040\163\157\144\151\160\157\144\151\072\156\157"
  "\144\145\164\171\160\145\163\075\042\143\143\143\143\143\042\057"
  "\076\074\057\163\166\147\076\012\000\000\050\165\165\141\171\051"
  "" };

static GStaticResource static_resource = { resources_resource_data.data, sizeof (resources_resource_data.data) - 1 /* nul terminator */, NULL, NULL, NULL };

G_MODULE_EXPORT
GResource *resources_get_resource (void);
GResource *resources_get_resource (void)
{
  return g_static_resource_get_resource (&static_resource);
}
/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_CONSTRUCTOR_H__
#define __G_CONSTRUCTOR_H__

/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a usable way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#ifndef __GTK_DOC_IGNORE__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER)

/*
 * Only try to include gslist.h if not already included via glib.h,
 * so that items using gconstructor.h outside of GLib (such as
 * GResources) continue to build properly.
 */
#ifndef __G_LIB_H__
#include "gslist.h"
#endif

#include <stdlib.h>

#define G_HAS_CONSTRUCTORS 1

/* We do some weird things to avoid the constructors being optimized
 * away on VS2015 if WholeProgramOptimization is enabled. First we
 * make a reference to the array from the wrapper to make sure its
 * references. Then we use a pragma to make sure the wrapper function
 * symbol is always included at the link stage. Also, the symbols
 * need to be extern (but not dllexport), even though they are not
 * really used from another object file.
 */

/* We need to account for differences between the mangling of symbols
 * for x86 and x64/ARM/ARM64 programs, as symbols on x86 are prefixed
 * with an underscore but symbols on x64/ARM/ARM64 are not.
 */
#ifdef _M_IX86
#define G_MSVC_SYMBOL_PREFIX "_"
#else
#define G_MSVC_SYMBOL_PREFIX ""
#endif

#define G_DEFINE_CONSTRUCTOR(_func) G_MSVC_CTOR (_func, G_MSVC_SYMBOL_PREFIX)
#define G_DEFINE_DESTRUCTOR(_func) G_MSVC_DTOR (_func, G_MSVC_SYMBOL_PREFIX)

#define G_MSVC_CTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _wrapper(void);              \
  int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
  __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define G_MSVC_DTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _constructor(void);              \
  int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
   __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __GTK_DOC_IGNORE__ */
#endif /* __G_CONSTRUCTOR_H__ */

#ifdef G_HAS_CONSTRUCTORS

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(resourcesresource_constructor)
#endif
G_DEFINE_CONSTRUCTOR(resourcesresource_constructor)
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(resourcesresource_destructor)
#endif
G_DEFINE_DESTRUCTOR(resourcesresource_destructor)

#else
#warning "Constructor not supported on this compiler, linking in resources will not work"
#endif

static void resourcesresource_constructor (void)
{
  g_static_resource_init (&static_resource);
}

static void resourcesresource_destructor (void)
{
  g_static_resource_fini (&static_resource);
}
