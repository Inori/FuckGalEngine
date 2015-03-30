#!/usr/bin/perl
# merge_dpng.pl, v1.0 2011/03/04
# coded by asmodean

# contact: 
#   web:   http://asmodean.reverse.net
#   email: asmodean [at] hush.com
#   irc:   asmodean on efnet (irc.efnet.net)

# This script merges DPNG fragments.  Requires ImageMagick convert on the path.

use File::Find;
use Cwd;

$top = getcwd();

sub scan_dpng {
  push(@{$groups{$1}}, $_) if ($File::Find::name =~ m@(.*)\+DPNG@);
}

find(\&scan_dpng, ".");

for $prefix (sort keys(%groups)) {
  @files = @{$groups{$prefix}};
  
  ($dir = $prefix)          =~ s@/[^/]*$@@;
  ($out_filename = $prefix) =~ s@.*/@@;
  
  # stupid imagemagick likes to barf on japanese filenames, so try to work around
  # it for directory names.
  chdir($dir);  
  
  @cmd = ("convert", "-background", "transparent");
  
  for $f (@files) {
    $f =~ m@\+x(\d+)y(\d+)@;
    push(@cmd, "-page", "+$1+$2", $f);
  }
  
  push(@cmd, "-mosaic", "${out_filename}+merged.png");
  
  system(@cmd);
  
  chdir($top);
}
