#ifndef TEMA2_H
#define TEMA2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mpi.h>
#include <stdbool.h>

// Definiții de constante
#define MAX_FILES 10
#define MAX_FILENAME 256
#define MAX_CHUNKS 100
#define HASH_SIZE 32
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define TRACKER_RANK 0

// Structura pentru un fișier deținut de client
typedef struct {
    char filename[MAX_FILENAME];     
    int num_chunks;                  
    char chunks[MAX_CHUNKS][HASH_SIZE + 1];
} OwnedFile;


// Structura pentru datele unui client
typedef struct {
    int num_owned_files;
    OwnedFile owned_files[MAX_FILES];
    int num_wanted_files;
    char wanted_files[MAX_FILES][MAX_FILENAME];
} ClientData;

// Structura pentru swarm
typedef struct {
    char filename[MAX_FILENAME];
    int num_segments;
    char hashes[MAX_CHUNKS][HASH_SIZE + 1];  
    int clients[MAX_CLIENTS];                
    int num_clients;
} Swarm;

// Structura pentru baza de date a tracker-ului
typedef struct {
    Swarm swarms[MAX_FILES];
    int num_swarms;
} TrackerDB;

// Structura pentru un fișier de descărcat
typedef struct {
    char filename[MAX_FILENAME];
    int num_segments;
    char segments[MAX_CHUNKS][HASH_SIZE + 1];  
    int owners[MAX_CHUNKS][MAX_CLIENTS];      
    int num_owners[MAX_CHUNKS];                
} DownloadFile;

// Structura pentru contextul de descărcare
typedef struct {
    int rank;
    DownloadFile *download_files;
    int num_files;
} DownloadContext;

// Structura pentru contextul de upload
typedef struct {
    int rank;
    ClientData *client_data;
} UploadContext;


void construct_input_path(char *buffer, size_t buffer_size, int rank);
void read_input_file(const char *filename, ClientData *client_data);
void *download_thread_func(void *arg);
void *upload_thread_func(void *arg);
void tracker(int numtasks, int rank);
void peer(int numtasks, int rank);
DownloadFile *initialize_download_files(Swarm *swarm, int num_files);
void afiseaza_download_files(DownloadFile *download_files, int num_files);
DownloadFile *request_file_owners(const char *filename, int rank);
void unusedVariable(DownloadFile *download_files, int num_files);
void checker_input_path(char *buffer, size_t buffer_size, int rank);
void afiseaza_informatii_download(DownloadContext *context);
#endif  // TEMA2_H
