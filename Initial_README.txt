Am inceput aceasta tema ocupandu-ma in peers de partea de citire a datelor si trimiterea lor catre tracker. In prima faza, fiecare client trimite catre tracker 
structura ClientData, care contine informatii atat despre fisierele pe care le are, cat si cele pe care le doreste, iar pe baza ei trackerul face TRACKERDB,
unde salveaza lista tuturor swarm-urilor.

Dupa ce se primeste ACK, fiecare client primeste de la tracker informatii despre fisierele pe care doreste sa le descarce prin intermediul request_file_owners. Tot
in aceasta functie fiecare client isi formeaza o structura proprie, cu informatii despre fisierele pe care trebuie sa le descarce. Tot acum se genereaza si hashurile
pentru fiecare segment, initial umplute cu 0-uri, pe care le va primi si ulterior inlocui cu hashurile reale detinute de alti clienti.
Fișier de descărcat: file1.txt
  Segment 1: 00000000000000000000000000000000
  Segment 2: 00000000000000000000000000000000

Toate aceste informatii sunt apoi furnizate catre threaduri, prin intermediul altor 2 structuri DownloadContext si UploadContext, prin care acestea primesc toate 
informatiile necesare pentru a isi face treaba. Pe baza informatiilor despre peers si seeders, fiecare client incepe sa trimita mesaje altor clienti pentru a isi 
descarca segmentele de care are nevoie.


Structuri de Date Importante

1. OwnedFile

Aceasta structură reține informații despre fișierele deținute de un client.

filename - Numele fișierului.

num_chunks - Numărul de segmente (chunk-uri) ale fișierului.

chunks - Hash-urile fiecărui segment.

2. ClientData

Structura care reține informații despre fișierele deținute și dorite de un client.

num_owned_files - Numărul de fișiere deținute de client.

owned_files - Lista fișierelor deținute.

num_wanted_files - Numărul de fișiere pe care clientul dorește să le descarce.

wanted_files - Lista fișierelor dorite.

3. Swarm

Aceasta structură reprezintă un grup de clienți care au segmente ale aceluiași fișier.

filename - Numele fișierului.

num_segments - Numărul de segmente ale fișierului.

hashes - Hash-urile fiecărui segment.

clients - Lista clienților care au segmente din fișier.

4. DownloadFile

Structura care reține informații despre fișierele care urmează să fie descărcate.

filename - Numele fișierului.

num_segments - Numărul de segmente.

segments - Hash-urile segmentelor descărcate.

owners - Lista clienților care dețin segmentele respective.

5. DownloadContext și UploadContext

Aceste structuri sunt folosite pentru a gestiona thread-urile de download și upload.

rank - Identificatorul clientului.

download_files - Lista fișierelor de descărcat.

client_data - Datele despre client (pentru upload).

Funcții Principale

1. void *download_thread_func(void *arg)

Aceasta este funcția executată de thread-ul de download.

Funcționalitate:

Iterează prin fișierele dorite de client și încearcă să descarce segmentele lipsă de la alți clienți.

Trimite cereri de segmente către clienții care dețin respectivele segmente folosind MPI_Send și primește răspunsuri prin MPI_Recv.

Dacă un segment este descărcat cu succes, acesta este salvat în structura DownloadFile.

Dacă toate fișierele sunt descărcate complet, trimite un mesaj către tracker.

2. void *upload_thread_func(void *arg)

Funcția executată de thread-ul de upload.

Funcționalitate:

Ascultă cereri de segmente primite de la alți clienți.

Verifică dacă clientul deține segmentul cerut și îl trimite solicitantului.

Dacă segmentul nu este găsit, trimite un mesaj "NACK".

3. void tracker(int numtasks, int rank)

Funcția care implementează logica tracker-ului. Tracker-ul coordonează informațiile despre fișierele disponibile și despre clienții care dețin segmentele acestora.

Funcționalitate:

Primește informații de la clienți despre fișierele deținute.

Menține o bază de date (TrackerDB) despre swarm-uri.

Răspunde la cererile clienților privind lista de proprietari pentru fișiere.

Monitorizează progresul descărcărilor și încheie execuția când toți clienții au terminat descărcările.

4. void peer(int numtasks, int rank)

Funcția principală pentru clienți (peers).

Funcționalitate:

Citește fișierul de intrare pentru a determina fișierele deținute și dorite.

Trimite informațiile către tracker.

Creează thread-uri pentru descărcare și încărcare.

Așteaptă finalizarea thread-urilor.

5. DownloadFile *request_file_owners(const char *filename, int rank)

