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

  bool is_cleared() {
    return is_cleared_;
  }

  t_type* get_type(std::string name) { return types_[name]; }

  size_t get_type_size() {
    for (auto type: types_) {
      if (type.first == "ads_view_type.ViewType") {
        printf("|=> %s\n", type.first.c_str());
        printf("|(exists)=> %i\n", (int)(type.second != nullptr));
      }
    }
    return types_.size();
  }

  void get_type_size_const() const {
    for (auto type: types_) {
      if (type.first == "ads_view_type.ViewType") {
        printf("||=> %s\n", type.first.c_str());
        printf("||(exists)=> %i\n", (int)(type.second != nullptr));
        printf("||(istypedef)=> %i %i\n", (int)(type.second->is_typedef()), (int)(type.second->is_typedef() && (((t_typedef *)type.second)->get_type() != nullptr)));
        printf("||(what)=> %s\n", type.second->get_name().c_str());
      }
    }
  }

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

  void add_constant(std::string name, t_const* constant, bool is_root);

  void add_lazy_constant(std::string name, t_program* program) {
    lazy_constants_[name] = program;
  }

  void set_lazy_on() {
    is_lazy_ = true;
  }

  void set_lazy_off() {
    is_lazy_ = false;
  }

  bool is_lazy() {
    return is_lazy_;
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

  std::set<t_program *> resolve_lazy_consts() {
    std::set<t_program *> programs;
    std::map<std::string, t_const*>::iterator iter_bck;
    std::map<std::string, t_program*>::iterator iter;
    for (iter = lazy_constants_.begin(); iter != lazy_constants_.end(); ++iter) {
      programs.insert((*iter).second);
    }
    lazy_constants_.clear();
    return programs;
  }

  void resolve_all_consts(bool is_force_stop_recursion) {
    std::map<std::string, t_const*>::iterator iter;
    for (iter = constants_.begin(); iter != constants_.end(); ++iter) {
      if (iter->second != nullptr) {
        t_const_value* cval = iter->second->get_value();
        t_type* ttype = iter->second->get_type();
        if (
          constants_ttype_root_scope_.find(iter->first) == constants_ttype_root_scope_.end() ||
          !constants_ttype_root_scope_[iter->first]->is_cleared()
        ) {
          resolve_const_value(cval, ttype, is_force_stop_recursion);
        }
      }
    }
  }

  bool resolve_const_value(t_const_value* const_val, t_type* ttype, bool is_force_stop_recursion);

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
    types_is_root_.clear();
    types_.clear();

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
    constants_is_root_.clear();
    constants_ttype_root_scope_.clear();
    constants_.clear();

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
    services_is_root_.clear();
    services_.clear();

    lazy_constants_.clear();

    is_cleared_ = true;
  }

private:
  // Map of names to types
  std::map<std::string, t_type*> types_;

  // Map of names to constants
  std::map<std::string, t_const*> constants_;

  // Map of names to lazy constants
  std::map<std::string, t_program*> lazy_constants_;
  bool is_lazy_;

  // Map of names to services
  std::map<std::string, t_service*> services_;

  // Map of names to types
  std::map<std::string, bool> types_is_root_;

  // Map of names to constants
  std::map<std::string, bool> constants_is_root_;
  std::map<std::string, t_scope *> constants_ttype_root_scope_;

  // Map of names to services
  std::map<std::string, bool> services_is_root_;

  bool is_cleared_;
};

#endif
