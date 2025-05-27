#include <iostream>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <signal.h>
#define PORT 1234
using namespace std;

typedef struct thData
{
    int idThread;
    int cl;
    sqlite3 *db;
} thData;

void en_to_ro(char *sir)
{
    if (strcmp(sir, "Monday") == 0)
        strcpy(sir, "Luni");
    if (strcmp(sir, "Tuesday") == 0)
        strcpy(sir, "Marti");
    if (strcmp(sir, "Wednesday") == 0)
        strcpy(sir, "Miercuri");
    if (strcmp(sir, "Thursday") == 0)
        strcpy(sir, "Joi");
    if (strcmp(sir, "Friday") == 0)
        strcpy(sir, "Vineri");
    if (strcmp(sir, "Saturday") == 0)
        strcpy(sir, "Sambata");
    if (strcmp(sir, "Sunday") == 0)
        strcpy(sir, "Duminica");
}

void raspuns_server(char *arg, sqlite3 *db, int cl);

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

static void *welcome_clients(void *arg)
{
    thData tdL;
    tdL = *((thData *)arg);
    cout << "[thread]- " << tdL.idThread << " - Asteptam mesajul..." << endl;
    fflush(stdout);
    pthread_detach(pthread_self());

    while (1)
    {
        int lung_primita;

        if (read(tdL.cl, &lung_primita, sizeof(int)) <= 0)
        {
            cout << "[Thread " << tdL.idThread << "]" << endl;
            perror("Eroare la citirea lungimii de la client.\n");
            break;
        }

        char buffer[lung_primita + 1];
        memset(buffer, 0, sizeof(buffer));

        if (read(tdL.cl, buffer, lung_primita * sizeof(char)) <= 0)
        {
            cout << "[Thread " << tdL.idThread << "]" << endl;
            perror("Eroare la citirea de la client.\n");
            break;
        }

        buffer[strlen(buffer)] = '\0';

        cout << "[Thread " << tdL.idThread << "] Mesajul a fost primit...<" << buffer << ">" << endl;
        fflush(stdout);

        if (strcmp(buffer, "quit") == 0)
        {
            cout << "[Thread " << tdL.idThread << "] Clientul a inchis conexiunea." << endl;
            break;
        }

        char raspuns[2056];
        raspuns[0] = '\0';
        strcpy(raspuns, buffer);

        raspuns_server(raspuns, tdL.db, tdL.cl);

        cout << "[Thread " << tdL.idThread << "] Trimitem mesajul inapoi..." << raspuns << endl;
        fflush(stdout);
    }

    close(tdL.cl);
    free(arg);
    return NULL;
}

void user_register(char *arg, sqlite3 *db, int cl)
{
    char comanda[10], username[50], statie[50];
    int ret_impartire = sscanf(arg, "%s %s %s", comanda, username, statie);
    if (ret_impartire != 3)
    {
        char errMsg[] = "[server] Comanda de register este urmatoarea : register <usname> <statie> .";
        int lung = strlen(errMsg);

        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la scrierea catre client (user_register)");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, errMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }

        return;
    }

    if ((strcmp(comanda, "register") == 0) && (strlen(username) >= 1) && (strlen(statie) >= 1))
    {

        char selectQuery[100];
        snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM useri WHERE username='%s'", username);

        sqlite3_stmt *selectStmt;
        if (sqlite3_prepare_v2(db, selectQuery, -1, &selectStmt, nullptr) == SQLITE_OK)
        {
            if (sqlite3_step(selectStmt) == SQLITE_ROW)
            {
                char errorMsg[] = "[server] Utilizatorul exista deja.";
                int lung = strlen(errorMsg);

                if (write(cl, &lung, sizeof(int)) < 0)
                {
                    perror("eroare user_register");
                    exit(1);
                }

                int totalScris = 0;
                while (totalScris < lung)
                {
                    int scris = write(cl, errorMsg + totalScris, lung - totalScris);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    totalScris += scris;
                }
                sqlite3_finalize(selectStmt);
                return;
            }
            sqlite3_finalize(selectStmt);
        }

        else
        {
            cerr << "Eroare la pregatirea interogarii SELECT pentru register." << endl;
            char errorMsg[] = "[server] Eroare la login.";
            int size = strlen(errorMsg);

            if (write(cl, &size, sizeof(int)) < 0)
            {
                perror("eroare in user_register");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < size)
            {
                int scris = write(cl, errorMsg + totalScris, size - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            return;
        }

        char insertQuery[1000];
        snprintf(insertQuery, sizeof(insertQuery), "INSERT INTO useri (username, unde_soseste) VALUES ('%s', '%s')", username, statie);

        if (sqlite3_exec(db, insertQuery, nullptr, nullptr, nullptr) == SQLITE_OK)
        {

            char successMsg[] = "[server] Inregistrare cu succes.";
            int lung = strlen(successMsg);

            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la user_register la partea cu sql u");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, successMsg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la user_register sql");
                    exit(1);
                }
                totalScris += scris;
            }
        }
        else
        {

            cerr << "Eroare la inserarea în baza de date pentru register." << endl;
            char errorMsg[] = "[server] Eroare la inregistrare.";
            int lung = strlen(errorMsg);

            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la inserare in sql");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, errorMsg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la user_register sql");
                    exit(1);
                }
                totalScris += scris;
            }
        }
    }
    else
    {

        char errorMsg[] = "[server] Comanda de register este urmatoarea : register <usname> <statie> .";

        int lung = strlen(errorMsg);

        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la register");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, errorMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la user_register sql");
                exit(1);
            }
            totalScris += scris;
        }
        return;
    }
}

