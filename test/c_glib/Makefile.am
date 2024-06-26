#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#
AUTOMAKE_OPTIONS = subdir-objects serial-tests nostdinc

noinst_LTLIBRARIES = libtestcglib.la
nodist_libtestcglib_la_SOURCES = \
	gen-c_glib/t_test_second_service.c \
	gen-c_glib/t_test_second_service.h \
	gen-c_glib/t_test_thrift_test.c \
	gen-c_glib/t_test_thrift_test.h \
	gen-c_glib/t_test_thrift_test_types.c \
	gen-c_glib/t_test_thrift_test_types.h

libtestcglib_la_LIBADD = $(top_builddir)/lib/c_glib/libthrift_c_glib.la

precross: libtestcglib.la test_client test_server

check_PROGRAMS = \
	test_client \
	test_server

test_client_SOURCES = \
	src/test_client.c

test_client_LDADD = \
	libtestcglib.la \
	$(top_builddir)/lib/c_glib/libthrift_c_glib.la

test_server_SOURCES = \
	src/thrift_test_handler.c \
	src/thrift_test_handler.h \
	src/thrift_second_service_handler.c \
	src/thrift_second_service_handler.h \
	src/test_server.c

test_server_LDADD = \
	libtestcglib.la \
	$(top_builddir)/lib/c_glib/libthrift_c_glib.la

#
# Common thrift code generation rules
#
gen-c_glib/t_test_second_service.c  gen-c_glib/t_test_second_service.h  gen-c_glib/t_test_thrift_test.c  gen-c_glib/t_test_thrift_test.h  gen-c_glib/t_test_thrift_test_types.c  gen-c_glib/t_test_thrift_test_types.h: $(top_srcdir)/test/v0.16/ThriftTest.thrift $(THRIFT)
	$(THRIFT) --gen c_glib -r $<

AM_CFLAGS = -g -Wall -Wextra $(GLIB_CFLAGS) $(GOBJECT_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/lib/c_glib/src -Igen-c_glib -I$(top_builddir)/lib/c_glib/src/thrift
AM_LDFLAGS = $(GLIB_LIBS) $(GOBJECT_LIBS) @GCOV_LDFLAGS@

clean-local:
	$(RM) -r gen-c_glib/
	$(RM) test_client
	$(RM) test_server
	$(RM) libtestcglib.la
	find . -type f -iname "*.o" | xargs rm -f

dist-hook:
	$(RM) -r $(distdir)/gen-c_glib/
	$(RM) $(distdir)/test_client
	$(RM) $(distdir)/test_server
	$(RM) $(distdir)/libtestcglib.la
	find $(distdir) -type f -iname "*.o" | xargs rm -f

distdir:
	$(MAKE) $(AM_MAKEFLAGS) distdir-am

EXTRA_DIST = \
	src
