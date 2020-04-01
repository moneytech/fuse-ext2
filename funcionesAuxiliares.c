/*
 * File:   funcionesAuxiliares.h
 * Author: Victor Manuel
 *
 * Created on December 25, 2010, 7:59 PM
 */

#include <sys/unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> //para el abort()


#include "estructuras.h"
#include "funcionesAuxiliares.h"

/*
    Funciones referentes al FileSystem
 */

int newPow(int n, int m)
{
    if(m == 0) return 1;
    if(m == 1) return n;
    return n*newPow(n,m-1);
}

int init_FileSystem(char *data_path, char *mount_path)
{

    int status;

    data_file_path=(char *)malloc(strlen(data_path)+1);
    strcpy(data_file_path,data_path);   //copiamos el path del archivo de los datos

    mount_point_path=(char *)malloc(strlen(mount_path)+1);
    strcpy(mount_point_path,mount_path);//copiamos el path del punto de montaje

    //abrimos el archivo para solo lectura
    data_file = fopen(data_path,"r");
    if(data_file == NULL)//si dio problemas abrir el archivo
        return EXT2_ERROR;

    //leemos el superbloque
    status = read_super_block();
    //verificamos si hubo problemas leyendo el superbloque
    if(status == EXT2_ERROR)
        return EXT2_ERROR;

    //leemos los descripores de grupos
    status = read_group_desc_table();
    //verificamos si hubo problemas cargando la tabla de descriptores de grupos
    if(status == EXT2_ERROR)
        return EXT2_ERROR;

    //cargar el directorio raiz de un filesystem
    inode_root=(struct ext2_inode *)malloc(EXT2_INODE_SIZE);
    status = get_inode_from_number(inode_root,EXT2_ROOT_INODE);


    return EXT2_NO_ERROR;
}

/*
 La tabla de descriptores de grupos es una tabla de tamaño 1 bloque situada
 después del superbloque y comenzando en un bloque nuevo.Esta contiene entradas
 de 32 bytes que describen un grupo (estructuras ext2_group_desc)
 */
int read_group_desc_table()
{
    /*La tabla de descriptores de grupo se copia en todos los grupos de
     bloques y por tanto solo vamos a leerla una vez*/
    int status;
    unsigned int size = EXT2_GROUP_COUNT*EXT2_GROUP_DESC_SIZE;
    unsigned int offset = EXT2_BLOCK_SIZE * (main_superblock->s_first_data_block+1);

    if (fseek(data_file,offset,SEEK_SET) == -1)//si hubo error
    {
        printf("\nError moviéndonos en el archivo: %s\n",data_file_path);
        return EXT2_ERROR;
    }

    group_desc_table = (char *)malloc(size);
    status = read_to_buffer(group_desc_table,size);
    return status;

    /*char buf[EXT2_BLOCK_SIZE];
    unsigned int current_size = size;
    int copy_size = EXT2_BLOCK_SIZE;*/
    //Vamos leyendo de bloque en bloque hasta llenar la tabla de
    //descriptores de grupo
   /* while(current_size > 0)
    {
		status=read_block_from_number(block_number,buf);
		memcpy(group_desc_table + (size - current_size),buf,copy_size);
		current_size -= copy_size;

		if(current_size < EXT2_BLOCK_SIZE)
			copy_size = current_size;

		if(status == EXT2_ERROR)
			return EXT2_ERROR;
	}*/
}

/*NO RESERVA MEMORIA*/
int get_group_desc_from_index(struct ext2_group_desc *group_desc, unsigned int index)
{
    memcpy(group_desc,group_desc_table+index * EXT2_GROUP_DESC_SIZE, EXT2_GROUP_DESC_SIZE);
    return EXT2_NO_ERROR;
}

//para leer el superbloque del FileSystem
int read_super_block()
{
    //el superbloque empieza en el offset 1024 y es de tamaño 1024

    //nos posicionamos en el archivo
    if ( fseek(data_file,EXT2_SBLOCK_OFFSET,SEEK_SET) == -1 )//si hubo error
    {
        printf("\nError moviéndonos en el archivo: %s\n",data_file_path);
        return EXT2_ERROR;
    }

     //reservamos memoria
     main_superblock = (struct ext2_super_block *)malloc(EXT2_SBLOCK_SIZE);

    //leemos la informacion del superbloque
    int status = read_to_buffer(main_superblock,EXT2_SBLOCK_SIZE);
    if(status == EXT2_ERROR)
        return EXT2_ERROR;

    //guardar algunas otras caracteristicas(no estoy seguro de que hagan falta)

    return EXT2_NO_ERROR;
}