void user_login(char *arg, sqlite3 *db, int cl)
{
    char comanda[10], username[50];
    int ret_impartire = sscanf(arg, "%s %s", comanda, username);
    if (ret_impartire != 2)
    {
        char errMsg[] = "[server] Comanda de login este urmatoarea : login <username> .";
        int lung = strlen(errMsg);

        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la scrierea spre client (user_login)");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, errMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }
        return;
    }

    if ((strcmp(comanda, "login") == 0) && (strlen(username) >= 1))
    {
        char selectQuery[100];

        snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM useri WHERE username='%s'", username);

        sqlite3_stmt *selectStmt;
        if (sqlite3_prepare_v2(db, selectQuery, -1, &selectStmt, nullptr) == SQLITE_OK)
        {
            if (sqlite3_step(selectStmt) == SQLITE_ROW)
            {
                char Msg[] = "[server] Te-ai logat cu succes, ";
                strncat(Msg, username, strlen(username));
                int lung = strlen(Msg);

                if (write(cl, &lung, sizeof(int)) < 0)
                {
                    perror("eroare la scrierea spre client (user_login)");
                    exit(1);
                }

                int totalScris = 0;
                while (totalScris < lung)
                {
                    int scris = write(cl, Msg + totalScris, lung - totalScris);
                    if (scris < 0)
                    {
                        perror("eroare la scrierea (2) spre server");
                        exit(1);
                    }
                    totalScris += scris;
                }
                cout << username << " s-a logat cu succes" << endl;
                sqlite3_finalize(selectStmt);
                return;
            }
            sqlite3_finalize(selectStmt);
        }

        else
        {

            cerr << "Eroare la pregatirea interogarii SELECT pentru register." << endl;

            char errorMsg[] = "[server] Eroare la login.";
            int lung = strlen(errorMsg);

            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la scrierea spre client (user_login)");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, errorMsg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            return;
        }
    }
    char Mesaj[] = "[server] Nu te-am gasit in baza de date.";
    int lung = strlen(Mesaj);

    if (write(cl, &lung, sizeof(int)) < 0)
    {
        perror("eroare la scrierea spre client (user_login)");
        exit(1);
    }

    int totalScris = 0;
    while (totalScris < lung)
    {
        int scris = write(cl, Mesaj + totalScris, lung - totalScris);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        totalScris += scris;
    }
    return;
}

