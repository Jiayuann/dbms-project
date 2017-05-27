
- Modify the "CODEROOT" variable in makefile.inc to point to the root
  of your code base if you can't compile the code.
 
- Implement the Record-based Files (RBF) Component:

   Go to folder "rbf" and type in:

    make clean
    make
    ./rbftest1


   The program should run. But it will generates an error. You are supposed to
   implement the API of the paged file manager defined in pfm.h and some
   of the methods in rbfm.h as explained in the project description.

- By default you should not change those functions of the PagedFileManager,
  FileHandle, and RecordBasedFileManager classes defined in rbf/pfm.h and rbf/rbfm.h.
  If you think some changes are really necessary, please contact us first.

In this project, you will implement a Paged File (PF) system and few
operations of a Record-Based File (RBF) manager.
- The PF component provides facilities for higher-level client
components to perform file I/O in terms of pages.
- In the PF component, methods are provided to create, destroy, open,
and close paged files, to read and write a specific page of a given file,
and to add pages to a given file.
- The record manager is going to be built on top of the basic paged file
system

 
Finally, I'll remind folks about a comment that I made in class yesterday (Thursday, April 12).  
Slide 11 of Lecture 3 presents two different formats for variable-length records.  The first format 
is also described in Section 9.7.2 of our textbook.  
You may use either format for Project 1.  However, the solution that we will provide for Project
 1 uses the first format(which does not in-line fixed-length fields), so perhaps you want to use
  that first format in your own solution.

 Lecture 3 from the class has slides which may be useful for the assignment.  Slide 11 (approach
  on top without in-lining) is particularly important,and Slides 16-17 may be helpful now 
  (for record insert), but will be critically important for Project 2.


1. Read up on the slides in Lecture 3 & 4 to get the right background for the project.
2. Ensure that you are familiar with the file handling api e.g. fseek, fopen, fread, fwrite etc.. 
   The links provided in the general guidance have links to tutorials.
3. Start with the pfm.cc file first, this should give you a strong foundation for the project.
4. A common question in section has been how to keep track of a file with a FileHandle. You 
   should add a private variable to the FileHandle class to keep track of the file you open in openFile(). 
   This can be a (FILE*), a file descriptor, a file stream, or whatever handle you'd like to use for reading 
   from and writing to a file.
5. Void *data : You can assume that these data pointers point to valid chunks of memory. In general,
   if you're performing a write operation, the data pointer will be pointing to data that you have to write. 
   For read operations, you'll be reading in data from something else and writing data to this data pointer.
6. memcpy() is a good function to look at for copying data from one memory address to another.
7. Look into malloc and calloc, these may be useful.
8. You may also find it helpful to cast data as a char*. This will allow you to perform some 
    nifty pointer arithmetic.
9. For rbfm, start with the various create/destroy/open/close methods, then move onto 
   printRecord(). printRecord() is a good place to start because it's much easier than insertRecord()
   or readRecord(), and it will get you familiar with the way that the data is formatted.
10 For segmentation faults, you can just run your program with gdb, then use "where" or "where full" 
   to see where it crashed.



