recoverable virtual memory (librvm.a)

Vikash Ramkumar
Rahol Rajan


HOW TO COMPILE:
1) Extract the contents of the .tar file into a directory. 
2) A makefile has been provided in the archive. Run make to make the library as 
  follows:
	
	$make
	
3) This creates the file librvm.a .
4) Go over to test_cases directory and run make to compile tests in this 
   directory
5) If you are compiling your own source with the library, run the following 
   command:

g++ -std=c++11 -Wall -g -I. myexample.c /path/to/librvm.a -o myexample
./myexample

NOTE: please specify path to rvm.h in the includes in myexample.c and
also replace /path/to with the directory in which rvm.a is located in

How you use logfiles to accomplish persistency plus transaction semantics.

There exists a logfile for each mapped segment on disk. Persistency is attained 
by committing all changes to the segment's main memory after a transaction to 
the logfile. Upon remapping of an existing segment, the logfile is truncated 
into the segment to ensure accuracy of transactions. Since the logfiles exist
on the disk and all transactions are committed to the logfile once the 
transaction executes, if there occurs a power failure or abort, the logfile
remains persistent for recovery.

What goes in them? How do the files get cleaned up, so that they do not expand
indefinitely?

- Each segment has a log file. The log files store the log records. Each record
  stores a segment offset, size and then the data. The records are distinguished
  between each other using delimiters.

- A Log file get cleaned up automatically every time its respective segment is
  mapped. Thus, when rvm_map is invoked, the respective log is truncated and the
  segment file gets updated. Only after this procedure, rvm allocates virtual 
  memory and loads the updated segment data into it from the backing store.




