# these lines were initially in initial.es. they were moved to a new
# file to be able to cat other files to the end of initial.es (renamed
# system.es now).

#	noexport lists the variables that are not exported.  It is not
#	exported, because none of the variables that it refers to are
#	exported. (Obviously.)  apid is not exported because the apid value
#	is for the parent process.  pid is not exported so that even if it
#	is set explicitly, the one for a child shell will be correct.
#	Signals are not exported, but are inherited, so $signals will be
#	initialized properly in child shells.  bqstatus is not exported
#	because it's almost certainly unrelated to what a child process
#	is does.  fn-%dispatch is really only important to the current
#	interpreter loop.

noexport += pid signals apid bqstatus fn-%dispatch path home
noexport += version mveetyrev buildstring corelib
noexport += apids last eval-depth runflags unixtime options

