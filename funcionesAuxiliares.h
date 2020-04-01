#ifndef FUNCIONESAUXILIARES_H_INCLUDED
#define FUNCIONESAUXILIARES_H_INCLUDED



#endif // FUNCIONESAUXILIARES_H_INCLUDED


#include <stdio.h>


//Funciones referentes al FileSystem

int newPow(int n, int m);

//para levantar lo necesario al iniciar el FileSystem
int init_FileSystem(char *data_path, char *mount_path);

//para leer el superbloque del FileSystem
int read_super_block();

//para leer la tabla de descriptores de grupos
int read_group_desc_table();

//para obtener un group descriptor de la tabla de descriptores de grupo
int get_group_desc_from_index(struct ext2_group_desc *group_desc, unsigned int index);

//lee el bloque que tiene numero block_number
int read_block_from_number(unsigned int block_number, void *block);

//lee el bloque que comienza en la posicion block_offset
int read_block_from_offset(void *buffer,unsigned int block_offset);

//lee el bloque block_number- ésimo del inode y lo devuelve en el buffer
int read_inode_block_from_index(void *buffer,struct ext2_inode *inode,unsigned int block_index);

//obtiene el inode inode_number-ésimo del FileSystem
int get_inode_from_number(struct ext2_inode *inode,unsigned int inode_number);

//lee un inode dado un offset
int get_inode_from_offset(struct ext2_inode *inode,unsigned int inode_offset);

//lee un inode dado un path
int get_inode_from_path(struct ext2_inode *inode,const char *path);

//va a iterar por todas sus dir_entry hasta encontrar name
int get_dir_entry_from_name(struct ext2_dir_entry *dir_entry, struct ext2_inode *inode,char *name);

//lee un ext2_dir_entry dado un offset
int get_dir_entry_from_offset(struct ext2_dir_entry *dir_entry, char *data_buffer,unsigned int offset);


/*
    Otras funciones auxiliares
 */

//lee la información para un buffer
int read_to_buffer(void *buffer,int amount_bytes);
