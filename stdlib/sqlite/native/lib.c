// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "ArgonTypes.h"
#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *Argon_connect_database(size_t argc, ArgonObject **argv,
                                    ArgonError *err, ArgonState *state,
                                    ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err)) {
    return api->ARGON_NULL;
  }

  struct string path_ar = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  char *path = malloc(path_ar.length + 1);
  path[path_ar.length] = '\0';
  memcpy(path, path_ar.data, path_ar.length);

  ArgonObject *database_obj = api->create_argon_buffer(sizeof(sqlite3 *));
  struct buffer db = api->argon_buffer_to_buffer(database_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  int rc = sqlite3_open(path, db.data);

  free(path);

  if (rc) {
    return api->throw_argon_error(err, "sqlite error", "%s",
                                  sqlite3_errmsg(*(sqlite3 **)db.data));
  }

  return database_obj;
}

ArgonObject *Argon_close_database(size_t argc, ArgonObject **argv,
                                  ArgonError *err, ArgonState *state,
                                  ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err)) {
    return api->ARGON_NULL;
  }

  struct buffer db = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  int rc = sqlite3_close(*(sqlite3 **)db.data);

  if (rc) {
    return api->throw_argon_error(err, "sqlite error", "%s",
                                  sqlite3_errmsg(*(sqlite3 **)db.data));
  }

  return api->ARGON_NULL;
}

ArgonObject *Argon_new_statement(size_t argc, ArgonObject **argv,
                                 ArgonError *err, ArgonState *state,
                                 ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err)) {
    return api->ARGON_NULL;
  }

  struct buffer db_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  struct string query = api->argon_to_string(argv[1], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *statement_obj = api->create_argon_buffer(sizeof(sqlite3_stmt *));
  struct buffer statement_buffer =
      api->argon_buffer_to_buffer(statement_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  sqlite3 **db = (sqlite3 **)db_buffer.data;
  sqlite3_stmt **stmt = (sqlite3_stmt **)statement_buffer.data;

  int rc = sqlite3_prepare_v2(*db, query.data, query.length, stmt,
                              (const char **)&query.data);

  if (rc) {
    return api->throw_argon_error(err, "sqlite error", "%s",
                                  sqlite3_errmsg(*db));
  }

  return statement_obj;
}

ArgonObject *Argon_finalise_statement(size_t argc, ArgonObject **argv,
                                      ArgonError *err, ArgonState *state,
                                      ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err)) {
    return api->ARGON_NULL;
  }

  struct buffer db_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  struct buffer statement_buffer = api->argon_buffer_to_buffer(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  sqlite3 **db = (sqlite3 **)db_buffer.data;
  sqlite3_stmt **stmt = (sqlite3_stmt **)statement_buffer.data;

  int rc = sqlite3_finalize(*stmt);

  if (rc) {
    return api->throw_argon_error(err, "sqlite error", "%s",
                                  sqlite3_errmsg(*db));
  }

  return api->ARGON_NULL;
}

