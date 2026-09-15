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

### Update: compile AND link both work

With the support libraries staged (astlib V1-4, slalib V1-9-4, timelib V1-8-6,
cfitsio V4-1), **15 of 19 sources compile** and **`ldppc -r` links them into a
252 KB relocatable `wfsLibrariesHrwfs.o`** — the exact module `startup.vws`
does `ld <` on. The undefined symbols left are what a vxWorks partial link
should leave: `__errno`, `__ctype`, `bzero`, `ca_array_get` (channel access),
`astSetctx` (astlib), `aio_read`. All are resolved at boot by the kernel,
`iocCore` and the deplibs the startup loads before this module.

The 4 remaining failures are **one missing header**, `dhs.h` — `autoPath`,
`detControl`, `seqControl` and `wfsHrwfsDb` include it via `detControl.h:61`.
Nothing else.

### Update 2: 19 of 19 compile, all 10 modules link

With `dhs.h` staged (`/gemini/dhs/dhs/include`, 44 headers), **every source
compiles and every module `startup.vws` loads links**:

```
wfsLibrariesHrwfs  252514      wfsResourceMonitor   5633
wfsHrwfsDb         324520      hrwfsConfig          2369
detControl         234384      wfsSite              1129
seqControl          16962      fpscr                 801
simpleLogHrwfs       8355      autoPath             2998
```

The module grouping is not guesswork: `src/Makefile.Vx` puts `LIBSRCS.c`
(cicsCarHealth, cicsLib, epToVxLib, errorLib, sdsuLib, timeoutLib, wfsLib,
wfsWcs) into `LIBNAME = wfsLibrariesHrwfs`, and each `APPLSRCS.c` entry becomes
its own `PROD` module.

**But this is not yet a faithful build, and the sizes prove it.** Against
production:

| module | production | mine | ratio |
|---|---|---|---|
| wfsHrwfsDb | 394285 | 324520 | 1.2x |
| detControl | 422049 | 234384 | 1.8x |
| wfsLibrariesHrwfs | 676104 | 252514 | 2.7x |
| seqControl | 97263 | 16962 | 5.7x |
| wfsSite | 24186 | 1129 | 21x |
| fpscr | 16622 | 801 | 21x |

`LD = ldppc -r` and `LINK.c = $(LD) $(LDFLAGS) -o`, so the mechanism is right,
but the real `PROD` rule evidently links in more than one object — most likely
`DEPLIBS` members, the `munch`-generated C++ constructor table (`RULES.Vx:114`
builds `%.out` from `nm | munch`), or debug sections. A 20x ratio on a
one-object module cannot be explained by flags alone.

**So what is proven is the toolchain, not the build.** gcc 2.96 accepts the
Tornado 2.0.2 headers, compiles all 19 sources, and `ldppc` links them — no
compiler or header incompatibility exists. What is *not* yet proven is that a
faithful UAE build reproduces production's modules. That needs the real
`gmake`, which needs the EPICS host tools built for `HOST_ARCH=Linux`.

Then, and only then, the crate test.

### Update 7: THE FULL UAE BUILD WORKS

`./tools/linux-build/setup.sh && gmake` completes with **exit 0** in
`rockylinux:9`, descending into all five directories (`capfast db par src
startup`) and producing the **complete payload** — every file production has in
`bin/ppc604`, `dbd`, `data` and `include`, with nothing missing and nothing
extra.

Three things had to be fixed to get there, all recorded in the scripts:

1. **`CONFIG_SITE.Vx.Linux.ppc604`** (new, in `tools/linux-build/patches/`).
   Upstream `CONFIG_HOST_ARCH.Linux:8` sets `WIND_HOST_TYPE = Linux`, and
   `CONFIG.Vx:20` derives `VX_GNU_YES = $(VX_DIR)/host/$(WIND_HOST_TYPE)`, so
   the build looks for `host/Linux/bin/ccppc` while ANL's toolchain unpacks
   into `host/x86-linux` — `Error 127`. Setting `WIND_HOST_TYPE` in the
   environment does not help: a makefile assignment beats the environment.
   This is the override point EPICS 3.13.4 already documents, with
   commented-out ORNL SNS examples of exactly this case.
2. **`applSetup.pl` must be invoked through `perl`.** Its shebang is the
   Solaris path `/usr/software/dev/solaris/bin/perl`. A symlink works, but not
   when `/usr/software` is a bind mount — the mount hides whatever the image
   put underneath it.
