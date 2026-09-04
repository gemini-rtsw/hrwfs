# hrwfs: SVN -> GitHub + Linux cross-build + gemini-rtsw-ci

Plan for doing to `hrwfs` what REL-4693 did to `gmoscc`. Written from the
V3-8-5 working copy in this directory, the gmoscc rehost procedure
(`gmoscc/docs/README-LINUX-REHOST.md`), and the dhs SVN migration
(`dhs/MIGRATION.md`).

Source: `http://source.gemini.edu/software/hrwfs-rt`, working copy is
**`tags/V3-8-5` @ r71124** (V3-8-5 = 04 March 2025, currutia, "startup updated
for Epics 7 rtconfig"). Same repository UUID as dhs
(`ee6ba543-07f8-4773-b02c-01d50e9d00ba`), so the same dump-and-convert route
applies.

Target: `git@github.com:gemini-rtsw/hrwfs.git`.

---

## 0. The headline: hrwfs shares nothing with gmoscc but the procedure

gmoscc's dependency RPMs are useless here. Every version differs:

| | gmoscc | hrwfs |
|---|---|---|
| GEM software tree | GEM8.6 | **GEM7** (`/gemini/external/GEM7`) |
| EPICS | 3.13.9 | **3.13.4** (`/gemini/epics3.13.4/...`) |
| target arch | `ppc604_long` | **`ppc604`** |
| support libs | astlib, slalib, timelib | astlib, slalib, timelib, **cfitsio**, **DHS client** |
| DHS libs | none | `/gemini/dhs/dhs/{lib,external/lib}/**mv2700T2**` |
| extra toolchain | none | **Motorola DSP56000** (`asm56000`, `dsplnk`, `cldlod`, `srec`) |
| sites built | MK only | **MK and CP both** (`hrwfsInstall` picks by hostname) |

The rehost doc budgets "roughly a day for the first IOC on a new GEM version".
hrwfs is the first IOC on GEM7, so a full dependency-RPM set has to be built:
`gem-epics3134gem7`, `gem7-epics-runtime`, `hrwfs-deplibs`,
`hrwfs-dhs-vxlibs`, and whichever Tornado the GEM7 tree wants.

## 0b. The toolchain gap — the biggest open risk

Measured from the production objects, not inferred:

```
$ file bin/ppc604/detControl
ELF 32-bit MSB relocatable, PowerPC
$ strings -a bin/ppc604/detControl | grep GCC:
GCC: (GNU) cygnus-2.7.2-960126
$ strings -a bin/ppc604/detControl | grep tornado2.0 | head -1
/usr/software/dev/packages/vxworks/tornado2.0/ppc/target/h/arch/ppc/ppc604.h
```

**hrwfs is built with Cygnus gcc 2.7.2 (January 1996), Tornado 2.0, vxWorks
5.4.** gmoscc is gcc 2.96, Tornado 2.2, vxWorks 5.5.

**ANL publishes a Linux rebuild for Tornado 2.2 only.** Its page is titled
"Tornado 2.2 and Linux", ships `gnu-tools.tor2_2-ppc-rhel5.tgz` (gcc 2.96) plus
the matching source tarball `cum.tor2_2-ppc.tgz`, and says to extract into a
Tornado *2.2.x* `WIND_BASE`. There is no 2.0 build.

This is why the gmoscc precedent does not carry over cleanly. gmoscc's rehost
compared gcc 2.96 (Solaris) against gcc 2.96 (ANL Linux) — the *same compiler
generation*, differing only in Wind River patch level, and the object
comparison found two benign systematic differences. hrwfs would compare
gcc 2.7.2 against gcc 2.96: five years and a major version apart. The
validation method still works but loses most of its force, because wholesale
differences are expected rather than suspicious.

### Where this stands after checking polaris

`$WIND_BASE/host` holds `sun4-solaris2`, `x86-win32`, `parisc-hpux10` — no
Linux. And `host/src` is **282 KB** of `demo`, `gnu.cpp`, `hutils`, `windview`:
host utilities and WindView sources, **not** the GNU compiler sources. So the
GPL sources are not on this installation.

| | approach | verdict |
|---|---|---|
| **A** | Build gcc 2.7.2 for Linux from Wind River's GPL sources, as ANL did for 2.2 | **Blocked on inputs.** Worth one question: does Gemini still hold the Tornado 2.0.2 media, or an entitlement to request the GNU sources? If yes this is the best path — it keeps production's exact compiler, so the object comparison regains full force. gcc 2.7.2 (Jan 1996) will not build on a modern toolchain, so expect an old container or a two-stage bootstrap. |
| **B** | ANL's gcc 2.96 against Tornado 2.0.2 headers, loaded by the 5.4 kernel | Plausible — both ELF PowerPC, same EABI — but needs crate testing, and the validation cannot separate a real fault from expected churn. Touches the instrument. |
| **C** | Move hrwfs to Tornado 2.2 / vxWorks 5.5 | **Larger than first assessed.** Not just a kernel swap: the EPICS target objects at `/gemini/external/GEM7` were built for 5.4 and would need rebuilding, cascading into moving hrwfs to GEM8.6. An EPICS upgrade project, not a build port. |
| **D** | Migrate everything except the ppc cross-compile | **Recommended now.** Host tools, the `gemini.dbd` byte-exact validation, the specs, dependency RPMs, history and deploy story all move to Linux/CI; ppc objects still come from a polaris build. No crate risk, nothing wasted if A later succeeds — same pipeline, one input swapped. This is the state gmoscc's rehost occupied at first (its §10 called CI output "verification-grade" and kept deploying from Solaris).

B versus C is an operational decision: both touch a running instrument.

Note either way: hrwfs targets `ppc604`, gmoscc `ppc604_long` — a different
data model, so the CONFIG differs regardless of which option is chosen.

Incidental, and useful: the production objects name their build directory as
`/home/gemvx/tcumming/hrwfs-rt/`. That is the checkout that produced the
deployed `gemini.Support`, so it is the best remaining candidate to hold a
real `db/` directory.

## 0b-RESULT. The gcc 2.96 experiment: it works

Run 2026-09-03. ANL's Linux gcc 2.96 (`gnu-tools.tor2_2-ppc-rhel5.tgz`, sha256
`b9881437...`, the same tarball gmoscc pins) against the staged Tornado 2.0.2
`target/h`, in `rockylinux:9` with `glibc.i686`, using the flags the EPICS
config actually specifies:

```
CONFIG.Vx:98            OP_SYS_CFLAGS = -DvxWorks -DV5_vxWorks -fno-builtin
CONFIG.Vx:105           -include $(VX_INCLUDE)/vxWorks.h
CONFIG.Vx.ppc604        -DCPU=PPC604 -D_GNU_TOOL -DTRUE=1
                        -mcpu=604 --no-builtin -mstrict-align
CONFIG_SITE.Vx.ppc604   -mlongcall,  VX_DIR_YES = .../tornado2.0/ppc
```

**13 of 19 sources compiled**, emitting `ELF 32-bit MSB relocatable, PowerPC`
— including the large ones (`sdsuLib.o` 124 KB, `epToVxLib.o` 92 KB,
`errorLib.o`). **Not one compiler or header incompatibility.** The 6 failures
are missing support-library headers only: `astLib.h`, `slalib.h`, `timeLib.h`,
`dhs.h`, `fitsio.h` — the five `-d` deplibs in `hrwfsInstall`, which are not
yet staged.

Warnings are cosmetic or pre-existing: `_BIG_ENDIAN` redefined (19x — gcc 2.96
predefines it, `types/vxArch.h:46` defines it again), 5 `cast to pointer from
integer of different size`, 2 const-qualifier warnings.

Also note `VX_GNU = $(VX_DIR)/host/$(WIND_HOST_TYPE)`, and `VX_DIR_YES` in
`CONFIG_SITE.Vx.ppc604` already names the tornado2.0 tree. So selecting the
Linux toolchain needs only `WIND_HOST_TYPE=x86-linux` and `HOST_ARCH=Linux`
— no new `CONFIG_SITE.Vx.Linux.ppc604` for the compiler path. The mechanism
ORNL SNS documented is doing exactly what it was designed for.