/*
    block_number:  numero de bloque que se quiere leer
    block:         buffer a donde se guardara esa informacion

 NOTA:  Aqui hemos asumido que el 1er bloque tiene offset de 0, si no fuera
        asi entonces habria que sumarle un desplazamiento al calculo de offset

 NOTA:  Se asume que este es un bloque de datos, es decir, ya se calculo
        lo de los bloques indirectos,etc y se obtuvo block_number
 */
int read_block_from_number(unsigned int block_number, void *block)
{
	unsigned int offset;

        //calculamos el offset
        offset = EXT2_BLOCK_SIZE * block_number;

        //leemos el bloque de ese offset
        int status = read_block_from_offset(block,offset);

        return status;
}


/*
    buffer:         puntero a void donde se va a guardar la informacion
    block_offset:   offset del bloque a leer
 */
int read_block_from_offset(void *buffer,unsigned int block_offset)
{
    int status;

    //nos posicionamos en el archivo
    if ( fseek(data_file,block_offset,SEEK_SET) == -1 )//si hubo error
    {
        printf("\nError moviéndonos en el archivo: %s\n",data_file_path);
        return EXT2_ERROR;
    }

    //leemos la informacion
    status = read_to_buffer(buffer,EXT2_BLOCK_SIZE);

    return status;
}


/*
                                REVISARLO
    block_number:   es el indice del bloque del inodo a leer(su offset en el archivo puede ser cualquiera)
    buffer:         es donde se devolvera la informacion del bloque
 */
int read_inode_block_from_index(void *buffer,struct ext2_inode *inode,unsigned int block_index)
{
    int status;
    unsigned int block_number;//va a ser el bloque block_number-ésimo en el archivo
    unsigned int resto;

    //es un bloque directo
    if(block_index < EXT2_DIR_BLOCKS)
    {
        block_number = inode->i_block[block_index];
        status = read_block_from_number(block_number,buffer);
        return status;
    }

    //es un bloque referenciado de manera simple indirecta
    if(block_index < EXT2_DIR_BLOCKS + EXT2_IND_BLOCKS)
    {
        //por ahora es el bloque que tiene los bloques de datos
        block_number = inode->i_block[12];

        //leemos el bloque
        status = read_block_from_number(block_number,buffer);
        //si hubo error nos salimos
        if(status == EXT2_ERROR)
           return EXT2_ERROR;

        block_index-=EXT2_DIR_BLOCKS;//para el nuevo index

        //ahora convertimos la direccion de 4 bytes al verdadero block number
        memcpy(&block_number,buffer+block_index*4,4);

        //leemos el bloque que nos interesa
        status = read_block_from_number(block_number,buffer);
        return status;
    }

    //es un bloque referenciado de manera doblemente indirecta
    if(block_index < EXT2_DIR_BLOCKS + EXT2_IND_BLOCKS + EXT2_DIND_BLOCKS)
    {
        //bloque que apunta a bloques simple indirectos
        block_number = inode->i_block[13];

        //leemos el bloque de doble indireccion
        status = read_block_from_number(block_number,buffer);
        //si hubo error nos salimos
        if(status == EXT2_ERROR)
           return EXT2_ERROR;

        //para el nuevo indice
        resto= (block_index - EXT2_DIR_BLOCKS - EXT2_IND_BLOCKS)%EXT2_IND_BLOCKS;
        block_index = (block_index - EXT2_DIR_BLOCKS - EXT2_IND_BLOCKS)/EXT2_IND_BLOCKS;

        //ahora convertimos la direccion de 4 bytes
        memcpy(&block_number,buffer+block_index*4,4);

        //leemos el bloque de simple indireccion
        status = read_block_from_number(block_number,buffer);
        //si hubo error nos salimos
        if(status == EXT2_ERROR)
           return EXT2_ERROR;

        block_index = resto;
        //obtenemos la direccion del bloque de datos
        memcpy(&block_number,buffer+block_index*4,4);

        //leemos el bloque de datos
        status = read_block_from_number(block_number,buffer);
        return status;
    }

    //es un bloque referenciado de manera triplemente indirecta
    if(block_index < EXT2_DIR_BLOCKS + EXT2_IND_BLOCKS + EXT2_DIND_BLOCKS + EXT2_TIND_BLOCKS)
    {
        //bloque que apunta a bloques doblemente indirectos
        block_number = inode->i_block[14];

        //leemos el bloque de triple indireccion
        status = read_block_from_number(block_number,buffer);
        //si hubo error nos salimos
        if(status == EXT2_ERROR)
           return EXT2_ERROR;

        //para el nuevo indice
        resto = (block_index - EXT2_DIR_BLOCKS - EXT2_IND_BLOCKS - EXT2_DIND_BLOCKS)%EXT2_DIND_BLOCKS;
        block_index =(block_index - EXT2_DIR_BLOCKS - EXT2_IND_BLOCKS - EXT2_DIND_BLOCKS)/EXT2_DIND_BLOCKS;

        //ahora convertimos la direccion de 4 bytes
        memcpy(&block_number,buffer+block_index*4,4);

        //leemos el bloque de doble indireccion
        status = read_block_from_number(block_number,buffer);
        //si hubo error nos salimos
        if(status == EXT2_ERROR)
           return EXT2_ERROR;

        block_index = resto / EXT2_IND_BLOCKS;
        resto = resto%EXT2_IND_BLOCKS;

        //ahora convertimos la direccion de 4 bytes
        memcpy(&block_number,buffer+block_index*4,4);

        //leemos el bloque de simple indireccion
        status = read_block_from_number(block_number,buffer);
        //si hubo error nos salimos
        if(status == EXT2_ERROR)
           return EXT2_ERROR;

        block_index = resto;

        //ahora convertimos la direccion de 4 bytes
        memcpy(&block_number,buffer+block_index*4,4);

        //finalmente leemos el bloque de datos
        status = read_block_from_number(block_number,buffer);
        return status;
    }

    //entonces el indice de bloque se sale de la cantidad
    return EXT2_ERROR;
}

