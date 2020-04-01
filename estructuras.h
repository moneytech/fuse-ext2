#ifndef ESTRUCTURAS_H_INCLUDED
#define ESTRUCTURAS_H_INCLUDED



#endif // ESTRUCTURAS_H_INCLUDED


#include <math.h>
#include <stdint.h>
#include <stdio.h>

/*
    Macros
 */
#define EXT2_NAME_LEN 255
#define EXT2_N_BLOCKS 15                                    /*12 directos,1 indirecto,1 doble indirecto y 1 triple indirecto*/
#define EXT2_INODE_SIZE 128                                 /*El tamaño del inode es 128 bytes*/
#define EXT2_SBLOCK_SIZE 1024                               /*El tamaño de la estructura ext2_super_block es 1024*/
#define EXT2_SBLOCK_OFFSET 1024                             /*En que posicion comienza la informacion del superbloque*/
#define EXT2_ERROR -1                                       /*Si se produce algun tipo de error vamos a devolver este valor*/
#define EXT2_NO_ERROR 0                                     /*Si no hay error devolvemos esto*/
#define EXT2_FILE_NOT_FOUND 1                               /*Si no se encontro el fichero a buscar*/
#define EXT2_BLOCK_SIZE (1024*newPow(2,main_superblock->s_log_block_size))   /*Tamaño de bloque del FileSystem*/
//conteo de bloques(i_block[])
#define EXT2_DIR_BLOCKS 12                                  /*Cantidad de bloques referenciados directos*/
#define EXT2_IND_BLOCKS     (EXT2_BLOCK_SIZE/4)             /*Cantidad de bloques referenciados indirectos*/
#define EXT2_DIND_BLOCKS    newPow(EXT2_BLOCK_SIZE/4,2)     /*Cantidad de bloques referenciados doblemente indirectos*/
#define EXT2_TIND_BLOCKS    newPow(EXT2_BLOCK_SIZE/4,3)     /*Cantidad de bloques referenciados triplemente indirectos*/
//tamaño del group un descriptor
#define EXT2_GROUP_DESC_SIZE 32
#define EXT2_GROUP_COUNT (main_superblock->s_blocks_count/main_superblock->s_blocks_per_group)

#define EXT2_FIRST_INODE 1
#define EXT2_ROOT_INODE 2

//Para verificar los permisos
#define IGNORE_PERMISSIONS 0    /*1: ignorar los permisos 0:tener en cuenta los permisos*/
#define USER 256
#define GROUP 32
#define OTHER 4

/*Variables del Sistema de Ficheros*/
char *data_file_path;                       /*Archivo donde se guardan los datos*/
char *mount_point_path;                     /*Ruta del punto de montaje*/
FILE *data_file;                            /*Archivo de los Datos*/
struct ext2_super_block *main_superblock;   /*Superbloque principal*/
struct ext2_inode *inode_root;              /*Inodo raíz*/
char *group_desc_table;                     /*Tabla de descriptores de grupo*/


/*Descriptores de grupo*/
struct ext2_group_desc
{
 uint32_t bg_block_bitmap;          /* Blocks bitmap block */
 uint32_t bg_inode_bitmap;          /* Inodes bitmap block */
 uint32_t bg_inode_table;           /* Inodes table block */
 uint16_t bg_free_blocks_count;     /* Free blocks count */
 uint16_t bg_free_inodes_count;     /* Free inodes count */
 uint16_t bg_used_dirs_count;       /* Directories count */
 uint16_t bg_pad;
 uint32_t bg_reserved[3];
};

/*Entradas de directorios*/
struct ext2_dir_entry
{
 uint32_t inode;                    /* Inode number */
 uint16_t rec_len;                  /* Directory entry length */
 uint8_t name_len;                  /* Name length */
 char name[EXT2_NAME_LEN];          /* File name */
 uint8_t file_type;                 /*File Type
                                        0 Unknown
                                        1 Regular file
                                        2 Directory
                                        3 Character device
                                        4 Block device
                                        5 Named pipe
                                        6 Socket
                                        7 Symbolic link*/
};

