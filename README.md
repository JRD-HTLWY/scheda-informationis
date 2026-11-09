# scheda-informationis

A ticket system with a graphical client and a command-line server.

## Project layout

- `clientApp/` contains the wxWidgets desktop client.
- `server/` contains the TCP server and its JSON support library.

## Build

Build the client and server separately from the repository root:

```sh
cmake -S clientApp -B clientApp/build
cmake --build clientApp/build --parallel

cmake -S server -B server/build
cmake --build server/build --parallel
```

## Run on one machine

Start the server from the repository root:

```sh
./server/build/server
```

In another terminal, also from the repository root, start the client:

```sh
./clientApp/build/clientApp
```

Enter `127.0.0.1` as the server address. The server uses `konten.txt` and
`alleNachrichten.txt` in its current working directory for local account and
ticket data.
