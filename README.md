An framework for working with files with "elastic" ability

This framework implements some kind of file system which have some features: - modifying operations (write, delete) do not depend on file size; - files are not overwritten every time when the data is inserted or deleted at any position.

In other words, the amount of excessive (supplementary) information being written depends only on the size of the serialized data, but NOT on the current size of the file itself.

This project contains a user interface for working with elastic files. Elastic files represents a structure with a data scattered in sectors. Therefore elastic files have a sectors table that contains all information about file fragmentation.

Project structure:

ElasticFileAPI - a directory contains ElasticFileAPI.h/.cpp and EFileController.h/.cpp. Represents an user interface for working with ElasticFiles ElasticFileAPI.h/.cpp - an user interface for working with ElasticFiles. Contain such standard file operating functions as open, close, read, write, get cursor, set cursor.

EFileController.h/.cpp - manages file handles of opened files. It have a table of handles of the files and contain such operations with handles table as - get file object by handle, - open file by name and return it's handle, - close file and free it's handle.

ElasticFile - a directory contains implementation of ElasticFiles logic.

ElasticFile.h/.cpp - represents an ElasticFile object that is created when ElasticFileAPI::FileOpen is called and handle of that is stored in handles table. This object has all standard file operating functions like open, close, read, write, get cursor, set cursor, but it is an lower interface that ElasticFileAPI. It does not check a file handle on access and does not use handles table.

TEFileCursor.h/.cpp - represents a logic of a cursor of ElesticFile. It is an separate class because the cursor works with sectors in a file sectors table, therefore current cursor position contains a reference to the current sector and an offset in that sector.

TEFileSectorsTable.h/.cpp - represents a logic of the sectors table of the files.

TEFileException.h - represents a specific exception in ElasticFile logic.

efile_types.h - defines all specific types, enums and constants using in the logic.