//inode tiene que tener reservada memoria de antemano
int get_inode_from_number(struct ext2_inode *inode,unsigned int inode_number)
{
    int status;
    unsigned int group_number = (inode_number - EXT2_FIRST_INODE) / main_superblock->s_inodes_per_group;
    unsigned int inode_index =  (inode_number - EXT2_FIRST_INODE) % main_superblock->s_inodes_per_group;

    //obtenemos el group descriptor correspondiente
    //CHANGE de MEMORIA
    //struct ext2_group_desc *current_group_desc=(struct ext2_group_desc *)malloc(EXT2_GROUP_DESC_SIZE);
    struct ext2_group_desc current_group_desc;

    get_group_desc_from_index(&current_group_desc,group_number);

    unsigned int offset_inode_table = current_group_desc.bg_inode_table*EXT2_BLOCK_SIZE;

    //leemos el inode correspondiente
    status = get_inode_from_offset(inode,offset_inode_table + inode_index*EXT2_INODE_SIZE);
    //verificamos si algo salio mal.
    if(status == EXT2_ERROR)
        return EXT2_ERROR;



    return EXT2_NO_ERROR;

}

/*NO RESERVA MEMORIA Y POR ESO LUEGO DE LLAMARLO NO HAY NECESIDAD DE LIBERAR MEMORIA*/
int get_inode_from_offset(struct ext2_inode *inode,unsigned int inode_offset)
{
    int status;

    //nos posicionamos en el archivo
    if ( fseek(data_file,inode_offset,SEEK_SET) == -1 )//si hubo error
    {
        printf("\nError moviéndonos en el archivo: %s\n",data_file_path);
        return EXT2_ERROR;
    }

    status = read_to_buffer(inode,EXT2_INODE_SIZE);
    //verificamos si algo salio mal.
    if(status == EXT2_ERROR)
        return EXT2_ERROR;

    return EXT2_NO_ERROR;
}

/*
    El inode tiene que tener reservada memoria de antemano
*/
int get_inode_from_path(struct ext2_inode *inode,const char *path)
{

    int status;

    struct ext2_inode current_inode;

    memcpy(&current_inode,inode_root,EXT2_INODE_SIZE);

    //el path tiene que ser relativo porque es a partir del punto de montaje
    if(path[0] != '/')
    {
        printf("El path recibido no es relativo\n");
        return EXT2_ERROR;
    }

    //si el directorio es el raiz entonces le devolvemos ese mismo inode
    if(strcmp(path,"/") == 0)
    {
        //porque inode tiene memoria reservada
        memcpy(inode,&current_inode,EXT2_INODE_SIZE);


        return EXT2_NO_ERROR;
    }

    //duplicamos el path
    char *path_copy=(char *)malloc(strlen(path)+1);
    strcpy(path_copy,path);

    path_copy = strtok(path_copy,"/");

    struct ext2_dir_entry current_dir_entry;
    while(path_copy != NULL)
    {
        status = get_dir_entry_from_name(&current_dir_entry,&current_inode,path_copy);
        if(status == EXT2_FILE_NOT_FOUND)
            return EXT2_FILE_NOT_FOUND;//devuelve not found o error.

        status = get_inode_from_number(&current_inode,current_dir_entry.inode);
        if(status == EXT2_ERROR)
            return EXT2_ERROR;

        path_copy=strtok(NULL,"/");//avanzamos al sgte nombre

    }

    //porque inode tiene memoria reservada
    memcpy(inode,&current_inode,EXT2_INODE_SIZE);

    /*change*/
    free(path_copy);/*last*/
    return EXT2_NO_ERROR;
}

