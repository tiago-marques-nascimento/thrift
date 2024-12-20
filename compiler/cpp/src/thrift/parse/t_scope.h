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

#ifndef T_SCOPE_H
#define T_SCOPE_H

#include <map>
#include <string>
#include <sstream>

#include "thrift/parse/t_type.h"
#include "thrift/parse/t_typedef.h"
#include "thrift/parse/t_service.h"
#include "thrift/parse/t_const.h"
#include "thrift/parse/t_const_value.h"
#include "thrift/parse/t_base_type.h"
#include "thrift/parse/t_map.h"
#include "thrift/parse/t_list.h"
#include "thrift/parse/t_set.h"

/**
 * This represents a variable scope used for looking up predefined types and
 * services. Typically, a scope is associated with a t_program. Scopes are not
 * used to determine code generation, but rather to resolve identifiers at
 * parse time.
 *
 */
class t_scope {
public:
  t_scope() = default;

  ~t_scope() {
  }

  void add_type(std::string name, t_type* type, bool is_root) {
    types_[name] = type;
    types_is_root_[name] = is_root;
  }

  t_type* get_type(std::string name) { return types_[name]; }

  const t_type* get_type(std::string name) const {
    const auto it = types_.find(name);
    if (types_.end() != it)
    {
       return it->second;
    }
    return nullptr;
  }

  void add_service(std::string name, t_service* service, bool is_root) {
    services_[name] = service;
    services_is_root_[name] = is_root;
  }

  t_service* get_service(std::string name) { return services_[name]; }

  const t_service* get_service(std::string name) const { 
    const auto it = services_.find(name);
    if (services_.end() != it)
    {
       return it->second;
    }
    return nullptr;
  }

  void add_constant(std::string name, t_const* constant, bool is_root) {
    if (constants_.find(name) != constants_.end()) {
      throw "Enum " + name + " is already defined!";
    } else {
      constants_[name] = constant;
      constants_is_root_[name] = is_root;
    }
  }

  t_const* get_constant(std::string name) { return constants_[name]; }

  const t_const* get_constant(std::string name) const { 
    const auto it = constants_.find(name);
    if (constants_.end() != it)
    {
       return it->second;
    }
    return nullptr;
  }

  void print() {
    std::map<std::string, t_type*>::iterator iter;
    for (iter = types_.begin(); iter != types_.end(); ++iter) {
      printf("%s => %s\n", iter->first.c_str(), iter->second->get_name().c_str());
    }
  }

  void resolve_all_consts() {
    std::map<std::string, t_const*>::iterator iter;
    for (iter = constants_.begin(); iter != constants_.end(); ++iter) {
      t_const_value* cval = iter->second->get_value();
      t_type* ttype = iter->second->get_type();
      resolve_const_value(cval, ttype);
    }
  }

