/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

use std::ffi::CString;
use std::os::raw::{c_char, c_void};
mod argon_sys;       // declare the module
use crate::argon_sys::*;  // then import everything

/// Helper: convert Rust string to C string pointer
fn c_str(s: &str) -> *mut c_char {
    CString::new(s).unwrap().into_raw()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn int_add(
    argc: usize,
    argv: *mut *mut ArgonObject,
    err: *mut ArgonError,
    _state: *mut ArgonState,
    api: *mut ArgonNativeAPI,
) -> *mut ArgonObject { unsafe {
    if argc != 2 {
        return ((*api).throw_argon_error.unwrap())(
            err,
            c_str("Runtime Error"),
            c_str("expected 2 arguments, got %lu"),
            argc as u64,
        );
    }

    let a = ((*api).argon_to_i64.unwrap())(*argv.add(0), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    let b = ((*api).argon_to_i64.unwrap())(*argv.add(1), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    ((*api).i64_to_argon.unwrap())(a + b)
}}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn double_add(
    argc: usize,
    argv: *mut *mut ArgonObject,
    err: *mut ArgonError,
    _state: *mut ArgonState,
    api: *mut ArgonNativeAPI,
) -> *mut ArgonObject { unsafe {
    if argc != 2 {
        return ((*api).throw_argon_error.unwrap())(
            err,
            c_str("Runtime Error"),
            c_str("expected 2 arguments, got %lu"),
            argc as u64,
        );
    }

    let a = ((*api).argon_to_double.unwrap())(*argv.add(0), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    let b = ((*api).argon_to_double.unwrap())(*argv.add(1), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    ((*api).double_to_argon.unwrap())(a + b)
}}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rational_add(
    argc: usize,
    argv: *mut *mut ArgonObject,
    err: *mut ArgonError,
    _state: *mut ArgonState,
    api: *mut ArgonNativeAPI,
) -> *mut ArgonObject { unsafe {
    if argc != 2 {
        return ((*api).throw_argon_error.unwrap())(
            err,
            c_str("Runtime Error"),
            c_str("expected 2 arguments, got %lu"),
            argc as u64,
        );
    }

    let a = ((*api).argon_to_rational.unwrap())(*argv.add(0), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    let b = ((*api).argon_to_rational.unwrap())(*argv.add(1), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    let result = rational {
        n: a.n * b.d + b.n * a.d,
        d: a.d * b.d,
    };

    ((*api).rational_to_argon.unwrap())(result)
}}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn hello_world(
    _argc: usize,
    _argv: *mut *mut ArgonObject,
    _err: *mut ArgonError,
    _state: *mut ArgonState,
    _api: *mut ArgonNativeAPI,
) -> *mut ArgonObject {
    println!("hello world from native code");
    std::ptr::null_mut()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn yooo(
    argc: usize,
    argv: *mut *mut ArgonObject,
    err: *mut ArgonError,
    _state: *mut ArgonState,
    api: *mut ArgonNativeAPI,
) -> *mut ArgonObject { unsafe {
    if ((*api).fix_to_arg_size.unwrap())(1, argc, err) {
        return (*api).ARGON_NULL;
    }

    let name = ((*api).argon_to_string.unwrap())(*argv.add(0), err);
    if ((*api).is_error.unwrap())(err) {
        return (*api).ARGON_NULL;
    }

    let prefix = b"Yooo ";
    let suffix = b", how are you?";

    let total_len = prefix.len() + name.length + suffix.len();
    let data = ((*api).malloc.unwrap())(total_len) as *mut c_char;

    std::ptr::copy_nonoverlapping(prefix.as_ptr() as *const c_char, data, prefix.len());
    std::ptr::copy_nonoverlapping(name.data, data.add(prefix.len()), name.length);
    std::ptr::copy_nonoverlapping(
        suffix.as_ptr() as *const c_char,
        data.add(prefix.len() + name.length),
        suffix.len(),
    );

    let output = ((*api).string_to_argon.unwrap())(string { data, length: total_len });
    ((*api).free.unwrap())(data as *mut c_void);

    output
}}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn argon_module_init(
    _vm: *mut ArgonState,
    api: *mut ArgonNativeAPI,
    _err: *mut ArgonError,
    reg: *mut ArgonObjectRegister,
) { unsafe {
    // Helper alias in scope
    let c_str = |s: &str| CString::new(s).unwrap().into_raw();

    ((*api).register_ArgonObject.unwrap())(
        reg,
        c_str("hello_world"),
        ((*api).create_argon_native_function.unwrap())(c_str("hello_world"), Some(hello_world)),
    );

    ((*api).register_ArgonObject.unwrap())(
        reg,
        c_str("int_add"),
        ((*api).create_argon_native_function.unwrap())(c_str("int_add"), Some(int_add)),
    );

    ((*api).register_ArgonObject.unwrap())(
        reg,
        c_str("double_add"),
        ((*api).create_argon_native_function.unwrap())(c_str("double_add"), Some(double_add)),
    );

    ((*api).register_ArgonObject.unwrap())(
        reg,
        c_str("rational_add"),
        ((*api).create_argon_native_function.unwrap())(c_str("rational_add"), Some(rational_add)),
    );

    ((*api).register_ArgonObject.unwrap())(
        reg,
        c_str("yooo"),
        ((*api).create_argon_native_function.unwrap())(c_str("yooo"), Some(yooo)),
    );

    let hello_str = string {
        data: b"hello world\0".as_ptr() as *mut c_char,
        length: 11,
    };

    ((*api).register_ArgonObject.unwrap())(
        reg,
        c_str("nice"),
        ((*api).string_to_argon.unwrap())(hello_str),
    );
}}