3. **`gmake` needs `HOST_ARCH` in the environment** (hence
   `tools/linux-build/gem-env.sh`), or the build resolves
   `CONFIG_HOST_ARCH.unsupported`. And when `applSetup.pl` fails, the
   top-level `Makefile` uses `-include $(APPLIC_TOP)/config/CONFIG`, so
   `gmake` silently falls through to its first target — `release` — and tars
   the source tree, **exiting 0**. `setup.sh` now asserts `config/CONFIG`,
   `config/CONFIG.Defs` and `config/RULES.Dirs` exist rather than letting that
   happen.

Confirmed incidentally: `APPLIC_VERSION = .` (which is why the generated
scripts use relative `./bin/ppc604/...`), `APPLIC_INSTALL = <checkout>`, and
`APPLIC_IOCPATH` empty — so re-homing for the RPM is setting `APPLIC_IOCPATH`,
exactly as §1b predicted.

### Update 8: the build is faithful — validated against production

**The code is equivalent, and the size difference is fully explained.**

With debug sections excluded, `.text` and `.data` are **identical sizes**:
`fpscr` `.text=92` both; `wfsSite` `.data=124+24` both. The whole raw-size
delta is stabs debug info that production carries and this build does not
(`fpscr`: 15,693 bytes of `.stab`/`.stabstr` in production, 0 here).

Stripped of debug and `.comment`, the built objects are consistently *slightly
larger* — `wfsHrwfsDb` 324268 vs 324420 (1.0005x), `detControl` 228036 vs
237428 (1.04x), `wfsLibrariesHrwfs` 241360 vs 261048 (1.08x). Disassembly shows
why, and it is the same cause gmoscc characterized:

| module | production `bl` | built `bl` | production `blrl` | built `blrl` |
|---|---|---|---|---|
| `wfsResourceMonitor` | 2 | **0** | 33 | 35 |
| `detControl` | 28 | **0** | 2195 | 2275 |
| `seqControl` | 0 | 0 | 220 | 237 |

gcc 2.7.2 emitted some direct `bl` despite `-mlongcall`; ANL's gcc 2.96 routes
**every** call through a register. Zero direct `bl` anywhere in the built
modules. A direct `bl` has only ±32 MB reach, so the newer behaviour is
*stricter and safer* — gmoscc's exact conclusion, now reproduced independently
on a second IOC across a wider compiler gap. `fpscr`'s instruction-mnemonic
histogram is byte-for-byte identical between the two builds.

**One deliberate difference to decide on:** this build emits no stabs debug
info where production has it. Not needed functionally, and gmoscc's §8 notes
that debug sections embed build paths (which is why its comparison stripped
them) — so leaving them out gives a smaller, more reproducible payload. But it
also means a Tornado target-server debugging session against a deployed crate
would have no symbols beyond the symbol table. Add `-gstabs` if that matters.

### Update 5: the EPICS host tools build for Linux

All seven directories (`tools include libCom toolsComm dbStatic sequencer
uae`) build cleanly with gcc 11.5.0 and **no source patches** — better than the
GEM8.6 tree, which needed two. We now have `dbExpand`, `macTest`, `snc`,
`antelope`, `e_flex`, `dbToRecordtypeH`, `dbToMenuH` plus the perl/sh helpers
including `applSetup.pl`. (`sf2db` is absent; it is the Capfast tool we do not
need.) Native x86-64 is fine.

> **Retracted:** an earlier version of this section claimed the tools had to be
> built 32-bit because a 64-bit `dbExpand` silently emitted a database with no
> `asl()`/`extra()` declarations, and suggested gmoscc might carry the same
> latent bug. **That was wrong, and gmoscc is unaffected.** Three things
> establish it: the full expansion produces *byte-identical* output from 64-bit
> and 32-bit tools (333265 bytes, md5 `cefca987ee6789fce07b82677db11800` from
> both); the 0-of-each measurement came from expanding `aiRecord.dbd` alone,
> which errors with "menu not found" and writes a 0-byte file — the 32-bit
> tools behave identically on that input; and the apparent shortfall was a
> misread **sorted** diff, where one extra record type's repeated `dbCommon`
> lines looked like missing declarations. Real counts: mine `asl 151 /
> extra 600 / 35 recordtypes`, production `asl 152 / extra 615 / 36`. The delta
> is exactly the one `cmdTimeout` record type.
>
> `build-host-tools.sh` still carries an optional `M32=yes` path, since
> matching the Solaris tools' 32-bit-ness is defensible, but it is **not
> required** and is off by default.

