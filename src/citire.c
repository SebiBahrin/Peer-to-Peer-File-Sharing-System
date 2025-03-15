#include "tema2.h"

// PT testare manuala
void construct_input_path(char *buffer, size_t buffer_size, int rank) {
    char input_directory[] = "../checker/tests/test1";

    if (snprintf(buffer, buffer_size, "%s/in%d.txt", input_directory, rank) >= buffer_size) {
        fprintf(stderr, "Path buffer overflow\n");
        exit(EXIT_FAILURE);
    }
}

//PT checker
void checker_input_path(char *buffer, size_t buffer_size, int rank) {
    if (snprintf(buffer, buffer_size, "in%d.txt", rank) >= buffer_size) {
        fprintf(stderr, "Path buffer overflow\n");
        exit(EXIT_FAILURE);
    }
}



void read_input_file(const char *filename, ClientData *client_data) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d", &client_data->num_owned_files) != 1) {
        fprintf(stderr, "Failed to read num_owned_files\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < client_data->num_owned_files; i++) {
        OwnedFile *file_data = &client_data->owned_files[i];

        if (fscanf(file, "%s %d", file_data->filename, &file_data->num_chunks) != 2) {
            fprintf(stderr, "Failed to read file name or chunk count\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < file_data->num_chunks; j++) {
            char chunk_buffer[HASH_SIZE + 1] = {0};

            if (fscanf(file, "%32s", chunk_buffer) != 1) {
                fprintf(stderr, "Failed to read chunk %d for file %s\n", j + 1, file_data->filename);
                fclose(file);
                exit(EXIT_FAILURE);
            }

            strncpy(file_data->chunks[j], chunk_buffer, HASH_SIZE);
            file_data->chunks[j][HASH_SIZE] = '\0';
        }
    }


    if (fscanf(file, "%d", &client_data->num_wanted_files) != 1) {
        fprintf(stderr, "Failed to read num_wanted_files\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < client_data->num_wanted_files; i++) {
        if (fscanf(file, "%s", client_data->wanted_files[i]) != 1) {
            fprintf(stderr, "Failed to read wanted file %d\n", i + 1);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
}

