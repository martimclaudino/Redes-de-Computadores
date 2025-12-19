RC Event Reservation Project 2025/2026

Shift: L08

Members: 
-ist1109490 Martim Claudino
-ist1109308 Diogo Gonçalves

This project implements a Client-Server Event Management System (ES) developed 
in C++, designed to handle concurrent requests. Key features include:
-User Management: Users can register and log in to the system securely.
-Event Publishing: Authenticated users can create new events by specifying details (date, time, capacity) and uploading description files.
-Event Discovery: Users can list all available events or retrieve details for aspecific event ID.
-Booking System: Users can book seats for active events, with the system managing capacity limits.
-Concurrency Control: The system uses file locking mechanisms to ensure safe unique ID generation and prevent data corruption during simultaneous accesses.
-Data Persistence: All data is stored in a structured local file system (src/ESDIR), organized by Users and Events.

# Compiling

The project is developed in C++ and can be compiled by running the following command on the current directory
make

This will generate two main executables:
user - The User Application
ES - The Event Server

Additional Makefile rules:
make clean - Removes object files and executables
make clean_db - Resets the server, removing all previous data (users, events, reservations)

!Important note: before running the server for the first time (since downloaded the repo) please run
make clean_db as it builds the server structure

# Executing the Event Server

The server can be executed by running the following command:
./ES [-p port] [-v]

Where the arguments in brackets are optional:
-p port: Specifies the port number for both UDP and TCP connections (Default: 58010)
-v: Enables verbose mode, displaying detailed information about received requests in the terminal

# Executing the User Application

The user app can be executed by running the following command:
./user [-n server_ip] [-p server_port]

Where the arguments in brackets are optional:
-n server_ip: Server IP address (Default: 127.0.0.1)
-p server_port: Server port number (Default: 58010)

Note: you can change the default IP and ports for both server and user app in common/common.hpp

# Project structure

-Common (common files with code shared by both the User App and the ES)
-EVENTS (downloaded event files by the user app from the server will be stored here)
-src
 |--ESDIR (Everything the ES needs)
 |--UserApp (Everything the user app needs)
-Makefile
-ReadMe.txt

You followed the suggested structure for the ESDIR directory, containing the code files and the 
suggested EVENTS and Users directories structured as recommended, with the exception of the names
we give to the generated files representing reservations (just for code convenience)