# RailSync

## Despre Proiect
RailSync este o aplicație server care oferă clienților înregistrați informații în timp real despre:

- 🚆 Mersul trenurilor – orare actualizate pentru toate trenurile disponibile
- 🚉 Status plecări și sosiri – notificări în timp real pentru plecările și sosirile din stații
- ⏳ Întârzieri – raportarea și actualizarea întârzierilor în timp real
- 🕒 Estimare sosire – calcularea timpului estimat pentru sosirea trenurilor

Scopul proiectului este de a oferi o soluție eficientă și rapidă pentru gestionarea și monitorizarea mersului trenurilor folosind o arhitectură robustă bazată pe rețele TCP și o bază de date locală.

## Tehnologii Utilizate
- Limbaj: C/C++ – pentru performanță și control direct asupra resurselor
- Sockets TCP: pentru comunicarea între client și server
- SQLite: pentru stocarea și gestionarea datelor despre trenuri, status și utilizatori
- Threads: pentru gestionarea conexiunilor multiple și procesarea în paralel

## Idee
Aplicația a început ca un proiect pentru cursul de Rețele de Calculatoare, unde scopul inițial era să implementez un server TCP pentru gestionarea conexiunilor și schimbul de date între client și server, urmând ca ulterior să adaug noi funcționalități și îmbunătățiri.
