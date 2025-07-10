# web-server
a web server build from the ground up. server-side scripts and custom database included.

## server-script
the renowned scripting language(/s) C_Snek developed by the well known(/s) and amazing developer(/s) Dimanari, within mweb files in the www folder the scripts are invoked using the <server></server> tag.

## database
the server uses a file storage database, defaults to "db/database.dbf" using a custom file format(unrelated to the dbf format from open-office, an unfortunate coincidence) storing tables containing keys and values for easy storage and retrieval.
> *requires auto-save optimization to remove save actions when database was not changed, works fine for now.

## terminal
the terminal communicates with the database directly and can shut down the server with the "EXIT" command.
