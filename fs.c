/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010,2012,2019 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096	// Tamanho de um cluster da FAT em bytes
#define FATCLUSTERS 65536	// Tamanho total da FAT em short (bytes/2)
#define DIRENTRIES 128	// Quantidade de arquivos no diretório

unsigned short fat[FATCLUSTERS];

typedef struct {
	char used;
	char name[25];
	unsigned short first_block;
	int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

/* Inicia o sistema de arquivos e suas estruturas internas. Esta função é automaticamente chamada pelo interpretador de comandos no
início do sistema. Esta função deve carregar dados do disco para restaurar um sistema já em uso e é um bom momento para verificar se o disco
está formatado.*/
int fs_init() {
	int fat_count = 2*FATCLUSTERS/CLUSTERSIZE;	// Multiplica por 2 pq a FATCLUSTERS está em short (bytes/2)

  	// Carregar a FAT
	char* buffer = (char *) fat;
	for (int i = 0 ; i < fat_count; i++) {	// Puxa os primeiros fat_count setores, lendo a FAT guardada no disco
		bl_read(i, buffer[i*SECTORSIZE]);
  	}

	// Carregar o diretório
	buffer = (char*)dir;
	bl_read(fat_count+1, buffer);

	// Checar se ta formatado
	for (int i = 0; i < DIRENTRIES; i++) {	// Checar se os arquivos estão certinhos
    	if (dir[i].used == 0)
			continue;
			
		// Procurando o fim, se eu encontrar UM arquivo desocupado eu fico NUCLEAR
		short index = fat[dir[i].first_block];
		while (index != 2) {
			if (fat[index] == 1)
				return 0;
			
			index = fat[index];
		}
  	}

  	return 1;
}

/* Inicia o dispositivo de disco para uso, iniciando e escrevendo as estruturas de dados necessárias */
int fs_format() {
	
	return 0;
}

int fs_free() {
	printf("Função não implementada: fs_free\n");
	return 0;
}

int fs_list(char *buffer, int size) {
	//printf("Função não implementada: fs_list\n");

  	for (int i = 0 ; i < DIRENTRIES ; i++) {
    
    	if(dir[i].used == 0) 
    	{
      		return 0; 
    	}
    	else{
      		printf("%s\n", dir[i].name);
    	} 
  	}

  	return 0;
}


//Itera sobre a lista de diretórios afim de achar o primeiro indice livre 
int find_first_empty_dir()
{
  	for (int i = 0; i < DIRENTRIES; i++)
  	{
    	if(dir[i].used == 0) return i;
  	}

  	return -1 ;
}

//Itera sobre a fat para achar um bloco sem nada escrito. 
//Parâmetro de index para caso queiramos começar a partir de um certo ponto
int find_first_empty_fat_index(int last_seen_index)
{ 

  	//Acha o primeiro bloco livre indicado por 1
  	for (int i = last_seen_index; i < FATCLUSTERS; i++)
  	{
      	//TODO
      	if(fat[i] == 1) return i;
  	}

  	return -1;
}

int fs_create(char* file_name) {
  	//printf("Função não implementada: fs_create\n");

  	dir_entry new;
  	new.used = 1;
  	strcpy(new.name, file_name);
  	new.first_block = find_first_empty_fat_index(0);
  	new.size = 0; 

  	dir[find_first_empty_dir()] = new;

  	return 0;
}

int fs_remove(char *file_name) {
  	printf("Função não implementada: fs_remove\n");
  	return 0;
}



// --------- PARTE 2 ----------------------

int fs_open(char *file_name, int mode) {
  	printf("Função não implementada: fs_open\n");
  

  	return -1;
}

int fs_close(int file)  {
  	printf("Função não implementada: fs_close\n");
  	return 0;
}

int fs_write(char *buffer, int size, int file) {
  	printf("Função não implementada: fs_write\n");
  	return -1;
}

int fs_read(char *buffer, int size, int file) {
  	printf("Função não implementada: fs_read\n");
  	return -1;
}