void plecari_trenuri(char *arg, sqlite3 *db, int cl)
{
    char mesaj[] = "[server] Va rog sa precizati ziua : ";
    int lungime = strlen(mesaj);

    if (write(cl, &lungime, sizeof(int)) < 0)
    {
        perror("eroare la trimiterea dim in sosiri_trenuri");
        exit(1);
    }

    int total = 0;
    while (total < lungime)
    {
        int scris = write(cl, mesaj + total, lungime - total);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        total += scris;
    }

    int len_rasp;
    if (read(cl, &len_rasp, sizeof(int)) <= 0)
    {
        perror("eroare (1) plecari");
        exit(1);
    }

    char rasp_primit[len_rasp + 1];
    if (read(cl, rasp_primit, len_rasp * sizeof(char)) < 0)
    {
        perror("eroare (2) plecari");
        exit(1);
    }

    rasp_primit[len_rasp] = '\0';

    char comanda[20], statie[20];
    int ret_impartire = sscanf(arg, "%s %s", comanda, statie);

    char selectQuery[256];
    snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM plecari WHERE ziua='%s' AND statie='%s'", rasp_primit, statie);

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr) == SQLITE_OK)
    {
        int ok = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *id_plecare = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *ziua = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const char *statie = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            const char *id_tren = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            const char *destinatie = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            int ora = sqlite3_column_int(stmt, 5);
            int intarziere = sqlite3_column_int(stmt, 6);

            char info[2056];
            snprintf(info, sizeof(info), "ID: %s, Tren: %s, Pleaca la: %s, Ora: %d, Intarziere: %d\n",
                     id_plecare, id_tren, destinatie, ora, intarziere);
            strcat(info, ".");

            int lung = strlen(info);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la plecari");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, info + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            info[0] = '\0';
            ok = 0;
        }

        if (ok == 1)
        {
            char Msg[] = "[server] Nu sunt plecari in ziua asta.";
            int lung = strlen(Msg);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la plecari");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, Msg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
        }

        char endMsg[] = "end.";
        int lung = strlen(endMsg);
        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la plecari");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, endMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }
    }
}

void sosiri_trenuri(char *arg, sqlite3 *db, int cl)
{
    char mesaj[] = "[server] Va rog sa precizati ziua : ";
    int lungime = strlen(mesaj);

    if (write(cl, &lungime, sizeof(int)) < 0)
    {
        perror("eroare la trimiterea dim in sosiri_trenuri");
        exit(1);
    }

    int total = 0;
    while (total < lungime)
    {
        int scris = write(cl, mesaj + total, lungime - total);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        total += scris;
    }

    int len_rasp;
    if (read(cl, &len_rasp, sizeof(int)) <= 0)
    {
        perror("eroare (1) mersul trenurilor");
        exit(1);
    }

    char rasp_primit[len_rasp + 1];
    if (read(cl, rasp_primit, len_rasp * sizeof(char)) < 0)
    {
        perror("eroare (2) mersul trenurilor");
        exit(1);
    }

    rasp_primit[len_rasp] = '\0';

    char comanda[20], statie[20];
    int ret_impartire = sscanf(arg, "%s %s", comanda, statie);

    char selectQuery[256];
    snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM sosiri WHERE ziua='%s' AND unde_soseste='%s'", rasp_primit, statie);

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr) == SQLITE_OK)
    {
        int ok = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *id_plecare = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *ziua = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const char *unde_vine = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            const char *id_tren = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            const char *statie = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            int ora = sqlite3_column_int(stmt, 5);
            int intarziere = sqlite3_column_int(stmt, 6);

            char info[2056];
            snprintf(info, sizeof(info), "ID: %s, Tren: %s, Soseste de la: %s, Ora: %d, Intarziere: %d\n",
                     id_plecare, id_tren, statie, ora, intarziere);
            strcat(info, ".");

            int lung = strlen(info);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la mersul trenurilor");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, info + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            info[0] = '\0';
            ok = 0;
        }

        if (ok == 1)
        {
            char Msg[] = "[server] Nu sunt sosiri in ziua asta.";
            int lung = strlen(Msg);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la mersul trenurilor");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, Msg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
        }

        char endMsg[] = "end.";
        int lung = strlen(endMsg);
        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la mersul trenurilor");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, endMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }
    }
}

