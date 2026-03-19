/*
hello i'm the dev i'm voidoxin
this programme is supported with comments you can read it to know how this work
                                                enjoy learning
*/
#include<iostream>
#include "L0-core/include/DatabaseManager.h"
#include "L0-core/include/config_manager.h"
using namespace std;
int main ()
{
        ConfigManager cfg("../../config/config.json");
        string database_path=cfg.get()["App"]["database"]["db_path"];
        VulnD vuln(database_path);
        ModuD mod(database_path);
        vuln.createTables();
        cout<<"every thing is ok"<<endl;
return 0;
}