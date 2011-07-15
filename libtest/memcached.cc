/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libtest/common.h>

#include <libmemcached/memcached.h>
#include <libmemcached/util.h>

#include "util/instance.h"
#include "util/operation.h"

using namespace gearman_util;
using namespace libtest;

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libtest/server.h>
#include <libtest/wait.h>

#include <libtest/memcached.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

class GetPid : public Instance::Finish
{
private:
  pid_t _pid;

public:
  GetPid() :
    _pid(-1)
  { }

  pid_t pid()
  {
    return _pid;
  }


  bool call(const bool success, const std::string &response)
  {
    _pid= -1;

    if (success and response.size())
    {
      _pid= atoi(response.c_str());
    }

    if (_pid < 1)
    {
      _pid= -1;
      return true;
    }

    return false;
  }
};

using namespace libtest;

class Memcached : public Server
{
public:
  Memcached(const std::string& host_arg, in_port_t port_arg) :
    Server(host_arg, port_arg)
  { }

  pid_t get_pid()
  {
    // Memcached is slow to start, so we need to do this
    if (not pid_file().empty())
    {
      Wait wait(pid_file(), 0);

      if (not wait.successful())
      {
        Error << "Pidfile was not found:" << pid_file();
        return -1;
      }
    }

    memcached_return_t rc;
    _pid= libmemcached_util_getpid(hostname().c_str(), port(), &rc);
    if (memcached_failed(rc))
    {
      Error << "libmemcached_util_getpid(" << memcached_strerror(NULL, rc) << ") pid: " << _pid << " for:" << *this;
    }

    return _pid;
  }

  bool ping()
  {
    // Memcached is slow to start, so we need to do this
    if (not pid_file().empty())
    {
      Wait wait(pid_file(), 0);

      if (not wait.successful())
      {
        Error << "Pidfile was not found:" << pid_file();
        return -1;
      }
    }

    memcached_return_t rc;
    bool ret= libmemcached_util_ping(hostname().c_str(), port(), &rc);
    if (memcached_failed(rc))
    {
      Error << "libmemcached_util_ping(" << memcached_strerror(NULL, rc) << ")";
    }
    return ret;
  }

  const char *name()
  {
    return "memcached";
  };

  const char *executable()
  {
    return MEMCACHED_BINARY;
  }

  const char *pid_file_option()
  {
    return "-P ";
  }

  const char *daemon_file_option()
  {
    return "-d";
  }

  const char *log_file_option()
  {
    return NULL;
  }

  const char *port_option()
  {
    return "-p ";
  }

  bool is_libtool()
  {
    return false;
  }

  // Memcached's pidfile is broken
  bool broken_pid_file()
  {
    return true;
  }

  bool build(int argc, const char *argv[]);
};


#include <sstream>

bool Memcached::build(int argc, const char *argv[])
{
  std::stringstream arg_buffer;

  if (getuid() == 0 or geteuid() == 0)
  {
    arg_buffer << " -u root ";
  }

  for (int x= 1 ; x < argc ; x++)
  {
    arg_buffer << " " << argv[x] << " ";
  }

  set_extra_args(arg_buffer.str());

  return true;
}

namespace libtest {

Server *build_memcached(const char *hostname, in_port_t try_port)
{
  return new Memcached(hostname, try_port);
}

}