void plecariSosiri_urmatoarea_ora(char *arg, sqlite3 *db, int cl)
{
    char mesaj[] = "[server] Va rog sa precizati statia: ";
    int lungime = strlen(mesaj);

    if (write(cl, &lungime, sizeof(int)) < 0)
    {
        perror("eroare la trimiterea dim in plecari/sosiri");
        exit(1);
    }

    int total = 0;
    while (total < lungime)
    {
        int scris = write(cl, mesaj + total, lungime - total);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        total += scris;
    }

    int len_rasp;
    if (read(cl, &len_rasp, sizeof(int)) <= 0)
    {
        perror("eroare (1) mersul trenurilor");
        exit(1);
    }

    char rasp_primit[len_rasp + 1];
    if (read(cl, rasp_primit, len_rasp * sizeof(char)) < 0)
    {
        perror("eroare (2) mersul trenurilor");
        exit(1);
    }

    rasp_primit[len_rasp] = '\0';

    char comanda[20];
    if (arg[0] == 'p')
    {
        strncpy(comanda, arg, 7);
        comanda[7] = '\0';
    }
    else if (arg[0] == 's')
    {
        strncpy(comanda, arg, 6);
        comanda[6] = '\0';
    }

    char selectQuery[256];
    char currentDateTime[20];
    char ziua_sapt[20];
    int ora, minute, secunde;

    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    strftime(currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", tm_info);
    strftime(ziua_sapt, sizeof(ziua_sapt), "%A", tm_info);

    en_to_ro(ziua_sapt);

    sscanf(currentDateTime, "%*d-%*d-%*d %d:%d:%d", &ora, &minute, &secunde);

    if (comanda[0] == 'p')
    {
        snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM plecari WHERE ziua='%s' AND statie='%s' AND (ora * 60) > '%d'*60 + '%d'", ziua_sapt, rasp_primit, ora, minute);
    }
    else if (comanda[0] == 's')
    {
        snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM sosiri WHERE ziua='%s' AND unde_soseste='%s' AND (ora * 60) > '%d'*60 + '%d'", ziua_sapt, rasp_primit, ora, minute);
    }

    // cout<<"Data curenta este : "<<currentDateTime<<endl;
    // cout<<"Ziua saptamanii este : "<<ziua_sapt<<endl;
    // cout<<"Ora este : "<<ora<<endl;

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr) == SQLITE_OK)
    {
        int ok = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *id_plecare = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *ziua = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const char *unde_vine = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            const char *id_tren = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            const char *statie = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            int ora = sqlite3_column_int(stmt, 5);
            int intarziere = sqlite3_column_int(stmt, 6);

            char info[2056];
            if (comanda[0] == 's')
            {
                snprintf(info, sizeof(info), "ID: %s, Tren: %s, Soseste de la: %s, Ora: %d, Intarziere: %d\n",
                         id_plecare, id_tren, statie, ora, intarziere);
                strcat(info, ".");
            }
            else if (comanda[0] == 'p')
            {
                snprintf(info, sizeof(info), "ID: %s, Tren: %s, Pleaca la: %s, Ora: %d, Intarziere: %d\n",
                         id_plecare, id_tren, statie, ora, intarziere);
                strcat(info, ".");
            }

            int lung = strlen(info);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la pleacari/sosiri");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, info + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            info[0] = '\0';
            ok = 0;
        }

        if (ok == 1)
        {
            char Msg[] = "[server] Nu sunt plecari/sosiri in urmatoarea ora.";
            int lung = strlen(Msg);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la plecari/sosiri");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, Msg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
        }

        char endMsg[] = "end.";
        int lung = strlen(endMsg);
        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la plecari/sosiri");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, endMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }
    }
}

