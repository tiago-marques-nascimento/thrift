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

t_type* t_typedef::get_type() {
  return const_cast<t_type*>(const_cast<const t_typedef*>(this)->get_type());
}

const t_type* t_typedef::get_type() const {
  if (type_ == nullptr) {
    const t_type* type = get_program()->scope()->get_type(symbolic_);
    if (type == nullptr) {
      printf("Type \"%s\" not defined\n", symbolic_.c_str());
      exit(1);
    }
    return type;
  }
  return type_;
}

bool t_typedef::does_type_exist() {
  return get_program()->scope()->get_type(symbolic_) != nullptr;
}