ArgonObject *Argon_execute_statement(size_t argc, ArgonObject **argv,
                                     ArgonError *err, ArgonState *state,
                                     ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(3, argc, err)) {
    return api->ARGON_NULL;
  }

  struct buffer db_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  struct buffer statement = api->argon_buffer_to_buffer(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  struct array values = api->argon_to_array(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  sqlite3 **db = (sqlite3 **)db_buffer.data;
  sqlite3_stmt **stmt = (sqlite3_stmt **)statement.data;
  sqlite3_reset(*stmt);
  sqlite3_clear_bindings(*stmt);

  for (size_t i = 0; i < values.size; i++) {
    ArgonObject *item = values.items[i];
    int type = api->argon_get_ArgonType(item);
    switch (type) {
    case TYPE_STRING: {
      struct string str = api->argon_to_string(item, err);
      if (api->is_error(err)) {
        return api->ARGON_NULL;
      }
      int rc = sqlite3_bind_text(*stmt, i + 1, str.data, str.length,
                                 SQLITE_TRANSIENT);
      if (rc != SQLITE_OK) {
        return api->throw_argon_error(err, "sqlite error", "%s",
                                      sqlite3_errmsg(*db));
      }
      break;
    }
    case TYPE_NUMBER: {
      if (api->argon_is_i64(item)) {
        int64_t num = api->argon_to_i64(item, err);
        if (api->is_error(err)) {
          return api->ARGON_NULL;
        }
        int rc = sqlite3_bind_int64(*stmt, i + 1, num);
        if (rc != SQLITE_OK) {
          return api->throw_argon_error(err, "sqlite error", "%s",
                                        sqlite3_errmsg(*db));
        }
      } else {
        double num = api->argon_to_double(item, err);
        if (api->is_error(err)) {
          return api->ARGON_NULL;
        }
        int rc = sqlite3_bind_double(*stmt, i + 1, num);
        if (rc != SQLITE_OK) {
          return api->throw_argon_error(err, "sqlite error", "%s",
                                        sqlite3_errmsg(*db));
        }
      }
      break;
    }
    case TYPE_BOOL: {
      int rc =
          sqlite3_bind_int64(*stmt, i + 1, item == api->ARGON_TRUE ? 1 : 0);
      if (rc != SQLITE_OK) {
        return api->throw_argon_error(err, "sqlite error", "%s",
                                      sqlite3_errmsg(*db));
      }
      break;
    }
    case TYPE_BUFFER: {
      struct buffer buf = api->argon_buffer_to_buffer(item, err);
      if (api->is_error(err))
        break;
      int rc =
          sqlite3_bind_blob(*stmt, i + 1, buf.data, buf.size, SQLITE_TRANSIENT);
      if (rc != SQLITE_OK) {
        return api->throw_argon_error(err, "sqlite error", "%s",
                                      sqlite3_errmsg(*db));
      }
      break;
    }
    case TYPE_NULL: {
      int rc = sqlite3_bind_null(*stmt, i + 1);
      if (rc != SQLITE_OK) {
        return api->throw_argon_error(err, "sqlite error", "%s",
                                      sqlite3_errmsg(*db));
      }
      break;
    }
    }
  }

  if (!sqlite3_stmt_readonly(*stmt)) {
    int rc = sqlite3_step(*stmt);

    if (rc != SQLITE_DONE) {
      return api->throw_argon_error(err, "sqlite error", "%s",
                                    sqlite3_errmsg(*db));
    }
  }

  return api->ARGON_NULL;
}

ArgonObject *Argon_statement_fetch(size_t argc, ArgonObject **argv,
                                   ArgonError *err, ArgonState *state,
                                   ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(3, argc, err)) {
    return api->ARGON_NULL;
  }

  struct buffer db_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  struct buffer statment = api->argon_buffer_to_buffer(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  ArgonObject *append = argv[2];
  sqlite3 **db = (sqlite3 **)db_buffer.data;
  sqlite3_stmt **stmt = (sqlite3_stmt **)statment.data;

  int rc = sqlite3_step(*stmt);

  if (rc == SQLITE_ROW) {
    int num_cols = sqlite3_column_count(*stmt);

    for (int i = 0; i < num_cols; i++) {
      const char *col_name = sqlite3_column_name(*stmt, i);
      int col_type = sqlite3_column_type(*stmt, i);

      switch (col_type) {
      case SQLITE_INTEGER:
        api->call(append, 1,
                  (ArgonObject *[]){
                      api->i64_to_argon(sqlite3_column_int64(*stmt, i))},
                  err, state);
        break;
      case SQLITE_FLOAT:
        api->call(append, 1,
                  (ArgonObject *[]){
                      api->double_to_argon(sqlite3_column_double(*stmt, i))},
                  err, state);
        break;
      case SQLITE_TEXT: {
        char *str = (char *)sqlite3_column_text(*stmt, i);
        api->call(append, 1,
                  (ArgonObject *[]){
                      api->string_to_argon((struct string){str, strlen(str)})},
                  err, state);
        break;
      }
      case SQLITE_BLOB: {
        size_t size = sqlite3_column_bytes(*stmt, i);
        ArgonObject *buffer_obj = api->create_argon_buffer(size);
        struct buffer buf = api->argon_buffer_to_buffer(buffer_obj, err);
        if (api->is_error(err))
          break;
        const void *blob_buf = sqlite3_column_blob(*stmt, i);
        memcpy(buf.data, blob_buf, size);
        api->call(append, 1, (ArgonObject *[]){buffer_obj}, err, state);
        break;
      }
      case SQLITE_NULL:
        api->call(append, 1, (ArgonObject *[]){api->ARGON_NULL}, err, state);
        break;
      }
    }
    return api->ARGON_NULL;
  } else if (rc != SQLITE_DONE) {
    return api->throw_argon_error(err, "sqlite error", "%s",
                                  sqlite3_errmsg(*db));
  }
  return api->END_ITERATION;
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  api->register_ArgonObject(reg, "connect_database",
                            api->create_argon_native_function(
                                "connect_database", Argon_connect_database));
  api->register_ArgonObject(reg, "close_database",
                            api->create_argon_native_function(
                                "close_database", Argon_close_database));

  api->register_ArgonObject(
      reg, "new_statement",
      api->create_argon_native_function("new_statement", Argon_new_statement));
  api->register_ArgonObject(
      reg, "finalise_statement",
      api->create_argon_native_function("finalise_statement",
                                        Argon_finalise_statement));

  api->register_ArgonObject(
      reg, "execute_statement",
      api->create_argon_native_function("execute", Argon_execute_statement));
  api->register_ArgonObject(reg, "statement_fetch",
                            api->create_argon_native_function(
                                "statement_fetch", Argon_statement_fetch));
}