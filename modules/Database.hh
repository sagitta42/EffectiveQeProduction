#ifndef _DATABASE_H
#define _DATABASE_H

#include <iostream>
#include <TSQLServer.h>
#include <TSQLResult.h>
#include <TSQLRow.h>

class Database{
    public:
        Database();
        ~Database();

        TSQLServer* db; 
        TSQLServer* dbrun;
        int LastValidEvnum(int runnum);
};

#endif
