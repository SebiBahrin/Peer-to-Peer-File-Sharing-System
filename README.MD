Peer-to-Peer File Sharing System

This project is an MPI-based peer-to-peer file sharing system that demonstrates how distributed processes can collaborate to share file segments over a network. The system comprises a tracker process that manages file swarms (lists of peers and seeds) and multiple peer processes that both upload and download file segments concurrently.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Overview
The project implements a simplified file sharing protocol with the following components:

1.Tracker:
A central process (with a designated rank) that maintains a database of swarms for every file. A swarm contains the file name, the total number of segments, and a list of client ranks that have either partial or complete copies of the file. The tracker:

Receives file ownership data from all peers.
Provides updated owner lists when requested.
Receives notifications when a file is completely downloaded.
Notifies peers when all downloads are complete (allowing upload threads to shut down).

2.Peer (Client):
Each peer process:

Reads its own input file containing owned and wanted files.
Sends its file ownership information to the tracker.
Initiates two threads:
Download Thread:
Requests missing segments from other peers and writes the complete file to disk once all segments are received.
Upload Thread:
Listens for incoming segment requests from other peers and sends the appropriate segment if available.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Project Flow Summary
Initialization:
MPI is initialized with multi-threading support. The tracker and peers are identified based on their rank.

Registration:
Each peer sends its file ownership details to the tracker. The tracker collects and builds a swarm database.

File Download:
Peers retrieve owner information for wanted files from the tracker. The download thread requests missing segments from other peers and periodically updates the owner list from the tracker.

File Upload:
Simultaneously, the upload thread listens for segment requests from other peers and responds accordingly.

Completion:
Once a file is fully downloaded, the peer notifies the tracker, writes the file to disk, and finally informs the tracker when all files are downloaded. The tracker then signals all peers to shut down upload threads.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Code Structure
Main Function:
Initializes MPI with support for multi-threading. It checks the process rank: if the rank corresponds to the tracker, it runs the tracker function; otherwise, it starts as a peer.

Tracker Functions:

tracker(): Collects initial file data from peers, manages the swarms, handles requests such as:
OWNERS – Responds with the current owner list for a given file.
ADD_PEER – Adds a new peer to a swarm.
FILE_COMPLETE – Updates the swarm when a peer completes downloading a file.
COMPLETE_ALL – Signals all peers when every client has finished downloads.
Peer Functions:

peer():
Reads the input file for the current client, registers file data with the tracker, retrieves owner information for wanted files, and launches separate threads for downloading and uploading.

Download Thread (download_thread_func):
Iterates over the list of files and their segments. For any missing segment (identified by a default zeroed hash), it:

Sends a SEGMENT request to available peers.
Receives the segment or a NACK response.
Updates the segment in the file structure.
Periodically requests an updated owner list from the tracker.
Once a file is completely downloaded, notifies the tracker and writes the file to disk.
Upload Thread (upload_thread_func):
Listens continuously for incoming messages from other peers. When a SEGMENT request is received:

It checks its list of owned files.
Sends the requested segment if found.
Sends a NACK if the segment is not available.
Exits upon receiving a shutdown signal (END).
Helper Function (request_file_owners):
This function contacts the tracker to obtain the swarm information (i.e., the list of peers owning segments) for a given file, and initializes a download structure with the default segment values and owner list.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Compilation and Execution
In the file tema2.c, locate the following lines in the peer function:

    //Pt testare manuala
    construct_input_path(input_path, sizeof(input_path), rank);

    //Pt checker
    //checker_input_path(input_path, sizeof(input_path), rank);

    
To run the program normally:
Make sure that the construct_input_path function is uncommented and the checker_input_path function is commented out.

To run the checker:
Ensure that the checker_input_path function is uncommented and the construct_input_path function is commented out.

Compilation
After setting the appropriate function, compile the project by running:

This will build the executable (e.g., tema2).

Running the Application
Normal Execution:
Run the program with the desired number of processes using mpirun (for example, with 3 processes):
mpirun -np 3 ./tema2
You can replace 3 with the required number of processes (<N>).

Running the Checker:
After adjusting the input path functions for the checker (as described above), run the checker.sh script located in the checker folder to perform the verification.


