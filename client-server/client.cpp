// HOW TO COMPILE THIS
//      g++ -std=c++11 client.cpp -o client -lsqlite3 -pthread

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sqlite3.h>
using namespace std;

int PORT;
char utiliz_actual[100];

char mesaj_global[256];
int contor;

bool shouldStop = false;

void comunicareWrite(char *buffer, int socketfd)
{
    int lung = strlen(buffer);
    if (write(socketfd, &lung, sizeof(int)) < 0)
    {
        perror("eroare la scrierea spre server");
        exit(1);
    }
    int totalScris = 0;
    while (totalScris < lung)
    {
        int scris = write(socketfd, buffer + totalScris, lung - totalScris);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        totalScris += scris;
    }
}

void comunicareRead(int socketfd)
{
    int lung_primita;

    if (read(socketfd, &lung_primita, sizeof(int)) <= 0)
    {
        perror("eroare la citirea lungimii mesajului de la server");
        exit(1);
    }

    char raspuns_server[lung_primita + 1];

    if (read(socketfd, raspuns_server, lung_primita * sizeof(char)) <= 0)
    {
        perror("eroare la citirea mesajului de la server");
        exit(1);
    }

    raspuns_server[lung_primita] = '\0';

    {
        cout << "Raspunsul de la server este: " << raspuns_server << endl;
    }

    if (strncmp(raspuns_server, "Te-ai logat cu succes, ", strlen("Te-ai logat cu succes, ")) == 0)
    {
        strcpy(utiliz_actual, raspuns_server + strlen("Te-ai logat cu succes, "));
    }
    raspuns_server[0] = '\0';
}

