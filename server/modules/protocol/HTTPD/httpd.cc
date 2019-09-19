/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2023-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

/**
 * @file httpd.c - HTTP daemon protocol module
 *
 * The httpd protocol module is intended as a mechanism to allow connections
 * into the gateway for the purpose of accessing information within
 * the gateway with a REST interface
 * databases.
 *
 * In the first instance it is intended to allow a debug connection to access
 * internal data structures, however it may also be used to manage the
 * configuration of the gateway via REST interface.
 */

#include <maxscale/protocol/httpd/module_names.hh>
#define MXS_MODULE_NAME MXS_HTTPD_PROTOCOL_NAME

#include "httpd.hh"
#include <cctype>
#include <maxbase/alloc.h>
#include <maxscale/listener.hh>
#include <maxscale/modinfo.hh>
#include <maxscale/session.hh>

#define ISspace(x) isspace((int)(x))
#define HTTP_SERVER_STRING "MaxScale(c) v.1.0.0"

static int                   httpd_get_line(int sock, char* buf, int size);
static void                  httpd_send_headers(DCB* dcb, int final, bool auth_ok);
static std::string           httpd_default_auth();

class HTTPDProtocolModule : public mxs::ProtocolModule
{
public:
    static HTTPDProtocolModule* create(const std::string& auth_name, const std::string& auth_opts)
    {
        return new (std::nothrow) HTTPDProtocolModule();
    }

    std::unique_ptr<mxs::ClientProtocol>
    create_client_protocol(MXS_SESSION* session, mxs::Component* component) override
    {
        std::unique_ptr<mxs::ClientProtocol> new_client_proto;
        new_client_proto = std::unique_ptr<mxs::ClientProtocol>(new(std::nothrow) HTTPDClientProtocol());
        return new_client_proto;
    }

    std::string auth_default() const override
    {
        return httpd_default_auth();
    }

    std::string name() const override
    {
        return MXS_MODULE_NAME;
    }

    int load_auth_users(SERVICE* service) override
    {
        return MXS_AUTH_LOADUSERS_OK;
    }

    void print_auth_users(DCB* output) override
    {
    }

    json_t* print_auth_users_json() override
    {
        return json_array();
    }
};

extern "C"
{
/**
 * The module entry point routine. It is this routine that
 * must populate the structure that is referred to as the
 * "module object", this is a structure with the set of
 * external entry points for this module.
 *
 * @return The module object
 */
MXS_MODULE* MXS_CREATE_MODULE()
{
    static MXS_MODULE info =
    {
        MXS_MODULE_API_PROTOCOL,
        MXS_MODULE_IN_DEVELOPMENT,
        MXS_PROTOCOL_VERSION,
        "An experimental HTTPD implementation for use in administration",
        "V1.2.0",
        MXS_NO_MODULE_CAPABILITIES,
        &mxs::ProtocolApiGenerator<HTTPDProtocolModule>::s_api,
        NULL,       /* Process init. */
        NULL,       /* Process finish. */
        NULL,       /* Thread init. */
        NULL,       /* Thread finish. */
        {
            {MXS_END_MODULE_PARAMS}
        }
    };

    return &info;
}
}

/**
 * The default authenticator name for this protocol
 *
 * @return name of authenticator
 */
static std::string httpd_default_auth()
{
    return MXS_HTTPAUTH_AUTHENTICATOR_NAME;
}

/**
 * Read event for EPOLLIN on the httpd protocol module.
 *
 * @param generic_dcb   The descriptor control block
 * @return
 */
void HTTPDClientProtocol::ready_for_reading(DCB* generic_dcb)
{
    auto dcb = static_cast<ClientDCB*>(generic_dcb);
    MXS_SESSION* session = dcb->session();

    int numchars = 1;
    char buf[HTTPD_REQUESTLINE_MAXLEN - 1] = "";
    char* query_string = NULL;
    char method[HTTPD_METHOD_MAXLEN - 1] = "";
    char url[HTTPD_SMALL_BUFFER] = "";
    size_t i, j;

    /**
     * get the request line
     * METHOD URL HTTP_VER\r\n
     */

    numchars = httpd_get_line(dcb->fd(), buf, sizeof(buf));

    i = 0;
    j = 0;
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    /* check allowed http methods */
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        // httpd_unimplemented(dcb->m_fd);
        return;
    }

    i = 0;

    while ((j < sizeof(buf)) && ISspace(buf[j]))
    {
        j++;
    }

    while ((j < sizeof(buf) - 1) && !ISspace(buf[j]) && (i < sizeof(url) - 1))
    {
        url[i] = buf[j];
        i++;
        j++;
    }

    url[i] = '\0';

    /**
     * Get the query string if availble
     */

    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
        {
            query_string++;
        }
        if (*query_string == '?')
        {
            *query_string = '\0';
        }
    }

    /** If listener->authenticator is the default authenticator, it means that
     * we don't need to check the user credentials. All other authenticators
     * cause a 401 Unauthorized to be returned on the first try. */
    bool auth_ok = true;

    /**
     * Get the request headers
     */

    while ((numchars > 0) && strcmp("\n", buf))
    {
        char* value = NULL;
        char* end = NULL;
        numchars = httpd_get_line(dcb->fd(), buf, sizeof(buf));
        if ((value = strchr(buf, ':')))
        {
            *value = '\0';
            value++;
            end = &value[strlen(value) - 1];
            *end = '\0';
        }
    }

    /**
     * Now begins the server reply
     */

    /* send all the basic headers and close with \r\n */
    httpd_send_headers(dcb, 1, auth_ok);

