// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2016 John Spray <john.spray@redhat.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 */

#ifndef MGR_CLIENT_H_
#define MGR_CLIENT_H_

#include "msg/Dispatcher.h"
#include "mon/MgrMap.h"

#include "msg/Connection.h"

#include "common/perf_counters.h"
#include "common/Timer.h"
#include "common/CommandTable.h"

class MMgrMap;
class MMgrConfigure;
class Messenger;
class MCommandReply;
class MPGStats;

class MgrSessionState
{
  public:
  // Which performance counters have we already transmitted schema for?
  std::set<std::string> declared;

  // Our connection to the mgr
  ConnectionRef con;
};

class MgrCommand : public CommandOp
{
  public:

  MgrCommand(ceph_tid_t t) : CommandOp(t) {}
  MgrCommand() : CommandOp() {}
};

class MgrClient : public Dispatcher
{
protected:
  CephContext *cct;
  MgrMap map;
  Messenger *msgr;

  unique_ptr<MgrSessionState> session;

  Mutex lock;

  uint32_t stats_period;
  SafeTimer     timer;

  CommandTable<MgrCommand> command_table;

  void wait_on_list(list<Cond*>& ls);
  void signal_cond_list(list<Cond*>& ls);

  list<Cond*> waiting_for_session;
  Context *report_callback;

  // If provided, use this to compose an MPGStats to send with
  // our reports (hook for use by OSD)
  std::function<MPGStats*()> pgstats_cb;

  void reconnect();

public:
  MgrClient(CephContext *cct_, Messenger *msgr_);

  void set_messenger(Messenger *msgr_) { msgr = msgr_; }

  void init();
  void shutdown();

  bool ms_dispatch(Message *m) override;
  bool ms_handle_reset(Connection *con) override;
  void ms_handle_remote_reset(Connection *con) override {}
  bool ms_handle_refused(Connection *con) override;

  bool handle_mgr_map(MMgrMap *m);
  bool handle_mgr_configure(MMgrConfigure *m);
  bool handle_command_reply(MCommandReply *m);

  void send_report();

  void set_pgstats_cb(std::function<MPGStats*()> cb_)
  {
    pgstats_cb = cb_;
  }

  int start_command(const vector<string>& cmd, const bufferlist& inbl,
		    bufferlist *outbl, string *outs,
		    Context *onfinish);
};

#endif

