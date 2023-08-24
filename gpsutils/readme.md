# GPS File utilities

These utilities are designed to work with files from AVIRIS-III, which are in the Atlans A7 Binary V5 format. 

# checker.c:

## To compile:

`gcc -O3 -march=native -std=c99 checker.c -o checker`

or if you have c11:

`gcc -O3 -march=native -std=c11 checker.c -o checker`

Or for best performance, if you have clang:

`clang -O3 -march=native checker.c -o checker`

## To run:

### Sinble file: 
You can check a single gps file like this:

`checker AV320230713t222106_gps`

### Compare and check:
You can also check a secondary file, such as the one above, against a primary file. The primary file should "match" the secondary file bit for bit during the recording process, and checker will tell you if it does:

`checker AV320230713t220957-gpsPrimary.bin AV320230713t222106_gps`

### Compare all and check: 
Lastly, you can run this program on an entire folder of secondary files, comparing against the primary file. This is the "ultimate" check:

`checker AV320230713t220957-gpsPrimary.bin AV3*_gps`


