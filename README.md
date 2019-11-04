# Linux / Windows C server based on gopher protocol


[![Build Status](https://travis-ci.org/joemccann/dillinger.svg?branch=master)](https://travis-ci.org/joemccann/dillinger)

The webserver is portable on Linux / Windows ( auto-detach the platform ) writter entirely on C, based on communication protocol GOPHER with focus on:

  - Multithread - Multiprocess ( choose the modality from the config.txt file)
  - Sockets
  - Support port changes, if there's a client connected to the old port, he'll be served
  - Log file written by a different process
  - On windows use MINGW (Minimalist GNU for windows)

# Installation

Just clone or download this repo, compile the project and enjoy it.
By default the config.txt is set:
    
    - port: 8080
    - type: multiprocess

PORT: you can choose the port you like
TYPE: multiprocess OR multithread


License
----

MIT

Developed with Jacopo97

**Free Software, Yeah!**

