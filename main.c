/*
 * File:   main.c
 * Author: Victor Manuel
 *
 * Created on December 25, 2010, 6:43 PM
 */

#define FUSE_USE_VERSION 25
#define _FILE_OFFSET_BITS 64 //flag que usa FUSE para redireccionaminetos y demas

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>

#include <ucontext.h>

#include "estructuras.h"
#include "funcionesAuxiliares.h"

/*Otras funciones*/

/*
    En linux los permisos sobre un fichero(ya sea archivo o directorio) se dividen
    en 3 tipos:
        -del usuario propietario
        -del grupo
        -otros

    Vamos a verificar que tipos de permisos(de lectura) se poseen sobre el fichero
    y vamos a retornar 0 si se puede leer y EACCES si no se puede leer.
*/
int check_permissions(struct ext2_inode *inode)
{

    if(IGNORE_PERMISSIONS)//se pueden ignorar los permisos y que se lea todo
        return 0;

    uid_t uid = getuid();   //obtenemos el usuario actual
    gid_t gid = getgid();   //obtenemos el grupo

    int permissions = inode->i_mode & 0777;//obtenemos los 4 digitos octales

    //si el usuario es root entonces tiene permisos
    if (uid == 0)
        return 0;

    //usuario propietario
    if(inode->i_uid == uid)
    {
        if((permissions & USER) == USER )
            return 0;
        return EACCES;
    }

    //perteneciente al grupo
    if(inode->i_uid == gid)
    {
        if((permissions & GROUP) == GROUP )
            return 0;
        return EACCES;
    }

    //otros
    if( (permissions & OTHER) == OTHER )
        return 0;

    return EACCES;
}


/*Funciones de FUSE*/

/*Obtiene los atributos de los archivos o directorios*/
static int ext2_getattr(const char *path, struct stat *stbuf)
{

    int status;

    memset(stbuf, 0, sizeof(struct stat));//se limpia lo que habia en el buffer

    struct ext2_inode inode;
    memset(&inode,0,EXT2_INODE_SIZE);

    status = get_inode_from_path(&inode,path);
    if(status == EXT2_FILE_NOT_FOUND || status == EXT2_ERROR)
		 return -ENOENT;

    stbuf->st_uid=inode.i_uid;              /*Owner ID*/
    stbuf->st_gid=inode.i_gid;              /*Group ID*/
    stbuf->st_mode=inode.i_mode;            /*File mode*/
    stbuf->st_nlink=inode.i_links_count;    /*Links count*/
    stbuf->st_size=inode.i_size;            /*Size in bytes*/
    stbuf->st_blocks=inode.i_blocks;        /* Blocks count */  /*last change*/
    stbuf->st_ctim.tv_sec=inode.i_ctime;    /*Creation time*/
    stbuf->st_atim.tv_sec=inode.i_atime;    /*Access time*/
    stbuf->st_mtim.tv_sec=inode.i_mtime;    /*Modification time*/

    return 0;
}

/*Obtiene el contenido de un directorio*/
static int ext2_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;  //hay que hacer esto para que el compilador no de unos warnings
    (void) fi;      //hay que hacer esto para que el compilador no de unos warnings

    int status;

    /*change*/
    struct ext2_inode inode;
    memset(&inode,0,EXT2_INODE_SIZE);

    status = get_inode_from_path(&inode,path);
    if(status == EXT2_FILE_NOT_FOUND || status == EXT2_ERROR)
        return -ENOENT;

    if(!S_ISDIR(inode.i_mode))
      return -ENOTDIR;

    //chequeamos los permisos sobre el directorio
    status = check_permissions(&inode);/*last change*/
    if(status == EACCES)
        return -EACCES;

    int i;
    int dir_entry_index;

    int block_count = (inode.i_size / EXT2_BLOCK_SIZE );
    block_count +=  (inode.i_size % EXT2_BLOCK_SIZE) == 0 ? 0 : 1;


    /*change lo vamos a reserva asi*/
    char buffer[EXT2_BLOCK_SIZE];

    //creamos el dir_entry current
    struct ext2_dir_entry current_dir_entry;

    for( i=0 ; i < block_count; i++)
    {
        status = read_inode_block_from_index(buffer,&inode,i);
        if(status == EXT2_ERROR)//verificamos si hubo error
            return EXT2_ERROR;


        //leemos todos los dir_entry en el bloque y buscamos el que tenga a name
        dir_entry_index=0;
        while(dir_entry_index < EXT2_BLOCK_SIZE)
        {
            if(i*EXT2_BLOCK_SIZE+dir_entry_index > inode.i_size)//nos pasamos del archivo
                return EXT2_FILE_NOT_FOUND;

            status = get_dir_entry_from_offset(&current_dir_entry,buffer,dir_entry_index);


            char *nameBuf = (char *)malloc(current_dir_entry.name_len + 1);
            strncpy(nameBuf,current_dir_entry.name,current_dir_entry.name_len + 1);

            filler(buf,nameBuf,NULL,0);

            dir_entry_index+=current_dir_entry.rec_len;

            /*change*/
            free(nameBuf);

        }

    }

    return 0;
}

