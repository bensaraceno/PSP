use strict;


&main();

#------------------------------------------------------------------------------
# Name:     main
# Summary:  Use this program to convert RGB values to the 16 bit BGR values
#           used to display images on a PSP
# Inputs:   none
# Outputs:  none
# Globals:  none
# Returns:  none
# Cautions: none
#------------------------------------------------------------------------------
sub main
{
   #for my $cRef ( [255,0,0, "RED"], [0,255,0, "GREEN"], [0,0,255, "BLUE"], [255,255,0,"YELLOW"],
   #               [128, 64, 0, "BROWN"] )
   #{
   #  RGBTo16BitHex(@$cRef);
   #}
   
   # takes 24 bit image in RGB color format / order
   ConvertImage($ARGV[0]) if (@ARGV);
}


#------------------------------------------------------------------------------
# Name:     RGBTo16BitHex
# Summary:  Converts an RGB value to a 16 bit PSP color value. 
# Inputs:   1. Red color value (0 - 255)
#           2. Green color value (0 - 255)
#           3. Blue color value (0 - 255)
# Outputs:  none
# Globals:  none
# Returns:  A 16 bit hex string of the form 0xFFFF
# Cautions: none
#------------------------------------------------------------------------------
sub RGBTo16BitHex
{
   my @rgb = @_;
   $rgb[0] = int($rgb[0]/8);
   $rgb[1] = int($rgb[1]/8);
   $rgb[2] = int($rgb[2]/8);
   
   my $str = pack("CCC", @rgb);
   my ($r, $g, $b) = unpack("B8B8B8", $str);
   print "r=$r\tg=$g\tb=$b\n";
   my $color16 = "0" . substr($b, -5) . substr($g, -5) . substr($r, -5);
   print "bitStr=$color16\n";
   my $binStr = pack("B*", $color16);
   $str = "0x" . (unpack("H4", $binStr)) . ",";
   print "hex=$str\n";
   return($str);
}

#------------------------------------------------------------------------------
# Name:     ConvertImage
# Summary:  Converts an image in RAW format to a comma separated list of hex
#           values that can be stored into an unsigned short array.  Use the 
#           array to display images on a PSP.  RAW format should contain 24 
#           bit color values in RGB order
# Inputs:   Name of RAW image file
# Outputs:  Creates a file with the same name as the input file, but with a 
#           .out file extension
# Globals:  none
# Returns:  none
# Cautions: none
#------------------------------------------------------------------------------
sub ConvertImage
{
  my $file = shift @_;
  (my $newName = $file) =~ s/(.*)\.(.*)/$1\.out/;
  print "FILE: $file\n";
  print "NEW NAME: $newName\n";
  
  open(IN, $file) or die ("could not open file: $!\n"); 
  open(OUT, ">$newName") or die ("Cannot open $newName for output: $!\n");
  binmode(IN);
  binmode(OUT);
  
  
  my ($r, $g, $b);
  my $count = 1;
  my ($binStr, $str);
  my $color16;
  while ( read(IN, $str, 3) )
  {
    ($r, $g, $b) = split("", $str);
    ($r, $g, $b) = (ord($r), ord($g), ord($b));
    # special transparent color
    if ($r == 255 && $g == 255 & $b == 255)
    {
       print OUT "0xFFFF,";
    }  
    else
    {
      $r = int($r/8);  # convert values into a number from 0 - 32
      $g = int($g/8);  # this allows 5 bits per color
      $b = int($b/8);
      $binStr = pack("CCC", $b, $g, $r); # pack as binary string

      ($b, $g, $r) = unpack("B8B8B8", $binStr); # unpack as three 8 bit string
      # create a 16 bit string
      # first bit is set to 0 since PSP ignores it in 16 bit color values
      $color16 = "0" . substr($b, -5) . substr($g, -5) . substr($r, -5);
      $binStr = pack("B*", $color16); # pack bits into a string
      $str = "0x" . (unpack("H4", $binStr)) . ","; # unpack string as hex
      print OUT $str;
    }
    
    if ($count % 50 == 0)
    {
      print OUT "\n";  
    }
    $count++;
  }
  close(OUT);
  close(IN);  
  
}