void estimari_sosiri(char *arg, sqlite3 *db, int cl)
{
    char mesaj[] = "[server] Furnizati username-ul si id-ul trenului care va intereseaza: ";
    int lungime = strlen(mesaj);

    if (write(cl, &lungime, sizeof(int)) < 0)
    {
        perror("eroare la trimiterea din estimari");
        exit(1);
    }

    int total = 0;
    while (total < lungime)
    {
        int scris = write(cl, mesaj + total, lungime - total);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        total += scris;
    }

    int len_rasp;
    if (read(cl, &len_rasp, sizeof(int)) <= 0)
    {
        perror("eroare (1) mersul trenurilor");
        exit(1);
    }

    char rasp_primit[len_rasp + 1];
    if (read(cl, rasp_primit, len_rasp * sizeof(char)) < 0)
    {
        perror("eroare (2) mersul trenurilor");
        exit(1);
    }
    rasp_primit[len_rasp] = '\0';

    char user[20];
    char idTren[20];

    if (sscanf(rasp_primit, "%s %s", user, idTren) != 2)
    {
        cout << "eroare la user idTren" << endl;
    }

    // cout<<"Userul este: "<<user<<endl;
    // cout<<"Id-ul la tren este: "<<idTren<<endl;

    char currentDateTime[20];
    int ora, minute, secunde;

    time_t t;
    struct tm *tm_info;
    char ziua_sapt[20];

    time(&t);
    tm_info = localtime(&t);

    strftime(currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", tm_info);
    strftime(ziua_sapt, sizeof(ziua_sapt), "%A", tm_info);

    en_to_ro(ziua_sapt);
    sscanf(currentDateTime, "%*d-%*d-%*d %d:%d:%d", &ora, &minute, &secunde);

    // cout<<"Ziua este: "<<ziua_sapt<<endl;

    char selectQuery[512];
    snprintf(selectQuery, sizeof(selectQuery), "SELECT s.unde_soseste,s.ora,s.intarziere FROM sosiri s join useri u on s.unde_soseste = u.unde_soseste where s.ziua ='%s' and s.id_tren = '%s' and (s.ora *60) > ('%d'*60)+'%d'", ziua_sapt, idTren, ora, minute);

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr) == SQLITE_OK)
    {
        int ok = 1;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *sosire = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            int oraSosire = sqlite3_column_int(stmt, 1);
            int intarziereSosire = sqlite3_column_int(stmt, 2);

            char info[2056];

            int calcul = (oraSosire * 60 + intarziereSosire) - (ora * 60 + minute);

            snprintf(info, sizeof(info), "[server] Trenul %s ajunge in statia %s in aproximativ %d minute.\n", idTren, sosire, calcul);

            strcat(info, ".");

            cout << info << endl;

            int lung = strlen(info);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la mersul trenurilor");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, info + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            info[0] = '\0';
            ok = 0;
        }

        if (ok == 1)
        {
            char Msg[] = "[server] Trenul pe care il cautati fie a plecat deja, fie nu exista.";
            int lung = strlen(Msg);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la estimari");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, Msg + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
        }

        char endMsg[] = "end.";
        int lung = strlen(endMsg);
        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la mersul trenurilor");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, endMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }
    }
}