Two real traps hit along the way, both worth keeping:

- **Never pass `USR_LDFLAGS=` on the make command line.** A command-line
  assignment overrides in-makefile `+=` appends, so `USR_LDFLAGS=-m32` wiped
  the `-L` paths `CONFIG.Host.UnixCommon` appends and every link died with
  `cannot find -lDb -lCom` — while the libraries sat in `lib/Linux`.
- **The build tree must live on a filesystem with 32-bit inode numbers**, for
  the *cross*-compiler (which really is 32-bit). The `EOVERFLOW` — "Value too
  large for defined data type", reported against a *header* — was not NFS:
  `/home` here is xfs with 64-bit inodes (`15572935680`), while `/tmp` is
  `268436222`. The tree is staged on `/tmp`.

If the compiler ever does need overriding, the point is `GCC`/`G++`, not
`ACC`/`CCC`: `CONFIG_COMMON` has `CC = $($(ANSI)_$(CMPLR))` with `ANSI=GCC`
and `CMPLR=STRICT`.

### Update 6: production's `gemini.dbd` is a foreign artifact

The 32-bit `dbExpand` still does not reproduce production's file byte-for-byte
(333265 vs 340726), and the reason is not the toolchain:

- production's contains `recordtype(cmdTimeout)` and `menu(menuCTOState)`,
  which **this tree cannot produce** — `geminiRec.dbd` includes no
  `cmdTimeoutRecord`;
- yet the staged tree is *newer* in `menuScan.dbd`, which gained a
  `.005 second` choice production's file lacks.

Both cannot be true of one tree. Together with the fact that hrwfs has never
had a `db/` directory — so its build never generated this file — the
explanation is that production's `dbd/gemini.dbd` was **copied from another
IOC's build**, against a tree with a newer `geminiRec.dbd`.

So byte-identity with it is the wrong target. The right test is coverage, and
it passes: **all 15 record types hrwfs loads** (`ao apply bo cad calc calcout
car fanout genSub longin longout seq sir stringin stringout`) **are defined**
in the generated database, which carries 35. The two menus production has extra
belong to the unused `cmdTimeout` record; hrwfs references `CTOState` nowhere
in its source or its `.db` files.

### Update 3: what the crate test actually has to prove

Every other module the crate loads at boot was built with **the same compiler
production's hrwfs was** — checked, not assumed:

```
iocCore     GCC: (GNU) cygnus-2.7.2-960126
seq         GCC: (GNU) cygnus-2.7.2-960126
astlib      GCC: (GNU) cygnus-2.7.2-960126
slalib      GCC: (GNU) cygnus-2.7.2-960126
timelib     GCC: (GNU) cygnus-2.7.2-960126
libdhs.a    GCC: (GNU) cygnus-2.7.2-960126
```

So under option B the risk is sharper than "does it run on a 5.4 kernel". Our
gcc 2.96 modules would be loaded into a runtime where **everything else is
gcc 2.7.2**, and hrwfs calls across that boundary constantly — into iocCore
for record processing, `seq` for the sequencer, astlib/slalib/timelib for
astrometry, DHS for data handling — with callbacks coming back the other way.

That is a cross-generation ABI interop question at every call. Mostly it should
hold: the PowerPC EABI calling convention is stable across those versions,
`-mstrict-align` is already forced, and `--no-builtin`/`-fno-builtin` remove a
class of divergence. The historical gcc 2.7-to-2.9x PowerPC gotchas are
struct-return conventions and bitfield allocation, which is where to look if
something misbehaves.

### Update 4: the ABI risk is low, measured

Checked rather than assumed, and it walks back most of the warning above.
The two known gcc 2.7-to-2.9x PowerPC divergences are bitfield allocation and
struct-return conventions. Neither is in play:

- **Zero bitfields** in the DHS headers or in hrwfs's cross-boundary headers
  (`dbTypes.h`, `detControl.h`, `epToVxLib.h`, `gemTypes.h`).
