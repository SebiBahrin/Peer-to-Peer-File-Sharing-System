#include "tema2.h"

void *download_thread_func(void *arg) {
    DownloadContext *context = (DownloadContext *)arg;
    int segmente_descărcate_global = 0; // global counter for downloaded segments

    // Process each file that needs to be downloaded
    for (int i = 0; i < context->num_files; i++) {
        DownloadFile *file = &context->download_files[i];
        int segmente_descărcate_local = 0; // counter for downloaded segments in current file

        // For each segment in the file
        for (int j = 0; j < file->num_segments; j++) {
            // Check if the segment is missing (all zeros)
            if (strcmp(file->segments[j], "00000000000000000000000000000000") == 0) {
                // Try each available owner for the missing segment
                for (int k = 0; k < file->num_owners[j]; k++) {
                    int target_client = file->owners[j][k];

                    // Debug: print request for segment info
                    printf("Download Thread (Client %d): Requesting segment %d of file %s from client %d.\n", 
                           context->rank, j, file->filename, target_client);

                    // Send segment request to the target client
                    char request_type[] = "SEGMENT";
                    MPI_Send(request_type, sizeof(request_type), MPI_CHAR, target_client, 100, MPI_COMM_WORLD);
                    MPI_Send(file->filename, strlen(file->filename) + 1, MPI_CHAR, target_client, 100, MPI_COMM_WORLD);
                    MPI_Send(&j, 1, MPI_INT, target_client, 100, MPI_COMM_WORLD);

                    // Receive response containing the segment hash or NACK
                    char response[HASH_SIZE + 1];
                    MPI_Recv(response, HASH_SIZE, MPI_CHAR, target_client, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    response[HASH_SIZE] = '\0';

                    // Debug: print response received
                    if (strcmp(response, "NACK") == 0) {
                        printf("Download Thread (Client %d): Received NACK for segment %d from client %d.\n", 
                               context->rank, j, target_client);
                        continue;
                    } else {
                        printf("Download Thread (Client %d): Received segment %d from client %d.\n", 
                               context->rank, j, target_client);
                    }

                    // Save the segment information into the file structure
                    strncpy(file->segments[j], response, HASH_SIZE);
                    file->segments[j][HASH_SIZE] = '\0';

                    segmente_descărcate_local++;
                    segmente_descărcate_global++;

                    // If a multiple of 10 segments have been downloaded, update the peer list
                    if (segmente_descărcate_global % 10 == 0) {
                        printf("Download Thread (Client %d): Updating peer/seed list for file %s.\n", 
                               context->rank, file->filename);
                        Swarm swarm;
                        char request_type[] = "OWNERS";

                        // Request updated owner list from the tracker
                        MPI_Send(request_type, sizeof(request_type), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
                        MPI_Send(file->filename, strlen(file->filename) + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
                        MPI_Recv(&swarm, sizeof(Swarm), MPI_BYTE, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        // Update each segment's owner list with the new swarm data
                        for (int m = 0; m < file->num_segments; m++) {
                            file->num_owners[m] = swarm.num_clients;
                            for (int n = 0; n < swarm.num_clients; n++) {
                                file->owners[m][n] = swarm.clients[n];
                            }
                        }
                    }
                    // Break out of the owner loop once the segment is downloaded
                    break;
                }
            }
        }

        // Check if the file has been completely downloaded
        bool complet = true;
        for (int j = 0; j < file->num_segments; j++) {
            if (strcmp(file->segments[j], "00000000000000000000000000000000") == 0) {
                complet = false;
                break;
            }
        }

        // If complete, notify the tracker and write the file to disk
        if (complet) {
            printf("Download Thread (Client %d): File %s has been completely downloaded.\n", 
                   context->rank, file->filename);

            char complete_msg[] = "FILE_COMPLETE";
            MPI_Send(complete_msg, sizeof(complete_msg), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
            MPI_Send(file->filename, strlen(file->filename) + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
            MPI_Send(&context->rank, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

            // Prepare output filename (prefixing with client number)
            char output_filename[MAX_FILENAME + 20];
            snprintf(output_filename, sizeof(output_filename), "client%d_%s", context->rank, file->filename);

            FILE *output_file = fopen(output_filename, "w");
            if (!output_file) {
                fprintf(stderr, "Download Thread (Client %d): Error creating output file %s\n", 
                        context->rank, output_filename);
                continue;
            }

            // Write all segments into the output file
            for (int j = 0; j < file->num_segments; j++) {
                fprintf(output_file, "%s\n", file->segments[j]);
            }
            fclose(output_file);
        }
    }

    // If all files are complete, notify the tracker that this client finished downloading everything
    char complete_all_msg[] = "COMPLETE_ALL";
    MPI_Send(complete_all_msg, sizeof(complete_all_msg), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
    MPI_Send(&context->rank, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
    printf("Download Thread (Client %d): Finished downloading all files. Tracker notified.\n", context->rank);

    return NULL;
}


void *upload_thread_func(void *arg) {
    UploadContext *context = (UploadContext *)arg;

    // Continuously listen for incoming upload requests
    while (1) {
        char request_type[BUFFER_SIZE];
        MPI_Status status;
        MPI_Recv(request_type, sizeof(request_type), MPI_CHAR, MPI_ANY_SOURCE, 100, MPI_COMM_WORLD, &status);

        // If SHUTDOWN message is received, break the loop and exit the thread
        if (strcmp(request_type, "END") == 0) {
            printf("Upload Thread (Client %d): Received SHUTDOWN message. Exiting upload thread.\n", context->rank);
            break;
        }

        // Handle SEGMENT requests from other clients
        if (strcmp(request_type, "SEGMENT") == 0) {
            char filename[MAX_FILENAME];
            int segment_index;

            // Receive the filename and the requested segment index from the requester
            MPI_Recv(filename, sizeof(filename), MPI_CHAR, status.MPI_SOURCE, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&segment_index, 1, MPI_INT, status.MPI_SOURCE, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("Upload Thread (Client %d): Received request for segment %d of file %s from client %d.\n", 
                   context->rank, segment_index, filename, status.MPI_SOURCE);

            int segment_found = 0;

            // Search through owned files to locate the requested segment
            for (int i = 0; i < context->client_data->num_owned_files; i++) {
                OwnedFile *file = &context->client_data->owned_files[i];
                if (strcmp(file->filename, filename) == 0 && segment_index < file->num_chunks) {
                    // If the segment is found, send it back to the requester
                    MPI_Send(file->chunks[segment_index], HASH_SIZE, MPI_CHAR, status.MPI_SOURCE, 200, MPI_COMM_WORLD);
                    printf("Upload Thread (Client %d): Sent segment %d of file %s to client %d.\n", 
                           context->rank, segment_index, filename, status.MPI_SOURCE);
                    segment_found = 1;
                    break;
                }
            }

            // If the segment is not found, reply with a "NACK" message
            if (!segment_found) {
                char nack_msg[] = "NACK";
                MPI_Send(nack_msg, sizeof(nack_msg), MPI_CHAR, status.MPI_SOURCE, 200, MPI_COMM_WORLD);
                printf("Upload Thread (Client %d): Segment %d of file %s not found. Sent NACK to client %d.\n", 
                       context->rank, segment_index, filename, status.MPI_SOURCE);
            }
        }
    }

    return NULL;
}

// Tracker function: manages the overall swarm, registers peers, and updates owner lists
void tracker(int numtasks, int rank) {
    TrackerDB tracker_db = {0};
    int clients_complete = 0;

    // Receive file ownership info from each client
    for (int i = 1; i < numtasks; i++) {
        ClientData client_data;
        MPI_Recv(&client_data, sizeof(ClientData), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int j = 0; j < client_data.num_owned_files; j++) {
            OwnedFile *owned_file = &client_data.owned_files[j];
            int swarm_index = -1;

            // Check if the file already has an associated swarm
            for (int k = 0; k < tracker_db.num_swarms; k++) {
                if (strcmp(tracker_db.swarms[k].filename, owned_file->filename) == 0) {
                    swarm_index = k;
                    break;
                }
            }

            // If not, create a new swarm for the file
            if (swarm_index == -1) {
                swarm_index = tracker_db.num_swarms++;
                strcpy(tracker_db.swarms[swarm_index].filename, owned_file->filename);
                tracker_db.swarms[swarm_index].num_segments = owned_file->num_chunks;
            }

            // Add the client to the swarm if it is not already present
            int client_already_added = 0;
            for (int c = 0; c < tracker_db.swarms[swarm_index].num_clients; c++) {
                if (tracker_db.swarms[swarm_index].clients[c] == i) {
                    client_already_added = 1;
                    break;
                }
            }
            if (!client_already_added) {
                tracker_db.swarms[swarm_index].clients[tracker_db.swarms[swarm_index].num_clients++] = i;
            }
        }
    }

    // Send ACK message to all clients after processing their file data
    char ack_msg[] = "ACK";
    for (int i = 1; i < numtasks; i++) {
        MPI_Send(ack_msg, sizeof(ack_msg), MPI_CHAR, i, 0, MPI_COMM_WORLD);
    }

    // Main loop to process tracker requests (OWNERS, ADD_PEER, FILE_COMPLETE, COMPLETE_ALL)
    while (1) {
        char request_type[BUFFER_SIZE];
        MPI_Status status;
        MPI_Recv(request_type, sizeof(request_type), MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        if (strcmp(request_type, "OWNERS") == 0) {
            char filename[MAX_FILENAME];
            MPI_Recv(filename, sizeof(filename), MPI_CHAR, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int swarm_index = -1;
            for (int i = 0; i < tracker_db.num_swarms; i++) {
                if (strcmp(tracker_db.swarms[i].filename, filename) == 0) {
                    swarm_index = i;
                    break;
                }
            }
            if (swarm_index != -1) {
                MPI_Send(&tracker_db.swarms[swarm_index], sizeof(Swarm), MPI_BYTE, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            }
        } else if (strcmp(request_type, "ADD_PEER") == 0) {
            char filename[MAX_FILENAME];
            int peer_rank;
            MPI_Recv(filename, sizeof(filename), MPI_CHAR, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&peer_rank, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int swarm_index = -1;
            for (int i = 0; i < tracker_db.num_swarms; i++) {
                if (strcmp(tracker_db.swarms[i].filename, filename) == 0) {
                    swarm_index = i;
                    break;
                }
            }
            if (swarm_index != -1) {
                int already_in_list = 0;
                for (int c = 0; c < tracker_db.swarms[swarm_index].num_clients; c++) {
                    if (tracker_db.swarms[swarm_index].clients[c] == peer_rank) {
                        already_in_list = 1;
                        break;
                    }
                }
                if (!already_in_list) {
                    tracker_db.swarms[swarm_index].clients[tracker_db.swarms[swarm_index].num_clients++] = peer_rank;
                    // Debug: print added peer info
                    printf("Tracker: Added client %d as a peer for file %s.\n", peer_rank, filename);
                }
            }
        } else if (strcmp(request_type, "FILE_COMPLETE") == 0) {
            char filename[MAX_FILENAME];
            int client_rank;
            MPI_Recv(filename, sizeof(filename), MPI_CHAR, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&client_rank, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int swarm_index = -1;
            for (int i = 0; i < tracker_db.num_swarms; i++) {
                if (strcmp(tracker_db.swarms[i].filename, filename) == 0) {
                    swarm_index = i;
                    break;
                }
            }
            if (swarm_index != -1) {
                int already_in_list = 0;
                for (int c = 0; c < tracker_db.swarms[swarm_index].num_clients; c++) {
                    if (tracker_db.swarms[swarm_index].clients[c] == client_rank) {
                        already_in_list = 1;
                        break;
                    }
                }
                if (!already_in_list) {
                    tracker_db.swarms[swarm_index].clients[tracker_db.swarms[swarm_index].num_clients++] = client_rank;
                    printf("Tracker: Client %d added as seed for file %s.\n", client_rank, filename);
                }
            }
        } else if (strcmp(request_type, "COMPLETE_ALL") == 0) {
            int client_rank;
            MPI_Recv(&client_rank, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            clients_complete++;
            printf("Tracker: Client %d finished downloading all files.\n", client_rank);
            if (clients_complete == numtasks - 1) {
                // Notify all clients to shutdown their upload threads.
                char ack_msg[] = "END";
                for (int i = 1; i < numtasks; i++) {
                    MPI_Send(ack_msg, sizeof(ack_msg), MPI_CHAR, i, 100, MPI_COMM_WORLD);
                }
                break;
            }
        }
    }
}

// Peer function: initializes client data, starts download and upload threads,
// and communicates with the tracker to get file ownership information.
void peer(int numtasks, int rank) {
    ClientData client_data;
    char input_path[MAX_FILENAME];

    //Pt testare manuala
    construct_input_path(input_path, sizeof(input_path), rank);

    //Pt checker
    //checker_input_path(input_path, sizeof(input_path), rank);

    // Read input file containing file information for this client
    read_input_file(input_path, &client_data);

    // Send owned file information to the tracker
    MPI_Send(&client_data, sizeof(ClientData), MPI_BYTE, TRACKER_RANK, 0, MPI_COMM_WORLD);

    // Wait for ACK from tracker confirming registration
    char ack_msg[BUFFER_SIZE];
    MPI_Recv(ack_msg, sizeof(ack_msg), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (strcmp(ack_msg, "ACK") == 0) {
        printf("Client %d: Received ACK from tracker!\n", rank);
    } else {
        fprintf(stderr, "Client %d: Error receiving ACK from tracker!\n", rank);
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the files to be downloaded
    DownloadFile *download_files = (DownloadFile *)malloc(client_data.num_wanted_files * sizeof(DownloadFile));
    if (!download_files) {
        fprintf(stderr, "Error allocating memory for download_files\n");
        exit(EXIT_FAILURE);
    }

    // Process each wanted file and retrieve its owner information from the tracker
    for (int i = 0; i < client_data.num_wanted_files; i++) {
        DownloadFile *file = request_file_owners(client_data.wanted_files[i], rank);
        if (file) {
            memcpy(&download_files[i], file, sizeof(DownloadFile));
            free(file);
        }
    }

    // Create context for the download thread
    DownloadContext download_context = {
        .rank = rank,
        .download_files = download_files,
        .num_files = client_data.num_wanted_files,
    };

    // Create context for the upload thread
    UploadContext *upload_context = (UploadContext *)malloc(sizeof(UploadContext));
    if (!upload_context) {
        fprintf(stderr, "Error allocating memory for upload_context\n");
        exit(EXIT_FAILURE);
    }
    upload_context->rank = rank;
    upload_context->client_data = &client_data;

    // Create both threads: one for downloading segments and one for handling upload requests
    pthread_t download_thread, upload_thread;
    int r;

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *)&download_context);
    if (r) {
        printf("Error creating download thread\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *)upload_context);
    if (r) {
        printf("Error creating upload thread\n");
        exit(-1);
    }

    // Wait for both threads to finish
    r = pthread_join(download_thread, NULL);
    if (r) {
        printf("Error joining download thread\n");
        exit(-1);
    }
    r = pthread_join(upload_thread, NULL);
    if (r) {
        printf("Error joining upload thread\n");
        exit(-1);
    }

    free(download_files);
    free(upload_context);
}

// Function to request file ownership details (swarm information) from the tracker
DownloadFile *request_file_owners(const char *filename, int rank) {
    char request_type[] = "OWNERS";
    MPI_Send(request_type, sizeof(request_type), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
    MPI_Send(filename, strlen(filename) + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);

    Swarm swarm;
    MPI_Recv(&swarm, sizeof(Swarm), MPI_BYTE, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Initialize the DownloadFile structure with swarm info
    DownloadFile *download_file = (DownloadFile *)malloc(sizeof(DownloadFile));
    strcpy(download_file->filename, swarm.filename);
    download_file->num_segments = swarm.num_segments;

    // Initialize each segment with zeros and copy the owner list from the swarm
    for (int i = 0; i < swarm.num_segments; i++) {
        memset(download_file->segments[i], '0', HASH_SIZE);
        download_file->segments[i][HASH_SIZE] = '\0';
        download_file->num_owners[i] = swarm.num_clients;
        for (int j = 0; j < swarm.num_clients; j++) {
            download_file->owners[i][j] = swarm.clients[j];
        }
    }

    return download_file;
}

int main (int argc, char *argv[]) {
    int numtasks, rank;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI does not support multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }
    MPI_Finalize();
}