/*
 Vamos a recorrer todos los dir_entry del inodo hasta encontrar el que tenga
 * como nombre a name.
 */
int get_dir_entry_from_name(struct ext2_dir_entry *dir_entry ,struct ext2_inode *inode,char *name)
{
    int i;
    int status;
    int dir_entry_index;

    int block_size = EXT2_BLOCK_SIZE;
    int block_count = (inode->i_size / block_size );
    block_count +=  (inode->i_size % block_size) == 0 ? 0 : 1;

    //contiene el bloque actual

    char *buffer= (char *)malloc(block_size);
    //creamos el dir_entry current
    struct ext2_dir_entry current_dir_entry;

    //int puntero_dir;
    for( i=0 ; i < block_count; i++)
    {
        status = read_inode_block_from_index(buffer,inode,i);
        if(status == EXT2_ERROR)//verificamos si hubo error
            return EXT2_ERROR;


        //leemos todos los dir_entry en el bloque y buscamos el que tenga a name
        dir_entry_index=0;
        while(dir_entry_index < block_size)
        {
            if(i*block_size+dir_entry_index > inode->i_size)//nos pasamos del archivo
                return EXT2_ERROR;

            status = get_dir_entry_from_offset(&current_dir_entry ,buffer,dir_entry_index);
            //quizas aqui comparar strncmp(current_dir_entry->name,name,name_len)
            if (strcmp(current_dir_entry.name,name) == 0)
             {
                 memcpy(dir_entry,&current_dir_entry,sizeof(struct ext2_dir_entry));
                return EXT2_NO_ERROR;
            }
            dir_entry_index+=current_dir_entry.rec_len;
        }

    }

    /*change*/
    free(buffer);
    //si llega aqui es que no encontro el dir_entry
    return EXT2_FILE_NOT_FOUND;
}

/*
    RESERVA MEMORIA, ASI QUE DESPUES DE CADA LLAMADA SE DEBERIA LIBERAR MEMORIA

    El offset representa donde empieza el dir_entry que se quiere leer
    dir_entry (no tiene reservada memoria por ser variable el size)
 */
int get_dir_entry_from_offset(struct ext2_dir_entry *dir_entry ,char *data_buffer,unsigned int offset)
{
    int current_offset=offset;

    uint32_t inode;                    /* Inode number */
    uint16_t rec_len;                  /* Directory entry length */
    uint8_t name_len;                 /* Name length */
    uint8_t file_type;


    //leemos el # del inodo
    memcpy(&inode,data_buffer+current_offset,sizeof(inode));
    current_offset+=sizeof(inode);

    //leemos la longitud del directory entry
    memcpy(&rec_len,data_buffer+current_offset,sizeof(rec_len));
    current_offset+=sizeof(rec_len);

    //leemos la longitud del nombre
    memcpy(&name_len,data_buffer+current_offset,sizeof(name_len));
    current_offset+= sizeof(name_len);

    //leemos el tipo de fichero que representa el dir_entry
    memcpy(&file_type,data_buffer+current_offset,sizeof(file_type));
    current_offset+= sizeof(file_type);

    uint8_t real_len = name_len;
    real_len += (name_len%4 == 0) ? 0: (4 - name_len%4);

    char name[real_len+1];          /* File name */
    //finalmente leemos el nombre
    memcpy(name,data_buffer+current_offset,real_len);

    dir_entry->inode = inode;
    dir_entry->rec_len = rec_len;
    dir_entry->name_len = name_len;
    dir_entry->file_type = file_type;

    strcpy(dir_entry->name,name);
    dir_entry->name[name_len]=0;

    return EXT2_NO_ERROR;
}
/*
    Otras funciones auxiliares
 */

/*
    Lee la información para un buffer.
    buffer: void* donde se va a devolver la informacion (tiene que tener reservada memoria)
*/
int read_to_buffer(void *buffer,int amount_bytes)
{
    int to_read=amount_bytes;
    int readed_bytes=0;

    do
    {
        readed_bytes=fread(buffer,1,to_read,data_file);
        if(readed_bytes > 0)
        {
            buffer+=readed_bytes;       //corremos la posicion donde comenzamos a escribir
            to_read-=readed_bytes;      //ahora tenemos que leer readed_bytes menos
        }
        else//hubo un error
        {
            printf("\nError leyendo del archivo: %s\n",data_file_path);
            return EXT2_ERROR;
        }
    }
    while(to_read > 0);//mientras haya qué leer

    buffer-=amount_bytes;

    return EXT2_NO_ERROR;
}