- **Zero functions returning a struct by value** in the DHS headers.
- **No `long long`** in the shared types.
- The DHS interface is entirely **opaque scalar handles** plus enums:
  `DHS_AV_ID`/`DHS_AV_LIST` are `long`, `DHS_BD_DATASET`/`_FRAME`/`_OBJECT`
  alias `DHS_AV_LIST`, `DHS_CONNECT`/`DHS_TAG` are `unsigned long`. **No struct
  crosses the boundary at all.**

`long`, `unsigned long` and `enum` pass in registers identically under the
PowerPC EABI in both compiler versions. So the mixed-generation runtime is far
less dangerous than the raw fact "our modules are 2.96, everything else is
2.7.2" suggests.

(Correcting an earlier count in this document's history: `dhsDataset` and
`dhsDataFrame`, which looked like a wide type boundary at 107 and 36
occurrences, are hrwfs's own *variable* names. The real type surface is the
seven typedefs above.)

**What this means for the two options.** Option 2 -- rebuild the EPICS runtime
and support libraries with 2.96 -- is still worth doing, but for the reasons
Hawi gave rather than as ABI mitigation: it is needed anyway if the mixed
approach fails, and ~25 IOCs on this tree need the same builds, so doing it
once serves all of them. Framing it as risk reduction overstates a risk that
measurement says is small.

Residual items to keep an eye on, both cheap: that no build passes
`-fshort-enums` (nothing does), and varargs across the boundary if any is
added later.

1. **The crate test should still be functional, not a boot test** -- not
   because of ABI fear, but because a boot test exercises almost none of the
   detector, DHS and astrometry paths that a release needs verified anyway. "It boots and the
   IOC initialises" exercises almost none of the boundary. Exercising the CAD/CAR
   records, a real detector exposure through DHS, and the WCS/astrometry paths
   does.
2. **The coherent variant, now agreed as worthwhile.** Rebuild the EPICS
   runtime *and* the support libraries with gcc 2.96 as well, so the whole
   loaded set is one generation, keeping the 5.4 kernel. The EPICS source is in
   the staged tree (2715 files under `base/src`), so `iocCore`/`seq`/`pvload`
   are rebuildable; the support-library sources are not staged and would have
   to be found. More work, but it removes the mixing rather than testing
   around it. The kernel itself stays 2.7.2-built, but the kernel/module
   boundary is a documented stable ABI, which library interop is not.

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

### The versions to pin (read off the live symlinks, 2026-09-03)

```
/gemini/epics3.13.4/astlib/astlib   -> V1-4
/gemini/epics3.13.4/slalib/slalib   -> V1-9-4
/gemini/epics3.13.4/timelib/timelib -> V1-8-6
/gemini/epics3.13.4/cfitsio/cfitsio -> V4-1
```

**astlib is V1-4, not gmoscc's V1-6** — different EPICS generations, so they
co-install without conflict, but the pin must not be copied from gmoscc.
slalib and timelib happen to match gmoscc's versions exactly.

**`/gemini/epics3.13.4/hrwfs/hrwfs -> V3-8-5` exists.** So hrwfs has the same
version-selecting symlink gmoscc called `setgmos` and removed. Two things
follow. First, the generated startup scripts `cd` to the *versioned* path
(`.../hrwfs/V3-8-5`), not through the symlink, so for the IOC boot the symlink
is currently vestigial -- but the crate's boot parameters may name it, which
is still unconfirmed. Second, it means the recommended deploy path
`/gemini/epics3.13.4/hrwfs/hrwfs` is *already* a working path: the RPM would
replace a symlink with a real directory of the same name, exactly the
transition gmoscc made.

### The DHS tree has no version, so the RPM has to become its version

`/gemini/dhs/dhs` is a **real directory**, not a selector symlink — owner
`ssa:gemini`, dated 10 Jun 2013 — sitting beside seven siblings (`dhs-0.19`,
`dhs-0.19b`, `dhs-0.19bDebug`, `dhs-0.19c`, `dhs-0.19c-hbf`, `dhs-2.0`,
`dhs-to`). Nothing in it declares a version: no RCS `$Id`, no version macro.
The only trace is three headers commenting "Initial install into CVS of
dhs-0.16", so the lineage is 0.16-era despite the 2013 date.

