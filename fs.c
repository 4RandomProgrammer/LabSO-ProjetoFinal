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
char file_status[DIRENTRIES] = {'F'};


#define MAXFILE CLUSTERSIZE * 200

//#define MAXFILE 10     
char writeBuff[MAXFILE];

typedef struct{
  char conteudo[MAXFILE];
  int file_id;
  int pos_read;
  
} readBuffer;

readBuffer readBuff;


int FatDirSize = 32+1;

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
  	for (int i = 0; i < FATCLUSTERS; i++)
  	{
      	//TODO
      	if(fat[i] == 1 && i != last_seen_index) return i;
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

void clean_write_buffer(){
	
	int size = strlen(writeBuff);

	for (int i = 0; i < size ; i++)
	{
		writeBuff[i] = '\0';
	}
}


//void print_dir() {
//	for (int i = 0; i < DIRENTRIES; i++){
//    	printf("%d: %d %d %d\n", i, dir[i].first_block, dir[i].used, dir[i].size);
//	}
//}
//

int create_file(char* file_name) {
	//Operação apenas possível em disco formatado
	if(!formatado){
		printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
		return 0;
	}

	//Checando o tamanho do nome do arquivo
	if(strlen(file_name) > 24)
	{
		printf("Erro: Nome do arquivo deve conter apenas 24 caracteres\n");
		return 0;
	}
	
	
	//checagem de nome repetido
	
	for(int i = 0; i < DIRENTRIES; i++){
		if(dir[i].used){
		
			if(!strcmp(dir[i].name, file_name)){
				//nome de arquivo igual causa erro
				printf("Erro: Já existe um arquivo com esse nome.\n");
				return 0;
			}
		}
	}

	//Nova entrada no dir
  	dir_entry new;
  	new.used = 1;
  	strcpy(new.name, file_name);
  	new.first_block = find_first_empty_fat_index(0);
  	new.size = 0; 

	//Checagem se é possível adicionar mais arquivos 
	int new_dir_index = find_first_empty_dir();
	if(new_dir_index == -1)
	{
		printf("Erro: Não é possível criar mais arquivos\n");
		return 0;
	}

  	dir[new_dir_index] = new;

	fat[new.first_block] = 2;

	if(write_fat() && write_dir()){
		return new_dir_index;
	}else{
		return -1;
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
	for (int i = 0; i < fat_count; i++) {	
		// Checar se os arquivos estão com índices corretos
	        if (fat[i] != 3) {
    	        printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
    	        return 1;
    	    }
  	}

	//Checando índice do diretório
  	if (fat[fat_count] != 4) {
  	    printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
  	    return 1;
  	}
  	
    formatado = 1;
	return 1;
}

/* Inicia o dispositivo de disco para uso, iniciando e 
escrevendo as estruturas de dados necessárias */
//Basicamente remove todas as entradas no diretório e reseta a FAT
int fs_format() {

	//Limpando todo o vetor de Dir
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

	//índice do diretório
	fat[32] = 4;

	//índices mostrando que o setor está livre 
	for (int i = 33; i < FATCLUSTERS; i++){
    	fat[i] = 1;
	}
	
	if(write_dir() && write_fat()){
		formatado=1;
		return 1;
	}else{
		return 0;
	}

  //return 0;
}


//Retorna o espaço livre no dispositivo em bytes
int fs_free() {

	//33 = 32 setores da FAT + 1 setor do Dir
	//Como não podem ser usados para escrever arquivos, subtraímos 
	int max_size = (bl_size() - 33) * SECTORSIZE ;

	//Somando o valor em bytes de todos os arquivos do sistema e subtraindo do total máximo
	for (int i = 0 ; i < DIRENTRIES ; i++) {

		if(dir[i].used){
			int actual_size = dir[i].size / SECTORSIZE; //divisao inteira do tamanho pelo setor
			
			//se a divisao nao for inteira ou o tamanho do arquivo for nulo, o arquivo ocupa um setor a mais.
			if( (dir[i].size % SECTORSIZE) || actual_size == 0) actual_size++;
			
			max_size = max_size - (actual_size * SECTORSIZE);
		 
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
	
	//Operação apenas possível em disco formatado
	if(!formatado){
		printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
		return 0;
	}

	buffer[0]='\0';
	char temp_buffer[150];
	
	//Escrevendo as informações da listagem no buffer
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
	
	//Operação apenas possível em disco formatado
	if(!formatado){
		printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
		return 0;
	}

	//Checando o tamanho do nome do arquivo
	if(strlen(file_name) > 24)
	{
		printf("Erro: Nome do arquivo deve conter apenas 24 caracteres\n");
		return 0;
	}
	
	
	//checagem de nome repetido
	
	for(int i = 0; i < DIRENTRIES; i++){
		if(dir[i].used){
		
			if(!strcmp(dir[i].name, file_name)){
				//nome de arquivo igual causa erro
				printf("Erro: Já existe um arquivo com esse nome.\n");
				return 0;
			}
		}
	}

	//Nova entrada no dir
  	dir_entry new;
  	new.used = 1;
  	strcpy(new.name, file_name);
  	new.first_block = find_first_empty_fat_index(0);
  	new.size = 0; 

	//Checagem se é possível adicionar mais arquivos 
	int new_dir_index = find_first_empty_dir();
	if(new_dir_index == -1)
	{
		printf("Erro: Não é possível criar mais arquivos\n");
		return 0;
	}

  	dir[new_dir_index] = new;
	file_status[new_dir_index] = 'F';

	fat[new.first_block] = 2;

	if(write_fat() && write_dir()){
		return 1;
	}else{
		return 0;
	}
}


int fs_remove(char *file_name) {

	
	if(!formatado){
		printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
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
			dir[i].size = 0;

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

	if(!removed) printf("Erro: o arquivo passado como parâmetro não pode ser removido.\n");

	return removed;
}


// ------------ PARTE 2 -------------//


int fs_open(char *file_name, int mode) {
	//Operação apenas possível em disco formatado
	if(!formatado){
		printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
		return -1;
	}

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
      		printf("Erro: Arquivo não existe!\n");
      		return -1;
    	}
		file_status[file_index] = 'R';
		readBuff.file_id = -1;
    
  	// Modo de escrita
  	} else {
    	if (file_index != -1) {
      		fs_remove(file_name);
    	}
    	
		file_index = create_file(file_name);
    	
		if (file_index == -1){
      		return -1;
		}
		
		file_status[file_index] = 'W';
  }
  
  return file_index;
}


int fs_close(int file)  {
	//verifica o arquivo no diretorio
	if(dir[file].used==0){
		printf("Erro: Arquivo não existe!\n");
		return 0;
	}
	//verificar se o arquivo em questao está aberto
	if(file_status[file]=='F'){
		printf("Erro: Arquivo não está aberto!\n");
		return 0;
	}

	//Ultima chamada para terminar de printar 
	if(file_status[file] == 'W' && fs_write(NULL,-1,file) == 0)
	{
		printf("Erro: arquivo não pode ser criado corretamente\n");
		clean_write_buffer();
		fs_remove(dir[file].name);
		return 0;
	}

	//se o arquivo existe no diretório e foi aberto, ele é marcado como fechado
	file_status[file] = 'F';	
  	//printf("Função não implementada: fs_close\n");
	
//	clean_write_buffer();

	return 1;
}



int fs_write(char *buffer, int size, int file) {
	//printf("Função não implementada: fs_write\n");
		
	//Operação apenas possível em disco formatado
	if(!formatado){
		printf("Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
		return 0;
	}

	//Operação apenas possível em arquivo com capacidade de escrita 
	if(file_status[file]!='W') 
	{
		printf("Erro: Arquivo não possui capacidade de escrita\n");
		return 0;
	}

	int writeBuffSize = strlen(writeBuff);

	//Checando se cabe em disco 
	if(writeBuffSize + size > fs_free())
	{
		printf("Erro: Não há espaço o suficiente em disco\n");
		clean_write_buffer();
		

		return 0;
	}

	//Checando se cabe no buffer de escrita 
	if(writeBuffSize + size > MAXFILE)
	{
		printf("Erro: Tamanho máximo de arquivo excedido\n");
		clean_write_buffer();
		

		return 0;
	}


	// A chamada fs_close indica que os últimos bytes foram salvos no buffer de escrita mandando o size como -1. 
	// Enquanto não for, apenas concatenamos o conteúdo a um buffer de escrita
	// Essa prática foi escolhida devido a quantidade de bytes mandados pelas funções de copia (apenas 10). 
	// Como o acesso a memória segundária é extremamente lento, com esse truque acessamos ele o menor número de vezes 
	if(size >= 0)
	{
		strncat(writeBuff,buffer,size);
		//puts(writeBuff);
		return size;
	}

	writeBuff[writeBuffSize] = '\0';


	//Caso o tamanho não caiba certinho em todos os setores, temos que levar isso em conta. 
	// Como não temos a função de teto da math.h, caso o size tenha resto, adicionamos +1 (meio que um teto artificial)
	int quebrado = (writeBuffSize % SECTORSIZE) != 0 ? 1 : 0;

	int iterations = (writeBuffSize / SECTORSIZE) + quebrado;

	int w_block = dir[file].first_block;

	//Flag indicando última iteração 
	int lastRide = 0;
	for (int i = 0; i < iterations; i++) {

		//Se for a última iteração temos que setar a fat com 2
		if(i+1 == iterations) 
		{
			fat[w_block] = 2;
			lastRide = 1;
		}

		

		//pegando o próximo bloco livre. O parâmetro da função indica para desconsiderar que o bloco atual está livre, se não, 
		// a função retornará sempre w_block já que sua posição na fat mudará apenas mais pra frente 
		int new_block = find_first_empty_fat_index(w_block);

		if(!lastRide) 
		{
			//Atualizando apontador pro próximo setor com informações
			fat[w_block] = new_block ;
			
		}


		bl_write(w_block, &writeBuff[i*SECTORSIZE]);
		w_block = new_block;
	}
		//bl_write(find_first_empty_fat_index(0),);


	//Ajustando o tamanho do arquivo
	dir[file].size += writeBuffSize;

	clean_write_buffer();

	//Escrevendo as modificações
	if(write_fat() && write_dir()){
		return 1;
	}else{
		return 0;
	}
	

	
}


int fs_read(char *buffer, int size, int file) {
    // printf("Função não implementada: fs_read\n");
  // return -1;
  // caso o arquivo n esteja senod usando
  //
  int bytes_lidos = 0;

  if (!formatado) {
    printf(
        "Erro: o disco não está pronto para uso. É necessário formatá-lo.\n");
    return 0;
  }

  if (!dir[file].used) {
    printf("Arquivo informado não esta sendo utilizado");
    return -1;
  }

  if (file_status[file] != 'R') {
    printf("Arquivo nao esta no modo de leitura.");
    return -1;
  }

  // Para a primeira chamada configurando todo o arquivo a ser lido;
 
  if (readBuff.file_id != file) {
    readBuff.file_id = file;
    readBuff.pos_read = 0;

    // Aq lê o arq completo
    // Pegando o primeiro bloco indexado
    int pos = dir[file].first_block;
    int nextPos = fat[pos];

    // Lendo o arquivo completamente para a fat
    readBuff.conteudo[0] = '\0';
    for (int i = 0; pos != 2; i++) {
      bl_read(pos, &readBuff.conteudo[i * SECTORSIZE]);

      pos = nextPos;
      nextPos = fat[pos];
    }

	//puts(readBuff.conteudo);
    
  }

  // passando o arquivo de size em size
  if (dir[readBuff.file_id].size > readBuff.pos_read) {

    int bytes_para_ler = size;
    if (dir[readBuff.file_id].size < readBuff.pos_read + size) {
      bytes_para_ler = dir[readBuff.file_id].size - readBuff.pos_read;
    }

    strncpy(buffer, &readBuff.conteudo[readBuff.pos_read], bytes_para_ler);
    bytes_lidos = bytes_para_ler;
    readBuff.pos_read += bytes_para_ler;
    return bytes_lidos;

  } else {
    readBuff.pos_read = 0;
    return 0;
  }
}

