#!/usr/bin/env bash
# Print the GitHub release notes for a version as Markdown bullets, derived
# from that version's AppStream <release> description. The metainfo entry is
# the only source; a body written beside it is what shipped v1.1 with no
# feature list.
#
#   ./tool/release-notes.sh           # VERSION
#   ./tool/release-notes.sh 1.2
#
# Empty output is a failure, not a blank body: no <release> for the version,
# or a <release> with no <li> features, exits non-zero.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FILE="$ROOT/packaging/linux/com.proximamagnifica.aoide.metainfo.xml"

if [[ $# -gt 1 ]]; then
  echo "release-notes: usage: release-notes.sh [version]" >&2
  exit 2
fi

if [[ $# -eq 1 ]]; then
  version="$1"
  if [[ ! "$version" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
    echo "release-notes: '$version' is not a dotted release number" >&2
    exit 1
  fi
else
  # shellcheck disable=SC1090
  eval "$(bash "$ROOT/tool/version.sh")"
fi

if [[ ! -f "$FILE" ]]; then
  echo "release-notes: $FILE is missing" >&2
  exit 1
fi

# Strip comments first. The file keeps a <screenshots> block commented out,
# and a text scan would treat a commented <release> as live.
notes="$(perl - "$FILE" "$version" <<'PERL'
use strict;
use warnings;

my ($file, $version) = @ARGV;
open my $fh, '<:encoding(UTF-8)', $file or do {
  print STDERR "release-notes: cannot read $file: $!\n";
  exit 1;
};
local $/;
my $xml = <$fh>;
close $fh;

$xml =~ s/<!--.*?-->//gs;

if ($xml !~ m{<release\b[^>]*\bversion="\Q$version\E"[^>]*>(.*?)</release>}s) {
  print STDERR "release-notes: no <release> for '$version'\n";
  exit 1;
}
my $block = $1;

sub decode_entities {
  my ($s) = @_;
  $s =~ s/&nbsp;/ /g;
  $s =~ s/&mdash;/chr(0x2014)/ge;
  $s =~ s/&ndash;/chr(0x2013)/ge;
  $s =~ s/&#(\d+);/chr($1)/ge;
  $s =~ s/&#x([0-9A-Fa-f]+);/chr(hex($1))/ge;
  $s =~ s/&lt;/</g;
  $s =~ s/&gt;/>/g;
  $s =~ s/&quot;/"/g;
  $s =~ s/&apos;/'/g;
  $s =~ s/&amp;/&/g;
  return $s;
}

my @items;
while ($block =~ m{<li\b[^>]*>(.*?)</li>}sg) {
  my $text = $1;
  $text =~ s/<[^>]+>//g;
  $text = decode_entities($text);
  $text =~ s/\s+/ /g;
  $text =~ s/^\s+|\s+$//g;
  push @items, $text if length $text;
}

if (!@items) {
  print STDERR "release-notes: <release version=\"$version\"> has no features\n";
  exit 1;
}

binmode STDOUT, ':utf8';
print "- $_\n" for @items;
PERL
)"

if [[ -z "$notes" ]]; then
  echo "release-notes: notes for '$version' came out empty" >&2
  exit 1
fi

printf '%s\n' "$notes"
