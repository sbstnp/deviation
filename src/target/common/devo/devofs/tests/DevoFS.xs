#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "../devofs.h"
extern char image_file[1024];

MODULE = DevoFS		PACKAGE = DevoFS		

FATFS *
mount(path)
	char *path
    CODE:
        strcpy(image_file, path);
        FATFS *fs = malloc(sizeof(FATFS));
        int res = df_mount(fs);
        if (res != FR_OK) {
            free(fs);
            fs = NULL;
        }
        RETVAL = fs;
    OUTPUT:
        RETVAL

int
switchfile(fs)
        FATFS *fs
    CODE:
        int res;
        res = df_switchfile(fs);
        RETVAL = res;
    OUTPUT:
        RETVAL

int
open(path, flags)
        char *path
        unsigned flags
    CODE:
        int res;
        res = df_open(path, flags);
        RETVAL = res;
    OUTPUT:
        RETVAL

long
read( buf , size , leng)
    SV* buf
    unsigned size
    unsigned leng
  CODE:
    u16 act;
    int res;
    // Tell that the scalar is string only (not integer, double, utf8...)
    SvPOK_only(buf) ;

    // Grow the scalar to receive the data and return a char* point:
    char* buffer = SvGROW( buf , size + 1 ) ;
    memset(buffer, 0, size);

    res = df_read(buffer, size, &act);
    leng = act;
    // Get the amount of data read in the socket:
    
    if ( leng > 0 ) {
      // null-terminate the buffer, not necessary, but does not hurt
      buffer[leng] = 0 ;
      // tell Perl how long the string is
      SvCUR_set( buf , leng ) ;
    }
    // Return the amount of data read, like Perl read().
    RETVAL = res ;
  OUTPUT:
    RETVAL
    leng
    buf


int
read1(buffer, len, actual)
        SV *buffer;
        unsigned len;
        unsigned actual;
    CODE:
        u16 act;
        char *tmp_buffer;
        tmp_buffer = (char *)malloc(len);
        RETVAL = df_read(tmp_buffer, len, &act);
        actual = act;
        //buffer = sv_2mortal(newSVpvn(tmp_buffer, actual));
        buffer = newSVpvn(tmp_buffer, actual);
        free(tmp_buffer);
    OUTPUT:
        RETVAL
        buffer
        actual


int
write(buf, len, actual)
        char *buf;
        unsigned len;
        unsigned actual;
    CODE:
        u16 act;
        RETVAL = df_write(buf, len, &act);
        actual = act;
    OUTPUT:
        RETVAL
        actual

int
lseek(addr)
        unsigned addr
    CODE:
        RETVAL = df_lseek(addr);
    OUTPUT:
        RETVAL

FATFS *
opendir(path)
        char *path;
    CODE:
        int res;
        FATFS *fs = malloc(sizeof(FATFS));
        res = df_opendir(fs, path);
        if (res != FR_OK) {
            free(fs);
            fs = NULL;
        }
        RETVAL = fs;
    OUTPUT:
        RETVAL

void
readdir(fs)
        FATFS *fs;
    INIT:
        int i;
        FILINFO fi;
    PPCODE:
        i = df_readdir(fs, &fi);
        if (i == FR_OK) {
            XPUSHs(sv_2mortal(newSVpv(fi.fname, 0)));
            XPUSHs(sv_2mortal(newSVnv(fi.fsize)));
            XPUSHs(sv_2mortal(newSVnv(fi.fattrib)));

        } else {
            XPUSHs(sv_2mortal(newSVnv(i)));
        }

int
close()
    CODE:
        RETVAL = df_close();
    OUTPUT:
        RETVAL

int
compact()
    CODE:
        RETVAL = df_compact();
    OUTPUT:
        RETVAL


MODULE = DevoFS PACKAGE = FATFSPtr PREFIX = fatfs_

void
fatfs_DESTROY(fs)
        FATFS *fs;
    CODE:
        if(fs)
            free(fs);