**So option B is viable at the compile level.** What remains unproven is
linking (`ldppc -r`, needs the deplib archives) and that the objects load and
run on the 5.4 kernel — which is the crate test, required for any hrwfs
release regardless of how it was built.

### Two operational gotchas worth keeping

**The 32-bit cross-compiler cannot read files on a filesystem with 64-bit
inode numbers.** `cpp` was built without large-file support, so `stat()`
returns `EOVERFLOW` and the error is the deeply unhelpful

```
cpp: .../archPpc.h: Value too large for defined data type
```

which looks like a header incompatibility and is not. It cost the first two
runs of this experiment. NFS-backed home directories hit it, which is most
Gemini workstations, so **a developer checkout on NFS will fail this way.**
Stage the trees on local disk or a tmpfs. CI is probably unaffected (fresh
ext4 on the runner), but it is worth a note in the build README.

**Docker `--tmpfs` defaults to `noexec`**, so the staged `ccppc` gives
`Permission denied`. Use `--tmpfs /build:size=2g,exec`.

## 0c. Build vs runtime — where the Tornado conflict actually is

Worth stating plainly, because it is easy to conflate:

- **Build time**, each IOC needs only its own generation. gmoscc
  `BuildRequires: gem-tornado22-linux`, hrwfs would `BuildRequires:
  gem-tornado20-linux`. Different names, different paths
  (`.../vxworks/tornado2.2/ppc` vs `.../tornado2.0/ppc`), and they need never
  be co-installed. **No conflict.**
- **Runtime**, the crate needs the vxWorks kernel and the relocatable objects
  it `ld`s. No compiler is involved at all.

**But there is a runtime packaging conflict, and it already exists.**
`gem-vxworks-tornado22`'s `%files` claims *both* trees:

```
/gemini/external/vxWorks/tornado2.2
/gemini/external/vxWorks/tornado2.0
```

It took the 2.0 tree in release 2.2-2 to get one file, `vxUsers`, which the
GMOS startup reads from the 2.0 directory. hrwfs reads **the same file** —
`bin/ppc604/local` and `bin/ppc604/startup_MINIMAL` both begin
`< /gemini/external/vxWorks/tornado2.0/vxUsers`.

Today that is a lucky accident: installing `gem-vxworks-tornado22` satisfies
hrwfs's `vxUsers` too. But if hrwfs's crate boots a 2.0 kernel from
`/gemini/external/vxWorks/tornado2.0/mv2700/vxWorks`, a `gem-vxworks-tornado20`
package shipping it **collides** with gmoscc's directory claim, and dnf will
refuse to have both on one boot server. Relevant because the pisces migration
(§1g) likely consolidates hrwfs onto the same boot server GMOS uses.

**Fix:** split `/gemini/external/vxWorks/tornado2.0` out of
`gem-vxworks-tornado22` into `gem-vxworks-tornado20`, and have gmoscc's package
`Requires:` it for `vxUsers`. One owner per generation, both co-installable —
the convention the rehost doc §9a already states. Note this modifies *gmoscc's*
packaging, so it needs coordinating rather than doing silently.

**Gated on hrwfs's boot parameters** — the `f` field naming the kernel. gmoscc's
is `/gemini/external/vxWorks/tornado2.2/mv2700/vxWorks`. If hrwfs's names a 2.0
path, the split is required; if it boots from elsewhere, `vxUsers` is the only
shared file and the present arrangement already covers it. This is now the
highest-value outstanding question.

## 1. The startup script: seven separate problems

This is where a mistake produces a crate that boots and then stops, so it gets
its own section. 1b, 1d, 1f and 1g are settled by reading the production tree
now in `~/work/hrwfs-production/MK-V3-8-5`. `startup/*.vws` are the sources; the UAE build turns each into
`bin/ppc604/<name>` with `$(install)`, `$(version)`, `$(arch)`, `$(iocpath)`
and `$(dbname)` substituted.

### 1a. `bin/ppc604/local` is not in this repository

Every startup variant does

```
< $(version)/bin/$(arch)/local
```

and `startup/Makefile.Vx` builds `bin/$(T_A)/<name>` from `$(wildcard ../*.vws)`
only. **There is no `local.vws` in `startup/`**, so nothing in this checkout
generates `local`. gmoscc has `startup/local.vws` and that is where its NFS
mounts, `routeAdd`s, `nfsAuthUnixSet` and the final `cd` to the deploy path
live.

