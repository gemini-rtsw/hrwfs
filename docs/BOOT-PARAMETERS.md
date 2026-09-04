# hrwfs crate boot parameters

## Production today (pisces-control)

```
boot device          : dc
unit number          : 0
processor number     : 0
host name            : pisces-control
file name            : /gemini/external/vxWorks/tornado2.0/mv2700/vxWorks
inet on ethernet (e) : 10.2.2.110:ffffff00
host inet (h)        : 10.2.2.57
gateway inet (g)     : 10.2.2.1
user (u)             : gemvx
flags (f)            : 0x8
target name (tn)     : hrwfs
startup script (s)   : /gemini/epics3.13.4/hrwfs/hrwfs/bin/ppc604/startup
```

## After the migration (mkotcsbootv2-lv1)

Only the boot server changes. The kernel stays Tornado 2.0 — hrwfs is
vxWorks 5.4 and is not moving to 2.2 — and the startup path does not change
at all.

```
boot device          : dc
unit number          : 0
processor number     : 0
host name            : mkotcsbootv2-lv1          <-- was pisces-control
file name            : /gemini/external/vxWorks/tornado2.0/mv2700/vxWorks
inet on ethernet (e) : 10.2.2.110:ffffff00
inet on backplane (b): .
host inet (h)        : 10.2.2.145                <-- was 10.2.2.57
gateway inet (g)     : .                         <-- was 10.2.2.1; see note
user (u)             : gemvx
ftp password (pw)    : .                         (blank = use rsh)
flags (f)            : 0x8
target name (tn)     : hrwfs
startup script (s)   : /gemini/epics3.13.4/hrwfs/hrwfs/bin/ppc604/startup
other (o)            : .
```

`gateway inet` — the crate (10.2.2.110/24) and the new boot server
(10.2.2.145/24) are on the same subnet, so no gateway is needed to boot;
gmoscc leaves it `.`. Production sets 10.2.2.1, which was equally unnecessary
against pisces-control at 10.2.2.57. Either works; `.` matches gmoscc. The
`routeAdd` lines in `startup/local.vws` still provide the runtime routes to
10.1.x.

## What these confirm

**1. The deploy path is `/gemini/epics3.13.4/hrwfs/hrwfs`.** The crate reads
its startup script *through the selector symlink*, not the versioned path. So
the RPM installing a real directory of that name needs **no boot-parameter
change at all** — the cleanest possible transition, and it is exactly what
gmoscc did when it replaced `setgmos` with a fixed `gmos` directory.

Note the split that exists today: the crate loads
`.../hrwfs/hrwfs/bin/ppc604/startup` through the symlink, and that script then
does `cd "/gemini/epics3.13.4/hrwfs/V3-8-5"` — the versioned path. Both have to
exist. Under the RPM there is one real directory and the `cd` re-homes to it.

**2. `gem-vxworks-tornado20` is required, and the packaging conflict is real.**
The kernel is `/gemini/external/vxWorks/tornado2.0/mv2700/vxWorks`, and
`startup/local.vws` reads `/gemini/external/vxWorks/tornado2.0/vxUsers`. Both
live under a directory that gmoscc's `gem-vxworks-tornado22` package currently
claims wholesale in its `%files`. Two packages cannot own it, so that tree has
to be split out into its own package both depend on.

**3. `/gemdata` must be exported by the new server.** `detControl.h:288`
defines `DET_CONTROL_DATA_FILE_PATH "/gemdata/ioc_data"`, so this mount is not
optional for hrwfs. gmoscc does not mount it and GMOS does not use it, so it
may not be exported on mkotcsbootv2-lv1 at all. Confirm before the first boot.
