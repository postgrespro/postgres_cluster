#include <iostream>
#include <string>
#include <vector>
#include <set>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/nontransaction>

using namespace std;
using namespace pqxx;

int main (int argc, char* argv[])
{
    if (argc == 1){
        printf("Use -h to show usage options\n");
        return 1;
    }
    vector<string> connections;
    set<string> prepared_xacts;
    set<string> committed_xacts;
    bool verbose = false;
    for (int i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) { 
              case 'C':
              case 'c':
                connections.push_back(string(argv[++i]));
                continue;
              case 'v':
                verbose = true;
                continue;
            }
        }
        printf("Perform recovery of pg_tsdtm cluster.\n"
               "Usage: dtm_recovery {options}\n"
               "Options:\n"
               "\t-c STR\tdatabase connection string\n"
               "\t-v\tverbose mode: print extra information while processing\n");
        return 1;
    }
    if (verbose) { 
        cout << "Collecting information about prepared transactions...\n";
    }
    for (vector<string>::iterator ic = connections.begin(); ic != connections.end(); ++ic)
    {
        if (verbose) { 
            cout << "Connecting to " << *ic << "...\n";
        }
        connection con(*ic);
        work txn(con);
        result r = txn.exec("select gid from pg_prepared_xacts");
        for (result::const_iterator it = r.begin(); it != r.end(); ++it)
        {
            string gid = it.at("gid").as(string());
            prepared_xacts.insert(gid);
        }
        txn.commit();
    }
    if (verbose) { 
        cout << "Prepared transactions: ";
        for (set<string>::iterator it = prepared_xacts.begin(); it != prepared_xacts.end(); ++it)
        {
            cout << *it << ", ";
        }
        cout << "\nChecking which of them are committed...\n";
    }        
    for (vector<string>::iterator ic = connections.begin(); ic != connections.end(); ++ic)
    {
        if (verbose) { 
            cout << "Connecting to " << *ic << "...\n";
        }
        connection con(*ic);
        work txn(con);
        con.prepare("commit-check", "select * from pg_committed_xacts where gid=$1");
        for (set<string>::iterator it = prepared_xacts.begin(); it != prepared_xacts.end(); ++it)
        {
            string gid = *it;
            result r = txn.prepared("commit-check")(gid).exec();
            if (!r.empty()) {
                committed_xacts.insert(gid);
            }
        }
        txn.commit();
    }
    if (verbose) { 
        cout << "Committed transactions: ";
        for (set<string>::iterator it = committed_xacts.begin(); it != committed_xacts.end(); ++it)
        {
            cout << *it << ", ";
        }
        cout << "\nCommitting them at all nodes...\n";
    }      
    for (vector<string>::iterator ic = connections.begin(); ic != connections.end(); ++ic)
    {
        if (verbose) { 
            cout << "Connecting to " << *ic << "...\n";
        }
        connection con(*ic);
        work txn(con);
        con.prepare("commit-check", "select * from pg_committed_xacts where gid=$1");
        con.prepare("commit-prepared", "commit prepared $1");
        con.prepare("rollback-prepared", "rollback prepared $1");
        result r = txn.exec("select gid from pg_prepared_xacts");
        for (result::const_iterator it = r.begin(); it != r.end(); ++it)
        {
            string gid = it.at("gid").as(string());
            result rc = txn.prepared("commit-check")(gid).exec();
            if (rc.empty()) {
                if (committed_xacts.find(gid) != committed_xacts.end()) {
                    if (verbose) { 
                        cout << "Commit transaction " << gid << "\n";
                    }
                    txn.prepared("commit-prepared")(gid);
                } else { 
                    if (verbose) { 
                        cout << "Rollback transaction " << gid << "\n";
                    }
                    txn.prepared("rollback-prepared")(gid);
                }
            }
        }
        txn.commit();
    }
    if (verbose) { 
        cout << "Recovery completed\n";
    }
    return 0;
}               