So hrwfs's `local` is a hand-maintained file on the NFS tree that no release
has ever contained, and a crate booting a freshly-installed RPM dies on that
line. **It has to be recovered from the live tree and committed as
`startup/local.vws`** (one per site, if MK and CP differ — they almost
certainly do; the two `hostAdd`s already differ, `archie`/`10.2.4.56` at MK vs
`reggie`/`172.17.4.15` and `cportnfs-lv1` at CP).

### 1b. Re-homing — much smaller than gmoscc's

**Settled by reading the production tree** (`~/work/hrwfs-production/MK-V3-8-5`, a Dec 2020
MK build). The generated scripts reference their own payload with **relative**
paths:

```
< ./bin/ppc604/local
ld < ./bin/ppc604/gemini.Support
dbLoadDatabase "./dbd/gemini.dbd"
iocInit ("./data/resource.def")
```

So `$(version)` resolved to `.` and `$(iocpath)` to the deploy directory —
the inverse of gmoscc, where `APPLIC_INSTALL` was the build path and got
smeared through every generated file. Here the absolute path appears in exactly
three places:

| file | line |
|---|---|
| `bin/ppc604/startup*` | `cd "/gemini/epics3.13.4/hrwfs/V3-8-5"` |
| `bin/ppc604/local` | `cd "/gemini/epics3.13.4/hrwfs/V3-8-5"` |
| `bin/ppc604/startup_MINIMAL` | `cd "/gemini/epics3.13.4/hrwfs/V3-8-5/bin/ppc604"` |

That makes the spec's `%build` re-homing three `cd` lines and a guard, not a
repo-wide `sed` — provided the build reproduces `APPLIC_INSTALL = .`. Verify
that first: if `applSetup.pl` writes an absolute `APPLIC_INSTALL` in the
container, every `ld <` line goes absolute too and we are back to gmoscc's
problem.

`$(dbname)` is **empty** — hence `gemini.dbd` and `gemini.Support`, not
`geminiHrwfs.*`.

The deploy path in production is `/gemini/epics3.13.4/hrwfs/V3-8-5`, but see
§1f: that directory name no longer matches what is in it.

### 1c. Which variant does production actually boot?

Ten of them: `startup`, `startupMK`, `startupCP`, `startupMK_nodhs`,
`startupCP_nodhs`, `startupMK_dhs`, `startupCP_dhs`, `startup_nodhs`,
`startup_MINIMAL`, `dhsscript`. `startup.vws` and `startupMK*.vws` disagree
about whether `ImpMaster` is spawned and where `detDhsParamInit` points. Only
the crate's boot parameters say which file is read, and the RPM has to keep
that name working.

`dhsscript.vws` is dead code either way: it loads DHS from
`/net/alba/sw3/gemini_DHS/dhs-0.13/...` on `vw68k`, a host, path, DHS version
and architecture that no longer exist.

### 1d. The version string is hand-maintained

**Confirmed stale in production.** Both `data/hrwfsDet.pv` and
`data/hrwfsSeq.pv` in the deployed tree read

```
string $(sadtop)version = "V3-7";
```

while the directory is named V3-8-5 and the content is V3-8-4-era. Three
releases behind, exactly as gmoscc's hand-maintained string was. Recommend the
same fix: put `@VERSION@` in the
`.pv`/startup sources and have `%build` substitute `%{version}-%{release}` and
fail if any placeholder survives. A boot log then names the exact package, git
hash included.

### 1e. Everything the crate `ld`s at boot must be packaged

Read straight out of `startup.vws`. Each line is an RPM dependency, and a
missing one is a crate that stops mid-startup:

| loaded at boot | package to build |
|---|---|
| `/gemini/external/GEM7/base/bin/ppc604/{iocCore,seq}`, `extensions/bin/ppc604/pvload` | `gem7-epics-runtime` |
| `/gemini/epics3.13.4/{slalib,timelib,astlib}/*/bin/ppc604/*` + `timeSeq` | `hrwfs-deplibs` |
| `/gemini/epics3.13.4/cfitsio/cfitsio/bin/ppc604/cfitsiolib` | `hrwfs-deplibs` |
| `/gemini/dhs/dhs/external/lib/mv2700T2/lib{ers,imp,sds}.o` | `hrwfs-dhs-vxlibs` |
| `/gemini/dhs/dhs/lib/mv2700T2/lib{gen,dhs}.a` | `hrwfs-dhs-vxlibs` |
| `/gemini/external/vxWorks/tornado2.0/vxUsers` (`startup_MINIMAL`, and `local`) | `gem-vxworks-tornado22` (already ships it, ≥ 2.2-2) |

Note the `/gemini/epics3.13.4/<lib>/<lib>` double directory: the inner name is
a version-selecting symlink, the same arrangement gmoscc replaced with
`gmos-deplibs` so that `rpm -q` names the version instead of a symlink on a
file server nobody audits. Do the same here — pin the versions the symlinks
currently point at, and make the spec fail if the staged tree disagrees.

`nfsMount "cportnfs-lv1", "/gem_conf/rt/tuning", "/rtconfig"` (CP, added in
V3-8-5) is a live host dependency, not a package. It stays as-is.

### 1f. The deploy directory name has drifted from the SVN tag

`/gemini/epics3.13.4/hrwfs/V3-8-5` does **not** contain V3-8-5:

- its `RELEASE.NOTES` differs from the V3-8-5 tag's by exactly the four lines
  of the V3-8-5 stanza — so it is the release *before* V3-8-5
- its `bin/ppc604/startupCP` has no `/rtconfig` mount, which is the entire
  content of V3-8-5 ("startup updated for Epics 7 rtconfig")
- payload files are dated 14 Dec 2020; the directory itself 19 Jan 2021

So the directory was named for the *next* release in Jan 2021, and the V3-8-5
tag was not cut until March 2025 — four years later — and has never been
deployed here. **Production at MK is running a Dec 2020 build.** Confirm the
same at CP before packaging, and decide whether the first RPM ships V3-8-5 (the
tag) or reproduces what is actually running.

This is also the argument for gmoscc's fixed-directory approach: a versioned
directory whose name is maintained by hand drifts, and nothing detects it.
`rpm -q` cannot.

### 1g. `local` still mounts /gemini from pisces

```
hostAdd ("pisces-control","10.2.2.57")
nfsMount "pisces-control", "/export/gemini", "/gemini"
nfsMount "pisces-control", "/export/gemdata", "/gemdata"
nfsAuthUnixSet "pisces-control", 2966, 10, 0
```

`data/resource.def` names the same host for `EPICS_IOC_LOG_INET` and
`EPICS_TS_NTP_INET`. gmoscc already did this migration — commit
`03c3552 Migrate off pisces` — and its `local.vws` now names
`mkotcsbootv2-lv1`. hrwfs needs the same change, and the CP tree needs its own
`local.vws` with CP's server.

## 2. Two build steps that cannot be ported

Same pathology as gmoscc's Capfast and `adl2dl`: dead on Solaris too, so
neither is a Linux regression. The mitigation is the same — commit the
generated output and seed it so make treats the chain as satisfied.

### 2a. Capfast — and hrwfs has no committed `.db` at all

`capfast/Makefile.Host` builds exactly two files:

```make
DATA = hrwfsTop.db hrwfsSadTop.db
```

and those are precisely what `startup.vws` loads with `dbLoadRecords`. The 86
`.sch`/`.sym` sources are here; the generated `.db` files are not, and
`sch2edif` fails a FlexLM licence check against a decommissioned Cerro Pachón
licence server. **Nothing in this repository can produce a loadable
database.** Both files have to come off the deployed tree and be committed
(gmoscc puts them in `capfast/db/`), after which future database changes must
edit the `.db` directly and the schematics become historical documentation
only.

### 2b. dspsrc — the Motorola DSP56000 toolchain

`dspsrc/Makefile` calls `/usr/software/dev/packages/asm56000/{asm56000,dsplnk,cldlod,srec}`
and installs with `/usr/ucb/install`, which is Solaris-only. It produces ten
`.lod` files into `bin/asm56000/`, and `startup/UAE.dist` rdists that directory
to the crate. If those binaries are SPARC ELF — which `stage-buildenv.sh
inventory` will confirm — then commit the ten `.lod` files and drop `dspsrc`
from the Linux build. They are DSP microcode for the SDSU controller and change
essentially never.

