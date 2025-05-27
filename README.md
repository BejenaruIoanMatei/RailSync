# RailSync

## About the Project

RailSync is a server-side application that provides registered clients with real-time information about:

* 🚆 **Train schedules** – up-to-date timetables for all available trains
* 🚉 **Departure and arrival status** – real-time notifications for station departures and arrivals
* ⏳ **Delays** – real-time reporting and updates of train delays
* 🕒 **Arrival estimates** – calculation of estimated arrival times for trains

The goal of the project is to offer an efficient and fast solution for managing and monitoring train schedules using a robust architecture based on TCP networks and a local database.

## Technologies Used

* **Language:** C/C++ – for performance and low-level resource control
* **TCP Sockets:** for communication between client and server
* **SQLite:** for storing and managing data about trains, status, and users
* **Threads:** for handling multiple connections and parallel processing

## Idea

The application started as a project for the **Computer Networks** course, with the initial goal of implementing a TCP server to manage connections and data exchange between the client and server. Over time, new features and improvements were added.

## Database Files

`mydb.db` and `client.db` are created locally through SQL scripts.

> **Note:** The code will **not compile** without the database files.