/*Es la implementacion del man 2 open de un fichero*/
static int ext2_open(const char *path, struct fuse_file_info *fi)
{

    int status;

    struct ext2_inode inode;
    memset(&inode,0,EXT2_INODE_SIZE);

    status = get_inode_from_path(&inode,path);
    if(status == EXT2_FILE_NOT_FOUND || status == EXT2_ERROR)
        return -ENOENT;

    //el archivo tiene que ser un fichero
    if(S_ISDIR(inode.i_mode))
    {
        printf("El fichero %s es un directorio\n",path);
        return -ENOENT;
    }

    //if((fi->flags & 3) != O_RDONLY)
    //    return -EACCES;

    /*last change*/
    //chequeamos los permisos
    status = check_permissions(&inode);
    if(status == EACCES)
        return -EACCES;

    return 0;
}

/*Lee los datos del fichero representado por path*/
static int ext2_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;

    int status;

    /*change voy a reservarle memoria*/
    struct ext2_inode inode;
    memset(&inode,0,EXT2_INODE_SIZE);

    status = get_inode_from_path(&inode,path);
    if(status == EXT2_FILE_NOT_FOUND)
        return -ENOENT;

    if(S_ISDIR(inode.i_mode))
    {
        printf("El fichero %s es un directorio\n",path);
        return -ENOTDIR;
    }

    /*last change*/
    //chequeamos los permisos
    status = check_permissions(&inode);
    if(status == EACCES)
        return -EACCES;

    len = inode.i_size;

    if(offset > len) return 0;

    if(offset + size > len)
	size = len - offset;

    int i;

    int start_block = offset / (EXT2_BLOCK_SIZE);
    int end_block = (offset + size) / (EXT2_BLOCK_SIZE);

    if(offset + size > EXT2_BLOCK_SIZE)
        end_block +=  ((offset + size) > end_block*(EXT2_BLOCK_SIZE)) ? 1 : 0;

    int current_offset = offset % (EXT2_BLOCK_SIZE);
    int current_size = size;
    int tempSize;
    //contiene el bloque actual
    char *buffer= (char *)malloc(EXT2_BLOCK_SIZE);
    //creamos el dir_entry current

    if(current_offset != 0)
    {
        status = read_inode_block_from_index(buffer,&inode,start_block);
        if(status == EXT2_ERROR)//verificamos si hubo error
            return EXT2_ERROR;

        tempSize = ((EXT2_BLOCK_SIZE) - current_offset) < size ? ((EXT2_BLOCK_SIZE) - current_offset) : size;
        memcpy(buf,buffer + current_offset, tempSize);

        current_size -= tempSize;

        start_block++;
    }


    for( i=start_block ; i <= end_block; i++)
    {
        if(current_size <= 0) break;

        status = read_inode_block_from_index(buffer,&inode,i);
        if(status == EXT2_ERROR)//verificamos si hubo error
            return EXT2_ERROR;

        tempSize = (current_size > EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE:  current_size;
        memcpy(buf + (size - current_size),buffer, tempSize);

        current_size -= tempSize;
    }

    /*change*/
    free(buffer);/*last*/

    return size;
}


//Punto de comunicacion entre las librerias de FUSE y nuestro FileSystem
static struct fuse_operations vbd_oper = {
    .getattr	= ext2_getattr,
    .readdir	= ext2_readdir,
    .open	= ext2_open,
    .read	= ext2_read,
};


/*
 * Main de nuestro FileSystem
 */
int main(int argc, char *argv[])
{
    int status;

    //argv[0] : ruta del ejecutable
    //argv[1] : ruta del fichero en que el File System guardara toda su informacion
    //argv[2] : ruta del punto de montaje.
    if(argc != 3)//ejecutado sin parametros
    {
        printf("\nDeben haber 2 parámetros:\n");
        printf("\t1- Ruta del fichero de datos.\n");
        printf("\t2- Ruta del punto de montaje.\n");
        printf("Presione una tecla para salir.\n");
        getchar();
        return (EXIT_FAILURE);
    }

    status = init_FileSystem(argv[1],argv[2]);

    //verificamos si hubo error cargando el FileSystem
    if(status == EXT2_ERROR)
    {
        printf("No se pudo cargar el Sistema de Ficheros\n");
        return EXIT_FAILURE;
    }

    char *fuse_argv[2];
    fuse_argv[0] = argv[0];
    fuse_argv[1] = argv[2];

  return fuse_main(2, fuse_argv, &vbd_oper);

}
