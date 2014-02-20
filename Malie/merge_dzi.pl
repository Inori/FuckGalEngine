#!/usr/bin/perl
# merge_dzi.pl, v1.01 2009/03/08
# coded by asmodean

# contact: 
#   web:   http://asmodean.reverse.net
#   email: asmodean [at] hush.com
#   irc:   asmodean on efnet (irc.efnet.net)

# This script merges image fragments as specified by a DZI.

die "merge_dzi.pl v1.01 by asmodean\n\nusage: $0 <input.dzi>\n"
  unless ($#ARGV == 0);

$filename = shift(@ARGV);

print "$filename\n";

$filename =~ m/(.*)\./;
$prefix = $1;

$tex_path = $1 if ($filename =~ m@(.*/)@);
$tex_path .= "tex";

open(INPUT, "$filename") or die "${filename}: $!\n";

$line   = <INPUT>;

$line   = <INPUT>;
$line   =~ m/(\d+),(\d+)/;
$width  =  $1;
$height =  $2;

$count  = <INPUT>;

for ($i = 0; $i < $count; $i++) {
  $line   = <INPUT>;
  $line   =~ m/(\d+),(\d+)/;
  $blockx =  $1;
  $blocky =  $2;

  @cmd = ("convert", "-background", "transparent");

  $py = 0;

  for ($j = 0; $j < $blocky; $j++) {
    $px = 0;

    @files = split(/,/, <INPUT>);

    foreach $f (@files) {
      # Kill CRLF or just LF
      $f =~ s/\s*$//;

      if ($f) {
        push(@cmd, "-page", "+${px}+${py}", "$tex_path\\${f}.png");
      }
      $px += 256;
    }

    $py += 256;
  }

  push(@cmd, 
       "-mosaic",
       "-crop", "${width}x${height}+0+0");

  # Don't know the exact dimensions of the other sizes...
  push(@cmd, "-trim") if ($j > 0);

  push(@cmd, sprintf("${prefix}+%03d.png", $i));

  system(@cmd);
}
