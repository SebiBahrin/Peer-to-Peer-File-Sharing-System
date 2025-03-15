# Peer-to-Peer File Sharing System

## Overview
This project is an MPI-based peer-to-peer file sharing system that demonstrates how distributed processes collaborate to share file segments over a network. The system consists of a tracker process that manages file swarms and multiple peer processes that handle concurrent file uploads and downloads.

## Components

### 1. Tracker
A central process (with a designated rank) that maintains a database of swarms for every file. A swarm includes:
- File name
- Total number of segments
- List of client ranks that have partial or complete copies of the file

The tracker:
- Receives file ownership data from all peers
- Provides updated owner lists when requested
- Receives notifications when a file is completely downloaded
- Notifies peers when all downloads are complete (allowing upload threads to shut down)

### 2. Peer (Client)
Each peer process:
- Reads its own input file containing owned and wanted files
- Sends its file ownership information to the tracker
- Initiates two threads:
  - **Download Thread:** Requests missing segments from other peers and writes the complete file to disk once all segments are received
  - **Upload Thread:** Listens for incoming segment requests from other peers and sends the appropriate segment if available

## Project Flow Summary

### Initialization
- MPI is initialized with multi-threading support.
- The tracker and peers are identified based on their rank.

### Registration
- Each peer sends its file ownership details to the tracker.
- The tracker collects and builds a swarm database.

### File Download
- Peers retrieve owner information for wanted files from the tracker.
- The download thread requests missing segments from other peers and periodically updates the owner list from the tracker.

### File Upload
- The upload thread listens for segment requests from other peers and responds accordingly.

### Completion
- Once a file is fully downloaded, the peer notifies the tracker and writes the file to disk.
- The peer informs the tracker when all files are downloaded.
- The tracker signals all peers to shut down upload threads.

## Code Structure

### Main Function
- Initializes MPI with support for multi-threading.
- Determines process rank:
  - If the rank corresponds to the tracker, it runs the tracker function.
  - Otherwise, it starts as a peer.

### Tracker Functions
- **tracker():** Collects initial file data from peers, manages swarms, and handles requests:
  - `OWNERS` – Responds with the current owner list for a given file.
  - `ADD_PEER` – Adds a new peer to a swarm.
  - `FILE_COMPLETE` – Updates the swarm when a peer completes downloading a file.
  - `COMPLETE_ALL` – Signals all peers when all clients have finished downloads.

### Peer Functions
- **peer():** Reads the input file for the current client, registers file data with the tracker, retrieves owner information for wanted files, and launches separate threads for downloading and uploading.

### Download Thread (download_thread_func)
- Iterates over the list of files and their segments.
- For missing segments (identified by a default zeroed hash):
  - Sends a `SEGMENT` request to available peers.
  - Receives the segment or a `NACK` response.
  - Updates the segment in the file structure.
  - Periodically requests an updated owner list from the tracker.
- Once a file is completely downloaded:
  - Notifies the tracker.
  - Writes the file to disk.

### Upload Thread (upload_thread_func)
- Listens continuously for incoming messages from other peers.
- When a `SEGMENT` request is received:
  - Checks its list of owned files.
  - Sends the requested segment if found.
  - Sends a `NACK` if the segment is not available.
- Exits upon receiving a shutdown signal (`END`).

### Helper Function (request_file_owners)
- Contacts the tracker to obtain swarm information (list of peers owning segments for a given file).
- Initializes a download structure with the default segment values and owner list.

## Compilation and Execution

### Code Modification for Execution
In the file **tema2.c**, locate the following lines in the `peer` function:

```c
// For manual testing
construct_input_path(input_path, sizeof(input_path), rank);

// For checker
//checker_input_path(input_path, sizeof(input_path), rank);
```

- **To run the program normally:**
  - Ensure that `construct_input_path` is **uncommented**.
  - Ensure that `checker_input_path` is **commented out**.

- **To run the checker:**
  - Ensure that `checker_input_path` is **uncommented**.
  - Ensure that `construct_input_path` is **commented out**.

### Running the Application

#### Normal Execution
To run the program with the desired number of processes using `mpirun` (e.g., with 3 processes):
```sh
mpirun -np 3 ./tema2
```
Replace `3` with the required number of processes (`<N>`).

#### Running the Checker
After adjusting the input path functions as described above, run the checker script:
```sh
cd checker
./checker.sh
```

