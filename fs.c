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

	return bl_write(32, buffer);
	
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
		//printf("%d: %d \n", i, fat[i]);
  	}

	// Carregar o diretório
	buffer = (char*)dir;
	bl_read(fat_count+1, buffer);

	// Checar se ta formatado
	for (int i = 0; i < fat_count; i++) {	// Checar se os arquivos estão certinhos
	        //printf("%d: %d \n", i, fat[i]);
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

	//33 = 32 setores da FAT + 1 setor do Dir
	//Como não podem ser usados para escrever arquivos, subtraímos 
	int max_size = (bl_size() - 33) * SECTORSIZE ;

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
	

	//Criando nova entrada no dir
  	dir_entry new;
  	new.used = 1;
  	strcpy(new.name, file_name);
  	new.first_block = find_first_empty_fat_index(0);
  	new.size = 0; 

  	dir[find_first_empty_dir()] = new;

	//Setando o índice como 2 (Último agrupamento de arquivo)
	fat[new.first_block] = 2;


	if(write_fat() && write_dir()){
		return 1;
	}else{
		return 0;
	}
}


int fs_remove(char *file_name) {

	//Pegar os nome dos arquivos da fat; - onde estão o nome dos arquivos? dir[i].name
	//Ir comparando nome a nome e remover
	//Como remover? Setar tudo para 0?
	//Ao remover enviar os agrupamentos livres
	//como que com a posição na fat eu chego nos arquivos?
	
	if(!formatado){
		printf("Disco não formatado, formata isso primeiro");
		return 0;
	}

	int removed = 0;
	int i = 0;

	while(i < DIRENTRIES){
		
		//procurando o arquivo
		if(strcmp(file_name,dir[i].name) == 0 && dir[i].used){

			//Setando removed para mostrar que houve um arquivo removido
			removed = 1;

			//Arquivo não é mais utilizado
			dir[i].used = 0;

			//Pegando o primeiro bloco indexado
			int pos = dir[i].first_block;
			int nextPos = fat[pos];

			//Removendo o arquivo da fat
			while(pos != 2){

				fat[pos] = 1;
				pos = nextPos;
				nextPos = fat[pos];
			}
			
			write_dir();
			write_fat();


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

