#!/usr/bin/env python3
"""Normalise an svnrdump subtree dump so `svnadmin load` accepts it.

svnrdump 1.7 emits the FIRST revision of the requested range with node paths
relative to the dumped URL (branches, tags, trunk) and every later revision
with paths absolute from the repository root (hrwfs-rt/branches/...).
`svnadmin load` therefore creates `branches` at top level and then dies on the
first absolute path with "File not found: path '/hrwfs-rt/branches/...'".

Make the first revision agree with the thousands of later nodes rather than the
other way round: prefix its three nodes with the project name, and add a node
creating the project directory itself (nothing else does -- it predates the
dumped range).

Byte-level and surgical: three Node-path lines are rewritten and one node block
is cloned as a template. Every other byte -- including all binary file content
and every content-length -- is untouched, which is why this is safe to run
blind. A dump that does not have the problem is passed through unchanged.

Discovered on the dhs/dhsClient migration (gemini-rtsw/dhs MIGRATION.md).

    fix-dump-paths.py <project> <in.dump.gz|in.dump> <out.dump>
"""
import gzip
import re
import sys

REL = [b'branches', b'tags', b'trunk']


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    proj, src, dst = sys.argv[1], sys.argv[2], sys.argv[3]
    pb = proj.encode()

    opener = gzip.open if src.endswith('.gz') else open
    with opener(src, 'rb') as fh:
        data = fh.read()
    orig_len = len(data)

    counts = {n: data.count(b'\nNode-path: ' + n + b'\n') for n in REL}
    if not any(counts.values()):
        # Already absolute throughout -- nothing to do.
        with open(dst, 'wb') as fh:
            fh.write(data)
        print(f"{proj}: no relative node paths found; copied unchanged "
              f"({orig_len} bytes)")
        return

    for name, n in counts.items():
        if n != 1:
            sys.exit(f"ERROR: expected exactly 1 relative "
                     f"'Node-path: {name.decode()}', found {n}. "
                     f"Refusing to guess -- inspect the dump.")

    # Clone the first node block (Node-path line through the next one) as the
    # template for the project directory itself.
    first = data.index(b'\nNode-path: ') + 1
    second = data.index(b'\nNode-path: ', first) + 1
    template = data[first:second]
    newblock = re.sub(rb'^Node-path: [^\n]*\n', b'Node-path: ' + pb + b'\n',
                      template, count=1)
    if newblock == template or not newblock.startswith(b'Node-path: ' + pb + b'\n'):
        sys.exit("ERROR: could not build the project-directory node block")

    data = data[:first] + newblock + data[first:]

    for name in REL:
        data = data.replace(b'\nNode-path: ' + name + b'\n',
                            b'\nNode-path: ' + pb + b'/' + name + b'\n', 1)

    for name in REL:
        if data.count(b'\nNode-path: ' + name + b'\n'):
            sys.exit(f"ERROR: {name.decode()} is still relative")

    with open(dst, 'wb') as fh:
        fh.write(data)
    print(f"{proj}: {orig_len} -> {len(data)} bytes "
          f"(+{len(data) - orig_len}), added '{proj}' directory node")


if __name__ == '__main__':
    main()