void citesteBazaDate(sqlite3 *db)
{

    while (!shouldStop)
    {
        const char *sql = "SELECT nume,idTrenAfectat,intarzie FROM clientiAfectati";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Eroare la pregatirea interogarii: " << sqlite3_errmsg(db) << endl;
            exit(1);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *nume = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *trenAfectat = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            int intarzie = sqlite3_column_int(stmt, 2);

            if (strcmp(nume, utiliz_actual) == 0)
            {
                contor++;
                snprintf(mesaj_global, sizeof(mesaj_global), " \n! ! ! MESAJ PENTRU %s, TRENUL CU ID-UL %s INTARZIE: %d minute.\n", nume, trenAfectat, intarzie);
            }
        }

        char deleteQuery[256];
        snprintf(deleteQuery, sizeof(deleteQuery), "DELETE FROM clientiAfectati WHERE nume='%s'", utiliz_actual);

        if (sqlite3_exec(db, deleteQuery, nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            cerr << "Eroare la stergerea inregistrarii: " << sqlite3_errmsg(db) << endl;
            exit(1);
        }

        sqlite3_finalize(stmt);

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

int main(int argc, char *argv[])
{
    int socketfd;
    sockaddr_in serverAddr;

    if (argc != 3)
    {
        cout << "Sintaxa este : " << argv[0] << " <adresa_server> <port>" << endl;
        return -1;
    }

    PORT = atoi(argv[2]);

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        perror("Eroare la socket().\n");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(PORT);

    int conn = connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
    if (conn < 0)
    {
        perror("[client]Eroare la connect().\n");
        exit(1);
    }

    cout << "[client]Conectat la server" << endl;

    char buffer[2056];

    sqlite3 *clientiDB;
    int retur = sqlite3_open("client.db", &clientiDB);
    if (retur != SQLITE_OK)
    {
        cerr << "Eroare la deschiderea bazei de date: " << sqlite3_errmsg(clientiDB) << endl;
        exit(1);
    }

    std::thread citireBDThread([&]() {
        citesteBazaDate(clientiDB);
    });

    int contor_main = 0;

    cout << "[client] Va rugam sa va inregistrati/logati pentru a folosi comenzile.\n";

    while (1)
    {

        if (mesaj_global[0] != '\0' && contor != contor_main)
        {
            cout << mesaj_global << endl;
            contor_main++;
        }

        cout << "[client] Introduceti un mesaj : ";

        cin.getline(buffer, 1024);

        int len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }

        char raspuns[2056];
        raspuns[0] = '\0';

        if (strcmp(buffer, "quit") == 0)
        {
            comunicareWrite(buffer, socketfd);
            cout << "Te-ai deconectat" << endl;
            break;
        }

        else if (strcmp(buffer, "help") == 0)
        {
            cout << "------Comenzi acceptate de server------" << endl;
            cout << "---register <username> <statie>---" << endl;
            cout << "---login <username>---" << endl;
            cout << "---help---" << endl;
            cout << "---info---" << endl;
            cout << "---mersul trenurilor---" << endl;
            cout << "---plecari <statie>---" << endl;
            cout << "---sosiri <statie>---" << endl;
            cout << "---plecari urmatoarea ora---" << endl;
            cout << "---sosiri urmatoarea ora---" << endl;
            cout << "---estimare sosire---" << endl;
            cout << "---intarziere <id_tren> <cate_minute>---" << endl;
            cout << "---quit---" << endl;
        }
        else if (strcmp(buffer, "info") == 0)
        {
            cout << "------Instructiuni de folosire------" << endl;
            cout << "1. Utilizatorul trebuie sa se inregistreze folosind comanda <register>." << endl;
            cout << "2. Odata inregistrat utilizatorul se logheaza folosind comanda <login>." << endl;
            cout << "3. Utilizatorul cere informatii despre <mersul trenurilor> , <plecari> , <sosiri>." << endl;
        }
        else if (strcmp(buffer, "estimare sosire") == 0)
        {
            if (utiliz_actual[0] == 0)
                cout << "Trebuie sa te autentifici mai intai." << endl;
            else
            {

                int lung = strlen(buffer);

                if (write(socketfd, &lung, sizeof(int)) < 0)
                {
                    perror("eroare la scrierea spre server");
                    exit(1);
                }

                int totalScris = 0;
                while (totalScris < lung)
                {
                    int scris = write(socketfd, buffer + totalScris, lung - totalScris);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    totalScris += scris;
                }

                int dim_primita;

                if (read(socketfd, &dim_primita, sizeof(int)) <= 0)
                {
                    perror("eroare la citirea dimens de la server");
                    exit(1);
                }

                char raspuns_primit[dim_primita + 1];

                if (read(socketfd, raspuns_primit, dim_primita * sizeof(char)) <= 0)
                {
                    perror("eroare la citirea mesajului de la server");
                    exit(1);
                }

                raspuns_primit[dim_primita] = '\0';

                cout << raspuns_primit;

                char buff[1024];

                cin.getline(buff, 1024);

                int buff_len = strlen(buff);

                if (write(socketfd, &buff_len, sizeof(int)) < 0)
                {
                    perror("eroare (1)");
                    exit(1);
                }

                int total = 0;
                while (total < buff_len)
                {
                    int scris = write(socketfd, buff + total, buff_len - total);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    total += scris;
                }
                while (1)
                {
                    int lung_primita;

                    if (read(socketfd, &lung_primita, sizeof(int)) <= 0)
                    {
                        perror("eroare la citirea dimens de la server");
                        exit(1);
                    }

                    char raspuns_server[lung_primita + 1];

                    if (read(socketfd, raspuns_server, lung_primita * sizeof(char)) <= 0)
                    {
                        perror("eroare la citirea mesajului de la server");
                        exit(1);
                    }

                    raspuns_server[lung_primita] = '\0';

                    if (strcmp(raspuns_server, "end.") == 0)
                    {
                        break;
                    }

                    cout << raspuns_server << endl;
                }
            }
        }
        else if (strcmp(buffer, "plecari urmatoarea ora") == 0 || strcmp(buffer, "sosiri urmatoarea ora") == 0 || strcmp(buffer, "sosiri ora urmatoare") == 0 || strcmp(buffer, "plecari ora urmatoare") == 0)
        {

            if (utiliz_actual[0] == 0)
            {
                cout << "Trebuie sa te autentifici mai intai." << endl;
            }
            else
            {
                int lung = strlen(buffer);

                if (write(socketfd, &lung, sizeof(int)) < 0)
                {
                    perror("eroare la scrierea spre server");
                    exit(1);
                }

                int totalScris = 0;
                while (totalScris < lung)
                {
                    int scris = write(socketfd, buffer + totalScris, lung - totalScris);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    totalScris += scris;
                }

                int dim_primita;

                if (read(socketfd, &dim_primita, sizeof(int)) <= 0)
                {
                    perror("eroare la citirea dimens de la server");
                    exit(1);
                }

                char raspuns_primit[dim_primita + 1];

                if (read(socketfd, raspuns_primit, dim_primita * sizeof(char)) <= 0)
                {
                    perror("eroare la citirea mesajului de la server");
                    exit(1);
                }

                raspuns_primit[dim_primita] = '\0';

                cout << raspuns_primit;

                char buff[1024];

                cin.getline(buff, 1024);

                int buff_len = strlen(buff);

                if (write(socketfd, &buff_len, sizeof(int)) < 0)
                {
                    perror("eroare (1)");
                    exit(1);
                }

                int total = 0;
                while (total < buff_len)
                {
                    int scris = write(socketfd, buff + total, buff_len - total);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    total += scris;
                }
                while (1)
                {
                    int lung_primita;

                    if (read(socketfd, &lung_primita, sizeof(int)) <= 0)
                    {
                        perror("eroare la citirea dimens de la server");
                        exit(1);
                    }

                    char raspuns_server[lung_primita + 1];

                    if (read(socketfd, raspuns_server, lung_primita * sizeof(char)) <= 0)
                    {
                        perror("eroare la citirea mesajului de la server");
                        exit(1);
                    }

                    raspuns_server[lung_primita] = '\0';

                    if (strcmp(raspuns_server, "end.") == 0)
                    {
                        break;
                    }

                    cout << raspuns_server << endl;
                }
            }
        }
        else if (((strncmp(buffer, "plecari", 7) == 0) || (strncmp(buffer, "sosiri", 6) == 0)) && (strlen(buffer) > 10))
        {

            if (utiliz_actual[0] == 0)
            {
                cout << "Trebuie sa te autentifici mai intai." << endl;
            }
            else
            {

                int lung = strlen(buffer);

                if (write(socketfd, &lung, sizeof(int)) < 0)
                {
                    perror("eroare la scrierea spre server");
                    exit(1);
                }

                int totalScris = 0;
                while (totalScris < lung)
                {
                    int scris = write(socketfd, buffer + totalScris, lung - totalScris);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    totalScris += scris;
                }

                int dim_primita;

                if (read(socketfd, &dim_primita, sizeof(int)) <= 0)
                {
                    perror("eroare la citirea dimens de la server");
                    exit(1);
                }

                char raspuns_primit[dim_primita + 1];

                if (read(socketfd, raspuns_primit, dim_primita * sizeof(char)) <= 0)
                {
                    perror("eroare la citirea mesajului de la server");
                    exit(1);
                }

                raspuns_primit[dim_primita] = '\0';

                cout << raspuns_primit;

                char buff[1024];

                cin.getline(buff, 1024);

                int buff_len = strlen(buff);

                if (write(socketfd, &buff_len, sizeof(int)) < 0)
                {
                    perror("eroare (1)");
                    exit(1);
                }

                int total = 0;
                while (total < buff_len)
                {
                    int scris = write(socketfd, buff + total, buff_len - total);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    total += scris;
                }

                while (1)
                {
                    int lung_primita;

                    if (read(socketfd, &lung_primita, sizeof(int)) <= 0)
                    {
                        perror("eroare la citirea dimens de la server");
                        exit(1);
                    }

                    char raspuns_server[lung_primita + 1];

                    if (read(socketfd, raspuns_server, lung_primita * sizeof(char)) <= 0)
                    {
                        perror("eroare la citirea mesajului de la server");
                        exit(1);
                    }

                    raspuns_server[lung_primita] = '\0';

                    if (strcmp(raspuns_server, "end.") == 0)
                    {
                        break;
                    }

                    cout << raspuns_server << endl;
                }
            }
        }
        else if (strcmp(buffer, "mersul trenurilor") == 0)
        {
            if (utiliz_actual[0] == 0)
            {
                cout << "Trebuie sa te autentifici mai intai." << endl;
            }
            else
            {

                int lung = strlen(buffer);

                if (write(socketfd, &lung, sizeof(int)) < 0)
                {
                    perror("eroare la scrierea spre server");
                    exit(1);
                }

                int totalScris = 0;
                while (totalScris < lung)
                {
                    int scris = write(socketfd, buffer + totalScris, lung - totalScris);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    totalScris += scris;
                }

                int dim_primita;

                if (read(socketfd, &dim_primita, sizeof(int)) <= 0)
                {
                    perror("eroare la citirea dimens de la server");
                    exit(1);
                }

                char raspuns_primit[dim_primita + 1];

                if (read(socketfd, raspuns_primit, dim_primita * sizeof(char)) <= 0)
                {
                    perror("eroare la citirea mesajului de la server");
                    exit(1);
                }

                raspuns_primit[dim_primita] = '\0';

                cout << raspuns_primit;

                char buff[1024];

                cin.getline(buff, 1024);

                int buff_len = strlen(buff);

                if (write(socketfd, &buff_len, sizeof(int)) < 0)
                {
                    perror("eroare (1)");
                    exit(1);
                }

                int total = 0;
                while (total < buff_len)
                {
                    int scris = write(socketfd, buff + total, buff_len - total);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    total += scris;
                }

                while (1)
                {
                    int lung_primita;

                    if (read(socketfd, &lung_primita, sizeof(int)) <= 0)
                    {
                        perror("eroare la citirea dimens de la server");
                        exit(1);
                    }

                    char raspuns_server[lung_primita + 1];

                    if (read(socketfd, raspuns_server, lung_primita * sizeof(char)) <= 0)
                    {
                        perror("eroare la citirea mesajului de la server");
                        exit(1);
                    }

                    raspuns_server[lung_primita] = '\0';

                    if (strcmp(raspuns_server, "end.") == 0)
                    {
                        break;
                    }

                    cout << raspuns_server << endl;
                    raspuns_server[0] = '\0';
                }
            }
        }
        else
        {

            comunicareWrite(buffer, socketfd);
            int lung_primita;

            if (read(socketfd, &lung_primita, sizeof(int)) <= 0)
            {
                perror("eroare la citirea lungimii mesajului de la server");
                exit(1);
            }

            char raspuns_server[lung_primita + 1];

            if (read(socketfd, raspuns_server, lung_primita * sizeof(char)) <= 0)
            {
                perror("eroare la citirea mesajului de la server");
                exit(1);
            }

            raspuns_server[lung_primita] = '\0';

            cout << raspuns_server << endl;

            if (strncmp(raspuns_server, "[server] Te-ai logat cu succes, ", strlen("[server] Te-ai logat cu succes, ")) == 0)
            {
                strcpy(utiliz_actual, raspuns_server + strlen("[server] Te-ai logat cu succes, "));
            }
            raspuns_server[0] = '\0';
        }
    }

    shouldStop = true;

    citireBDThread.join();
    sqlite3_close(clientiDB);
    return 0;
}