### 2c. `adl`, and the missing `db` directory

`adl` is already excluded from `Makefile.Dirs` (it is only in `hrwfsInstall`'s
`-I` list), so nothing more is needed — same as gmoscc, where `adl2dl`/`edd`
have not existed for years. Better than gmoscc here: the deployed tree contains
**no `.dl` files at all**, so there are not even stale screens to explain.

**`db/` is genuinely missing and genuinely needed.** `Makefile.Dirs` lists it
twice and `hrwfsInstall` passes `-I db`, but no `db/` exists in the SVN tag.
What it builds is not optional:

| built by `db/` | loaded by `startup.vws` |
|---|---|
| `dbd/gemini.dbd` | `dbLoadDatabase "./dbd/gemini.dbd"` |
| `bin/ppc604/gemini.Support` | `ld < ./bin/ppc604/gemini.Support` |

Checked on polaris: `$EPICS` is
`/usr/software/dev/packages/epics/epics3.13.4GEM7`, and it contains **no**
`gemini.dbd` or `gemini.Support` — only the four component files
`gemini{Rec,Drv,BancommDev,SoftDev}.dbd` in `$EPICS/base/dbd/`. So
`gemini.dbd` is a `dbExpand` output, exactly as gmoscc's `db/Makefile.Vx`
produces it.

> Recorded because it cost a wrong turn: the deployed `gemini.dbd` contains
> zero occurrences of "hrwfs", which I first read as proof it was a stock file
> copied from the EPICS tree. That inference is invalid — `src/Makefile.Vx` has
> `DBDINSTALL =` empty, so hrwfs contributes no dbd of its own and an expansion
> of purely stock inputs also contains no hrwfs content. Absence of app content
> does not distinguish "copied" from "expanded from stock inputs".

**Ported from gmoscc — done.** `db/Makefile` and `db/Makefile.Vx` are now in
this repo, verbatim from gmoscc plus a provenance header. Nothing in them is
hrwfs-specific: `APPLIC_DBNAME` is empty for both projects, so both build the
same unsuffixed names, and `DBFILES` names only stock GEM tree files. All five
inputs and all five `*LIBOBJS` files were confirmed present under GEM7 on
polaris:

```
$EPICS = /usr/software/dev/packages/epics/epics3.13.4GEM7
  base/dbd/gemini{Rec,Drv,BancommDev,SoftDev}.dbd        391/111/211/198 bytes
  base/dbd/gemini{Rec,Drv,BancommDev,SoftDev}LIBOBJS     408/146/76/164 bytes
  base/templates/makeBaseApp/top/exampleApp/src/base.dbd      4661 bytes
  base/templates/makeBaseApp/top/exampleApp/src/baseLIBOBJS   4210 bytes
  base/bin/solaris/dbExpand
```

`Makefile.Dirs` listed `DIRS += db` twice; the duplicate is removed, keeping
the trailing entry as gmoscc has it (`db/Makefile.Vx` globs
`$(APPLIC_INSTALL)/dbd` for app dbd files, so it must run last).

Neither hrwfs checkout on polaris has `db/` **or** `dbd/` — so neither has ever
been built there, and the directory is not recoverable from that host. The port
plus the validation below is the answer.

**Free exact validation:** `dbExpand` over those inputs should reproduce
production's `dbd/gemini.dbd` byte-for-byte — 340726 bytes, md5
`d2dd08d4919b6b776e9a3155381d7c6c`. If it does, the ported `db/` is provably
the right one. That is a stronger check than anything available for the
compiled objects.

## 3. Work plan

### Phase 0 — history to GitHub (independent of everything else)

`source.gemini.edu` is unreachable from `hstecher-ld1` (ICMP answers, every TCP
port filtered), so the dump has to be taken elsewhere and converted here. This
is the dhs route, already proven.

- [ ] `tools/migration/svn-dump.sh info` on `hbfswgbld-lv1` — layout, tag list,
      revision count, author list
