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

/*
bl_write escreve SECTORZISE bytes

4kiB = 1 setor = 4096 bytes

Setores
FAT -> 32 setores [0-31]
DIR -> 1 setor [32]
ARQUIVOS -> [33+]
*/

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096     // Tamanho de um cluster da FAT em bytes
#define FATCLUSTERS 65536    // Tamanho total da FAT em short (bytes/2)
#define DIRENTRIES 128       // Quantidade de arquivos no diretório

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];
int formatado = 0;


/*FUNÇÕES AUXILIARES*/

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

int write_fat(){
	int sector = 0;
	int fat_count = 2*FATCLUSTERS/CLUSTERSIZE;	// Multiplica por 2 pq a FATCLUSTERS está em short (bytes/2)

  	// Carregar a FAT
	char* buffer = (char *) fat;
	for (int i = 0; i < fat_count; i++) {	
		if(bl_write(i, &buffer[i*SECTORSIZE])){
			sector++;
		}else{
			return 0;
		}
		  	}


	return 1;
}

int write_dir(){
	char* buffer = (char *) dir;
	int fat_count = 2*FATCLUSTERS/CLUSTERSIZE;
	return bl_write(fat_count, buffer);
	
}


void print_dir() {
	for (int i = 0; i < DIRENTRIES; i++){
    	printf("%d: %d %d %d\n", i, dir[i].first_block, dir[i].used, dir[i].size);
	}
}


int create_file(char* file_name) {
	//checagem de nome
	for(int i = 0; i < DIRENTRIES; i++){
		if(dir[i].used){
			if(!strcmp(dir[i].name, file_name)){
				//nome de arquivo igual causa erro
				printf("Erro: Já existe um arquivo com esse nome.\n");
				return 0;
			}
		}
	}

  	dir_entry new;
  	new.used = 1;
  	strcpy(new.name, file_name);
  	new.first_block = find_first_empty_fat_index(0);
  	new.size = 0;
  	int file_index = find_first_empty_dir();
  	dir[file_index] = new;

	fat[new.first_block] = 2;

	if(write_fat() && write_dir()){
		return file_index;
	}else{
		return 0;
	}
}


// ------------ PARTE 1 -------------//


/* Inicia o sistema de arquivos e suas estruturas internas. Esta função é automaticamente chamada pelo interpretador de comandos no
início do sistema. Esta função deve carregar dados do disco para restaurar um sistema já em uso 
e é um bom momento para verificar se o disco está formatado.*/
int fs_init() {
	int fat_count = 2*FATCLUSTERS/CLUSTERSIZE;	// Multiplica por 2 pq a FATCLUSTERS está em short (bytes/2)

  	// Carregar a FAT
	char* buffer = (char *) fat;
	for (int i = 0; i < fat_count; i++) {	// Puxa os primeiros fat_count setores, lendo a FAT guardada no disco
		bl_read(i, &buffer[i*SECTORSIZE]);
  	}

	// Carregar o diretório
	buffer = (char*)dir;
	bl_read(fat_count, buffer);


	// Checar se ta formatado
	for (int i = 0; i < fat_count; i++) {	// Checar se os arquivos estão certinhos
	        if (fat[i] != 3) {
    	        printf("Este disco não está formatado ainda =(\n");
    	        return 1;
    	    }
  	}

  	if (fat[fat_count] != 4) {
  	    printf("Este disco não está formatado ainda =(\n");
  	    return 1;
  	}
  	
    formatado = 1;
  	return 1;
}

/* Inicia o dispositivo de disco para uso, iniciando e 
escrevendo as estruturas de dados necessárias */
//Basicamente remove todas as entradas no diretório e reseta a FAT
int fs_format() {

	for (int i = 0; i < DIRENTRIES; i++){
    	//strcpy(dir[i].name, NULL);
		dir[i].first_block = 0;
		dir[i].used = 0;
		dir[i].size = 0;
	}
	
	//formatar a fat
	for (int i = 0; i < 32; i++){
    	fat[i] = 3;
	}

	fat[32] = 4;

	for (int i = 33; i < FATCLUSTERS; i++){
    	fat[i] = 1;
	}
	
	if(write_dir() && write_fat()){
		return 1;
	}else{
		return 0;
	}

  //return 0;
}


//Retorna o espaço livre no dispositivo em bytes
//TODO: conferir se está certo
int fs_free() {
	int max_size = bl_size() * SECTORSIZE;

	for (int i = 0 ; i < DIRENTRIES ; i++) {

		if(dir[i].used){
			max_size = max_size - dir[i].size; 
		}
	}
  //printf("Função não implementada: fs_free\n");
  return max_size;
}


//Lista os arquivos do diretório, 
//colocando a saída formatada em buffer.
int fs_list(char *buffer, int size) {
	//printf("Função não implementada: fs_list\n");
	//buffer = NULL;
	buffer[0]='\0';
	char temp_buffer[150];
	
  	for (int i = 0 ; i < DIRENTRIES ; i++) {
    
    	if(dir[i].used == 1) 
    	{
			sprintf(temp_buffer, "%s\t\t%d\n", dir[i].name, dir[i].size);
			strcat(buffer, temp_buffer);
      		//printf("%s\n", dir[i].name);
    	} 
  	}

	//printf("%s", buffer);
	return 1;
}

//Cria um novo arquivo com nome file_name e tamanho 0. 
//Um erro deve ser gerado se o arquivo já existe.
int fs_create(char* file_name) {
	if (create_file(file_name))
	  return 1;
	else
	  return 0;
}


int fs_remove(char *file_name) {

	//Pegar os nome dos arquivos da fat; - onde estão o nome dos arquivos?
	//Ir comparando nome a nome e remover
	//Como remover? Setar tudo para 0?
	//Ao remover enviar os agrupamentos livres
	//como que com a posição na fat eu chego nos arquivos?

	int removed = 0;
	int i = 0;

	while(i < DIRENTRIES){
		
		//Remover
		if(strcmp(file_name,dir[i].name) == 0){

			//Setando removed para 1 já que ele foi removido
			removed = 1;

			//Como remover
			dir[i].used = 0;

			//Criar um buffer de 1s com o para ser escrito no disco
			int pos = dir[i].first_block;
			int nextPos = fat[pos];

			//Removido da fat
			while(pos != 2){

				fat[pos] = 1;
				pos = nextPos;
				nextPos = fat[pos];
			}
			
			//Remover o diretório do disco e da fat
			char *buffer = (char *) dir;
			bl_write(2*FATCLUSTERS/CLUSTERSIZE,buffer);

			break;
		}
		//Precisa ir até o final devido a fat ser esparsa. E se fossemos compactar a fat, deveriamos compactar o disco e isso é uma operação mt custosa.
		i++;

	}

	if(!removed) printf("Não há arquivos para se remover");

	return removed;
}

// ------------ PARTE 2 -------------//

int fs_open(char *file_name, int mode) {
  int file_index = -1;
  // Encontrar arquivo
  for (int i = 0 ; i < DIRENTRIES ; i++) {   
    	if(dir[i].used == 0)
    	  continue;
    	  
    	if (strcmp(file_name, dir[i].name) == 0) {
    	  file_index = i;
    	  break;
    	}
  } 

  // Modo de leitura
  if (mode == FS_R) {
    if (file_index == -1) {
      printf("ERRO: Arquivo não existe!\n");
      return -1;
    }
    
  // Modo de escrita
  } else {
    if (file_index != -1) {
      fs_remove(file_name);
    }
    file_index = create_file(file_name);
    if (file_index == 0)
      return -1;
  }
  
  return file_index;
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

