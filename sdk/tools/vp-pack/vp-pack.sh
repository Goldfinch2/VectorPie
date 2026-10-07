#!/bin/sh
# vp-pack — make a VectorPie game package (.vpk) from a folder.
#
#   vp-pack <folder> [<output dir>]
#
# The folder holds the game as it will be installed: game.ini, the
# executable it names, and whatever else the game reads (data/, sounds/,
# marquee.png ...). Writes <id>-<version>.vpk (a .tar.gz of one <id>/
# folder) in the output dir (default: the current one). A save/ folder is
# left out: the cabinet keeps its own.
#
# Checks: game.ini has id (a-z 0-9 _), name, manufacturer, executable and
# version; no value holds ':' or '|'; the executable exists and is a 64-bit
# ARM Linux program (the cabinet's); the folder has no links.
set -eu

die() { echo "vp-pack: $*" >&2; exit 1; }

[ $# -ge 1 ] && [ $# -le 2 ] || die "usage: vp-pack <folder> [<output dir>]"
src=$(cd "$1" && pwd) || die "no folder $1"
out=${2:-.}
mkdir -p "$out"
out=$(cd "$out" && pwd)
ini="$src/game.ini"
[ -f "$ini" ] || die "$src has no game.ini"

# key = value ; comment  ->  the value
field() {
    sed -n "s/^[[:space:]]*$1[[:space:]]*=[[:space:]]*//p" "$ini" | head -n 1 |
        sed 's/[[:space:]][;#].*$//; s/[[:space:]]*$//; s/\r$//'
}

id=$(field id)
name=$(field name)
manufacturer=$(field manufacturer)
executable=$(field executable)
version=$(field version)

echo "$id" | grep -Eq '^[a-z0-9_]{1,32}$' || die "game.ini: id must be a-z 0-9 _ (got '$id')"
[ -n "$name" ]         || die "game.ini: no name"
[ -n "$manufacturer" ] || die "game.ini: no manufacturer"
[ -n "$executable" ]   || die "game.ini: no executable"
[ -n "$version" ]      || die "game.ini: no version"
if grep -Ev '^[[:space:]]*([;#]|$)' "$ini" | grep -q '[:|]'; then
    die "game.ini: ':' or '|' in a value (the menu's separators)"
fi
case "$executable" in /*|*..*) die "game.ini: executable must be inside the folder" ;; esac
[ -f "$src/$executable" ] || die "no executable $executable in $src"
if command -v file >/dev/null 2>&1; then
    file -b "$src/$executable" | grep -q 'ELF 64-bit.*aarch64' ||
        echo "vp-pack: warning: $executable is not a 64-bit ARM program (the Pi build?): $(file -b "$src/$executable")" >&2
fi
[ -z "$(find "$src" -type l | head -n 1)" ] || die "the folder holds links (not allowed in a package)"

# The archive's one folder is named after the id, whatever the source is called.
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/$id"
(cd "$src" && tar -cf - --exclude=./save --exclude=./launch.log .) | (cd "$tmp/$id" && tar -xf -)
chmod +x "$tmp/$id/$executable"

vpk="$out/$id-$version.vpk"
tar -czf "$vpk" --owner=0 --group=0 --sort=name -C "$tmp" "$id"
echo "vp-pack: $vpk ($name $version, $(du -h "$vpk" | cut -f1))"
