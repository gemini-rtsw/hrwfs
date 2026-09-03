#!/bin/bash
# Dump the hrwfs-rt SVN history to a file that can be converted to git locally.
#
# Needs NOTHING installed beyond subversion: svnrdump works over HTTP, so it
# needs no access to the SVN server's filesystem and no git-svn on this host.
# Run on a box that can reach source.gemini.edu -- hbfswgbld-lv1 or
# sbfswgdev4. NOT hstecher-ld1: every TCP port to that host is filtered
# ("No route to host"), which is why the dump has to be taken elsewhere and
# copied back.
#
#   scp tools/linux-build/svn-dump.sh <host>:/tmp/
#   ssh <host> 'bash /tmp/svn-dump.sh info'      # 1. quick -- tells us the cost
#   ssh <host> 'bash /tmp/svn-dump.sh dump'      # 2. the long one -- use screen
#   scp <host>:/tmp/hrwfs-dump/hrwfs-rt.dump.gz .
#
# Everything here is read-only against the repository.

set -u
ROOT=http://source.gemini.edu/software
PROJECT="${2:-hrwfs-rt}"
SVN=$ROOT/$PROJECT
OUT=/tmp/hrwfs-dump

case "${1:-info}" in

info)
    echo "== reachable? =="
    svn info "$SVN" 2>&1 | grep -E 'URL|Root|UUID|Revision|^svn:' || exit 1
    echo
    echo "== layout of $PROJECT (expect trunk/branches/tags) =="
    svn ls "$SVN" 2>&1
    echo
    echo "== tags (which one is production?) =="
    svn ls "$SVN/tags" 2>&1 | tail -20
    echo
    echo "== branches =="
    svn ls "$SVN/branches" 2>&1
    echo
    echo "== is trunk ahead of tags/V3-8-5? =="
    svn log -q --limit 5 "$SVN/trunk" 2>&1
    echo
    echo "== other hrwfs* projects in SVN =="
    svn ls "$ROOT" 2>&1 | grep -i -E 'hrwfs|acqcam|wfs'
    echo
    echo "== oldest revision touching $PROJECT (dump starts here, not r1) =="
    svn log -q -r 1:HEAD --limit 1 "$SVN" 2>/dev/null | grep '^r' \
        || echo "  (could not determine -- dump would start at r1)"
    echo
    echo "== number of revisions touching $PROJECT =="
    svn log -q "$SVN" 2>/dev/null | grep -c '^r' || echo "  (could not count)"
    echo
    echo "== authors (for the git author map) =="
    svn log -q "$SVN" 2>/dev/null \
        | awk -F'|' '/^r/ {gsub(/ /,"",$2); print $2}' | sort -u | tr '\n' ' '
    echo
    echo
    echo "== svnrdump present? =="
    svnrdump --version 2>&1 | head -1 || echo "  MISSING -- tell me and we find another way"
    ;;

dump)
    mkdir -p "$OUT"
    # Start at the first revision that touches hrwfs-rt rather than r1. The
    # repository is shared across all of /software and is at r71k+ globally;
    # svnrdump replays every revision in the range even though almost none of
    # them touch this project, so the start revision is the whole cost.
    FIRST=$(svn log -q -r 1:HEAD --limit 1 "$SVN" 2>/dev/null \
            | grep -o '^r[0-9]*' | tr -d 'r')
    FIRST=${FIRST:-1}
    echo "Dumping $SVN from r$FIRST to HEAD."
    echo "Expect hours, not minutes (dhs: ~71k revisions at ~1.7/sec)."
    echo "Run it under screen/tmux so an ssh drop does not kill it."
    echo

    # Write to .part and rename only on success, so the finished file
    # appearing IS the completion signal -- scp-ing a half-written dump
    # silently yields a truncated archive that loads most of the way and
    # then fails.
    svnrdump dump "$SVN" -r "${FIRST}:HEAD" 2>"$OUT/$PROJECT.err" \
        | gzip > "$OUT/$PROJECT.dump.gz.part"
    rc=${PIPESTATUS[0]}
    [ "$rc" -eq 0 ] && mv "$OUT/$PROJECT.dump.gz.part" "$OUT/$PROJECT.dump.gz"

    echo
    if [ "$rc" -ne 0 ]; then
        echo "svnrdump FAILED (exit $rc). Last lines of stderr:"
        tail -20 "$OUT/$PROJECT.err"
        exit 1
    fi
    echo "DONE -> $OUT/$PROJECT.dump.gz ($(du -h "$OUT/$PROJECT.dump.gz" | cut -f1))"
    echo "First revision dumped: r$FIRST"
    ;;

authors)
    svn log -q "$SVN" 2>/dev/null \
      | awk -F'|' '/^r/ {gsub(/ /,"",$2); print $2}' | sort -u \
      | while read -r a; do
            [ -n "$a" ] && echo "$a = $a <$a@gemini.edu>"
        done | tee /tmp/hrwfs-authors.txt
    echo
    echo "Wrote /tmp/hrwfs-authors.txt -- scp it back with the dump."
    ;;

*)
    echo "usage: $0 {info|dump|authors} [project]   (default project: hrwfs-rt)"; exit 1 ;;
esac
