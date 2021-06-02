/**
 * @file bug547.cpp regression case for bug 547 and bug 594 ( "get_dcb fails if slaves are not available" and
 *"Maxscale fails to start without anything in the logs if there is no slave available" )
 * Behaviour has been changed and this test check only for crash
 * - block all slaves
 * - try some queries (create table, do INSERT using RWSplit router)
 * - check there is no crash
 */

/*
 *  Vilho Raatikka 2014-09-16 07:43:54 UTC
 *  get_dcb function returns the backend descriptor for router. Some merge has broken the logic and in case of
 * non-existent slave the router simply fails to find a backend server although master would be available.
 *  Comment 1 Vilho Raatikka 2014-09-16 09:40:14 UTC
 *  get_dcb now searches master if slaves are not available.
 */

// also relates to bug594
// all slaves in MaxScale config have wrong IP

#include <maxtest/testconnections.hh>

int main(int argc, char* argv[])
{
    TestConnections test(argc, argv);

    for (int i = 1; i < test.repl->N; i++)
    {
        test.reset_timeout();
        test.repl->block_node(i);
    }

    test.reset_timeout();
    test.maxscale->wait_for_monitor();

    test.reset_timeout();
    test.tprintf("Connecting to all MaxScale services, expecting no errors");
    test.expect(test.maxscale->connect_maxscale() == 0, "Connection should not fail");

    test.reset_timeout();
    test.tprintf("Trying some queries, expecting no failures");
    test.try_query(test.maxscale->conn_rwsplit[0], "DROP TABLE IF EXISTS t1");
    test.try_query(test.maxscale->conn_rwsplit[0], "CREATE TABLE t1 (x INT)");
    test.try_query(test.maxscale->conn_rwsplit[0], "INSERT INTO t1 (x) VALUES (1)");
    test.try_query(test.maxscale->conn_rwsplit[0], "select 'rwsplit' from t1");
    test.try_query(test.maxscale->conn_master, "select 'rconn master' from t1");
    test.try_query(test.maxscale->conn_slave, "select 'rconn slave' from t1 ");

    test.reset_timeout();
    test.maxscale->close_maxscale_connections();

    test.reset_timeout();
    test.repl->unblock_all_nodes();

    return test.global_result;
}