  void resolve_const_value(t_const_value* const_val, t_type* ttype) {
    while (ttype->is_typedef()) {
      ttype = ((t_typedef*)ttype)->get_type();
    }

    if (ttype->is_map()) {
      printf("hummmm10\n");
      printf("hummmm100 %s\n", const_val->get_identifier().c_str());
      const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = const_val->get_map();
      std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
      for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
        resolve_const_value(v_iter->first, ((t_map*)ttype)->get_key_type());
        resolve_const_value(v_iter->second, ((t_map*)ttype)->get_val_type());
      }
    } else if (ttype->is_list()) {
      printf("hummmm9\n");
      const std::vector<t_const_value*>& val = const_val->get_list();
      std::vector<t_const_value*>::const_iterator v_iter;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        resolve_const_value((*v_iter), ((t_list*)ttype)->get_elem_type());
      }
    } else if (ttype->is_set()) {
      printf("hummmm8\n");
      const std::vector<t_const_value*>& val = const_val->get_list();
      std::vector<t_const_value*>::const_iterator v_iter;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        resolve_const_value((*v_iter), ((t_set*)ttype)->get_elem_type());
      }
    } else if (ttype->is_struct()) {
      printf("hummmm7\n");
      auto* tstruct = (t_struct*)ttype;
      const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = const_val->get_map();
      std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
      for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
        t_field* field = tstruct->get_field_by_name(v_iter->first->get_string());
        if (field == nullptr) {
          throw "No field named \"" + v_iter->first->get_string()
              + "\" was found in struct of type \"" + tstruct->get_name() + "\"";
        }
        resolve_const_value(v_iter->second, field->get_type());
      }
    } else if (const_val->get_type() == t_const_value::CV_IDENTIFIER) {
      if (ttype->is_enum()) {
        printf("hummmm6\n");
        const_val->set_enum((t_enum*)ttype);
      } else {
        t_const* constant = get_constant(const_val->get_identifier());
        if (constant == nullptr) {
          printf("hummmm5\n");
          throw "No enum value or constant found named \"" + const_val->get_identifier() + "\"!";
        }

        // Resolve typedefs to the underlying type
        t_type* const_type = constant->get_type()->get_true_type();

        if (const_type->is_base_type()) {
          printf("hummmm4\n");
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
            throw "Constants cannot be of type VOID";
          }
        } else if (const_type->is_map()) {
          printf("hummmm3\n");
          const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = constant->get_value()->get_map();
          std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;

          const_val->set_map();
          for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
            const_val->add_map(v_iter->first, v_iter->second);
          }
        } else if (const_type->is_list()) {
          printf("hummmm2\n");
          const std::vector<t_const_value*>& val = constant->get_value()->get_list();
          std::vector<t_const_value*>::const_iterator v_iter;

          const_val->set_list();
          for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
            const_val->add_list(*v_iter);
          }
        }
      }
    } else if (ttype->is_enum()) {
      // enum constant with non-identifier value. set the enum and find the
      // value's name.
      printf("hummmm1\n");
      auto* tenum = (t_enum*)ttype;
      t_enum_value* enum_value = tenum->get_constant_by_value(const_val->get_integer());
      if (enum_value == nullptr) {
        std::ostringstream valstm;
        valstm << const_val->get_integer();
        throw "Couldn't find a named value in enum " + tenum->get_name() + " for value "
            + valstm.str();
      }
      const_val->set_identifier(tenum->get_name() + "." + enum_value->get_name());
      const_val->set_enum(tenum);
    }
  }

  bool resolve_const_value(t_const_value* const_val, t_type* ttype, bool is_force_stop_recursion) {
    printf("hummmm00\n");
    while (ttype->is_typedef()) {
      if (is_force_stop_recursion) {
        bool is_same_program = ((t_typedef*)ttype)->get_symbolic().find(".") == std::string::npos;
        if (!is_same_program) {
          return false;
        }
      }
      ttype = ((t_typedef*)ttype)->get_type();
    }

    if (ttype->is_map() && !is_force_stop_recursion) {
      printf("hummmm10\n");
      printf("hummmm100 str: %s\n", const_val->get_identifier().c_str());
      printf("hummmm1000\n");
      const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = const_val->get_map();
      printf("hummmm101\n");
      std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
      printf("hummmm102\n");
      for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
        if( !resolve_const_value(v_iter->first, ((t_map*)ttype)->get_key_type(), is_force_stop_recursion) ||
            !resolve_const_value(v_iter->second, ((t_map*)ttype)->get_val_type(), is_force_stop_recursion)) {
          return false;
        }
      }
      printf("hummmm103\n");
    } else if (ttype->is_list() && !is_force_stop_recursion) {
      printf("hummmm9\n");
      const std::vector<t_const_value*>& val = const_val->get_list();
      std::vector<t_const_value*>::const_iterator v_iter;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        if(!resolve_const_value((*v_iter), ((t_list*)ttype)->get_elem_type(), is_force_stop_recursion)) {
          return false;
        }
      }
    } else if (ttype->is_set() && !is_force_stop_recursion) {
      printf("hummmm8\n");
      const std::vector<t_const_value*>& val = const_val->get_list();
      std::vector<t_const_value*>::const_iterator v_iter;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        if(!resolve_const_value((*v_iter), ((t_set*)ttype)->get_elem_type(), is_force_stop_recursion)) {
          return false;
        }
      }
    } else if (ttype->is_struct() && !is_force_stop_recursion) {
      printf("hummmm7\n");
      auto* tstruct = (t_struct*)ttype;
      const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = const_val->get_map();
      std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
      for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
        t_field* field = tstruct->get_field_by_name(v_iter->first->get_string());
        if (field == nullptr) {
          throw "No field named \"" + v_iter->first->get_string()
              + "\" was found in struct of type \"" + tstruct->get_name() + "\"";
        }
        if(!resolve_const_value(v_iter->second, field->get_type(), is_force_stop_recursion)) {
          return false;
        }
      }
    } else if (const_val->get_type() == t_const_value::CV_IDENTIFIER) {
      if (ttype->is_enum()) {
        printf("hummmm6\n");
        const_val->set_enum((t_enum*)ttype);
      } else {
        t_const* constant = get_constant(const_val->get_identifier());
        if (constant == nullptr) {
          printf("hummmm5\n");
          throw "No enum value or constant found named \"" + const_val->get_identifier() + "\"!";
        }

        // Resolve typedefs to the underlying type
        t_type* const_type = constant->get_type()->get_true_type();

        if (const_type->is_base_type()) {
          printf("hummmm4\n");
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
            throw "Constants cannot be of type VOID";
          }
        } else if (const_type->is_map()) {
          printf("hummmm3\n");
          const std::map<t_const_value*, t_const_value*, t_const_value::value_compare>& map = constant->get_value()->get_map();
          std::map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;

          const_val->set_map();
          for (v_iter = map.begin(); v_iter != map.end(); ++v_iter) {
            const_val->add_map(v_iter->first, v_iter->second);
          }
        } else if (const_type->is_list()) {
          printf("hummmm2\n");
          const std::vector<t_const_value*>& val = constant->get_value()->get_list();
          std::vector<t_const_value*>::const_iterator v_iter;

          const_val->set_list();
          for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
            const_val->add_list(*v_iter);
          }
        }
      }
    } else if (ttype->is_enum()) {
      // enum constant with non-identifier value. set the enum and find the
      // value's name.
      printf("hummmm1\n");
      auto* tenum = (t_enum*)ttype;
      t_enum_value* enum_value = tenum->get_constant_by_value(const_val->get_integer());
      if (enum_value == nullptr) {
        std::ostringstream valstm;
        valstm << const_val->get_integer();
        throw "Couldn't find a named value in enum " + tenum->get_name() + " for value "
            + valstm.str();
      }
      const_val->set_identifier(tenum->get_name() + "." + enum_value->get_name());
      const_val->set_enum(tenum);
    }
    return true;
  }

  void clear() {
    std::map<std::string, bool>::iterator is_root_iter;
    std::map<std::string, t_type*>::iterator types_iter;
    for (types_iter = types_.begin(); types_iter != types_.end(); ++types_iter) {
      for (is_root_iter = types_is_root_.begin(); is_root_iter != types_is_root_.end(); ++is_root_iter) {
        if ((*is_root_iter).first == (*types_iter).first) {
          if ((*is_root_iter).second) {
            delete (*types_iter).second;
            (*types_iter).second = nullptr;
          }
          break;
        }
      }
    }

    std::map<std::string, t_const*>::iterator constants_iter;
    for (constants_iter = constants_.begin(); constants_iter != constants_.end(); ++constants_iter) {
      for (is_root_iter = constants_is_root_.begin(); is_root_iter != constants_is_root_.end(); ++is_root_iter) {
        if ((*is_root_iter).first == (*constants_iter).first) {
          if ((*is_root_iter).second) {
            delete (*constants_iter).second;
            (*constants_iter).second = nullptr;
          }
          break;
        }
      }
    }

    std::map<std::string, t_service*>::iterator services_iter;
    for (services_iter = services_.begin(); services_iter != services_.end(); ++services_iter) {
      for (is_root_iter = services_is_root_.begin(); is_root_iter != services_is_root_.end(); ++is_root_iter) {
        if ((*is_root_iter).first == (*services_iter).first) {
          if ((*is_root_iter).second) {
            delete (*services_iter).second;
            (*services_iter).second = nullptr;
          }
          break;
        }
      }
    }
    services_.clear();
  }

private:
  // Map of names to types
  std::map<std::string, t_type*> types_;

  // Map of names to constants
  std::map<std::string, t_const*> constants_;

  // Map of names to services
  std::map<std::string, t_service*> services_;

    // Map of names to types
  std::map<std::string, bool> types_is_root_;

  // Map of names to constants
  std::map<std::string, bool> constants_is_root_;

  // Map of names to services
  std::map<std::string, bool> services_is_root_;
};

#endif
