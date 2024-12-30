/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <cstdio>

#include "thrift/parse/t_typedef.h"
#include "thrift/parse/t_program.h"

void t_scope::add_constant(std::string name, t_const* constant, bool is_root) {
    if (!is_lazy_ && constants_.find(name) != constants_.end()) {
      throw "Enum " + name + " is already defined!";
    } else {
      constants_[name] = constant;
      constants_is_root_[name] = is_root;
      if (constant->get_type()->get_program() != nullptr) {
        constants_ttype_root_scope_[name] = constant->get_type()->get_program()->scope();
      }
    }
  }

bool t_scope::resolve_const_value(t_const_value* const_val, t_type* ttype, bool is_force_stop_recursion) {
  while (ttype->is_typedef()) {
    if (!((t_typedef*)ttype)->does_type_exist()) {
      return false;
    }
    ttype = ((t_typedef*)ttype)->get_type();
  }

  if (ttype->is_map()) {
    const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = const_val->get_map();
    std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
    for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
      if (
          (
            is_force_stop_recursion &&
            ((t_map*)ttype)->get_key_type()->get_program() != nullptr &&
            ((t_map*)ttype)->get_key_type()->get_program()->scope() != this
          ) ||
          !resolve_const_value(v_iter->first, ((t_map*)ttype)->get_key_type(), is_force_stop_recursion)
      ) {
        return false;
      }
      if (
          (
            is_force_stop_recursion &&
            ((t_map*)ttype)->get_val_type()->get_program() != nullptr &&
            ((t_map*)ttype)->get_val_type()->get_program()->scope() != this
          ) ||
          !resolve_const_value(v_iter->second, ((t_map*)ttype)->get_val_type(), is_force_stop_recursion)
      ) {
        return false;
      }
    }
  } else if (ttype->is_list()) {
    const std::vector<t_const_value*>& val = const_val->get_list();
    std::vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      if (
        (
          is_force_stop_recursion &&
          ((t_list*)ttype)->get_elem_type()->get_program() != nullptr &&
          ((t_list*)ttype)->get_elem_type()->get_program()->scope() != this
        ) ||
        !resolve_const_value((*v_iter), ((t_list*)ttype)->get_elem_type(), is_force_stop_recursion)
      ) {
        return false;
      }
    }
  } else if (ttype->is_set()) {
    const std::vector<t_const_value*>& val = const_val->get_list();
    std::vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      if (
        (
          is_force_stop_recursion &&
          ((t_set*)ttype)->get_elem_type()->get_program() != nullptr &&
          ((t_set*)ttype)->get_elem_type()->get_program()->scope() != this
        ) ||
        !resolve_const_value((*v_iter), ((t_set*)ttype)->get_elem_type(), is_force_stop_recursion)
      ) {
        return false;
      }
    }
  } else if (ttype->is_struct()) {
    auto* tstruct = (t_struct*)ttype;
    const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = const_val->get_map();
    std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
    for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
      t_field* field = tstruct->get_field_by_name(v_iter->first->get_string());
      if (field == nullptr) {
        if (is_lazy_) {
          throw "No field named \"" + v_iter->first->get_string()
              + "\" was found in struct of type \"" + tstruct->get_name() + "\"";
        }
        return false;
      }
      if (
        (
          is_force_stop_recursion &&
          field->get_type()->get_program() != nullptr &&
          field->get_type()->get_program()->scope() != this
        ) ||
        !resolve_const_value(v_iter->second, field->get_type(), is_force_stop_recursion)
      ) {
        return false;
      }
    }
  } else if (const_val->get_type() == t_const_value::CV_IDENTIFIER) {
    if (ttype->is_enum()) {
      const_val->set_enum((t_enum*)ttype);
    } else {
      t_const* constant = get_constant(const_val->get_identifier());
      if (constant == nullptr) {
        if (is_lazy_) {
          throw "No enum value or constant found named \"" + const_val->get_identifier() + "\"!";
        }
        return false;
      }

      // Resolve typedefs to the underlying type
      t_type* const_type = constant->get_type()->get_true_type();

      if (const_type->is_base_type()) {
        switch (((t_base_type*)const_type)->get_base()) {
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
        case t_base_type::TYPE_BOOL:
        case t_base_type::TYPE_I8:
          const_val->set_integer(constant->get_value()->get_integer());
          break;
        case t_base_type::TYPE_STRING:
          const_val->set_string(constant->get_value()->get_string());
          break;
        case t_base_type::TYPE_UUID:
          const_val->set_uuid(constant->get_value()->get_uuid());
          break;
        case t_base_type::TYPE_DOUBLE:
          const_val->set_double(constant->get_value()->get_double());
          break;
        case t_base_type::TYPE_VOID:
          if (is_lazy_) {
            throw "Constants cannot be of type VOID";
          }
          return false;
        }
      } else if (const_type->is_map()) {
        const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = constant->get_value()->get_map();
        std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;

        const_val->set_map();
        for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
          const_val->add_map(v_iter->first, v_iter->second);
        }
      } else if (const_type->is_list()) {
        const std::vector<t_const_value*>& val = constant->get_value()->get_list();
        std::vector<t_const_value*>::const_iterator v_iter;

        const_val->set_list();
        for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
          const_val->add_list(*v_iter);
        }
      }
    }
  } else if (ttype->is_enum()) {
    auto* tenum = (t_enum*)ttype;
    t_enum_value* enum_value = tenum->get_constant_by_value(const_val->get_integer());
    if (enum_value == nullptr) {
      std::ostringstream valstm;
      valstm << const_val->get_integer();
      if (is_lazy_) {
        throw "Couldn't find a named value in enum " + tenum->get_name() + " for value "
            + valstm.str();
      }
      return false;
    }
    const_val->set_identifier(tenum->get_name() + "." + enum_value->get_name());
    const_val->set_enum(tenum);
  }
  return true;
}