- [ ] `tools/migration/svn-dump.sh dump` there, under `screen` (hours: svnrdump
      replays every revision in the range, and the repo is at r71k+)
- [ ] copy `hrwfs-rt.dump.gz` + `hrwfs-authors.txt` back
- [ ] `tools/migration/svn-to-git.sh load` then `convert` then `push`
- [ ] verify the converted tip against this working copy file-for-file

### Phase 1 — build environment (blocks the build)

- [ ] `tools/linux-build/stage-buildenv.sh inventory` on the Solaris host that
      builds hrwfs — this answers the GEM7/Tornado/asm56000 questions
- [ ] `... buildenv` there; `... runtime` and `... deployed` on the boot server
- [ ] assemble the trees at their original absolute paths, add the ANL Linux
      cross-toolchain for whichever Tornado GEM7 uses
- [ ] rebuild the EPICS 3.13.4 host tools for `HOST_ARCH=Linux`
      (rehost doc §5; expect the same two patches)
- [ ] `setup.sh` equivalent: `.applTop`, `applSetup.pl -T ppc604 ...` with
      `hrwfsInstall`'s exact arguments minus `adl` and `dspsrc`, `.db` seeding

### Phase 2 — dependency RPMs (the "copy, don't build" packages)

Specs with no `%build`, only `cp -a` of the staged trees, per
`gmoscc/tools/linux-build/rpm/`:

- [ ] `gem-epics3134gem7` — the build tree under `/usr/software/dev/packages/...`
      plus `/etc/profile.d/gem7.sh`
- [ ] `gem7-epics-runtime` — `/gemini/external/GEM7/**/bin/ppc604`
- [ ] `hrwfs-deplibs` — astlib/slalib/timelib/cfitsio, versions pinned
- [ ] `hrwfs-dhs-vxlibs` — the `mv2700T2` DHS client libraries
- [ ] confirm `gem-vxworks-tornado22` covers the kernel hrwfs's crate boots

### Phase 3 — pipeline

- [ ] `git submodule add -b main https://github.com/gemini-rtsw/gemini-rtsw-ci.git gemini-rtsw-ci`
- [ ] `.github/workflows/ci.yml` — `el: ['9']`, plus the `publish` job
- [ ] `hrwfs.spec` — pinned `BuildRequires`, mandatory `%package devel`,
      re-homing + `@VERSION@` substitution in `%build`, payload mirroring
      `startup/UAE.dist`
- [ ] grant the repo **Write** on the `rpm-repo` package
- [ ] `custom-repo-setup.sh` if the 32-bit cross-tools need the glibc lockstep
      upgrade gmoscc needs
- [ ] `.gitignore` for the generated dirs (`config/ bin/ lib/ include/ dbd/
      data/ O.*/ Distfile .applTop resource.def UAE.dist`)

### Phase 4 — validation

- [ ] object-level comparison against a Solaris build of the same commit,
      per-function not per-address (rehost doc §8)
- [ ] `rpm -ql` the result against the live deployed tree, file for file — this
      is what caught the two dangerous defects in the dhs port

## 4. What I need from you

Numbered so you can answer piecemeal. **1-3 unblock Phase 0, 4-8 unblock the
build.**

1. ~~The SVN dump and conversion.~~ **DONE — history is complete.**
   `hrwfs-rt` contains all 26 years: **191 commits, 1999-03-16 to 2025-03-05**,
   31 tags (V0 .. V3-8-5) and the `epics-3-12` branch. Converted and verified:
   the git `V3-8-5` tag tree is byte-identical to the SVN working copy.

   The r68863/2020-02-20 `A /hrwfs-rt` revision was a bare `svn mkdir` followed
   by an `svnadmin load` of a cvs2svn dump. `svnadmin load` preserves
   `svn:date`/`svn:author`, so those commits carry their original 1999-2019
   dates and the tag commits still say "manufactured by cvs2svn". **The sibling
   `hrwfs` project is not needed.** What misled me was
   `svn log --limit 1 -r 1:HEAD`, which returns the first revision by *global
   number* -- the empty mkdir -- not the oldest content.

   Two gotchas worth keeping, both now handled in `tools/migration/`:
   - `svnrdump` elides the dump root's own directory node, so r68863 arrives
     with no nodes and every child path absolute (`hrwfs-rt/...`). The load
     fails on the first child until `hrwfs-rt` is created. (dhs hit the mirror
     image: relative paths in the first revision, absolute after.)
   - 30 revisions carry **no** `svn:author` property. `svn log --xml` omits the
     element entirely; only the text output shows `(no author)`, which is the
     literal key git-svn looks up -- and with `--authors-file` it aborts
     partway through on a missing key.

