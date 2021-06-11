# DuplicateFileFinder

This program is a multithreaded duplicate file finder that allows you to identify duplicate files that takes up unnecessary space on your pc.

Things to consider:
-------------------
Files are considered identical if they have matching hashes:
The SHA256 algorithm is used to compute the hash of a given file.
This application makes use of the SHA256 library called PicoSHA2 found at:

https://github.com/okdshin/PicoSHA2/blob/master/picosha2.h

The above library is included in this repository as Picosha2.hpp,
so there's no need to download it from the above GitHub link.

Big Picture
------------
A pipeline is setup such that one thread finds all the files in a directory and sub-directories and loads them into a queue to be hashed. The threads then go into the queue to perform the hashing and store the results into a map. Duplicates are identified by duplicate keys within the map. 

How to Run
----------
g++ --std=c++17 -Wall -Wextra DuplicateFileFinder.cpp -o finder
./finder "Specify the file path to begin your search"

Example
-------
I Created a folder on my desktop on my mac that holds a few duplicate files.
The name of the folder is DuplicateStuffs. To Search for duplicates files I ran
the following lines of code.

g++ --std=c++17 -Wall -Wextra DuplicateFileFinder.cpp -o finder
./finder /Users/chevanogordon/desktop/DuplicateStuffs

If you get an error you might need to link the POSIX thread library(pthread) to your program,
to do so run the following lines of code.

g++ --std=c++17 -Wall -Wextra DuplicateFileFinder.cpp -o finder -pthread
./finder "Specify the file path to begin your search"
