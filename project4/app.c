#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "vsfs.h"

int main(int argc, char **argv)
{
    int ret;
    int fd1, fd2, fd; 
    int i;
    char c; 
    char buffer[2048];
    char buffer2[8] = {50, 51, 52, 53, 54, 55, 56, 57};
    int size;
    char vdiskname[200]; 

    printf ("started\n");

    if (argc != 2) {
        printf ("usage: app  <vdiskname>\n");
        exit(0);
    }
    strcpy (vdiskname, argv[1]); 
    
    ret = vsmount (vdiskname);
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1);
    }

 


    vscreate("file1.bin");
    vscreate("file2.bin");
    vscreate("file3.bin");
    fd1 = vsopen ("file1.bin", MODE_APPEND);
    fd2 = vsopen ("file2.bin", MODE_APPEND);
    fd = vsopen("file3.bin", MODE_APPEND);
    size = vssize (fd);
    printf("File 3 size is %d\n",size);

/*     for (i = 0; i < 10000; ++i) {
        buffer[0] =   (char) 65;
        vsappend (fd1, (void *) buffer, 1);
    }

    for (i = 0; i < 1000; ++i) {
        buffer[0] = (char) 65;
        buffer[1] = (char) 66;
        buffer[2] = (char) 67;
        buffer[3] = (char) 68;
        vsappend(fd2, (void *) buffer, 4);
    }

    printf("File 1 size is %d\n",vssize(fd1)); 
    printf("File 2 size is %d\n",vssize(fd2)); */
    
    vsclose(fd1);
    vsclose(fd2);

    for (i = 0; i < 1000; ++i) {
        memcpy (buffer, buffer2, 8); // just to show memcpy
        vsappend(fd, (void *) buffer, 8);
    }
    

    vsclose (fd);

    fd = vsopen("file3.bin", MODE_READ);
    size = vssize (fd);
    printf("File 3 size is %d\n",size);
    for (i = 0; i < size; ++i) {
        vsread (fd, (void *) buffer, 1);
        c = (char*) buffer[0];
        printf("Value %s read from file file3.bin\n",&c);
    }
    vsclose (fd);
    vsdelete("file3.bin");
    
    ret = vsumount();
}