2. ~~Is trunk ahead of `tags/V3-8-5`?~~ **No** — trunk's tip is r71123 and the
   tag is r71124, so the tag *is* the tip of trunk. (The `db/` half of this
   question is moot; see §2c.)
3. **Import trunk only, or trunk + all tags + branches?** I have written the
   converter for `--stdlayout` (everything), because the tags are the release
   history — say if you want trunk only.
4. ~~Which host builds hrwfs today?~~ **`polaris`, environment alias `GEM7`**
   — same host as gmoscc, different alias. Still needed from it:
   `stage-buildenv.sh inventory` and `buildenv`, for the EPICS 3.13.4 / GEM7
   tree, the Tornado version, and whether the `asm56000` binaries are SPARC.
5. ~~The deployed tree.~~ **Received** — the MK tree, as
   `~/work/hrwfs-production/MK-V3-8-5`. It carried everything asked for, plus
   `bin/ppc604/gemini.Support` and `dbd/gemini.dbd` (the missing `db/`
   directory's output). **Still needed: the CP tree**, for its own `local`,
   `resource.def`, and to check `defDetContHRCP.dat` drift (see question 11).
6. **What fixed deploy path do you want?** Production is
   `/gemini/epics3.13.4/hrwfs/V3-8-5` — a versioned directory whose name has
   already drifted four years out of step with its contents (§1f). gmoscc
   replaced exactly this with a fixed `/gemini/GEM8.6/gmos/gmos`, so I would
   suggest `/gemini/epics3.13.4/hrwfs/hrwfs` — same parent, fixed leaf, and
   `rpm -q` becomes the version of record. The crate's boot parameters name
   this path, so I need the current ones before changing it.
7. ~~One package or two for MK and CP?~~ **Settled: two.** `src/Makefile.Vx`
   passes `-D$(APPLIC_SITE)`, and five `#if (MK)` blocks act on it —
   `wfsHrwfsDb.c:440` (the `dc:detTemp` CAD default and its limits),
   `detControl.c` x3 (which `defDetContHR{MK,CP}.dat` is read), and
   `wfsLib.c:888` (the `Gemini-North`/`Gemini-South` fallback). The **compiled
   objects differ**, so one RPM cannot serve both sites: build the spec twice
   (`--define site=MK|CP`) into `hrwfs-mk` / `hrwfs-cp`, or do MK first as
   gmoscc did.

   Worth noting for later, not now: three of the five only pick a filename that
   already ships in the payload, and the startup scripts already
   `putenv "SITE=MK"`. So the site *could* become a runtime lookup and collapse
   this to one package — but that is a code change, not a mechanical port.
8. **Which startup variant do the crates actually boot?** From the boot
   parameters — I need the real `startup script (s)` line, not a guess.

Two more, lower priority:

9. Is HRWFS still operational at either site, and is a reboot for testing
   possible? The rehost doc's conclusion is that normal IOC release testing is
   the remaining validation, which needs a crate.
10. Are the Motorola DSP56000 tools licensed to Gemini in any form that runs on
    Linux? If yes, `dspsrc` can stay in the build; if not, the `.lod` files get
    committed and it does not. (All ten are now in hand from production.)
11. ~~`par/defDetContHRCP.dat` drift.~~ **Resolved, nothing to decide.** It is
    the other way round from how I first read the diff: **SVN has `2355`**
    (trunk, the V3-8-5 tag and the working copy), committed 2023-03-02 by
    tgagg as "(Updated with CR-23 3925)"; **production has the old `2344`**.
    So this is not uncommitted drift — it is a committed 2023 change that was
    never deployed, because production is a Dec 2020 build (§1f). SVN is
    authoritative.