#if 0
    /**
     * ToDO: launch proper content handling based on the requested URI, later REST interface
     *
     */
    if (strcmp(url, "/show") == 0)
    {
        if (query_string && strlen(query_string))
        {
            if (strcmp(query_string, "dcb") == 0)
            {
                dprintAllDCBs(dcb);
            }
            if (strcmp(query_string, "session") == 0)
            {
                dprintAllSessions(dcb);
            }
        }
    }
    if (strcmp(url, "/services") == 0)
    {
        ResultSet* set, * seviceGetList();
        if ((set = serviceGetList()) != NULL)
        {
            resultset_stream_json(set, dcb);
            resultset_free(set);
        }
    }
#endif
    GWBUF* uri;
    if (auth_ok && (uri = gwbuf_alloc(strlen(url) + 1)) != NULL)
    {
        strcpy((char*)GWBUF_DATA(uri), url);
        gwbuf_set_type(uri, GWBUF_TYPE_HTTP);
        mxs_route_query(session, uri);
    }

    /* force the client connecton close */
    DCB::close(dcb);
    return;
}

/**
 * EPOLLOUT handler for the HTTPD protocol module.
 *
 * @param dcb   The descriptor control block
 * @return
 */
void HTTPDClientProtocol::write_ready(DCB* dcb)
{
    dcb->writeq_drain();
}

/**
 * Write routine for the HTTPD protocol module.
 *
 * Writes the content of the buffer queue to the socket
 * observing the non-blocking principles of the gateway.
 *
 * @param dcb   Descriptor Control Block for the socket
 * @param queue Linked list of buffes to write
 */
int32_t HTTPDClientProtocol::write(DCB* dcb, GWBUF* queue)
{
    return dcb->writeq_append(queue);
}

/**
 * Handler for the EPOLLERR event.
 *
 * @param dcb   The descriptor control block
 */
void HTTPDClientProtocol::error(DCB* dcb)
{
    DCB::close(dcb);
}

/**
 * Handler for the EPOLLHUP event.
 *
 * @param dcb   The descriptor control block
 */
void HTTPDClientProtocol::hangup(DCB* dcb)
{
    DCB::close(dcb);
}

bool HTTPDClientProtocol::init_connection(DCB* dcb)
{
    return session_start(dcb->session());
}

void HTTPDClientProtocol::finish_connection(DCB* dcb)
{
}

HTTPDClientProtocol::HTTPDClientProtocol()
{
}

/**
 * HTTPD get line from client
 */
static int httpd_get_line(int sock, char* buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                {
                    if (recv(sock, &c, 1, 0) < 0)
                    {
                        c = '\n';
                    }
                }
                else
                {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }

    buf[i] = '\0';

    return i;
}

/**
 * HTTPD send basic headers with 200 OK
 */
static void httpd_send_headers(DCB* dcb, int final, bool auth_ok)
{
    char date[64] = "";
    const char* fmt = "%a, %d %b %Y %H:%M:%S GMT";
    time_t httpd_current_time = time(NULL);

    struct tm tm;
    localtime_r(&httpd_current_time, &tm);
    strftime(date, sizeof(date), fmt, &tm);
    const char* response = auth_ok ? "200 OK" : "401 Unauthorized";
    dcb_printf(dcb,
               "HTTP/1.1 %s\r\n"
               "Date: %s\r\n"
               "Server: %s\r\n"
               "Connection: close\r\n"
               "WWW-Authenticate: Basic realm=\"MaxInfo\"\r\n"
               "Content-Type: application/json\r\n",
               response,
               date,
               HTTP_SERVER_STRING);

    /* close the headers */
    if (final)
    {
        dcb_printf(dcb, "\r\n");
    }
}