///* The ext2 blockgroup. Creo que no  es imprescindible */
//struct ext2_block_group
//{
//  uint32_t block_id;
//  uint32_t inode_id;
//  uint32_t inode_table_id;
//  uint16_t free_blocks;
//  uint16_t free_inodes;
//  uint16_t used_dirs;
//  uint16_t pad;
//  uint32_t reserved[3];
//};

/*The superblock is located at the fixed offset 1024 in the device.
 *Its length is 1024 bytes also.*/
struct ext2_super_block
{
uint32_t s_inodes_count;            /* Inodes count */
uint32_t s_blocks_count;            /* Blocks count */
uint32_t s_r_blocks_count;          /* Reserved blocks count */
uint32_t s_free_blocks_count;       /* Free blocks count */
uint32_t s_free_inodes_count;       /* Free inodes count */
uint32_t s_first_data_block;        /* First Data Block */
uint32_t s_log_block_size;          /* Block size */
uint32_t s_log_frag_size;           /* Fragment size */
uint32_t s_blocks_per_group;        /* # Blocks per group */
uint32_t s_frags_per_group;         /* # Fragments per group */
uint32_t s_inodes_per_group;        /* # Inodes per group */
uint32_t s_mtime;                   /* Mount time */
uint32_t s_wtime;                   /* Write time */
uint16_t s_mnt_count;               /* Mount count */
uint16_t s_max_mnt_count;           /* Maximal mount count */
uint16_t s_magic;                   /* Magic signature */
uint16_t s_state;                   /* File system state */
uint16_t s_errors;                  /* Behaviour when detecting errors */
uint16_t s_pad;
uint32_t s_lastcheck;               /* time of last check */
uint32_t s_checkinterval;           /* max. time between checks */
uint32_t s_creator_os;              /* OS */
uint32_t s_rev_level;               /* Revision level */
uint16_t s_def_resuid;              /* Default uid for reserved blocks */
uint16_t s_def_resgid;              /* Default gid for reserved blocks */
uint32_t s_reserved[235];           /* Padding to the end of the block */
};

struct ext2_inode
{
 uint16_t i_mode;                   /* File mode */
 uint16_t i_uid;                    /* Owner Uid */
 uint32_t i_size;                   /* Size in bytes */
 uint32_t i_atime;                  /* Access time */
 uint32_t i_ctime;                  /* Creation time */
 uint32_t i_mtime;                  /* Modification time */
 uint32_t i_dtime;                  /* Deletion Time */
 uint16_t i_gid;                    /* Group Id */
 uint16_t i_links_count;            /* Links count */
 uint32_t i_blocks;                 /* Blocks count */
 uint32_t i_flags;                  /* File flags */
 union
 {
    struct
    {
        uint32_t l_i_reserved1;
    } linux1;

    struct
    {
        uint32_t h_i_translator;
    } hurd1;

    struct
    {
        uint32_t m_i_reserved1;
    } masix1;

} osd1; /* OS dependent 1 */

uint32_t i_block[EXT2_N_BLOCKS];    /* Pointers to blocks */
uint32_t i_version;                 /* File version (for NFS) */
uint32_t i_file_acl;                /* File ACL */
uint32_t i_dir_acl;                 /* Directory ACL */
uint32_t i_faddr;                   /* Fragment address */
union
{
    struct
    {
        uint8_t l_i_frag;           /* Fragment number */
        uint8_t l_i_fsize;          /* Fragment size */
        uint16_t i_pad1;
        uint32_t l_i_reserved2[2];
    } linux2;

    struct
    {
        uint8_t h_i_frag;           /* Fragment number */
        uint8_t h_i_fsize;          /* Fragment size */
        uint16_t h_i_mode_high;
        uint16_t h_i_uid_high;
        uint16_t h_i_gid_high;
        uint32_t h_i_author;
    } hurd2;

    struct
    {
        uint8_t m_i_frag;           /* Fragment number */
        uint8_t m_i_fsize;          /* Fragment size */
        uint16_t m_pad1;
        uint32_t m_i_reserved2[2];
    } masix2;
} osd2; /* OS dependent 2 */

};//fin de la estructura