Funcția folosită de clienți pentru a solicita tracker-ului lista de proprietari ai unui fișier dorit.

Funcționalitate:

Trimite o cerere de tip "OWNERS" către tracker.

Primește structura Swarm de la tracker, care conține informațiile despre fișier și lista de clienți.

Inițializează structura DownloadFile pentru fișierul respectiv.

6. void construct_input_path(char *buffer, size_t buffer_size, int rank)

Funcție auxiliară pentru generarea căii către fișierul de intrare.

7. void read_input_file(const char *filename, ClientData *client_data)

Funcție pentru citirea fișierului de intrare.

Funcționalitate:

Citește informațiile despre fișierele deținute și dorite de client.
Structuri de Date Importante

1. OwnedFile

Aceasta structură reține informații despre fișierele deținute de un client.

filename - Numele fișierului.

num_chunks - Numărul de segmente (chunk-uri) ale fișierului.

chunks - Hash-urile fiecărui segment.

2. ClientData

Structura care reține informații despre fișierele deținute și dorite de un client.

num_owned_files - Numărul de fișiere deținute de client.

owned_files - Lista fișierelor deținute.

num_wanted_files - Numărul de fișiere pe care clientul dorește să le descarce.

wanted_files - Lista fișierelor dorite.

3. Swarm

Aceasta structură reprezintă un grup de clienți care au segmente ale aceluiași fișier.

filename - Numele fișierului.

num_segments - Numărul de segmente ale fișierului.

hashes - Hash-urile fiecărui segment.

clients - Lista clienților care au segmente din fișier.

4. DownloadFile

Structura care reține informații despre fișierele care urmează să fie descărcate.

filename - Numele fișierului.

num_segments - Numărul de segmente.

segments - Hash-urile segmentelor descărcate.

owners - Lista clienților care dețin segmentele respective.

5. DownloadContext și UploadContext

Aceste structuri sunt folosite pentru a gestiona thread-urile de download și upload.

rank - Identificatorul clientului.

download_files - Lista fișierelor de descărcat.

client_data - Datele despre client (pentru upload).

Funcții Principale

1. void *download_thread_func(void *arg)

Aceasta este funcția executată de thread-ul de download.

Funcționalitate:

Iterează prin fișierele dorite de client și încearcă să descarce segmentele lipsă de la alți clienți.

Trimite cereri de segmente către clienții care dețin respectivele segmente folosind MPI_Send și primește răspunsuri prin MPI_Recv.

Dacă un segment este descărcat cu succes, acesta este salvat în structura DownloadFile.

Dacă toate fișierele sunt descărcate complet, trimite un mesaj către tracker.

2. void *upload_thread_func(void *arg)

Funcția executată de thread-ul de upload.

Funcționalitate:

Ascultă cereri de segmente primite de la alți clienți.

Verifică dacă clientul deține segmentul cerut și îl trimite solicitantului.

Dacă segmentul nu este găsit, trimite un mesaj "NACK".

3. void tracker(int numtasks, int rank)

Funcția care implementează logica tracker-ului. Tracker-ul coordonează informațiile despre fișierele disponibile și despre clienții care dețin segmentele acestora.

Funcționalitate:

Primește informații de la clienți despre fișierele deținute.

Menține o bază de date (TrackerDB) despre swarm-uri.

Răspunde la cererile clienților privind lista de proprietari pentru fișiere.

Monitorizează progresul descărcărilor și încheie execuția când toți clienții au terminat descărcările.

4. void peer(int numtasks, int rank)

Funcția principală pentru clienți (peers).

Funcționalitate:

Citește fișierul de intrare pentru a determina fișierele deținute și dorite.

Trimite informațiile către tracker.

Creează thread-uri pentru descărcare și încărcare.

Așteaptă finalizarea thread-urilor.

5. DownloadFile *request_file_owners(const char *filename, int rank)

Funcția folosită de clienți pentru a solicita tracker-ului lista de proprietari ai unui fișier dorit.

Funcționalitate:

Trimite o cerere de tip "OWNERS" către tracker.

Primește structura Swarm de la tracker, care conține informațiile despre fișier și lista de clienți.

Inițializează structura DownloadFile pentru fișierul respectiv.

6. void construct_input_path(char *buffer, size_t buffer_size, int rank)

Funcție auxiliară pentru generarea căii către fișierul de intrare.

7. void read_input_file(const char *filename, ClientData *client_data)

Funcție pentru citirea fișierului de intrare.

Funcționalitate:

Citește informațiile despre fișierele deținute și dorite de client.