Same situation as gmoscc's `gem-vxworks-tornado22`, whose spec says it plainly:
binaries with no internal version marker, "which is the reason for packaging
them rather than copying: after this, `rpm -q` identifies which kernel a crate
loaded." Follow that — version by date, and record the hashes in the changelog
so the package becomes the version of record:

```
libers.o     7590  fab652769e6d6447...
libimp.o   244650  2c7220dd8b395834...
libsds.o    74357  ab39cb37b69f1de7...
libdhs.a   177014  ebe830f62270bd77...
libgen.a    22018  4548eccbfb306659...
```

### These deplibs are shared platform, not hrwfs's

The same listing shows ~25 IOCs on this tree -- `ag`, `agSeq`, `crcs`, `ecs`,
`gcal`, `gis`, `gmos`, `gmosdc`, `gnirs`, `gws`, `lis`, `ltcss`, `mchCCS`,
`mcs`, `niri`, `oiwfs`, `pcs`, `prm`, `pwfs`, `pwfs1`, `pwfs2`, `scs`, `tcs`
-- all resolving astlib/slalib/timelib through the *same* shared symlinks. So
`gem7-epics-runtime` and the support libraries are groundwork for every future
GEM7 port, not hrwfs-private work.

Follow gmoscc's reasoning on naming anyway: it made `gmos-deplibs` private
precisely because a shared symlink hid which version was running, and these
are `ld`'d at boot, so the version on the server *is* the version running. An
hrwfs-private path with exact pins keeps that property.

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

## 2d. Support-library packaging: one macro, no special cases

The four support libraries (astlib, slalib, timelib, cfitsio) **build from CVS
source** — verified: slalib V1-9-4 compiles clean and its debug-stripped object
is 557612 vs the deployed 544576, the same ~1.02x `-mlongcall` ratio hrwfs
showed. So unlike gmoscc's locked prebuilt `gmos-deplibs`, these get real repos
and go through the full pipeline (pipeline Workflow A, "EPICS support module").

**The constraint that shapes everything:** VxWorks has no shared libraries, so
the crate `ld <`s these at boot — the version on the file server *is* the
version running. Different IOCs may need different versions **resident at the
same time**, which RTEMS IOCs never require because they link theirs in.

RPM permits only one version per package *name*, so a plain
`Name: gem7-slalib` + `Version: 1.9.4` cannot co-exist with 1.9.7 no matter how
the paths are arranged. Two ways out, both tested:

| | same name + `Provides: installonlypkg(kernel)` | version in the name |
|---|---|---|
| co-installs | yes (that provide is in dnf's default `installonlypkgs`) | yes |
| auto-prunes | **yes — `installonly_limit=3` silently removed the oldest** | no |
| `dnf remove` | takes every version at once | exactly one |
| host config | needs `installonly_limit=0` to be safe; dnf4 on EL9 has no drop-in for `[main]` | none |

Both were measured in a container, not assumed. The first is rejected because
its failure mode is a running instrument losing the library it boots, with
nothing linking cause to effect.

**So: version in the name, derived from one macro.**

```spec
%global libver 1.9.4
%global libdir V%(echo %{libver} | tr . -)
Name:          gem7-slalib-%{libdir}
Version:       %{libver}
%global instdir /gemini/epics3.13.4/support/slalib/%{libdir}
Provides:      gem7-slalib = %{version}
```

Bumping `%global libver` moves the package name, the version, and the install
path together. The name mirrors the deployed directory exactly
(`gem7-slalib-V1-9-4` -> `.../slalib/V1-9-4`), so `dnf` output and the `.vws`
`ld <` line read the same. Otherwise it is an ordinary spec — no special
handling for anyone.

**This needs a two-line fix to `gemini-rtsw-ci/build_rpm.sh`**, in
`docs/gemini-rtsw-ci-macro-name.patch`. The script reads the name by grepping
the spec as *text* (lines 133-137), so a macro-derived Name arrives unexpanded
and becomes the GHCR image tag verbatim. `PACKAGE_VERSION` already has an
`rpmspec` fallback for precisely this reason — its comment says "the version
living in ONE place" — and the patch gives Name the same treatment. It only
fires when the value still contains a `%`, so `gmoscc`, `slalib` and every
other existing spec are untouched (verified against all three shapes).

**The residual catch, and it is the important one.** The pin does not control
what runs. `BuildRequires` gets headers, `Requires` gets the tree onto the
server, but the crate loads whatever literal path is in the `.vws` — **30
references across 6 startup files**. Bump the pin without the `.vws` and the
crate boots the old library silently, or fails if the directory is gone. Fix it
the way gmoscc fixed its version string: put `@SLALIB_VER@` in the startup
sources, substitute from the spec in `%build`, and fail the build if any
placeholder survives. Then one macro drives the pin *and* every load path, and
divergence is a build error rather than a boot failure.

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


---

# Status as of 2026-09-15

## Done

**History.** All five repos on GitHub with full history, converted with cvs2git
/ svnrdump and verified tag-by-tag against `cvs export`:

| repo | commits | tags | span |
|---|---|---|---|
| `hrwfs` | 191 + packaging | 31 | 1999-2025 |
| `gem7-slalib` | 50 | 15 | 1998-2016 |
| `gem7-timelib` | 56 | 13 | 1998-2016 |
| `gem7-astlib` | 40 | 6 | 1998-2003 |
| `gem7-cfitsio` | 7 | 6 | 1999-2001 |

**Toolchain.** ANL's Linux gcc 2.96 against Tornado 2.0.2 headers compiles all
19 hrwfs sources and links all 10 modules. Objects are equivalent to
production: identical `.text`/`.data` sizes, differing only because 2.96
applies `-mlongcall` consistently where 2.7.2 emitted some direct `bl` (28 in
detControl, 0 in ours) -- the same benign difference gmoscc characterised, and
the stricter direction. Built with `-gstabs` so the debug format matches every
deployed GEM7 object.

**Packaging.** Four support libraries and five dependency RPMs built,
published and resolving from rpm-repo. hrwfs builds and publishes.

**Boot server.** `nfsv2-bootserver` carries the hrwfs crate in exports and
rhosts plus the `/gemdata` export the detector needs, tagged
`gmos-prod-verified` at the last GMOS-tested state as a rollback point. Pushed,
not yet installed.

## Open: the generated `local` does not match its source

`bin/ppc604/local` ships with `pisces-control` / `/export/gemini`, no
`hostAdd` line, and `vxUsers` missing its `tornado2.0/` component. The source
`startup/local.vws` says `mkotcsbootv2-lv1` / `/gemini`, has the `hostAdd`,
and has the full vxUsers path.

Eliminated so far, each tested in isolation:

- **The source is right** in the built commit (`git show <sha>:startup/local.vws`).
- **macTest is innocent.** Run by hand with the build's exact flags it
  reproduces the source faithfully, including `tornado2.0/`, the `hostAdd`
  line and the correct `cd`.
- **The tarball staging is right.** Reproducing `build_rpm.sh`'s
  `find | xargs cp --parents` stages `local.vws` with the correct content.
- **It is not the `iocpath` fix.** The `cd` in that same generated file is now
  correct; only the other lines are wrong.
- **It reproduces.** Two independent builds -- CI and local -- produce the
  same wrong output.

Remaining hypothesis: a stale build tree inside the committed build-environment
image, so `rpmbuild` extracts or reuses an older `local.vws` than the one
staged. The next test is a plain `setup.sh && make` outside `rpmbuild`: if that
produces the correct `local`, the fault is in the rpmbuild path rather than the
UAE build.

Noted separately and worth chasing: that same staging reproduction copied **276
of 457 files**. Some of the difference is the `gemini-rtsw-ci` submodule, but
not obviously all of it, and a tarball that silently drops files would affect
every repo on this pipeline.

**Do not boot a crate off the current RPM.** The `cd` is now right but the NFS
mounts still name pisces-control, so it would mount the wrong server.

## Remaining after that

1. Resolve the `local` discrepancy and republish.
2. Install the boot-server config (pushed, not deployed).
3. **Crate test** -- and it needs to be functional, not just a boot. The
   gcc 2.96 modules call into gcc 2.7.2 `iocCore`, `seq`, astrometry and DHS at
   every boundary, and that is verified only by static analysis: no bitfields,
   no struct-by-value returns, no `long long`, and the DHS interface is opaque
   scalar handles. Exercise a real exposure through DHS, the CAD/CAR paths and
   the WCS/astrometry calls.
4. **CP.** Five `#if (MK)` blocks mean the objects genuinely differ, so CP
   needs its own package built `-S CP`. No CP production tree has been
   captured, so its `local.vws` and `resource.def` are unknown.