void intarziereTren(char *arg, sqlite3 *db, int cl)
{
    char comanda[50], id_tren[50];
    int catIntarzie;

    int ret_impartire = sscanf(arg, "%s %s %d", comanda, id_tren, &catIntarzie);
    if (ret_impartire != 3)
    {
        char errMsg[] = "[server] Comanda de intarziere este urmatoarea : intarziere <id_tren> <catIntarzie> .";
        comunicareWrite(errMsg, cl);
        return;
    }

    // cout<<"comanda : "<<comanda<<endl;
    // cout<<"id_tren : "<<id_tren<<endl;
    // cout<<"cat intarzie : "<<catIntarzie<<endl;

    char currentDateTime[50];
    int ora, minute, secunde;
    time_t t;
    struct tm *tm_info;
    char ziua_sapt[20];

    time(&t);
    tm_info = localtime(&t);

    strftime(currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", tm_info);
    strftime(ziua_sapt, sizeof(ziua_sapt), "%A", tm_info);

    en_to_ro(ziua_sapt);
    sscanf(currentDateTime, "%*d-%*d-%*d %d:%d:%d", &ora, &minute, &secunde);

    char selectQuery[256];
    snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM plecari WHERE ziua='%s' and id_tren='%s'", ziua_sapt, id_tren);

    sqlite3_stmt *statement;
    if (sqlite3_prepare_v2(db, selectQuery, -1, &statement, 0) != SQLITE_OK)
    {
        cerr << "Eroare la pregătirea interogării SELECT." << endl;
        return;
    }

    int result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_ROW)
    {

        char errMsg[256];
        snprintf(errMsg, sizeof(errMsg), "[server] Nu există trenul %s în ziua %s.", id_tren, ziua_sapt);
        comunicareWrite(errMsg, cl);
        return;
    }

    char updateQuery[256];
    snprintf(updateQuery, sizeof(updateQuery), "UPDATE plecari SET intarziere=(intarziere + %d) WHERE id_tren='%s' and ziua = '%s'", catIntarzie, id_tren, ziua_sapt);

    if (sqlite3_exec(db, updateQuery, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        char errMsg[] = "[server] Eroare la actualizarea intarzierii in baza de date.";
        comunicareWrite(errMsg, cl);
        return;
    }

    sqlite3 *baza;
    if (sqlite3_open("client.db", &baza) != SQLITE_OK)
    {
        cerr << "Eroare la deschiderea bazei de date pentru clienti afectati." << endl;
        exit(1);
    }

    const char *create = "CREATE TABLE IF NOT EXISTS clientiAfectati (nume TEXT PRIMARY KEY,idTrenAfectat, TEXT, intarzie INTEGER )";
    if (sqlite3_exec(baza, create, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Eroare la crearea tabelului in baza de date." << endl;
        exit(1);
    }

    // char selectIntarziere[256];
    // snprintf(selectIntarziere,sizeof(selectIntarziere),"SELECT intarziere ")

    char selectUser[512];
    snprintf(selectUser, sizeof(selectUser), "SELECT u.username,p.intarziere FROM useri u join plecari p on u.unde_soseste = p.statie WHERE p.ziua='%s' and p.id_tren='%s'", ziua_sapt, id_tren);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, selectUser, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *nume = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            int intarziere_tabel = sqlite3_column_int(stmt, 1);

            // cout<<nume<<intarziere_tabel<<endl;

            char selectNume[512];
            snprintf(selectNume, sizeof(selectNume), "SELECT count(nume) FROM clientiAfectati WHERE nume = '%s'", nume);

            sqlite3_stmt *state;
            int numarClientiAfectati;
            if (sqlite3_prepare_v2(baza, selectNume, -1, &state, nullptr) == SQLITE_OK)
            {
                if (sqlite3_step(state) == SQLITE_ROW)
                {
                    numarClientiAfectati = sqlite3_column_int(state, 0);
                    cout << "Numărul de clienți afectați cu numele " << nume << " este: " << numarClientiAfectati << endl;
                }
                sqlite3_finalize(state);
            }
            else
            {
                cerr << "Eroare la pregătirea interogării SELECT pentru clientiAfectati." << endl;
            }

            if (numarClientiAfectati == 0)
            {
                char insert[512];
                snprintf(insert, sizeof(insert), "INSERT INTO clientiAfectati (nume,idTrenAfectat,intarzie) VALUES ('%s','%s','%d')", nume, id_tren, intarziere_tabel);

                if (sqlite3_exec(baza, insert, nullptr, nullptr, nullptr) != SQLITE_OK)
                {
                    cerr << "Eroare la inserarea datelor în clientiAfectati: " << sqlite3_errmsg(baza) << endl;
                }
            }
            else
            {

                char updateClienti[256];
                snprintf(updateClienti, sizeof(updateClienti), "UPDATE clientiAfectati SET intarzie= %d WHERE nume = '%s'", intarziere_tabel, nume);

                if (sqlite3_exec(baza, updateClienti, nullptr, nullptr, nullptr) != SQLITE_OK)
                {
                    char errMsg[] = "Eroare la actualizarea intarzierii in baza de date.";
                    comunicareWrite(errMsg, cl);
                    return;
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    char successMsg[256];
    snprintf(successMsg, sizeof(successMsg), "[server] Intarzierea trenului %s a fost actualizata cu succes.", id_tren);
    comunicareWrite(successMsg, cl);
    return;
}

void mersul_trenurilor(char *arg, sqlite3 *db, int cl)
{
    char mesaj[] = "[server] Va rog sa precizati ziua : ";
    int lungime = strlen(mesaj);

    if (write(cl, &lungime, sizeof(int)) < 0)
    {
        perror("eroare la trimiterea dim in mersul_trenurilor");
        exit(1);
    }

    int total = 0;
    while (total < lungime)
    {
        int scris = write(cl, mesaj + total, lungime - total);
        if (scris < 0)
        {
            perror("eroare la scrierea (2) spre server");
            exit(1);
        }
        total += scris;
    }

    int len_rasp;
    if (read(cl, &len_rasp, sizeof(int)) <= 0)
    {
        perror("eroare (1) mersul trenurilor");
        exit(1);
    }

    char rasp_primit[len_rasp + 1];
    if (read(cl, rasp_primit, len_rasp * sizeof(char)) < 0)
    {
        perror("eroare (2) mersul trenurilor");
        exit(1);
    }

    rasp_primit[len_rasp] = '\0';

    char selectQuery[256];
    snprintf(selectQuery, sizeof(selectQuery), "SELECT * FROM plecari WHERE ziua='%s'", rasp_primit);

    char unionQuery[256];
    snprintf(unionQuery, sizeof(unionQuery), " UNION SELECT * FROM sosiri WHERE ziua='%s'", rasp_primit);

    strncat(selectQuery, unionQuery, sizeof(selectQuery) - strlen(selectQuery) - 1);
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *id_plecare = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *ziua = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const char *statie = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            const char *id_tren = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            const char *destinatie = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            int ora = sqlite3_column_int(stmt, 5);
            int intarziere = sqlite3_column_int(stmt, 6);

            char info[2056];
            snprintf(info, sizeof(info), "ID: %s, Ziua: %s, Statie: %s, Tren: %s, Destinatie: %s, Ora: %d, Intarziere: %d\n",
                     id_plecare, ziua, statie, id_tren, destinatie, ora, intarziere);
            strcat(info, ".");

            int lung = strlen(info);
            if (write(cl, &lung, sizeof(int)) < 0)
            {
                perror("eroare la mersul trenurilor");
                exit(1);
            }

            int totalScris = 0;
            while (totalScris < lung)
            {
                int scris = write(cl, info + totalScris, lung - totalScris);
                if (scris < 0)
                {
                    perror("eroare la scrierea (2) spre server");
                    exit(1);
                }
                totalScris += scris;
            }
            info[0] = '\0';
        }

        char endMsg[] = "end.";
        int lung = strlen(endMsg);
        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la mersul trenurilor");
            exit(1);
        }

        int totalScris = 0;
        while (totalScris < lung)
        {
            int scris = write(cl, endMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre server");
                exit(1);
            }
            totalScris += scris;
        }
    }
    else
    {
        cerr << "Eroare la pregatirea interogarii SELECT: " << sqlite3_errmsg(db) << endl;
    }

    sqlite3_finalize(stmt);
}

void raspuns_server(char *arg, sqlite3 *db, int cl)
{
    if (strncmp(arg, "login", 5) == 0)
    {
        user_login(arg, db, cl);
        return;
    }
    else if (strncmp(arg, "intarziere", 10) == 0 && strlen(arg) > 15)
    {
        intarziereTren(arg, db, cl);
        return;
    }
    else if ((strncmp(arg, "register", 8) == 0))
    {
        user_register(arg, db, cl);
        return;
    }
    else if (strcmp(arg, "estimare sosire") == 0)
    {
        estimari_sosiri(arg, db, cl);
        return;
    }
    else if (strcmp(arg, "plecari urmatoarea ora") == 0 || strcmp(arg, "sosiri urmatoarea ora") == 0 || strcmp(arg, "plecari ora urmatoare") == 0 || strcmp(arg, "sosiri ora urmatoare") == 0)
    {
        plecariSosiri_urmatoarea_ora(arg, db, cl);
        return;
    }
    else if ((strncmp(arg, "plecari", 7) == 0) && strlen(arg) > 8)
    {
        plecari_trenuri(arg, db, cl);
        return;
    }
    else if ((strncmp(arg, "sosiri", 6) == 0) && strlen(arg) > 8)
    {
        sosiri_trenuri(arg, db, cl);
        return;
    }
    else if (strcmp(arg, "mersul trenurilor") == 0)
    {
        mersul_trenurilor(arg, db, cl);
        return;
    }
    else
    {
        char errorMsg[] = "[server] Comanda necunoscuta. Incercati <help> pentru a vedea toate comenzile acceptate.";
        int lung = strlen(errorMsg);

        if (write(cl, &lung, sizeof(int)) < 0)
        {
            perror("eroare la scrierea spre client (comanda necunoscuta)");
            exit(1);
        }

        int totalScris = 0;

        while (totalScris < lung)
        {
            int scris = write(cl, errorMsg + totalScris, lung - totalScris);
            if (scris < 0)
            {
                perror("eroare la scrierea (2) spre client");
                exit(1);
            }
            totalScris += scris;
        }
        return;
    }
}

void data_base(sqlite3 *db)
{

    const char *createQuery = "CREATE TABLE IF NOT EXISTS plecari (id_plecare TEXT PRIMARY KEY ,ziua TEXT, statie TEXT, id_tren TEXT, destinatie TEXT, ora INTEGER, intarziere INTEGER)";
    if (sqlite3_exec(db, createQuery, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Eroare la crearea tabelului in baza de date." << endl;
        exit(1);
    }

    const char *createQuery_2 = "CREATE TABLE IF NOT EXISTS sosiri (id_sosire TEXT PRIMARY KEY,ziua TEXT , unde_soseste TEXT, id_tren TEXT, de_unde_soseste TEXT, ora INTEGER, intarziere INTEGER)";
    if (sqlite3_exec(db, createQuery_2, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Eroare la crearea tabelului in baza de date." << endl;
        exit(1);
    }
    /*
    const char *insertQuery = "INSERT INTO plecari (id_plecare, ziua, statie, id_tren, destinatie, ora, intarziere) VALUES ('A1','Luni','Iasi','IR421','Brasov', 12, 0)";

    if (sqlite3_exec(db, insertQuery, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Eroare la inserarea in baza de date." << endl;
        exit(1);
    }
    */
}

void data_base_clientiAfectati(sqlite3 *baza)
{
    const char *create = "CREATE TABLE IF NOT EXISTS clientiAfectati (nume TEXT PRIMARY KEY)";
    if (sqlite3_exec(baza, create, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Eroare la crearea tabelului in baza de date." << endl;
        exit(1);
    }
}

void data_base_login(sqlite3 *db)
{

    const char *createQuery = "CREATE TABLE IF NOT EXISTS useri (username TEXT PRIMARY KEY, unde_soseste TEXT )";
    if (sqlite3_exec(db, createQuery, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Eroare la crearea tabelului in baza de date." << endl;
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    sockaddr_in serverAddr;
    sockaddr_in from;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        perror("[server]Eroare la socket().\n");
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("eroare la SO_REUSEADDR");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&from, 0, sizeof(from));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (::bind(socketfd, (sockaddr *)&serverAddr, sizeof(sockaddr)) < 0)
    {
        perror("[server]Eroare la bind().\n");
        exit(1);
    }

    if (listen(socketfd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        exit(1);
    }

    sqlite3 *db;
    if (sqlite3_open("mydb.db", &db) != SQLITE_OK)
    {
        cerr << "Eroare la deschiderea bazei de date." << endl;
        exit(1);
    }

    data_base(db);
    data_base_login(db);

    pthread_t th[10];
    int i = 0;

    while (1)
    {
        thData *td;

        cout << "[server] Asteptam la portul " << PORT << "..." << endl;
        fflush(stdout);

        int length = sizeof(from);
        int client = accept(socketfd, (sockaddr *)&from, (socklen_t *)&length);
        if (client < 0)
        {
            perror("eroare la accept");
            continue;
        }

        td = (thData *)malloc(sizeof(thData));
        td->idThread = i++;
        td->cl = client;
        td->db = db;

        if (pthread_create(&th[i], NULL, &welcome_clients, td) != 0)
        {
            perror("Eroare la crearea firului de execuție.\n");
            break;
        }
    }

    sqlite3_close(db);
    return 0;
}