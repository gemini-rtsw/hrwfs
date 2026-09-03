#!/bin/bash
# Convert the hrwfs-rt SVN dump to a git repository, locally. Run AFTER
# tools/migration/svn-dump.sh has produced hrwfs-rt.dump.gz on a host that can
# reach source.gemini.edu and you have copied it here.
#
#   ./tools/migration/svn-to-git.sh load     # dump.gz -> a local SVN repo
#   ./tools/migration/svn-to-git.sh convert  # local SVN repo -> git, with history
#   ./tools/migration/svn-to-git.sh push     # -> git@github.com:gemini-rtsw/hrwfs.git
#
# Everything happens under $WORK; nothing touches the checkout.
set -u

DUMP="${DUMP:-$HOME/work/hrwfs-rt.dump.gz}"
AUTHORS="${AUTHORS:-$HOME/work/hrwfs-authors.txt}"
WORK="${WORK:-$HOME/work/hrwfs-migration}"
REPO=$WORK/svnrepo
GIT=$WORK/hrwfs
REMOTE=git@github.com:gemini-rtsw/hrwfs.git
PROJECT=hrwfs-rt

case "${1:-load}" in

load)
    [ -f "$DUMP" ] || { echo "ERROR: no dump at $DUMP (set DUMP=...)"; exit 1; }
    mkdir -p "$WORK"
    rm -rf "$REPO"
    svnadmin create "$REPO"

    # svnrdump 1.7 on the dumping host writes the FIRST revision of the range
    # with paths relative to the dumped URL (branches, tags, trunk) and every
    # later revision with paths absolute from the repo root
    # (hrwfs-rt/branches/...). svnadmin load then creates `branches` and dies
    # on the first absolute path with "File not found". This normalises the
    # first revision to match the rest, byte-for-byte elsewhere. It is a no-op
    # if the dump does not have the problem.
    #
    # Discovered on the dhs migration; see gemini-rtsw/dhs MIGRATION.md.
    echo "Normalising the dump (svnrdump relative/absolute first-revision bug)..."
    python3 "$(dirname "$0")/fix-dump-paths.py" "$PROJECT" "$DUMP" "$WORK/$PROJECT.dump"

    # svnrdump elides the dump root's own directory node: r68863 is `A /hrwfs-rt`
    # in the repository, but in the dump that revision has NO nodes at all, and
    # every later path is written absolute as `hrwfs-rt/...`. So the load dies on
    # the first child with "File not found: path '/hrwfs-rt'". Create the parent
    # first. (dhs hit the mirror image of this -- paths RELATIVE in the first
    # revision and absolute after; hence fix-dump-paths.py, which passes this
    # dump through untouched.)
    if ! grep -aq "^Node-path: $PROJECT\$" "$WORK/$PROJECT.dump"; then
        echo "Dump has no '$PROJECT' root node; creating it first."
        svn mkdir -q -m "Create $PROJECT (elided by svnrdump as the dump root)" \
            "file://$REPO/$PROJECT"
    fi

    echo "Loading (slow -- one revision at a time)..."
    svnadmin load --quiet "$REPO" < "$WORK/$PROJECT.dump"
    echo "Loaded. HEAD = $(svnlook youngest "$REPO")"
    echo "Top level: $(svn ls "file://$REPO/$PROJECT" 2>/dev/null | tr '\n' ' ')"
    ;;

convert)
    [ -d "$REPO" ] || { echo "ERROR: run '$0 load' first"; exit 1; }
    if [ ! -f "$AUTHORS" ]; then
        echo "No authors file at $AUTHORS -- generating one from the log."
        echo "Edit it to put real names/addresses in, then re-run convert."
        svn log -q "file://$REPO/$PROJECT" 2>/dev/null \
          | awk -F'|' '/^r/ {gsub(/ /,"",$2); print $2}' | sort -u \
          | while read -r a; do
                [ -n "$a" ] && echo "$a = $a <$a@gemini.edu>"
            done > "$AUTHORS"
        cat "$AUTHORS"
        exit 1
    fi

    rm -rf "$GIT"
    # --stdlayout: the project has trunk/branches/tags, and the tags ARE the
    # release history (V0-4 .. V3-8-5 in RELEASE.NOTES). Importing trunk only
    # would drop the thing that records what each site ever ran.
    # --no-metadata: no git-svn-id trailers; this is a one-way move.
    echo "Converting (slow)..."
    git svn clone "file://$REPO/$PROJECT" \
        --stdlayout \
        --authors-file="$AUTHORS" \
        --no-metadata \
        --prefix=svn/ \
        "$GIT" 2>&1 | tail -20

    cd "$GIT" || exit 1
    git branch -M main

    # git svn leaves tags as remote refs, not real tags. Convert them.
    for t in $(git for-each-ref --format='%(refname:short)' refs/remotes/svn/tags/); do
        name=${t#svn/tags/}
        git tag -- "$name" "$t" 2>/dev/null && echo "  tag $name"
    done
    # ...and branches as remote refs, not local branches.
    for b in $(git for-each-ref --format='%(refname:short)' refs/remotes/svn/ \
               | grep -v '^svn/tags/' | grep -v '^svn/trunk$'); do
        name=${b#svn/}
        git branch -- "$name" "$b" 2>/dev/null && echo "  branch $name"
    done

    echo
    echo "== result"
    printf "  commits: %s\n" "$(git rev-list --count --all)"
    printf "  tags:    %s\n" "$(git tag | wc -l)"
    printf "  authors: %s\n" "$(git log --format='%an' --all | sort -u | tr '\n' ' ')"
    git log --oneline -5
    echo
    echo "Sanity check before pushing -- the tip should match the V3-8-5"
    echo "working copy in the checkout this script came from:"
    echo "  diff -r --exclude=.svn --exclude=.git --exclude=tools $GIT <checkout>"
    ;;

push)
    cd "$GIT" || { echo "ERROR: run '$0 convert' first"; exit 1; }
    git remote add origin "$REMOTE" 2>/dev/null || git remote set-url origin "$REMOTE"
    # --mirror would also push git-svn's remote refs. Push what we made
    # explicitly instead: main, the real branches, and the real tags.
    git push origin main
    git push origin --tags
    for b in $(git for-each-ref --format='%(refname:short)' refs/heads/ | grep -v '^main$'); do
        git push origin "$b"
    done
    ;;

*)
    echo "usage: $0 {load|convert|push}"; exit 1 ;;
esac
