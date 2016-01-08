#!/bin/bash
# DO NOT USE environment = COPY_ALL
#@ job_type = parallel
#@ class = CLASS
#@ node = NNODES
#@ tasks_per_node = 28
#@ wall_clock_limit = 0:35:00
#@ job_name = min-element
#@ network.MPI = sn_all,not_shared,us
#@ initialdir = $(home)/code/dash/dash/examples/bench.08.min-element/run
#@ output = job.$(schedd_host).$(jobid).out
#@ error =  job.$(schedd_host).$(jobid).err
#@ notification=never
#@ notify_user=Karl.Fuerlinger@nm.ifi.lmu.de
#@ queue
. /etc/profile
. /etc/profile.d/modules.sh

poe ../min-element